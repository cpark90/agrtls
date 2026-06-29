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

int main() {
    test_quality();
    test_registry();
    test_color();
    test_color_cap();
    test_frame();
    if (g_fail == 0) printf("ALL WINDOW TESTS PASSED\n");
    else             printf("%d CHECK(S) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
