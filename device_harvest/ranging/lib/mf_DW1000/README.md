# mf_DW1000 라이브러리 배치

DW1000 변종(`*_dw1000_*`)이 사용. Makerfabs `Makerfabs-ESP32-UWB`의 mf_DW1000 소스를
이 폴더에 배치한다 (library.json 또는 library.properties + src/ 포함).

확인:
- `BIAS_500_16`/`BIAS_500_64` range bias 테이블 포함 및 적용 여부 (CLAUDE.md)
- `setAntennaDelay()` 지원 여부 (없으면 Jim Remington 캘리브레이션 포크 사용)
