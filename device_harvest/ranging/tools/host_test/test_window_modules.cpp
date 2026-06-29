// Host unit tests (g++, no radio/Arduino) for the window-TDMA pure modules (W-1).
//   build: g++ -std=c++17 -I ../../src/common test_window_modules.cpp -o tw && ./tw
// Modules: tag_quality / tag_registry / window_color / window_frame
#include <cstdio>
#include <cmath>
#include <cstdint>
#include "tag_quality.h"
#include "tag_registry.h"
#include "window_color.h"
#include "window_frame.h"
#include "mesh_msg.h"

static int g_fail = 0;
#define CHECK(c) do { if(!(c)){ printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); g_fail++; } } while(0)
static bool approx(float a, float b){ return std::fabs(a-b) < 1e-3f; }

// Example from SYSTEM_OVERVIEW (theta_link = -85 dBm):
//   T0: A0(-70) A1(-72) A2(-75)
//   T1: A2(-78) A3(-74) A4(-80)
//   T2: A4(-76) A5(-82)
//   T3: A0(-90)  -> below theta_link, no eligible anchor -> candidate
static void fillRegistry(TagRegistry& r) {
    r.begin();
    r.report(0,0,-70,1); r.report(1,0,-72,1); r.report(2,0,-75,1);   // T0
    r.report(2,1,-78,1); r.report(3,1,-74,1); r.report(4,1,-80,1);   // T1
    r.report(4,2,-76,1); r.report(5,2,-82,1);                         // T2
    r.report(0,3,-90,1);                                              // T3 (weak)
}

static void test_quality() {
    CHECK(linkEligible(-70, 1));        // >= -85
    CHECK(!linkEligible(-90, 1));       // < -85
    CHECK(approx(linkQuality(-72, 5), -72));   // first-cut = rxp
}

static void test_registry() {
    TagRegistry r; fillRegistry(r);
    uint16_t buf[8];

    CHECK(r.effectiveAnchorCount(0) == 3);
    CHECK(r.effectiveAnchorCount(2) == 2);
    CHECK(r.effectiveAnchorCount(3) == 0);          // T3 weak -> candidate

    CHECK(r.conflict(0, 1));    // share A2
    CHECK(r.conflict(1, 2));    // share A4
    CHECK(!r.conflict(0, 2));   // no shared effective anchor

    CHECK(approx(r.meanQuality(0), (-70 - 72 - 75) / 3.0f));
    CHECK(r.meanQuality(3) < -1e8f);                // candidate -> sentinel

    // update overwrites
    r.report(0, 0, -60, 1);
    uint16_t ea = r.effectiveAnchors(0, buf, 8);
    CHECK(ea == 3);
}

static void test_color() {
    TagRegistry r; fillRegistry(r);
    WindowColor wc; wc.build(r, /*maxWindows*/ 8);

    // T0 & T2 don't conflict -> same window; T1 differs
    CHECK(wc.colorOf(0) == wc.colorOf(2));
    CHECK(wc.colorOf(0) != wc.colorOf(1));
    CHECK(wc.numWindows() == 2);
    CHECK(wc.isCandidate(3));                        // T3 weak -> candidate
    uint16_t cand[4]; uint8_t nc = wc.candidates(cand, 4);
    CHECK(nc == 1 && cand[0] == 3);

    // anchor A4 ranges T1 (color of T1) and T2 (color of T2): one tag per window
    uint8_t wT1 = wc.colorOf(1), wT2 = wc.colorOf(2);
    CHECK(wc.tagForWindow(r, 4, wT1) == 1);
    CHECK(wc.tagForWindow(r, 4, wT2) == 2);
    // A0 only ranges T0
    CHECK(wc.tagForWindow(r, 0, wc.colorOf(0)) == 0);
    CHECK(wc.tagForWindow(r, 0, wT1) == WC_NONE);    // A0 has no tag in T1's window
    printf("  windows: T0=%u T1=%u T2=%u (n=%u)\n", wc.colorOf(0), wc.colorOf(1), wc.colorOf(2), wc.numWindows());
}

static void test_color_cap() {
    // 3 mutually-conflicting tags but only 2 windows -> lowest-priority tag becomes candidate.
    TagRegistry r; r.begin();
    r.report(0,10,-60,1); r.report(0,11,-70,1); r.report(0,12,-80,1);   // all share anchor 0 -> clique
    WindowColor wc; wc.build(r, /*maxWindows*/ 2);
    CHECK(wc.numWindows() == 2);
    // highest quality (T10) and next (T11) get windows; T12 (weakest) overflows -> candidate
    CHECK(!wc.isCandidate(10) && !wc.isCandidate(11));
    CHECK(wc.isCandidate(12));
}

static void test_frame() {
    WindowFrame f;
    f.begin({2 /*windows*/, 3 /*slots*/, 100 /*slotLen*/, 10 /*guard*/});
    CHECK(!f.isMyTurn(1000, 0, 0));        // unsynced
    f.setEpoch(1000);
    CHECK(f.windowLenMs() == 300);
    CHECK(f.superframeLenMs() == 600);
    CHECK(f.windowIndexNow(1000) == 0);    // phase 0 -> window 0
    CHECK(f.slotIndexNow(1000) == 0);
    CHECK(!f.isWork(1000));                // leading guard
    CHECK(f.isWork(1050));                 // slot 0 work
    CHECK(f.isMyTurn(1050, 0, 0));         // window 0, slot 0, work
    CHECK(f.slotIndexNow(1150) == 1);      // phase 150 -> slot 1
    CHECK(f.isMyTurn(1150, 0, 1));
    CHECK(f.windowIndexNow(1350) == 1);    // phase 350 -> window 1, slot 0
    CHECK(f.isMyTurn(1350, 1, 0));
    CHECK(f.windowIndexNow(1600) == 0);    // wrap (1000 + 600)
}

// W-2: TAGINFO message (registry share over the mesh)
static void test_taginfo() {
    // eligible is the owner's authoritative decision (carried on the wire)
    TagInfoEntry in[3] = {{0, -70.5f, 1.23f, true}, {1, -82.0f, 4.56f, true}, {2, -90.25f, 0.0f, false}};
    uint8_t buf[MESH_MAX_FRAME];
    uint8_t len = packTagInfo(7, in, 3, buf);
    CHECK(meshMsgType(buf, len) == MESH_TAGINFO);

    uint16_t aid; TagInfoEntry out[8]; uint8_t n;
    CHECK(unpackTagInfo(buf, len, aid, out, 8, n));
    CHECK(aid == 7 && n == 3);
    CHECK(out[0].tagId == 0 && approx(out[0].rxp_dBm, -70.5f) && approx(out[0].range_m, 1.23f) && out[0].eligible);
    CHECK(out[1].tagId == 1 && approx(out[1].rxp_dBm, -82.0f) && approx(out[1].range_m, 4.56f) && out[1].eligible);
    CHECK(out[2].tagId == 2 && approx(out[2].rxp_dBm, -90.25f) && !out[2].eligible);

    // apply a received TAGINFO into a registry as a PEER (eligibility adopted verbatim)
    TagRegistry r; r.begin();
    for (uint8_t i = 0; i < n; i++) r.reportPeer(aid, out[i].tagId, out[i].rxp_dBm, out[i].range_m, out[i].eligible);
    CHECK(r.effectiveAnchorCount(0) == 1);   // owner said eligible
    CHECK(r.effectiveAnchorCount(1) == 1);   // owner said eligible
    CHECK(r.effectiveAnchorCount(2) == 0);   // owner said NOT eligible

    CHECK(!unpackTagInfo(buf, 3, aid, out, 8, n));   // truncated
    uint8_t v[MESH_MAX_FRAME]; ValueMsg vm{1,2,3,4}; packValue(vm, v);
    CHECK(!unpackTagInfo(v, 10, aid, out, 8, n));     // wrong type
}

// aging + anchor-slot rank + dynamic slot count
static void test_aging_slots() {
    TagRegistry r; r.begin();
    // T0 effective anchors A0,A1,A2 (by id); A3 weak -> not effective
    r.report(0, 0, -70, 1, 1000); r.report(1, 0, -72, 1, 1000); r.report(2, 0, -75, 1, 1000);
    r.report(3, 0, -90, 1, 1000);
    // anchor-slot = rank among effective anchors (id order); weak anchor -> 0xFF
    CHECK(r.anchorRank(0, 0) == 0);
    CHECK(r.anchorRank(0, 1) == 1);
    CHECK(r.anchorRank(0, 2) == 2);
    CHECK(r.anchorRank(0, 3) == 0xFF);
    CHECK(r.maxEffectiveAnchorCount() == 3);

    // add T1 with 2 effective anchors -> max stays 3
    r.report(4, 1, -70, 1, 1000); r.report(5, 1, -71, 1, 1000);
    CHECK(r.maxEffectiveAnchorCount() == 3);

    // aging: not stale within TTL, dropped past it
    r.expire(2000, 5000);                 // 1000ms old -> keep
    CHECK(r.effectiveAnchorCount(0) == 3);
    r.expire(7000, 5000);                 // 6000ms old -> all expire
    CHECK(r.effectiveAnchorCount(0) == 0 && r.maxEffectiveAnchorCount() == 0);

    // nowMs==0 entries (default, e.g. non-aging tests) are never expired
    TagRegistry r2; r2.begin();
    r2.report(0, 0, -70, 1);              // no timestamp
    r2.expire(1000000, 5000);
    CHECK(r2.effectiveAnchorCount(0) == 1);
}

int main() {
    test_quality();
    test_registry();
    test_color();
    test_color_cap();
    test_frame();
    test_taginfo();
    test_aging_slots();
    if (g_fail == 0) printf("ALL WINDOW TESTS PASSED\n");
    else             printf("%d CHECK(S) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
