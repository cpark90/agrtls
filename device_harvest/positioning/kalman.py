"""
칼만 필터 모듈.
1차원(스칼라) 칼만 필터. 검사(check_outlier)와 갱신(accept/reject)을 분리해
여러 채널의 거부 결정을 한꺼번에 내릴 수 있게 했습니다.
"""

import math


class KalmanFilter1D:
    """1차원(스칼라) 칼만 필터 - 평활화 + 아웃라이어 게이팅."""

    def __init__(self, q=0.01, r=0.1, gate_sigma=3.0):
        self.q = q                  # 프로세스 잡음 분산
        self.r = r                  # 측정 잡음 분산
        self.gate_sigma = gate_sigma
        self.x = None               # 추정값(상태)
        self.p = 1.0                # 추정 오차 공분산
        self._p_pred = None         # 마지막 예측 공분산 캐시
        self._innov = None          # 마지막 혁신(innovation) 캐시

    def check_outlier(self, z):
        """상태를 바꾸지 않고 아웃라이어 여부만 판단."""
        if self.x is None:
            # 미초기화 상태: 첫 값은 무조건 수용
            self._p_pred = self.p + self.q
            self._innov = 0.0
            return False
        p_pred = self.p + self.q
        innov = z - self.x
        s = p_pred + self.r                 # 혁신 분산
        self._p_pred = p_pred
        self._innov = innov
        return abs(innov) > self.gate_sigma * math.sqrt(s)

    def accept(self, z):
        """정상 측정값으로 상태 갱신."""
        if self.x is None:
            self.x = z
            self.p = 1.0
            return self.x
        p_pred = self._p_pred
        k = p_pred / (p_pred + self.r)      # 칼만 이득
        self.x += k * self._innov
        self.p = (1 - k) * p_pred
        return self.x

    def reject(self):
        """아웃라이어 거부: 갱신 안 하고 불확실성만 누적(게이트가 점점 넓어짐)."""
        if self._p_pred is not None:
            self.p = self._p_pred
        return self.x
