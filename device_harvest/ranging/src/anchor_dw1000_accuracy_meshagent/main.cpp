/*
 * Variant: anchor_dw1000_accuracy_meshagent
 *
 * Physically an "anchor", but in mf-DW1000 it acts as the initiator (startAsInitiator).
 * That is, the anchor polls nearby tags (responders) one device at a time on a schedule (TWR).
 *   (role-inversion rationale: docs/DESIGN_FLOW_mesh_tdma.md Pivot 4-5)
 *
 * CORE mesh-TDMA skeleton (docs/ARCHITECTURE_mesh_tdma.md, DESIGN_P3_scope1.md):
 *   superframe        : slot timing
 *   peer_scheduler(L4): which tag to poll inside my slot (priority, the original request)
 *   interference (L2) : interfering neighbors (UWB overhearing + shared-tag)
 *   mgm_agent    (L3) : inter-anchor slot assignment (mesh message exchange)
 *
 * F-a scope: validate the UWB scheduled-polling + L4 priority pipeline on hardware.
 *   L3/L2 mesh send/recv are still stubbed (no-op) -> ESP-WIFI-MESH is wired in F-b.
 *   With no mesh, neighbors = 0 -> mgm_agent converges to slot 0 (single-anchor operation).
 */
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>        // top-level so PlatformIO LDF resolves the framework WiFi/ESP-NOW libs
#include <esp_now.h>
#include <DW1000Ranging.h>
#include "rf_config_dw1000.h"
#include "logging.h"
#include "superframe.h"
#include "peer_scheduler.h"
#include "interference.h"
#include "mgm_agent.h"
#include "mesh_link.h"   // pulls in mesh_msg.h

// Anchor number (different per board). byte0 = ANCHOR_ID -> short 0x00NN -> "A{ANCHOR_ID}".
// For multi-anchor tests, override per board at build time:
//   PLATFORMIO_BUILD_FLAGS="-DANCHOR_ID=1" pio run -e anchor_dw1000_accuracy_meshagent -t upload --upload-port /dev/ttyUSBx
#ifndef ANCHOR_ID
#define ANCHOR_ID 0
#endif
static const uint16_t SELF_SHORT = (ANCHOR_ID);

// Slot parameters. K_s is small for low anchor counts: per-anchor airtime ~= 1/K_s, and each
// slot's work-window holds several TWR exchanges. Raise K_s if many anchors mutually interfere.
static const SuperframeConfig    SF_CFG  = {4, 120, 10};
static const MgmConfig           MGM_CFG = {4, 500, 200, 3600};
static const PeerSchedulerConfig PS_CFG  = {0.001f, 8.0f, PS_WEAK_RXPOWER_DBM, 5, 0.2f};
static const uint32_t LEASE_MS        = 3600;
static const float    AUDIBLE_THRESH  = -90.0f;
// A single DS-TWR cycle (POLL->ACK->RANGE->REPORT) is ~35-40ms in 110kbps ACCURACY mode
// (two 7ms reply delays + long frames). The timeout must be comfortably larger so it does
// not cut an in-flight exchange short.
static const uint32_t POLL_TIMEOUT_MS = 100;
static const uint16_t EXCHANGE_BUDGET = 8;
static const uint32_t PUBLISH_PERIOD_MS = 700;   // ~1 superframe: publish TAGLIST/AUDIBLE

Superframe        sf;
PeerScheduler     sched;
InterferenceGraph ig;
MgmAgent          agent;
MeshLink          mesh;

// In-flight poll tracking (single-poll guard)
static uint16_t pollTarget  = PS_NONE;
static uint32_t pollStartMs = 0;
static uint32_t lastPublishMs = 0;

// Periodic status telemetry (slot/neighbor/tag/range health)
static const uint32_t STATUS_PERIOD_MS = 2000;
static uint32_t g_ranges   = 0;
static uint32_t g_meshRx   = 0;   // mesh frames received (ESP-NOW reception sanity)
static uint32_t lastStatMs = 0;

// ---- L3 outbound (ESP-NOW broadcast) ----
static void meshSendValue(const ValueMsg& v) {
    uint8_t b[MESH_MAX_FRAME]; mesh.send(b, packValue(v, b));
}
static void meshSendGain(const GainMsg& g) {
    uint8_t b[MESH_MAX_FRAME]; mesh.send(b, packGain(g, b));
}

// ---- UWB callbacks ----
void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    if (d == nullptr) return;
    uint16_t sa = d->getShortAddress();
    float r = d->getRange();
    float p = d->getRXPower();
    char devId[8];
    shortAddrToId(sa, devId, sizeof(devId));
    logRange(devId, r, p);                         // CSV: deviceId,range,rxp,ts,nlos
    sched.reportResult(sa, millis(), true, r, p);  // update L4 priority
    g_ranges++;
    if (sa == pollTarget) pollTarget = PS_NONE;    // in-flight done
}
void newDevice(DW1000Device* d)      { sched.addTag(d->getShortAddress()); }
void inactiveDevice(DW1000Device* d) { sched.removeTag(d->getShortAddress()); }
void overheard(uint16_t shortAddr, float rxp) {
    // Only other anchors (initiators, short addr in 0x0000..0x003F) are interferers;
    // ignore overheard tags and any bogus/misparsed addresses.
    if (shortAddr < 0x40) ig.onOverheard(shortAddr, rxp, millis());
}

void setup() {
    Serial.begin(115200); delay(200);
    char selfAddr[24];
    snprintf(selfAddr, sizeof(selfAddr), "%02X:00:5B:D5:A9:9A:E2:9C", (ANCHOR_ID));
    Serial.print("# anchor_dw1000_accuracy_meshagent (initiator) A"); Serial.println(ANCHOR_ID);

    sf.begin(SF_CFG);
    sched.begin(PS_CFG);
    ig.begin(SELF_SHORT, LEASE_MS, AUDIBLE_THRESH);
    agent.begin(SELF_SHORT, MGM_CFG);

    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    DW1000Ranging.attachOverheard(overheard);
    DW1000Ranging.setScheduledMode(true);                        // disable auto-broadcast POLL
    DW1000Ranging.startAsInitiator(selfAddr, RF_MODE, false);    // anchor drives the polling
    applyRfConfigDW1000();

    // Control plane: ESP-NOW broadcast for L2/L3 coordination messages.
    if (!mesh.begin()) Serial.println("# mesh init failed");

    // F-b: slot epoch still local (gossip slot-phase sync is the next sub-step).
    // MGM coordination + interference detection work over the mesh without tight epoch sync.
    sf.setEpoch(millis());
}

// Drain received mesh frames and dispatch to L2/L3 (called from loop, single-threaded).
static void meshPump(uint32_t now) {
    MeshLink::Frame fr;
    for (int i = 0; i < 8 && mesh.poll(fr); i++) {
        g_meshRx++;
        switch (meshMsgType(fr.data, fr.len)) {
        case MESH_VALUE: {
            ValueMsg m;
            if (unpackValue(fr.data, fr.len, m) && m.agentId != SELF_SHORT) agent.onValue(m, now);
            break;
        }
        case MESH_GAIN: {
            GainMsg m;
            if (unpackGain(fr.data, fr.len, m) && m.agentId != SELF_SHORT) agent.onGain(m, now);
            break;
        }
        case MESH_TAGLIST: {
            uint16_t fromId; uint16_t ids[MESH_MAX_IDS]; uint8_t k;
            if (unpackIdList(fr.data, fr.len, fromId, ids, MESH_MAX_IDS, k))
                ig.onTagList(fromId, ids, k, now);
            break;
        }
        case MESH_AUDIBLE: {
            uint16_t fromId; uint16_t ids[MESH_MAX_IDS]; uint8_t k;
            if (unpackIdList(fr.data, fr.len, fromId, ids, MESH_MAX_IDS, k))
                ig.onAudibleReport(fromId, ids, k, now);
            break;
        }
        case MESH_SYNC: {
            uint16_t fromId; uint32_t phase;
            // Adopt the superframe phase of any lower-id anchor -> all converge to the lowest
            // id's phase, so owned slots fall at distinct absolute times (collision-free airtime).
            if (unpackSync(fr.data, fr.len, fromId, phase) && fromId < SELF_SHORT)
                sf.setEpoch(now - phase);
            break;
        }
        }
    }
}

// Periodically publish my tag set and what I overhear (drives L2 interference detection).
static void meshPublish(uint32_t now) {
    if ((uint32_t)(now - lastPublishMs) < PUBLISH_PERIOD_MS) return;
    lastPublishMs = now;
    uint8_t  b[MESH_MAX_FRAME];
    uint16_t myTags[PS_MAX_TAGS];
    uint8_t  nt = sched.tagIds(myTags, PS_MAX_TAGS);
    ig.setMyTags(myTags, nt);
    mesh.send(b, packIdList(MESH_TAGLIST, SELF_SHORT, myTags, nt, b));
    uint16_t aud[IG_MAX_NEIGHBORS];
    uint8_t  na = ig.myAudibleList(aud, IG_MAX_NEIGHBORS, now);
    mesh.send(b, packIdList(MESH_AUDIBLE, SELF_SHORT, aud, na, b));
    // slot-phase gossip: lower-id anchors are the reference; higher-id anchors align to them.
    mesh.send(b, packSync(SELF_SHORT, sf.phaseMs(now), b));
}

void loop() {
    DW1000Ranging.loop();                 // UWB state machine + overhearing
    uint32_t now = millis();

    // --- L2/L3 over the mesh ---
    meshPump(now);                        // ingest neighbor VALUE/GAIN/TAGLIST/AUDIBLE
    ig.tick(now);
    agent.setNeighbors(ig.neighborIds(), ig.neighborCount());
    agent.tick(now);
    ValueMsg v; GainMsg g;
    if (agent.pendingValue(v)) meshSendValue(v);
    if (agent.pendingGain(g))  meshSendGain(g);
    meshPublish(now);                     // periodic TAGLIST + AUDIBLE

    // --- L4: single-poll a tag by priority inside my slot's work-window ---
    uint8_t mySlot = agent.slot();

    // in-flight timeout -> record failure (badStreak up, sent to the back)
    if (pollTarget != PS_NONE && (uint32_t)(now - pollStartMs) > POLL_TIMEOUT_MS) {
        sched.reportResult(pollTarget, now, false, 0.0f, 0.0f);
        pollTarget = PS_NONE;
    }

    // No neighbors (solo, F-a) -> poll continuously without slot gating; no contention means
    // we may use all airtime. Once neighbors appear (F-b+), poll only in my work-window.
    bool slotOpen = (ig.neighborCount() == 0) ||
                    (sf.isMyWorkWindow(now, mySlot) &&
                     sf.workRemainingMs(now, mySlot) >= EXCHANGE_BUDGET);

    if (pollTarget == PS_NONE && slotOpen) {
        uint16_t target;
        if (sched.pick(now, target)) {
            byte sa[2] = { (byte)(target & 0xFF), (byte)((target >> 8) & 0xFF) };
            DW1000Device* d = DW1000Ranging.searchDistantDevice(sa);
            if (d != nullptr) {
                DW1000Ranging.pollDevice(d);
                pollTarget  = target;
                pollStartMs = now;
            }
        }
    }

    // status telemetry: nbr>0 means the mesh detected the other anchor; two anchors should
    // converge to different slot values.
    if ((uint32_t)(now - lastStatMs) >= STATUS_PERIOD_MS) {
        lastStatMs = now;
        Serial.print("# A");      Serial.print(ANCHOR_ID);
        Serial.print(" slot=");   Serial.print(agent.slot());
        Serial.print(" nbr=");    Serial.print(ig.neighborCount());
        Serial.print("[");
        for (uint8_t i = 0; i < ig.neighborCount(); i++) {
            if (i) Serial.print(",");
            Serial.print(ig.neighborIds()[i], HEX);
        }
        Serial.print("]");
        Serial.print(" tags=");   Serial.print(sched.tagCount());
        Serial.print(" rx=");     Serial.print(g_meshRx);
        Serial.print(" ranges="); Serial.println(g_ranges);
    }
}
