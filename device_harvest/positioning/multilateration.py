"""
측위 모듈.
가우스-뉴턴 비선형 최소제곱으로 위치를 추정하고(2D/3D 공용),
좌표를 축별 1D 칼만으로 평활화합니다.
"""

import numpy as np

from kalman import KalmanFilter1D
import config


def multilaterate(anchor_pos, ranges, init):
    """가우스-뉴턴 비선형 최소제곱으로 위치 추정 (차원 무관: 2D/3D 공용).

    anchor_pos: (M, D) 앵커 좌표 (D=2 또는 3)
    ranges:     (M,)   측정 거리
    init:       (D,)   초기 추정 위치
    반환:       (위치 (D,), 잔차 RMS)
    """
    p = np.array(init, dtype=float)
    D = p.shape[0]
    M = len(ranges)
    for _ in range(config.GN_MAX_ITER):
        diff = p - anchor_pos               # (M,D)
        d = np.linalg.norm(diff, axis=1)    # (M,) 추정 거리
        d_safe = np.where(d < 1e-6, 1e-6, d)
        residual = d - ranges               # (M,)
        J = diff / d_safe[:, None]          # (M,D) 자코비안
        # 정규방정식 (J^T J) dp = -J^T r
        # 1e-9 정규화 항: 앵커가 한 직선/평면에 몰려 특이에 가까울 때 발산 방지
        JtJ = J.T @ J
        Jtr = J.T @ residual
        try:
            dp = np.linalg.solve(JtJ + 1e-9 * np.eye(D), -Jtr)
        except np.linalg.LinAlgError:
            break
        p = p + dp
        if np.linalg.norm(dp) < config.GN_TOL:
            break
    # 최종 잔차 RMS (품질 지표: 크면 거리들이 서로 모순 → NLOS/이상 앵커 의심)
    d = np.linalg.norm(p - anchor_pos, axis=1)
    rms = float(np.sqrt(np.mean((d - ranges) ** 2))) if M > 0 else None
    return p, rms


# 하위 호환용 별칭 (기존 3D 호출부)
def multilaterate_3d(anchor_pos, ranges, init):
    return multilaterate(anchor_pos, ranges, init)


class PosKalmanND:
    """위치 좌표 평활화 (좌표축마다 독립 1D 칼만). 차원 D는 생성 시 지정."""

    def __init__(self, dim):
        # 위치는 거리보다 게이트를 넉넉히(5σ) 잡아 정상 이동을 막지 않음
        self.dim = dim
        self.kf = [
            KalmanFilter1D(config.POS_Q, config.POS_R, gate_sigma=5.0)
            for _ in range(dim)
        ]

    def update(self, p):
        out = np.empty(self.dim)
        for i in range(self.dim):
            if self.kf[i].check_outlier(p[i]):
                self.kf[i].reject()
            out[i] = self.kf[i].accept(p[i])
        return out


# 하위 호환용 별칭
class PosKalman3D(PosKalmanND):
    def __init__(self):
        super().__init__(3)
