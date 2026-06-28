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
#include <DW1000Ranging.h>
#include "rf_config_dw1000.h"
#include "logging.h"
#include "superframe.h"
#include "peer_scheduler.h"
#include "interference.h"
#include "mgm_agent.h"

// This node's address (anchor #0: byte0=0x00 -> short 0x0000 -> "A0"). Change per node.
#define SELF_ADDR "00:00:5B:D5:A9:9A:E2:9C"
static const uint16_t SELF_SHORT = 0x0000;

// Initial parameters (DESIGN_P3_scope1.md section M)
static const SuperframeConfig    SF_CFG  = {16, 45, 5};
static const MgmConfig           MGM_CFG = {16, 500, 200, 3600};
static const PeerSchedulerConfig PS_CFG  = {0.001f, 8.0f, NLOS_RXPOWER_THRESHOLD_DBM, 5, 0.2f};
static const uint32_t LEASE_MS        = 3600;
static const float    AUDIBLE_THRESH  = -90.0f;
// A single DS-TWR cycle (POLL->ACK->RANGE->REPORT) is ~35-40ms in 110kbps ACCURACY mode
// (two 7ms reply delays + long frames). The timeout must be comfortably larger so it does
// not cut an in-flight exchange short.
static const uint32_t POLL_TIMEOUT_MS = 100;
static const uint16_t EXCHANGE_BUDGET = 8;

Superframe        sf;
PeerScheduler     sched;
InterferenceGraph ig;
MgmAgent          agent;

// In-flight poll tracking (single-poll guard)
static uint16_t pollTarget  = PS_NONE;
static uint32_t pollStartMs = 0;

// ---- mesh abstraction (replaced by ESP-WIFI-MESH in F-b) ----
// TODO(F-b): esp_mesh init + send/recv. On receive, wire callbacks to ig.onTagList/onAudibleReport
//            and agent.onValue/onGain.
static void meshSendValue(const ValueMsg&) { /* TODO(F-b) */ }
static void meshSendGain(const GainMsg&)   { /* TODO(F-b) */ }

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
    if (sa == pollTarget) pollTarget = PS_NONE;    // in-flight done
}
void newDevice(DW1000Device* d)      { sched.addTag(d->getShortAddress()); }
void inactiveDevice(DW1000Device* d) { sched.removeTag(d->getShortAddress()); }
void overheard(uint16_t shortAddr, float rxp) { ig.onOverheard(shortAddr, rxp, millis()); }

void setup() {
    Serial.begin(115200); delay(200);
    Serial.println("# anchor_dw1000_accuracy_meshagent (initiator/polling master)");

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
    DW1000Ranging.setScheduledMode(true);                       // disable auto-broadcast POLL
    DW1000Ranging.startAsInitiator(SELF_ADDR, RF_MODE, false);  // anchor drives the polling
    applyRfConfigDW1000();

    // F-a: no mesh sync -> set a local epoch immediately (single-node validation).
    // TODO(F-b): replace setEpoch with gossip slot-phase consensus.
    sf.setEpoch(millis());
}

void loop() {
    DW1000Ranging.loop();                 // UWB state machine + overhearing
    uint32_t now = millis();

    // --- L2/L3 (F-a: mesh stubbed -> neighbors 0 -> agent converges to slot 0) ---
    ig.tick(now);
    agent.setNeighbors(ig.neighborIds(), ig.neighborCount());
    agent.tick(now);
    ValueMsg v; GainMsg g;
    if (agent.pendingValue(v)) meshSendValue(v);
    if (agent.pendingGain(g))  meshSendGain(g);

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
}
