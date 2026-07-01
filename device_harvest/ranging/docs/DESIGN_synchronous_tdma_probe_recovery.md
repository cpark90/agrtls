# DESIGN: synchronous TDMA probe/recovery 안정화

`anchor_dw1000_synchronous` (+ `tag_dw1000_responder`)에서 all-ineffective 상태의
재-bootstrap 불능과 slot당 중복 poll을 해결한 기록. 관련: `ARCHITECTURE_synchronous_tdma.md`,
`DESIGN_dstwr_multianchor.md`.

## 1. 증상

3-anchor + 1-tag 구성에서 관측:
- 모든 anchor의 tag link가 ineffective가 된 뒤, 설계상 epoch master 하나만 probe해야 하는데
  두 anchor가 동시에 range를 찍음.
- tag를 멀리 보내 all-ineffective를 일정 시간 유지하면 registry의 self-link이 사라지고
  (`myRxp=-200`), **이후 신호를 강하게 해도 복구되지 않음**. 부팅 직후에는 정상 동작.
- latch 수정 후 재-bootstrap은 되나, 비-master anchor에서 **slot당 poll이 1회를 초과**.

## 2. Root cause

### 2-1. probe-master election이 두 소스로 갈라짐

probe frame의 단일 TX 주체 선정이 `myAnchorSlot() == 0`(= `knownAnchors` 중 최저 id)로
되어 있었으나, 주석의 의도와 SYNC epoch 정렬은 `epochMaster()`(= 최저 effective anchor,
없으면 최저 known)였다. 두 값은 다를 수 있다.

또한 `knownAnchors`는 **`MESH_SYNC`에서만** `noteAnchor()`로 학습되고 aging이 없었다.
`MESH_TAGINFO`(registry를 채우는 경로)는 peer의 `aid`를 `noteAnchor`하지 않았다. 따라서
한 anchor가 다른 anchor의 SYNC만 놓치면(TAGINFO는 수신) `knownAnchors`가 불완전해져
자신을 master로 오판 → **두 anchor가 동시에 probe TX** → POLL 충돌/음수 range.

### 2-2. poll latch가 1×1 스케줄에서 영구 고정 (stuck의 직접 원인)

"one poll per slot" latch가 `slotKey = (frameIdx<<8 | slotIdx)`로 keying되어 있었다.
all-ineffective면 `activeW=0` → `numFrames=1`, `slotsPerFrame=1`(1 frame × 1 slot)이라
`frameIdx=slotIdx=0`이 **영구 고정** → `slotKey` 불변 → `polledHere`가 리셋 안 됨.

결과: master가 probe를 **딱 한 번** 쏘고 latch가 굳음. 그 한 번이 실패하면 재시도 없음 →
registry 미충전 → 영구 stuck. 부팅은 그 한 번이 성공(근거리)해 `activeW≥1`로 스케줄이
커지며 latch가 순환하기 시작하므로 동작. link 상실 후 1×1 붕괴 시점의 단 한 번이 실패하면
복구 불가 = "부팅은 되는데 복구는 안 됨".

계측으로 배제한 오해들: device-list eviction(축출되지만 재발견 churn 정상), registry
saturation(`regN=0`, 포화 아님), RF/discovery(강신호·`blink/riRx/ndev` 정상), overhear
(stuck 상태에서 tag가 ranging frame을 안 뱉으니 overhear할 것이 없음). 병목은 **master가
`pollDevice`를 아예 재호출하지 않는 것**(`pollTx` 정지)이었고, 원인이 위 latch 고정.

### 2-3. latch 교체 시 resync 중복 poll

1×1을 고치려 latch를 monotonic `wf.slotNumber()`로 바꾸면, **비-master는 master SYNC마다
`setEpoch`로 epoch를 재정렬**하므로 `slotNumber = (now-epoch)/slotLen`가 점프 → 같은 slot
안에서 latch가 리셋되어 **slot당 2회 poll**. work-window rising-edge(off≈guard) 방식도
resync가 phase를 slot 시작 부근에 떨어뜨리면 자연 edge로 오인해 동일하게 샜다
(관측: `dt=108ms<slotLen`, `ph=10`).

여러 anchor가 같은 SYNC로 동시에 resync → 동시 중복 poll → 같은 tag에 충돌 위험.

## 3. 수정 (src/anchor_dw1000_synchronous/main.cpp)

### 3-1. probe-master 통일 + anchor-set 학습 경로 통일
- `MESH_TAGINFO` 핸들러에서 `noteAnchor(aid)` 호출 → anchor set을 registry 경로에서도 학습.
  SYNC 유실만으로 election이 갈라지지 않음.
- probe 조건을 `myAnchorSlot() == 0` → `epochMaster() == SELF_SHORT`로. 주석/실동작/ SYNC
  정렬이 모두 같은 master를 가리킴. dead code가 된 `myAnchorSlot()` 제거.

### 3-2. poll latch를 role-split 키로 재설계
```c
uint32_t slotKey = (epochMaster() == SELF_SHORT) ? wf.slotNumber(now)          // master
                                                 : (((uint32_t)k << 8) | curSlot);  // non-master
if (slotKey != curSlotKey) { curSlotKey = slotKey; polledHere = false; }
```
두 실패 모드가 배타적이라 role로 분기한다.
- **master**: 스스로 resync 안 함(epoch 기준점) → `slotNumber` 단조 → 1×1에서도 매 slot
  probe 재시도 → 재-bootstrap 가능.
- **non-master**: resync로 `slotNumber` 비단조지만 `(frameIdx,slotIdx)`는 phase-relative라
  resync에 연속 → 중복 poll 없음. non-master는 1×1에서 poll하지 않으므로 slotNumber의
  1×1 이점이 불필요.

## 4. 검증
- 정상 ranging: 비-master 포함 slot당 poll 1회(`dt≈superframeLen`), 중복 없음.
- all-ineffective 유도 후 강신호 복귀: 재-bootstrap 정상.
- `pio run` 5개 dw1000 변종 전부 SUCCESS(라이브러리·common 무변경, main.cpp만 수정).

## 5. 남은 개선 (미착수)
- **discovery/BLINK storm**: scheduledMode에서도 라이브러리 `timerTick`의 자율 BLINK가 계속
  돌아 tag가 매 blink에 RANGING_INIT로 응답 → device add/remove churn. 이번 latch 수정으로
  stuck은 해소됐으나 churn 자체는 남음. probe frame에 명시적(app-driven) discovery 창을
  두어 BLINK/RANGING_INIT을 TDMA에 편입하는 설계가 후속 과제.
- **epoch master flapping**: `epochMaster()`가 `lowestEffectiveAnchor()`에 직결이라 최저 id
  anchor의 eligibility가 흔들리면 master가 진동. hold/hysteresis 미적용(별도 과제).
