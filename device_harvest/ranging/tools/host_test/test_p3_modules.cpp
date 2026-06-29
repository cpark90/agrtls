// Host unit tests (g++, no radio/Arduino) -- verify the pure CORE 1st-scope modules.
//   build: g++ -std=c++17 -I ../../src/common test_p3_modules.cpp -o t && ./t
// 4 modules: superframe / peer_scheduler / interference / mgm_agent
#include <cstdio>
#include <cmath>
#include <cstdint>
#include "superframe.h"
#include "peer_scheduler.h"
#include "interference.h"
#include "mgm_agent.h"
#include "mesh_msg.h"

static int g_fail = 0;
#define CHECK(cond) do { if(!(cond)){ printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); g_fail++; } } while(0)
static bool approx(float a, float b){ return std::fabs(a-b) < 1e-4f; }

static void test_superframe() {
    Superframe sf;
    sf.begin({4, 100, 10});                 // 4 slots x 100ms, guard 10
    CHECK(!sf.isMyWorkWindow(1015, 0));     // unsynced -> closed
    sf.setEpoch(1000);
    CHECK(sf.synced());
    CHECK(sf.superframeLenMs() == 400);
    CHECK(sf.slotIndexNow(1000) == 0);
    CHECK(!sf.isMyWorkWindow(1000, 0));     // leading guard
    CHECK(sf.isMyWorkWindow(1015, 0));      // work
    CHECK(sf.workRemainingMs(1015, 0) == 75);
    CHECK(!sf.isMyWorkWindow(1095, 0));     // trailing guard
    CHECK(sf.slotIndexNow(1150) == 1);
    CHECK(!sf.isMyWorkWindow(1150, 0));
    CHECK(sf.isOthersSlot(1150, 0));        // slot1 work, not my slot 0 -> overhear window
    CHECK(sf.isMyWorkWindow(1415, 0));      // wrap (1000+400+15)
}

static void test_peer_scheduler() {
    PeerScheduler ps;
    ps.begin({1.0f /*rate/ms*/, 8.0f, -82.0f, 5, 0.2f});
    CHECK(ps.addTag(100) && ps.addTag(101) && ps.addTag(102));
    CHECK(ps.tagCount() == 3);

    uint16_t pickAddr = 0;
    CHECK(ps.pick(100, pickAddr) && pickAddr == 100);   // tie -> first tag
    ps.reportResult(100, 100, true, 1.0f, -60.0f);      // good
    CHECK(ps.pick(200, pickAddr) && pickAddr == 101);   // 100 just polled -> to the back

    CHECK(approx(ps.demand(), 3.0f));                   // all weight 1
    ps.reportResult(102, 200, true, 20.0f, -60.0f);     // far -> bad streak 1
    CHECK(approx(ps.weightOf(102), 0.8f));
    CHECK(approx(ps.demand(), 2.8f));
    for (int k = 0; k < 10; k++) ps.reportResult(102, 200, true, 20.0f, -60.0f);
    CHECK(approx(ps.weightOf(102), 0.2f));              // floor weightMin
    ps.reportResult(102, 300, true, 1.0f, -60.0f);      // recovery
    CHECK(approx(ps.weightOf(102), 1.0f));

    CHECK(ps.removeTag(101) && ps.tagCount() == 2);
}

static void test_interference() {
    InterferenceGraph ig;
    ig.begin(/*myId*/1, /*lease*/1000, /*thresh*/-90.0f);
    uint16_t myTags[] = {100, 101};
    ig.setMyTags(myTags, 2);

    uint16_t t2[] = {101, 200};
    ig.onTagList(2, t2, 2, 0);          // 101 overlaps -> shared edge with 2
    ig.onOverheard(3, -80.0f, 0);       // > thresh -> audible with 3
    ig.onOverheard(4, -95.0f, 0);       // < thresh -> ignored
    uint16_t heard5[] = {1, 9};
    ig.onAudibleReport(5, heard5, 2, 0);  // contains me (1) -> 5 hears me -> neighbor
    uint16_t heard6[] = {9};
    ig.onAudibleReport(6, heard6, 1, 0);  // no me -> ignored
    ig.tick(0);

    CHECK(ig.neighborCount() == 3);
    CHECK(ig.isNeighbor(2) && ig.isNeighbor(3) && ig.isNeighbor(5));
    CHECK(!ig.isNeighbor(4) && !ig.isNeighbor(6));

    uint16_t aud[8];
    uint8_t na = ig.myAudibleList(aud, 8, 0);
    CHECK(na == 1 && aud[0] == 3);      // only 3 was heard locally

    ig.tick(500);                        // still within lease
    CHECK(ig.neighborCount() == 3);
    ig.tick(2000);                       // past lease (1000) -> all expire
    CHECK(ig.neighborCount() == 0);
}

// --- MGM 3-agent clique convergence simulation ---
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

    // Verify collision-free (all distinct slots) + HELD
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

static void test_mesh_msg() {
    uint8_t buf[MESH_MAX_FRAME];

    // VALUE round-trip
    ValueMsg v{7, 0x0042, 3, 123456}, v2{};
    uint8_t n = packValue(v, buf);
    CHECK(n == 10 && meshMsgType(buf, n) == MESH_VALUE);
    CHECK(unpackValue(buf, n, v2));
    CHECK(v2.round == 7 && v2.agentId == 0x0042 && v2.slot == 3 && v2.leaseExpiry == 123456u);

    // GAIN round-trip (incl. negative gain)
    GainMsg g{9, 0x00AB, (int16_t)-5, 2}, g2{};
    n = packGain(g, buf);
    CHECK(n == 8 && meshMsgType(buf, n) == MESH_GAIN);
    CHECK(unpackGain(buf, n, g2));
    CHECK(g2.round == 9 && g2.agentId == 0x00AB && g2.gain == -5 && g2.propSlot == 2);

    // id-list round-trip (TAGLIST)
    uint16_t ids[] = {0x80, 0x81, 0x82};
    n = packIdList(MESH_TAGLIST, 0x0001, ids, 3, buf);
    CHECK(meshMsgType(buf, n) == MESH_TAGLIST);
    uint16_t agentId = 0; uint16_t out[8]; uint8_t cnt = 0;
    CHECK(unpackIdList(buf, n, agentId, out, 8, cnt));
    CHECK(agentId == 0x0001 && cnt == 3 && out[0] == 0x80 && out[1] == 0x81 && out[2] == 0x82);

    // SYNC round-trip
    n = packSync(0x0000, 314159, buf);
    CHECK(n == 7 && meshMsgType(buf, n) == MESH_SYNC);
    uint16_t sid = 0xFFFF; uint32_t phase = 0;
    CHECK(unpackSync(buf, n, sid, phase));
    CHECK(sid == 0x0000 && phase == 314159u);

    // wrong type / truncated
    CHECK(!unpackValue(buf, n, v2));          // buf holds a SYNC, not VALUE
    CHECK(!unpackIdList(buf, 2, agentId, out, 8, cnt));  // too short
}

int main() {
    test_superframe();
    test_peer_scheduler();
    test_interference();
    test_mgm_clique();
    test_mesh_msg();
    if (g_fail == 0) printf("ALL TESTS PASSED\n");
    else             printf("%d CHECK(S) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
