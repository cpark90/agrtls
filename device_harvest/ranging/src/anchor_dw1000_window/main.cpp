/*
 * Variant: anchor_dw1000_window
 * Window-based two-level TDMA anchor (initiator). docs/ARCHITECTURE_window_tdma.md, SYSTEM_OVERVIEW.md.
 *
 * Each anchor:
 *   - ranges the tags it hears (records RXP/range), publishes them as TAGINFO over ESP-NOW, and
 *     merges others' TAGINFO into a shared TagRegistry -> every anchor computes the SAME coloring.
 *   - colors tags into windows (window_color): conflict = share an effective anchor; far/weak tags
 *     get fewer windows / become candidates.
 *   - in the current window k (window_frame, epoch-synced), at its anchor-slot, polls the tag that
 *     window_color assigns to window k -> all anchors of that tag measure it in the same window
 *     => the tag's ranges are clustered (for localization).
 *
 * W-3 = wiring of the W-1/W-2 pure modules with this loop + a simple bootstrap. Timing/anchor-slot
 * are first-cut (anchor-slot = ANCHOR_ID); HW validation/tuning + fair anchor-slot coloring = W-4.
 *
 * Pairs with tag_dw1000_responder. Build per node: -D ANCHOR_ID=n.
 */
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <DW1000Ranging.h>
#include "rf_config_dw1000.h"
#include "logging.h"
#include "tag_registry.h"
#include "tag_quality.h"
#include "window_color.h"
#include "window_frame.h"
#include "mesh_link.h"   // pulls mesh_msg.h

#ifndef ANCHOR_ID
#define ANCHOR_ID 0
#endif
static const uint16_t SELF_SHORT = (ANCHOR_ID);

// First-cut timing (tune in W-4): window = slotsPerWindow*slotLen; superframe = numWindows*window.
static const WindowFrameConfig WF_CFG = {4 /*windows*/, 4 /*slots*/, 120 /*slotLen*/, 10 /*guard*/};
static const uint8_t  MY_SLOT          = (ANCHOR_ID) % 4;   // anchor-slot within a window (< slotsPerWindow)
static const uint32_t PUBLISH_MS       = 700;
static const uint32_t RECOLOR_MS       = 700;
static const uint32_t STATUS_MS        = 2000;
static const uint32_t POLL_TIMEOUT_MS  = 100;
#define MY_MEAS_MAX 16

TagRegistry  reg;
WindowColor  wc;
WindowFrame  wf;
MeshLink     mesh;

// my own measurements (for publishing TAGINFO)
static TagInfoEntry myMeas[MY_MEAS_MAX];
static uint8_t      myMeasN = 0;
// discovered tags (for bootstrap seeding of unranged tags)
static uint16_t disc[MY_MEAS_MAX];
static uint8_t  discN = 0, seedIdx = 0;

static uint16_t pollTarget  = WC_NONE;
static uint32_t pollStartMs = 0;
static uint32_t lastPublish = 0, lastRecolor = 0, lastStat = 0;
static uint32_t g_ranges = 0;

static void rememberMeasurement(uint16_t tag, float rxp, float range) {
    for (uint8_t i = 0; i < myMeasN; i++)
        if (myMeas[i].tagId == tag) { myMeas[i].rxp_dBm = rxp; myMeas[i].range_m = range; return; }
    if (myMeasN < MY_MEAS_MAX) myMeas[myMeasN++] = TagInfoEntry{tag, rxp, range};
}
static void rememberDiscovered(uint16_t tag) {
    for (uint8_t i = 0; i < discN; i++) if (disc[i] == tag) return;
    if (discN < MY_MEAS_MAX) disc[discN++] = tag;
}

// ---- UWB callbacks ----
void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    if (d == nullptr) return;
    uint16_t sa = d->getShortAddress();
    float r = d->getRange(), p = d->getRXPower();
    char devId[8]; shortAddrToId(sa, devId, sizeof(devId));
    logRange(devId, r, p);                 // CSV: deviceId,range,rxp,ts,nlos
    reg.report(SELF_SHORT, sa, p, r);      // my own link into the shared registry
    rememberMeasurement(sa, p, r);
    g_ranges++;
    if (sa == pollTarget) pollTarget = WC_NONE;
}
void newDevice(DW1000Device* d)      { rememberDiscovered(d->getShortAddress()); }
void inactiveDevice(DW1000Device* d) { /* keep in registry; coloring/candidate handles it */ (void)d; }

// ---- mesh ----
static void meshPump(uint32_t now) {
    MeshLink::Frame fr;
    for (int i = 0; i < 8 && mesh.poll(fr); i++) {
        switch (meshMsgType(fr.data, fr.len)) {
        case MESH_TAGINFO: {
            uint16_t aid; TagInfoEntry e[MESH_MAX_TAGINFO]; uint8_t n;
            if (unpackTagInfo(fr.data, fr.len, aid, e, MESH_MAX_TAGINFO, n) && aid != SELF_SHORT)
                for (uint8_t k = 0; k < n; k++) reg.report(aid, e[k].tagId, e[k].rxp_dBm, e[k].range_m);
            break;
        }
        case MESH_SYNC: {
            uint16_t fromId; uint32_t phase;
            if (unpackSync(fr.data, fr.len, fromId, phase) && fromId < SELF_SHORT)
                wf.setEpoch(now - phase);   // align my windows to the lowest-id anchor
            break;
        }
        }
    }
}
static void meshPublish(uint32_t now) {
    if ((uint32_t)(now - lastPublish) < PUBLISH_MS) return;
    lastPublish = now;
    uint8_t b[MESH_MAX_FRAME];
    mesh.send(b, packTagInfo(SELF_SHORT, myMeas, myMeasN, b));   // share my measurements
    mesh.send(b, packSync(SELF_SHORT, wf.phaseMs(now), b));      // window-phase gossip
}

void setup() {
    Serial.begin(115200); delay(200);
    char selfAddr[24];
    snprintf(selfAddr, sizeof(selfAddr), "%02X:00:5B:D5:A9:9A:E2:9C", (ANCHOR_ID));
    Serial.print("# anchor_dw1000_window (initiator) A"); Serial.println(ANCHOR_ID);

    reg.begin();
    wf.begin(WF_CFG);

    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    DW1000Ranging.setScheduledMode(true);
    DW1000Ranging.startAsInitiator(selfAddr, RF_MODE, false);
    applyRfConfigDW1000();

    if (!mesh.begin()) Serial.println("# mesh init failed");
    wf.setEpoch(millis());   // local until a lower-id anchor's SYNC arrives
}

void loop() {
    DW1000Ranging.loop();
    uint32_t now = millis();

    meshPump(now);
    if ((uint32_t)(now - lastRecolor) >= RECOLOR_MS) {
        lastRecolor = now;
        wc.build(reg, WF_CFG.numWindows);
    }
    meshPublish(now);

    if (pollTarget != WC_NONE && (uint32_t)(now - pollStartMs) > POLL_TIMEOUT_MS) {
        reg.report(SELF_SHORT, pollTarget, /*weak*/ -120.0f, 0.0f);   // failed -> ineligible sample
        pollTarget = WC_NONE;
    }

    // poll only in my anchor-slot of the current window, during work time
    if (pollTarget == WC_NONE && wf.slotIndexNow(now) == MY_SLOT && wf.isWork(now)) {
        uint8_t  k = wf.windowIndexNow(now);
        uint16_t target = wc.tagForWindow(reg, SELF_SHORT, k);
        if (target == WC_NONE && discN > 0) {       // bootstrap: seed an unranged/known tag
            target = disc[seedIdx % discN];
            seedIdx++;
        }
        if (target != WC_NONE) {
            byte sa[2] = { (byte)(target & 0xFF), (byte)((target >> 8) & 0xFF) };
            DW1000Device* d = DW1000Ranging.searchDistantDevice(sa);
            if (d != nullptr) { DW1000Ranging.pollDevice(d); pollTarget = target; pollStartMs = now; }
        }
    }

    if ((uint32_t)(now - lastStat) >= STATUS_MS) {
        lastStat = now;
        Serial.print("# A"); Serial.print(ANCHOR_ID);
        Serial.print(" win="); Serial.print(wf.windowIndexNow(now));
        Serial.print("/");     Serial.print(wc.numWindows());
        Serial.print(" disc="); Serial.print(discN);
        Serial.print(" ranges="); Serial.print(g_ranges);
        // schedule dump: tag -> window (or 'c' = candidate). Compare across anchors: should match.
        Serial.print(" sched:");
        uint16_t tl[TR_MAX_TAGS]; uint8_t nt = reg.tags(tl, TR_MAX_TAGS);
        for (uint8_t i = 0; i < nt; i++) {
            char id[8]; shortAddrToId(tl[i], id, sizeof(id));
            uint8_t c = wc.colorOf(tl[i]);
            Serial.print(' '); Serial.print(id); Serial.print('=');
            if (c == WC_CANDIDATE) Serial.print('c'); else Serial.print(c);
        }
        Serial.println();
    }
}
