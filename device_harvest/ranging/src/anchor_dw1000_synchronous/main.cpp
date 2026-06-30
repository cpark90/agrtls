/*
 * Variant: anchor_dw1000_synchronous
 * Frame-based two-level TDMA anchor (initiator). docs/ARCHITECTURE_synchronous_tdma.md, SYSTEM_OVERVIEW.md.
 *
 * Each anchor:
 *   - ranges the tags it hears (records RXP/range), publishes them as TAGINFO over ESP-NOW, and
 *     merges others' TAGINFO into a shared TagRegistry -> every anchor computes the SAME coloring.
 *   - colors tags into frames (frame_color): conflict = share an effective anchor; far/weak tags
 *     get fewer frames / become candidates.
 *   - in the current frame k (frame_schedule, epoch-synced), at its anchor-slot, polls the tag that
 *     frame_color assigns to frame k -> all anchors of that tag measure it in the same frame
 *     => the tag's ranges are clustered (for localization).
 *
 * Anchor-slot within a frame = this anchor's RANK among the frame-tag's effective anchors,
 * computed from the shared registry (deterministic -> collision-free, no MGM negotiation needed).
 * numFrames and slotsPerFrame are both sized dynamically from the registry each recolor; links
 * that vanish age out (TagRegistry::expire) so a gone tag stops being scheduled.
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
#include "frame_color.h"
#include "frame_schedule.h"
#include "mesh_msg.h"    // TAGINFO (+ mesh_wire: SYNC, meshMsgType)
#include "mesh_link.h"   // ESP-NOW transport

#ifndef ANCHOR_ID
#define ANCHOR_ID 0
#endif
static const uint16_t SELF_SHORT = (ANCHOR_ID);

// Timing: frame = slotsPerFrame*slotLen; superframe = numFrames*frame. BOTH numFrames and
// slotsPerFrame are sized dynamically each recolor from the shared registry (numFrames = active
// frames + 1 probe; slotsPerFrame = max effective anchors over any tag), so the schedule fits
// exactly with no wasted slots. The config values here are just the initial/bootstrap state.
#define MAX_SLOTS_PER_FRAME 12   // cap on anchor-slots/frame (<= MAX_DEVICES; bounds superframe len)
static const FrameScheduleConfig WF_CFG = {1 /*frames (dyn)*/, 1 /*slots (dyn)*/, 120 /*slotLen*/, 10 /*guard*/};
static const uint32_t PUBLISH_MS       = 700;
static const uint32_t RECOLOR_MS       = 700;
static const uint32_t POLL_TIMEOUT_MS  = 100;
#define MY_MEAS_MAX 16

TagRegistry  reg;
FrameColor  wc;
FrameSchedule  wf;
MeshLink     mesh;

// discovered tags (for bootstrap / candidate probing of tags without a normal frame)
static uint16_t disc[MY_MEAS_MAX];
static uint8_t  discN = 0;
// probe set rebuilt each recolor: discovered tags that have NO active frame (candidates + unranged).
// Polled round-robin in the single trailing probe frame so no tag is ever permanently excluded.
static uint16_t probeList[FC_MAX_TAGS];
static uint8_t  probeN = 0, probeIdx = 0;
static uint8_t  activeW = 0;   // number of active (localization) frames = wc.numFrames()

static uint16_t pollTarget  = FC_NONE;
static uint32_t pollStartMs = 0;
static uint32_t lastPolledSlot = 0xFFFFFFFF;   // last slot instance we already polled in (one poll/slot)
static uint32_t lastPublish = 0, lastRecolor = 0;

static void rememberDiscovered(uint16_t tag) {
    for (uint8_t i = 0; i < discN; i++) if (disc[i] == tag) return;
    if (discN < MY_MEAS_MAX) disc[discN++] = tag;
}

// ---- UWB callbacks ----
void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    if (d == nullptr) return;
    uint16_t sa = d->getShortAddress();
    float r = d->getRange();
    float p = d->getRXPower();
    char devId[8]; shortAddrToId(sa, devId, sizeof(devId));
    logRange(devId, r, p);                       // CSV: deviceId,range,rxp,ts
    reg.report(SELF_SHORT, sa, p, r, millis());  // my own link into the shared registry (timestamped)
    if (sa == pollTarget) pollTarget = FC_NONE;  // exchange completed
}
void newDevice(DW1000Device* d)      { rememberDiscovered(d->getShortAddress()); }
void inactiveDevice(DW1000Device* d) { /* keep in registry; coloring/candidate handles it */ (void)d; }

// ---- mesh ----
static void meshPump(uint32_t now) {
    MeshLink::Frame fr;
    for (int i = 0; i < 8 && mesh.poll(fr); i++) {
        switch (meshMsgType(fr.data, fr.len)) {
        case MESH_TAGINFO: {
            uint16_t aid;
            TagInfoEntry e[MESH_MAX_TAGINFO];
            uint8_t n;
            if (unpackTagInfo(fr.data, fr.len, aid, e, MESH_MAX_TAGINFO, n) && aid != SELF_SHORT)
                for (uint8_t k = 0; k < n; k++)             // peer-owned: adopt its eligibility verbatim
                    reg.reportPeer(aid, e[k].tagId, e[k].rxp_dBm, e[k].range_m, e[k].eligible, now);
            break;
        }
        case MESH_SYNC: {
            uint16_t fromId; uint32_t phase;
            if (unpackSync(fr.data, fr.len, fromId, phase) && fromId < SELF_SHORT)
                wf.setEpoch(now - phase);   // align my frames to the lowest-id anchor
            break;
        }
        }
    }
}
static void meshPublish(uint32_t now) {
    if ((uint32_t)(now - lastPublish) < PUBLISH_MS) return;
    lastPublish = now;
    // publish my own links (smoothed rxp + MY eligibility decision) so peers adopt them verbatim
    uint16_t t[MESH_MAX_TAGINFO]; float rx[MESH_MAX_TAGINFO], rg[MESH_MAX_TAGINFO]; bool el[MESH_MAX_TAGINFO];
    uint8_t pn = reg.selfLinks(SELF_SHORT, t, rx, rg, el, MESH_MAX_TAGINFO);
    TagInfoEntry pub[MESH_MAX_TAGINFO];
    for (uint8_t i = 0; i < pn; i++) pub[i] = TagInfoEntry{t[i], rx[i], rg[i], el[i]};
    uint8_t b[MESH_MAX_FRAME];
    mesh.send(b, packTagInfo(SELF_SHORT, pub, pn, b));          // share my links
    mesh.send(b, packSync(SELF_SHORT, wf.phaseMs(now), b));     // frame-phase gossip
}

void setup() {
    Serial.begin(115200); delay(200);
    char selfAddr[24];
    snprintf(selfAddr, sizeof(selfAddr), "%02X:00:5B:D5:A9:9A:E2:9C", (ANCHOR_ID));
    Serial.print("# anchor_dw1000_synchronous (initiator) A"); Serial.println(ANCHOR_ID);

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
    DW1000Ranging.loopInitiator();
    uint32_t now = millis();

    meshPump(now);
    if ((uint32_t)(now - lastRecolor) >= RECOLOR_MS) {
        lastRecolor = now;
        reg.expire(now);                           // drop links that vanished (stale entries)
        wc.build(reg, FC_MAX_FRAMES);
        activeW = wc.numFrames();                 // shared across anchors (deterministic on registry)
        // anchor-slots/frame = max effective anchors over any tag (so every effective anchor of a
        // frame's tag gets a distinct rank-slot). Shared-deterministic -> stays equal across anchors.
        uint8_t slots = reg.maxEffectiveAnchorCount();
        if (slots < 1) slots = 1;
        if (slots > MAX_SLOTS_PER_FRAME) slots = MAX_SLOTS_PER_FRAME;
        wf.setSlotsPerFrame(slots);
        // probe set (local) = discovered tags THIS anchor is not yet effective for. Covers: bootstrap
        // (never ranged -> must measure once to learn its link), global candidates (far/weak tags), and
        // weak-link re-probe. Tags SELF is already effective for are ranged in their active frame, so
        // they are excluded here. Without this an anchor could never start ranging an active tag it had
        // not previously measured (it is not in tagForFrame until effective -> deadlock).
        probeN = 0;
        for (uint8_t i = 0; i < discN && probeN < FC_MAX_TAGS; i++)
            if (!reg.isEffectiveAnchor(disc[i], SELF_SHORT)) probeList[probeN++] = disc[i];
        // Cycle ONLY active frames + one trailing probe frame. Empty frames are never traversed.
        // total must be identical on every anchor (epoch sync) -> derive from shared activeW only.
        wf.setNumFrames((uint8_t)(activeW + 1));
    }
    meshPublish(now);

    if (pollTarget != FC_NONE && (uint32_t)(now - pollStartMs) > POLL_TIMEOUT_MS) {
        // Poll timed out: just clear it and retry next slot. Do NOT report a fake weak (-120) sample --
        // a single transient timeout (collision/jitter) must not wipe a good link and flip the tag to
        // candidate, which would desync the per-anchor coloring (different superframe lengths -> epoch
        // drift). Stale/unreachable links should be handled by registry-entry aging (future work).
        pollTarget = FC_NONE;
    }

    // poll AT MOST ONCE per slot, during work time. Which slot is mine depends on the frame:
    //  - active frame k: I range the frame's tag iff I'm one of its effective anchors, in the slot
    //    = my rank among them (collision-free, every anchor of the tag gets a distinct slot).
    //  - probe frame: best-effort, id-based slot (clustering not required for probes).
    // (one poll/slot: a greedy re-poll would spill the exchange past the slot edge into the next slot.)
    if (pollTarget == FC_NONE && wf.isWork(now) && wf.slotNumber(now) != lastPolledSlot) {
        uint8_t  k       = wf.frameIndexNow(now);
        uint8_t  curSlot = wf.slotIndexNow(now);
        uint16_t target  = FC_NONE;
        if (k < activeW) {
            uint16_t t = wc.tagForFrame(reg, SELF_SHORT, k);          // the tag I effectively range
            if (t != FC_NONE && reg.anchorRank(t, SELF_SHORT) == curSlot) target = t;
        } else if (probeN > 0 && (uint8_t)((ANCHOR_ID) % wf.slotsPerFrame()) == curSlot) {
            target = probeList[probeIdx % probeN];                     // trailing probe: re-admit/bootstrap
            probeIdx++;
        }
        if (target != FC_NONE) {
            byte sa[2] = { (byte)(target & 0xFF), (byte)((target >> 8) & 0xFF) };
            DW1000Device* d = DW1000Ranging.searchDistantDevice(sa);
            if (d != nullptr) {
                DW1000Ranging.pollDevice(d);
                pollTarget = target; pollStartMs = now;
                lastPolledSlot = wf.slotNumber(now);          // consume this slot's single poll
            }
        }
    }

}
