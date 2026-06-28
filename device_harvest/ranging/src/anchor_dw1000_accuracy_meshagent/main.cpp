/*
 * 변종: anchor_dw1000_accuracy_meshagent
 *
 * 물리적 "앵커"이지만 mf-DW1000 에서는 initiator(startAsTag) 역할로 동작한다.
 * 즉 앵커가 주변 태그(responder)를 스케줄에 따라 단일-디바이스로 폴(TWR)한다.
 *   (역할 반전 근거: docs/DESIGN_FLOW_mesh_tdma.md Pivot 4~5)
 *
 * CORE mesh-TDMA 골격 (docs/ARCHITECTURE_mesh_tdma.md, DESIGN_P3_scope1.md):
 *   superframe        : 슬롯 타이밍
 *   peer_scheduler(L4): 내 슬롯 안에서 어느 태그를 폴할지 (우선순위, 최초 요청)
 *   interference (L2) : 간섭 이웃 (UWB overhearing + shared-tag)
 *   mgm_agent    (L3) : 앵커 간 슬롯 배정 (mesh 메시지 교환)
 *
 * F-a 범위: UWB 스케줄 폴링 + L4 우선순위 파이프라인을 실측 검증.
 *   L3/L2 의 mesh 송수신은 아직 스텁(무동작) → ESP-WIFI-MESH 는 F-b 에서 실배선.
 *   mesh 가 없으면 이웃 0 → mgm_agent 는 slot 0 으로 수렴(단일 앵커 동작).
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

// 이 노드 주소 (앵커 #0: byte0=0x00 → short 0x0000 → "A0"). 노드마다 교체.
#define SELF_ADDR "00:00:5B:D5:A9:9A:E2:9C"
static const uint16_t SELF_SHORT = 0x0000;

// 초기 파라미터 (DESIGN_P3_scope1.md §M)
static const SuperframeConfig    SF_CFG  = {16, 45, 5};
static const MgmConfig           MGM_CFG = {16, 500, 200, 3600};
static const PeerSchedulerConfig PS_CFG  = {0.001f, 8.0f, NLOS_RXPOWER_THRESHOLD_DBM, 5, 0.2f};
static const uint32_t LEASE_MS        = 3600;
static const float    AUDIBLE_THRESH  = -90.0f;
// 단일 DS-TWR 한 사이클(POLL→ACK→RANGE→REPORT)은 110kbps ACCURACY에서 reply delay
// 7ms×2 + 긴 프레임으로 ~35~40ms. 타임아웃은 그보다 충분히 커야 진행 중 교환을 안 끊는다.
static const uint32_t POLL_TIMEOUT_MS = 100;
static const uint16_t EXCHANGE_BUDGET = 8;

Superframe        sf;
PeerScheduler     sched;
InterferenceGraph ig;
MgmAgent          agent;

// 진행 중 폴 추적 (단일-폴 in-flight 가드)
static uint16_t pollTarget  = PS_NONE;
static uint32_t pollStartMs = 0;

// ---- mesh 추상화 (F-b 에서 ESP-WIFI-MESH 로 교체) ----
// TODO(F-b): esp_mesh init + 송수신. 수신 측은 ig.onTagList/onAudibleReport,
//            agent.onValue/onGain 을 호출하도록 콜백 연결.
static void meshSendValue(const ValueMsg&) { /* TODO(F-b) */ }
static void meshSendGain(const GainMsg&)   { /* TODO(F-b) */ }

// ---- UWB 콜백 ----
void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    if (d == nullptr) return;
    uint16_t sa = d->getShortAddress();
    float r = d->getRange();
    float p = d->getRXPower();
    char devId[8];
    shortAddrToId(sa, devId, sizeof(devId));
    logRange(devId, r, p);                         // CSV: deviceId,range,rxp,ts,nlos
    sched.reportResult(sa, millis(), true, r, p);  // L4 우선순위 갱신
    if (sa == pollTarget) pollTarget = PS_NONE;    // in-flight 완료
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
    DW1000Ranging.setScheduledMode(true);            // 자동 브로드캐스트 POLL off
    DW1000Ranging.startAsTag(SELF_ADDR, RF_MODE, false);  // initiator 역할
    applyRfConfigDW1000();

    // F-a: mesh 동기 없음 → 로컬 epoch 즉시 설정(단일 노드 검증).
    // TODO(F-b): gossip 슬롯위상 합의로 setEpoch 교체.
    sf.setEpoch(millis());
}

void loop() {
    DW1000Ranging.loop();                 // UWB 상태머신 + overhearing
    uint32_t now = millis();

    // --- L2/L3 (F-a: mesh 스텁 → 이웃 0 → agent slot 0 수렴) ---
    ig.tick(now);
    agent.setNeighbors(ig.neighborIds(), ig.neighborCount());
    agent.tick(now);
    ValueMsg v; GainMsg g;
    if (agent.pendingValue(v)) meshSendValue(v);
    if (agent.pendingGain(g))  meshSendGain(g);

    // --- L4: 내 슬롯 work-window 에서 태그를 우선순위로 단일-폴 ---
    uint8_t mySlot = agent.slot();

    // in-flight 타임아웃 → 실패 기록(badStreak↑, 뒤로 밀기)
    if (pollTarget != PS_NONE && (uint32_t)(now - pollStartMs) > POLL_TIMEOUT_MS) {
        sched.reportResult(pollTarget, now, false, 0.0f, 0.0f);
        pollTarget = PS_NONE;
    }

    // 이웃 없음(단독, F-a) → 슬롯 게이팅 없이 상시 폴. 경쟁 없으면 전 airtime 사용.
    // 이웃 생기면(F-b+) 내 슬롯 work-window 에서만 폴.
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
