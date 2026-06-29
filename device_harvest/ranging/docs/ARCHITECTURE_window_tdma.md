# Architecture: Window-based Two-Level TDMA (authoritative)

Consolidated design after the requirement was clarified to **localization-grade ranging**: for a given tag, all of its anchor ranges must be **temporally clustered** so they can be used together in a position fix. This supersedes the earlier anchor-centric design in [`ARCHITECTURE_mesh_tdma.md`](./ARCHITECTURE_mesh_tdma.md) (kept as history).

> Written in English for clarity (consistent with the other design docs).

---

## 1. Confirmed requirements

| # | Requirement |
|---|---|
| R1 | **Anchor is the initiator** (anchors poll; tags respond). |
| R2 | One measurement **window = exactly one tag**; in that window all anchors range that same tag. |
| R3 | A tag's ranges (from all anchors) must be **clustered in time** → used for localization. |
| R4 | The **range is obtained at the tag side** (tag ends up with its ranges to all anchors). |
| R5 | **Anchor↔anchor slot coordination** (within a window) is fair — **no** far/weak priority. |
| R6 | **far/weak tags are deprioritized** — measured in fewer windows. far/weak is judged **by the anchors** (from RXP/range). |
| R7 | Tags may be **out of communication range of each other** yet **share anchors** (hidden terminal). So tags cannot self-coordinate; the **anchors mediate**. |
| R8 | Anchors are numerous; tags are numerous. Both move (occasional relocation), power may toggle. |

## 2. Key consequence of R7 (hidden tags)

Two tags that cannot hear each other but share an anchor will collide at that anchor if both transmit. They cannot coordinate directly. The common point is the **anchors**, and the anchor is the initiator (R1), so **the anchor mesh owns the whole schedule**:

- All anchors agree that **window k is for tag `T_k`** → only `T_k` is polled/active in window k → no other tag transmits → **no hidden-tag collision**, and `T_k`'s ranges are clustered (R2, R3).
- Tags are pure responders; they do **not** self-schedule.

## 3. Structure — window-based two-level TDMA

```
Superframe = a sequence of WINDOWS (one tag each), repeating.
   schedule:  window 0 -> T_a,  window 1 -> T_b,  window 2 -> T_a, ...   (far/weak tags appear less)

Window(T_x) = a sequence of ANCHOR-SLOTS:
   [anchor 0 polls T_x][anchor 1 polls T_x]...   (fair order; anchors don't collide)

Result: every anchor ranges T_x inside Window(T_x) -> T_x's ranges are clustered.
```

Two coordination jobs, **both on the anchor mesh**:

| Job | Mechanism | Priority |
|---|---|---|
| **Window → tag schedule** | shared tag registry + weights → deterministic schedule | **far/weak ↓** (R6) |
| **Anchor-slot order** (within a window) | distributed coloring (MGM) | fair (R5) |

## 4. Window assignment: distributed tag→window coloring (+ far/weak weighting)

The window→tag assignment is a **distributed graph coloring of tags into windows**, computed by the anchor mesh from a shared tag registry. This subsumes the multi-cluster question and handles overlaps/spatial-reuse automatically.

### 4.1 Conflict graph and coloring
- Node = tag. **Edge (conflict) iff two tags share an _effective_ anchor** — an anchor that *actually ranges both* (each link passes `θ_link`, §4.3a), so it would have to serve both in one window. An anchor that merely *hears* both but ranges only one (the other link far/weak) is **not** a conflict point. Judging overlap by **effective anchors, not all hearing anchors**, avoids over-counting conflicts → more spatial reuse.
- Color = window. **Conflicting tags get different windows; non-conflicting tags reuse the same window** → spatial reuse (disjoint anchor regions measure concurrently).
- Computed by **distributed coloring (MGM)** over the anchor mesh (same machinery as the anchor-slot coloring, applied to tags), using the shared registry (which anchor hears which tags).
- Per anchor this means **exactly one tag per window** (its tags all have distinct colors) → never double-booked. A border tag has one color → all its anchors (across regions) measure it in that one window → ranges clustered (R3).

### 4.2 Clusters are emergent, not configured
- A "cluster" = anchors that (transitively) share tags — **determined by physical placement, discovered at runtime**: each anchor publishes the tags it hears (`TAGINFO`); "share a tag" = a coordination edge. No manual cluster assignment, no cluster IDs.
- Overlapping **anchors** are shared nodes that **bridge** regions of the conflict graph; overlapping **tags** simply get a single color. Coordination is **local** (a tag's color need only differ from tags it conflicts with) → windows needed = local tag density (max tags sharing one anchor), not the global tag count. Mobility/power churn → registry refresh → coloring re-converges locally.

### 4.3 far/weak via link quality: anchor selection + window priority
Everything keys off a per **anchor–tag link quality** `q(a, T)` (higher = nearer/stronger), which is **pluggable** (first-cut `q = rxp`; later add range/NLOS/geometry) and isolated from the scheduler.

**(a) Anchor selection within a window** — *which anchors in the cluster range this tag*: in tag T's window, anchor `a` ranges T only if `q(a,T) ≥ θ_link` (a good link). Anchors whose link to T is far/weak **skip it** → only good ranges are collected (accurate, useful for localization) and no airtime is wasted on unreliable links. This is **orthogonal to the fair anchor-slot order** (§3, R5): slots are fair; *selection* is by link quality.

**(b) Window priority between conflicting tags** — *which tag gets a contested window*: compare using **only their shared _effective_ anchors** (anchors that range *both* — same as the conflict definition in §4.1; apples-to-apples, calibration-robust), with a **count-normalized** aggregate so the number of anchors does not bias it:
- `W̄_S(T) = mean_{s ∈ sharedEffectiveAnchors(T1,T2)} q(s, T)` — the **mean** shared-anchor quality, **not** a sum. A raw sum would unfairly favor tags/clusters with more anchors (e.g. 10 weak links could out-sum 3 strong ones); the mean reflects far/weak independent of anchor count.
- higher `W̄_S` → near/strong → more windows; lower → far/weak → fewer.
- the aggregate is **pluggable** (mean first-cut; later median / k-th-best for robustness). All shared anchors compute the same value (same registry+formula) → consistent decision.

```
// tag_quality.h  (pure, swappable)
inline float linkQuality(float rxp_dBm, float range_m);  // higher = nearer/stronger; first-cut: = rxp_dBm
```

### 4.4 Time
`window index k = (now - epoch) / windowLen`, epoch gossip-synced **locally** (within conflict neighborhoods). Reuse regions need not be globally aligned (they don't interfere). Transient registry disagreement self-heals as it converges; rate-limit registry changes to minimize churn.

### 4.5 Candidate-only windows (no permanent exclusion / anti-starvation)
Anchor selection (§4.3a) can leave a tag with **no eligible anchor**. That tag is not dropped — it is kept as a **candidate** so it can re-enter when it moves closer / a link improves:

- **Candidate trigger**: if **every anchor in the cluster is excluded** for a tag (no link passes `θ_link` — the tag is far/weak from all of them), that tag's window becomes **candidate-only**: not normally measured, but **kept in the registry as a candidate** (cluster formation still tracks it), never deleted.
- **Probe (anti-starvation)**: each candidate is still measured at a **minimum rate** — at least one window per `K_probe` superframes. The probe re-measures all of the tag's links.
- **Re-entry**: if a probe shows some link is now good (`q ≥ θ_link`), the tag returns to active. So exclusion is reversible and follows mobility.
- `θ_link`, `K_probe` are pluggable alongside `linkQuality` (§4.3) — simple first-cut, easily tuned.

## 5. Measurement flow (one window)

1. Window k opens (epoch-synced). All anchors know `T_k` from the shared schedule.
2. Each anchor, in **its anchor-slot** (fair order), polls `T_k` (TWR). Anchor-slots are offset so the anchors' polls/exchanges don't overlap.
3. `T_k` responds to each anchor; the tag obtains its range to each anchor (R4) — all within window k → **clustered** (R3).
4. Next window → `T_{k+1}` per the schedule.

(Anchor-initiated TWR; the tag computes/collects ranges from the exchanges, or anchors report the range to the tag. SS-TWR or DS-TWR per the detailed design.)

## 6. Roles / coordination domains

| Entity | Role | Coordinates | Over |
|---|---|---|---|
| Anchor | initiator + infrastructure | window schedule (far/weak) **and** anchor-slot order (fair) | anchor mesh (ESP-NOW) |
| Tag | responder; collects its ranges | nothing (mediated by anchors) | — |

There is **no tag mesh** (tags can't reliably talk — R7). All coordination is anchor-side.

## 7. Reuse from F-a / F-b

- **ESP-NOW mesh** (anchor↔anchor) — carries `TAGINFO`, MGM `VALUE/GAIN`, `SYNC`.
- **epoch gossip sync** — now aligns **windows** across anchors (not just slots).
- **MGM / interference** — assigns the fair **anchor-slot** order within a window.
- **peer_scheduler far/weak logic** — repurposed to compute **per-tag weight → window allocation**.
- **mf-DW1000 single-poll hooks** — anchor polls one tag (`T_k`) per slot.

## 8. Changes vs the current (F-a/F-b) implementation

- **Now**: each anchor independently polls *its own* discovered tags → a tag's ranges are spread across the superframe (NOT clustered). ✗ for localization.
- **New**: all anchors poll the **same** `T_k` per window → that tag's ranges are clustered. ✓
- Add: shared tag registry + weights, deterministic window→tag schedule, window timing layer.
- Keep: anchor mesh, epoch sync, MGM anchor-slot, single-poll.

## 9. Implementation plan (incremental)

- **W-1**: pure modules (host-testable):
  - `tag_registry.h` — merge per-anchor `(tagId, rxp, range)` reports; expose each tag's **effective** anchors (links passing `θ_link`) and the **effective shared** anchor set of any tag pair.
  - `window_color.h` conflict = two tags share an **effective** anchor (§4.1).
  - `tag_quality.h` — pluggable `linkQuality(rxp, range)` (first-cut: `= rxp`); anchor selection by `θ_link`; `W̄_S` (mean, count-normalized) comparison of two tags over their shared anchors.
  - `window_color.h` — distributed tag→window coloring (conflict = share-anchor), weighted by `W_S` (far/weak → fewer windows); candidate-only windows + `K_probe` anti-starvation; `tagForWindow(k)`.
  - extend `superframe.h` with the window / anchor-slot two-level timing.
- **W-2**: mesh `TAGINFO` message (tagId + RXP) in `mesh_msg.h`; share/merge over ESP-NOW.
- **W-3**: meshagent rework — poll `T_k` (shared) instead of own tags; anchor-slot offset within window.
- **W-4**: HW validation — multiple anchors + multiple tags (some far): confirm (a) a tag's ranges are clustered, (b) far tags measured less, (c) hidden tags don't collide.

## 10. Open parameters

windowLen, anchor-slot length, superframe/schedule length `L`, link-quality `linkQuality` (RXP/range→quality), anchor-selection threshold `θ_link`, candidate probe rate `K_probe`, registry update/rate-limit period, epoch-sync period.

## 11. Responder layer & W-4 hardware validation (구현/검증 완료)

§1~9는 스케줄러(앵커) 계층의 설계다. 실제 HW 검증(W-4)에서 **응답기(물리 태그) 계층**이 stock
mf-DW1000 데모의 단일-피어 수동 설계라 영구배제 방지 불변식을 깨는 것이 드러나 함께 보강했다.

### 11.1 응답기 계층 (mf_DW1000, `_type==ANCHOR`)
역할 반전 모델에서 물리 태그가 응답기다. 여러 앵커가 한 태그를 측정(클러스터링)하려면 응답기가
다중 initiator를 동시 보유·서비스해야 한다. 보강 내용:

- **다중 device 보유**: `addNetworkDevices()`의 "1 TAG" 리셋 제거(+ `MAX_DEVICES` 바운드). 응답기가
  여러 앵커를 동시에 등록.
- **per-device 프로토콜 상태**: `_expectedMsgId`/`_protocolFailed`를 `DW1000Device`로 이동. 여러
  앵커의 POLL→RANGE 교환이 인터리브돼도 서로 깨지지 않음(초기자 경로는 전역 상태 유지).
- **admit-on-POLL (+ 타겟 체크)**: 미지 initiator가 **self를 타겟한** POLL을 보내면 즉시 (재)등록.
  aged-out된 앵커가 BLINK 없이 재진입 → 영구배제 해소(§4.5의 candidate 원칙을 라디오 계층에서 보장).
  엿들은(다른 태그 대상) POLL로는 등록하지 않음 → device table churn 방지.
- **short-address dedup**: 등록 식별자를 short 주소로 통일(admit-on-POLL과 BLINK 중복 엔트리 방지).

### 11.2 앵커 폴링/윈도우 (anchor_dw1000_window)
- **동적 윈도우 수**: 프레임이 고정 N윈도우가 아니라 `activeW = wc.numWindows()` + **후행 probe
  윈도우 1개**만 순회(빈 윈도우 미순회). probe 윈도우 유무를 공유-결정적(`activeW+1`)으로 두어
  앵커 간 superframe 길이를 일치시킴(epoch 동기 유지).
- **probe/부트스트랩 대상** = "SELF가 아직 effective가 **아닌** 발견 태그"(bootstrap=미측정,
  global candidate, 약링크 재탐색 모두 포함). 이게 없으면 한 번도 측정 안 한 active 태그를 영영
  못 잰다(effective여야 active 윈도우에서 폴 → 데드락).
- **슬롯당 1폴**: greedy 다중 폴 제거 → 교환이 슬롯 경계를 넘지 않음. `slotsPerWindow`는 작게
  유지(superframe < 응답기 inactivity timeout). 5~10 앵커 확장 시 anchor-slot coloring + inactivity
  timeout 재조정 필요(향후).
- **타임아웃 시 가짜 -120 보고 금지**: 일시적 타임아웃 한 번이 좋은 링크를 candidate로 뒤집어
  coloring/epoch을 흔들던 문제 제거.

### 11.3 검증 결과 (앵커 2 + 태그 2)
- ✓ **클러스터링**: 두 앵커가 같은 태그(T0)를 모두 측정. 영구배제 없음(약한 T1도 probe로 측정).
- ✓ device table churn 최소화, per-device 상태로 동시 교환 안정.
- ⚠ **남은 1건 — link-quality flap**: 임계값 근처(RXP mean ≈ −85, 편차 −67~−102 멀티패스) 링크에서
  first-cut `linkQuality`(raw RXP)가 샘플마다 eligibility를 뒤집어 coloring이 active↔candidate로
  진동 → superframe 길이가 흔들려 epoch 동기 불안정. **대응: `tag_quality.h`의 pluggable
  `linkQuality`에 EMA/히스테리시스 추가**(§4.3의 "단순하되 개선 가능" 지점).

### 11.4 개발 보조
- `.claude/skills/uwb-bench` — 노드 빌드/업로드(`flash.sh`), 비-TTY 시리얼 캡처(`serial_capture.py`),
  디바이스별 range 집계(`tally.py`). `pio device monitor`가 비대화형에서 TTY를 요구해 실패하므로 사용.
