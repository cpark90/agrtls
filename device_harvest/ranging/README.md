# UWB 정밀 위치추적 펌웨어 (PlatformIO, 다중 변종)

Makerfabs ESP32 UWB(DW1000/DW3000) 앵커·태그 펌웨어. 변종을 PlatformIO env로 관리.
설계/규칙은 CLAUDE.md, 변종 목록은 docs/VARIANTS.md 참고.

## 빠른 시작
1. lib/mf_DW1000, lib/mf_DW3000에 각 라이브러리 배치
2. calibration/README.md 절차로 안테나 딜레이 캘리브레이션 → 칩별 rf_config 반영
3. 빌드:
```bash
pio run -e anchor_dw1000_accuracy -t upload
pio run -e tag_dw1000_accuracy -t upload
pio device monitor
```

## 변종
docs/VARIANTS.md 의 표 참고. 네이밍: {role}_{chip}_{rfmode}[_{features}]

## 구조
src/common(칩독립) · src/common_dw1000 · src/common_dw3000 · src/{변종}/ · lib/ · calibration/ · docs/ · tools/
