# mf_DW3000 라이브러리 배치

DW3000 변종(`*_dw3000_*`)이 사용. Makerfabs `Makerfabs-ESP32-UWB-DW3000`의
DW3000 라이브러리(NConcepts 개발)를 이 폴더에 배치한다.

주의:
- DW1000과 API가 완전히 다름 (저수준 dwt_* 또는 자체 래퍼).
- src/common_dw3000/rf_config_dw3000.h의 DWT_* 상수가 이 라이브러리 정의와 맞는지 확인.
- DW3000 변종 main.cpp는 스켈레톤이므로 이 라이브러리의 실제 초기화/DS-TWR 함수로 채울 것.
