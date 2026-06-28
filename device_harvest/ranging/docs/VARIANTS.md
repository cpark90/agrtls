# 변종 카탈로그 (Variants)

이 프로젝트의 모든 앵커/태그 변종 목록. 새 변종을 추가하면 이 표를 갱신할 것.

## 네이밍 규칙

```
{role}_{chip}_{rfmode}[_{features...}]
```
- `role`: `anchor` | `tag`
- `chip`: `dw1000` | `dw3000`
- `rfmode`: `accuracy` | `lowpower` | `fast`
- `features` (선택, 복수): `oled` | `wifi` | `filtered` ...

폴더명 = env명 = 변종명. 셋이 항상 일치한다.

## 현재 변종 목록

| 변종 (env) | role | chip | rfmode | features | 용도 |
|---|---|---|---|---|---|
| `anchor_dw1000_accuracy_meshagent` | anchor | DW1000 | accuracy | mesh, agent | CORE mesh-TDMA 앵커 = **initiator**(태그를 스케줄 폴). HW 검증됨 |
| `tag_dw1000_responder` | tag | DW1000 | accuracy | — | CORE mesh-TDMA 태그 = **responder**(폴에 응답만). 주소는 `-D TAG_ID=n` |
| `anchor_dw3000_accuracy` | anchor | DW3000 | accuracy | — | DW3000 앵커 (스켈레톤) |
| `tag_dw3000_fast_filtered` | tag | DW3000 | fast | filtered | 고속+필터 태그 (스켈레톤) |

> 구 브로드캐스트 DW1000 변종(`anchor_dw1000_accuracy`, `tag_dw1000_accuracy`,
> `..._lowpower_oled`, `..._accuracy_wifi`)은 mesh-TDMA 역할반전 변종으로 대체되어 제거됨.

## CORE mesh-TDMA 역할 반전 (주의)

`*_meshagent` / `*_responder` 변종은 **물리 명칭과 mf-DW1000 역할이 뒤바뀐다**:
- 물리 **앵커**(`anchor_..._meshagent`) = **initiator** — `startAsInitiator()` 로 시작, 태그를 폴.
- 물리 **태그**(`tag_..._responder`) = **responder** — `startAsResponder()` 로 시작, 폴에 응답만.

`startAsInitiator()/startAsResponder()` 는 `startAsTag()/startAsAnchor()` 의 가독성 별칭(동작 동일).
근거·설계는 `docs/ARCHITECTURE_mesh_tdma.md`, `docs/DESIGN_FLOW_mesh_tdma.md`(Pivot 4~5) 참고.
스케줄 폴링은 라이브러리 `setScheduledMode(true)` + `pollDevice()` 로 구동(하위호환 가드드).

## 새 변종 추가 절차

1. `src/{변종명}/main.cpp` 생성 (가까운 기존 변종을 복사해서 시작)
2. `platformio.ini`에 `[env:{변종명}]` 추가:
   - DW1000이면 `extends = chip_dw1000`, DW3000이면 `extends = chip_dw3000`
   - `build_src_filter`에 `+<common/> +<common_{chip}/> +<{변종명}/>`
   - `build_flags`에 `-D ROLE_{ANCHOR|TAG}`, `-D RFMODE_{...}`, 필요 features 매크로
   - 추가 라이브러리(OLED 등)는 해당 env의 `lib_deps`에 append
3. 이 카탈로그 표에 행 추가
4. `pio run -e {변종명}`으로 빌드 확인

## Short Address Assignment Rule

DW1000에서 `startAsAnchor/Tag(addr, mode, false)` 호출 시 short address는
EUI-64의 **첫 두 바이트**에서 파생된다 (`short_addr = byte[1]*256 + byte[0]`).
`byte[1]`을 항상 `0x00`으로 고정하면 `short_addr = byte[0]`.

```
EUI-64 포맷: "BB:00:XX:XX:XX:XX:XX:XX"
              └┘ └┘
              │  고정(0x00)
              └── short address 결정 바이트 (BB)
```

| 역할 | byte[0] 범위 | short addr | 표현 | EUI-64 예 |
|------|-------------|------------|------|-----------|
| Anchor n | `0x00 + n` (0x00–0x3F) | n (0–63) | `"A{n}"` | `"0n:00:5B:D5:A9:9A:E2:9C"` |
| Tag n | `0x80 + n` (0x80–0xBF) | 128+n (128–191) | `"T{n}"` | `"(80+n):00:22:EA:82:60:3B:9C"` |

### 현재 변종별 주소 할당

| 변종 | 역할 | 번호 | byte[0] | EUI-64 |
|------|------|------|---------|--------|
| `anchor_dw1000_accuracy_meshagent` | Anchor(initiator) | 0 | `0x00` | `"00:00:5B:D5:A9:9A:E2:9C"` |
| `tag_dw1000_responder` (`-D TAG_ID=n`) | Tag(responder) | n | `0x80+n` | `"(80+n):00:22:EA:82:60:3B:9C"` |

태그는 보드마다 `-D TAG_ID=n` 으로 byte[0]=0x80+n 을 주입 → 다중 태그 검증 시 T0/T1/T2…

### logRange deviceId 생성

`newRange()` 콜백에서 distant device의 short address를 `shortAddrToId()`로 변환:

```cpp
char devId[8];
shortAddrToId(d->getShortAddress(), devId, sizeof(devId));
logRange(devId, d->getRange(), d->getRXPower());
```

- 앵커에서 호출 시: distant device = 태그 → `"T0"`, `"T1"`, ...
- 태그에서 호출 시: distant device = 앵커 → `"A0"`, `"A1"`, ...

`shortAddrToId()`는 `src/common/logging.h`에 정의.

## 칩별 주의

- **DW1000**: `DW1000Ranging` 고수준 콜백 API 사용. `rf_config_dw1000.h` + `applyRfConfigDW1000()`.
- **DW3000**: 라이브러리 API가 다름(저수준 dwt_*). 110kbps 없음, 채널 5/9 사용.
  `rf_config_dw3000.h` 기반. main.cpp는 현재 스켈레톤이므로 실제 라이브러리로 채워야 함.

## 공유되는 칩 독립 코드

- `src/common/logging.h` — CSV 출력 포맷 (모든 변종 공통, 필드 순서 고정)
- `src/common/range_filter.h` — EMA + outlier 필터 (filtered 변종)
