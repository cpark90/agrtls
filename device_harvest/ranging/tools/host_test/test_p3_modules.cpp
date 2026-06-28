// 호스트 단위테스트 (g++, 라디오/Arduino 없음) — CORE 1차 순수 모듈 검증.
//   build: g++ -std=c++17 -I ../../src/common test_p3_modules.cpp -o t && ./t
// 4개 모듈: superframe / peer_scheduler / interference / mgm_agent
#include <cstdio>
#include <cmath>
#include <cstdint>
#include "superframe.h"
#include "peer_scheduler.h"
#include "interference.h"
#include "mgm_agent.h"

static int g_fail = 0;
#define CHECK(cond) do { if(!(cond)){ printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); g_fail++; } } while(0)
static bool approx(float a, float b){ return std::fabs(a-b) < 1e-4f; }

static void test_superframe() {
    Superframe sf;
    sf.begin({4, 100, 10});                 // 4 slots × 100ms, guard 10
    CHECK(!sf.isMyWorkWindow(1015, 0));     // 미동기 → 닫힘
    sf.setEpoch(1000);
    CHECK(sf.synced());
    CHECK(sf.superframeLenMs() == 400);
    CHECK(sf.slotIndexNow(1000) == 0);
    CHECK(!sf.isMyWorkWindow(1000, 0));     // 선행 guard
    CHECK(sf.isMyWorkWindow(1015, 0));      // work
    CHECK(sf.workRemainingMs(1015, 0) == 75);
    CHECK(!sf.isMyWorkWindow(1095, 0));     // 후행 guard
    CHECK(sf.slotIndexNow(1150) == 1);
    CHECK(!sf.isMyWorkWindow(1150, 0));
    CHECK(sf.isOthersSlot(1150, 0));        // slot1 work, 내 슬롯0 아님 → overhear 창
    CHECK(sf.isMyWorkWindow(1415, 0));      // wrap (1000+400+15)
}

static void test_peer_scheduler() {
    PeerScheduler ps;
    ps.begin({1.0f /*rate/ms*/, 8.0f, -82.0f, 5, 0.2f});
    CHECK(ps.addTag(100) && ps.addTag(101) && ps.addTag(102));
    CHECK(ps.tagCount() == 3);

    uint16_t pickAddr = 0;
    CHECK(ps.pick(100, pickAddr) && pickAddr == 100);   // 동점 → 첫 태그
    ps.reportResult(100, 100, true, 1.0f, -60.0f);      // good
    CHECK(ps.pick(200, pickAddr) && pickAddr == 101);   // 100은 방금 폴 → 뒤로

    CHECK(approx(ps.demand(), 3.0f));                   // 전부 weight 1
    ps.reportResult(102, 200, true, 20.0f, -60.0f);     // far → bad streak 1
    CHECK(approx(ps.weightOf(102), 0.8f));
    CHECK(approx(ps.demand(), 2.8f));
    for (int k = 0; k < 10; k++) ps.reportResult(102, 200, true, 20.0f, -60.0f);
    CHECK(approx(ps.weightOf(102), 0.2f));              // 바닥 weightMin
    ps.reportResult(102, 300, true, 1.0f, -60.0f);      // 회복
    CHECK(approx(ps.weightOf(102), 1.0f));

    CHECK(ps.removeTag(101) && ps.tagCount() == 2);
}

static void test_interference() {
    InterferenceGraph ig;
    ig.begin(/*myId*/1, /*lease*/1000, /*thresh*/-90.0f);
    uint16_t myTags[] = {100, 101};
    ig.setMyTags(myTags, 2);

    uint16_t t2[] = {101, 200};
    ig.onTagList(2, t2, 2, 0);          // 101 겹침 → shared edge with 2
    ig.onOverheard(3, -80.0f, 0);       // > thresh → audible with 3
    ig.onOverheard(4, -95.0f, 0);       // < thresh → 무시
    uint16_t heard5[] = {1, 9};
    ig.onAudibleReport(5, heard5, 2, 0);  // 나(1) 포함 → 5 가 나를 들음 → 이웃
    uint16_t heard6[] = {9};
    ig.onAudibleReport(6, heard6, 1, 0);  // 나 없음 → 무시
    ig.tick(0);

    CHECK(ig.neighborCount() == 3);
    CHECK(ig.isNeighbor(2) && ig.isNeighbor(3) && ig.isNeighbor(5));
    CHECK(!ig.isNeighbor(4) && !ig.isNeighbor(6));

    uint16_t aud[8];
    uint8_t na = ig.myAudibleList(aud, 8, 0);
    CHECK(na == 1 && aud[0] == 3);      // 로컬로 들은 건 3뿐

    ig.tick(500);                        // 아직 lease 내
    CHECK(ig.neighborCount() == 3);
    ig.tick(2000);                       // lease(1000) 초과 → 전부 만료
    CHECK(ig.neighborCount() == 0);
}

// --- MGM 3-에이전트 클리크 수렴 시뮬레이션 ---
static void test_mgm_clique() {
    const int N = 3;
    uint16_t ids[N] = {10, 20, 30};
    MgmAgent ag[N];
    MgmConfig cfg{4 /*slots*/, 300 /*round*/, 50 /*collect*/, 100000};
    for (int i = 0; i < N; i++) ag[i].begin(ids[i], cfg);

    struct Pkt { bool isVal; ValueMsg v; GainMsg g; int from; };
    for (uint32_t now = 0; now <= 6000; now += 10) {
        Pkt buf[2 * N]; int nb = 0;
        for (int i = 0; i < N; i++) {
            uint16_t nbr[N - 1]; int k = 0;
            for (int j = 0; j < N; j++) if (j != i) nbr[k++] = ids[j];
            ag[i].setNeighbors(nbr, N - 1);
            ag[i].tick(now);
            ValueMsg v; GainMsg g;
            if (ag[i].pendingValue(v)) buf[nb++] = Pkt{true, v, {}, i};
            if (ag[i].pendingGain(g))  buf[nb++] = Pkt{false, {}, g, i};
        }
        for (int p = 0; p < nb; p++)
            for (int i = 0; i < N; i++) if (i != buf[p].from) {
                if (buf[p].isVal) ag[i].onValue(buf[p].v, now);
                else              ag[i].onGain(buf[p].g, now);
            }
    }

    // 충돌-프리(전부 다른 슬롯) + HELD 검증
    for (int i = 0; i < N; i++) {
        CHECK(ag[i].state() == MgmState::HELD);
        CHECK(ag[i].conflictCount() == 0);
    }
    bool distinct = ag[0].slot() != ag[1].slot() &&
                    ag[1].slot() != ag[2].slot() &&
                    ag[0].slot() != ag[2].slot();
    CHECK(distinct);
    printf("  MGM slots: %u %u %u\n", ag[0].slot(), ag[1].slot(), ag[2].slot());
}

int main() {
    test_superframe();
    test_peer_scheduler();
    test_interference();
    test_mgm_clique();
    if (g_fail == 0) printf("ALL TESTS PASSED\n");
    else             printf("%d CHECK(S) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
