# Host unit tests (CORE 1st-scope pure modules)

라디오/Arduino 없이 `src/common`의 순수 모듈을 호스트(g++)에서 검증한다.
대상: `superframe.h`, `peer_scheduler.h`, `interference.h`, `mgm_agent.h`.

## 실행
```bash
g++ -std=c++17 -Wall -Wextra -I ../../src/common test_p3_modules.cpp -o t && ./t
```
기대 출력: `ALL TESTS PASSED` (그리고 MGM 수렴 슬롯 출력).

포함 테스트:
- superframe: 슬롯/work-window/guard/wrap/미동기
- peer_scheduler: 에이징 폴 순서, far/weak 가중치 하향, demand
- interference: shared-tag·audible(로컬/보고) 융합, lease 만료, myAudibleList
- mgm_agent: 3-에이전트 클리크가 충돌-프리 슬롯으로 수렴(HELD)
