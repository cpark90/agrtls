# Host unit tests (pure modules)

라디오/Arduino 없이 `src/common`의 순수 모듈을 호스트(g++)에서 검증한다.

## 실행
```bash
# F-a/F-b 모듈
g++ -std=c++17 -Wall -Wextra -I ../../src/common test_p3_modules.cpp -o t  && ./t
# synchronous TDMA 모듈 (W-1)
g++ -std=c++17 -Wall -Wextra -I ../../src/common test_window_modules.cpp -o tw && ./tw
```
기대 출력: `ALL TESTS PASSED` / `ALL FRAME TESTS PASSED`.

## test_p3_modules — F-a/F-b 모듈
- superframe: 슬롯/work-window/guard/wrap/미동기
- peer_scheduler: 에이징 폴 순서, far/weak 가중치 하향, demand
- interference: shared-tag·audible(로컬/보고) 융합, lease 만료, myAudibleList
- mgm_agent: 3-에이전트 클리크가 충돌-프리 슬롯으로 수렴(HELD)
- mesh_msg: VALUE/GAIN/TAGLIST/AUDIBLE/SYNC 직렬화 라운드트립

## test_window_modules — synchronous TDMA 모듈 (docs/ARCHITECTURE_synchronous_tdma.md)
- tag_quality: θ_link 적격성, 1차 품질(=RXP)
- tag_registry: 유효앵커, 충돌(유효앵커 공유), meanQuality, meanSharedQuality
- frame_color: 태그→윈도우 컬러링(충돌=유효앵커 공유, 비충돌 재사용), 상한 초과·무자격 → candidate
- frame_schedule: 2계층(frame×anchor-slot) 타이밍/동기
