# CLAUDE.md

이 파일은 이 리포지토리에서 작업하는 Claude Code를 위한 컨텍스트와 규칙을 정의합니다.

## 프로젝트 개요

Makerfabs ESP32 UWB 기반 정밀 위치추적 펌웨어. **여러 앵커/태그 변종**을 PlatformIO 다중
환경(env)으로 관리한다. 각 변종은 거의 독립 구현이며, 변종별 폴더 + 변종별 env로 격리한다.
향후 IMU 센서퓨전(EKF)은 펌웨어 외부 시스템에서 처리하므로, 펌웨어는 그에 맞는 출력 포맷만 갖춘다.

**하드웨어**: ESP32 + DW1000(Makerfabs ESP32 UWB Pro) **및** DW3000(Makerfabs ESP32 UWB DW3000)
**빌드 시스템**: PlatformIO (arduino-cli 아님)
**라이브러리**: DW1000 → mf-DW1000(`lib/mf_DW1000`), DW3000 → Makerfabs DW3000 lib(`lib/mf_DW3000`)

## 변종(Variant) 시스템 — 가장 중요

변종 = 독립 소스 폴더 + 독립 env. **전체 목록과 추가 절차는 `docs/VARIANTS.md`가 단일 진실 공급원.**
새 변종 작업 시 항상 VARIANTS.md를 먼저 읽고, 변종을 추가/수정하면 VARIANTS.md도 갱신할 것.

네이밍: `{role}_{chip}_{rfmode}[_{features}]` (예: `tag_dw3000_fast_filtered`)
폴더명 = env명 = 변종명, 셋이 항상 일치.

### 빌드 / 업로드
```bash
pio run -e anchor_dw1000_accuracy           # 특정 변종 빌드
pio run -e tag_dw1000_accuracy_wifi -t upload
pio device monitor                          # 115200
pio run                                       # default_envs(대표 변종)만
```
- 변종마다 `extends`로 칩 베이스(`chip_dw1000`/`chip_dw3000`)를 상속받고,
  `build_src_filter`로 `common/` + `common_{chip}/` + `{변종폴더}/`만 컴파일한다.
- 역할/모드/기능은 매크로로 주입: `ROLE_ANCHOR|ROLE_TAG`, `RFMODE_ACCURACY|LOWPOWER|FAST`,
  `FEATURE_OLED|WIFI|FILTERED`.

### 디렉토리 구조
```
platformio.ini        모든 변종 env + 칩 베이스 (단일 설정)
/src
  /common             칩 독립 공통: logging.h, range_filter.h
  /common_dw1000      DW1000 전용: rf_config_dw1000.h (+ applyRfConfigDW1000)
  /common_dw3000      DW3000 전용: rf_config_dw3000.h
  /{변종명}/main.cpp  각 변종 (VARIANTS.md 참고)
/lib
  /mf_DW1000          DW1000 라이브러리 (로컬)
  /mf_DW3000          DW3000 라이브러리 (로컬)
/calibration  /docs  /tools
```

## 칩별 핵심 차이 (반드시 인지)

DW1000과 DW3000은 **라이브러리/API/RF 파라미터가 다르다.** 한쪽 코드를 다른 쪽에 그대로 쓰지 말 것.

| 항목 | DW1000 | DW3000 |
|---|---|---|
| 라이브러리 | mf-DW1000 (DW1000Ranging 고수준 콜백) | Makerfabs DW3000 (NConcepts, 저수준 dwt_*) |
| 데이터레이트 | 110k / 850k / 6.8M | 850k / 6.8M (110k 없음) |
| 주 채널 | 2 (또는 1/3/5) | 5(6.5GHz) / 9(8GHz) |
| 안테나딜레이 기본 | 16384 | 16385 (별도 캘리브레이션) |
| RF 설정 헤더 | rf_config_dw1000.h | rf_config_dw3000.h |

- DW3000 변종 main.cpp는 현재 **스켈레톤**(TODO 주석). 실제 라이브러리 함수로 채워야 함.
- DW3000은 DW1000Ranging 같은 고수준 콜백이 없을 수 있어, DS-TWR 상태머신을 직접 구성해야 할 수 있음.

## 이미 결정된 기술 사항 (재검토 말고 전제로)

### RF 모드
- ACCURACY(정확도 우선): DW1000은 110k/64MHz/2048preamble. DW3000은 850k 기반.
- 앵커·태그의 RF 파라미터 4종(채널/코드/데이터레이트/프리앰블)은 반드시 일치해야 통신됨.
- DW1000 변종은 `applyRfConfigDW1000()` 공용 함수로 설정해 불일치를 구조적으로 차단.

### 안테나 딜레이 캘리브레이션
- 모든 정확도 문제의 1차 의심 대상. 기본값(16384/16385)을 프로덕션에 쓰지 말 것.
- 모듈 개체별 + 칩별 + Pro/일반별로 값이 다를 수 있음. 재사용 금지.
- 미캘리브레이션 기본값이면 rf_config 헤더가 `#warning` 발생. 절차/기록은 `calibration/`.

### RX Power / Range Bias
- `getRXPower()`(DW1000) 값을 항상 거리와 함께 로깅. logging.h의 CSV 포맷 유지.
- range bias lookup table(`BIAS_500_16/64`)이 실제 적용되는지 확인.
- Pro 근거리 RX Power가 -40dBm 이상이면 새추레이션 의심 → TX power 하향.

### 멀티패스/NLOS
- 움직이는 물체로 인한 출렁임은 멀티패스+NLOS로 간주.
- 1차 대응은 `range_filter.h`(EMA+outlier). 강한 필터는 반응속도 저하 트레이드오프.
- 채널 1/2/3/5 대역폭 동일 → 채널보다 앵커 높이/배치/필터 우선.

### 멀티 디바이스
- mf-DW1000 데모는 time-multiplexing 미지원. 다중 태그 확장 시 직접 스케줄링 필요.

## 출력 포맷 (외부 EKF 연동)
- logging.h의 CSV: `deviceId,range_m,rxPower_dBm,timestamp_ms,nlosFlag`
- 필드 순서 변경 금지, 새 필드는 뒤에 append.
- 펌웨어에 EKF/IMU 융합 로직을 넣지 않음 (외부 시스템 범위).

## 코딩 규칙
- 핀/RF 상수는 칩별 common(common_dw1000/common_dw3000)에만. 변종에서 중복 정의 금지.
- 칩 독립 로직(필터/로깅)은 `src/common`에 두고 모든 변종이 공유.
- 매직 넘버 대신 named constant.
- 새 변종은 가까운 기존 변종을 복사해 시작하고, VARIANTS.md + platformio.ini를 같이 갱신.

## 검증 체크리스트 (변경 후 항상)
1. 변경된 변종의 env가 빌드되는가? `pio run -e {변종명}`
2. common 또는 칩별 common을 건드렸다면, 그 레이어를 쓰는 **모든 변종**을 빌드했는가?
3. 앵커/태그 짝의 RF 파라미터(채널/코드/데이터레이트/프리앰블)가 일치하는가?
4. 안테나 딜레이가 캘리브레이션 값인가 (기본값 방치 금지)?
5. 로그에 RX Power가 거리와 함께 출력되는가?
6. VARIANTS.md가 실제 변종 목록과 일치하는가?

## Claude에게 당부
- 변종 작업 전 항상 `docs/VARIANTS.md`를 읽을 것.
- common/칩별 common 수정 시 영향받는 모든 변종을 빌드 검증할 것.
- DW1000 코드와 DW3000 코드를 혼동하지 말 것 (API 다름).
- RF 파라미터 추천 시 "정확도 vs 전력/속도" 트레이드오프 명시.
- 정확도 이슈는 안테나 딜레이/캘리브레이션을 가장 먼저 의심.
- 코드 수정은 변경 전/후 diff로 제시.
- arduino-cli 명령 사용 금지. PlatformIO 기준.
- 주석과 .md 파일 작성에만 한글을 사용하고 이외에는 영어로 작성.
