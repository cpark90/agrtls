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
| `anchor_dw1000_accuracy` | anchor | DW1000 | accuracy | — | 기본 앵커 |
| `anchor_dw1000_lowpower_oled` | anchor | DW1000 | lowpower | OLED | 저전력 + 화면표시 앵커 |
| `tag_dw1000_accuracy` | tag | DW1000 | accuracy | — | 기본 태그 |
| `tag_dw1000_accuracy_wifi` | tag | DW1000 | accuracy | WiFi(UDP) | 원격 전송 태그 |
| `anchor_dw3000_accuracy` | anchor | DW3000 | accuracy | — | DW3000 앵커 (스켈레톤) |
| `tag_dw3000_fast_filtered` | tag | DW3000 | fast | filtered | 고속+필터 태그 (스켈레톤) |

## 새 변종 추가 절차

1. `src/{변종명}/main.cpp` 생성 (가까운 기존 변종을 복사해서 시작)
2. `platformio.ini`에 `[env:{변종명}]` 추가:
   - DW1000이면 `extends = chip_dw1000`, DW3000이면 `extends = chip_dw3000`
   - `build_src_filter`에 `+<common/> +<common_{chip}/> +<{변종명}/>`
   - `build_flags`에 `-D ROLE_{ANCHOR|TAG}`, `-D RFMODE_{...}`, 필요 features 매크로
   - 추가 라이브러리(OLED 등)는 해당 env의 `lib_deps`에 append
3. 이 카탈로그 표에 행 추가
4. `pio run -e {변종명}`으로 빌드 확인

## 칩별 주의

- **DW1000**: `DW1000Ranging` 고수준 콜백 API 사용. `rf_config_dw1000.h` + `applyRfConfigDW1000()`.
- **DW3000**: 라이브러리 API가 다름(저수준 dwt_*). 110kbps 없음, 채널 5/9 사용.
  `rf_config_dw3000.h` 기반. main.cpp는 현재 스켈레톤이므로 실제 라이브러리로 채워야 함.

## 공유되는 칩 독립 코드

- `src/common/logging.h` — CSV 출력 포맷 (모든 변종 공통, 필드 순서 고정)
- `src/common/range_filter.h` — EMA + outlier 필터 (filtered 변종)
