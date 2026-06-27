/*
 * range_filter.h  (칩 독립)
 *
 * 거리값 평활화 + outlier 제거. FEATURE_FILTERED 변종에서 사용.
 * 멀티패스/NLOS로 인한 순간 출렁임을 1차 완화한다 (CLAUDE.md 4번).
 *
 * 주의: 필터가 강할수록 실제 이동에 대한 반응(latency)이 느려지는 트레이드오프.
 */

#ifndef RANGE_FILTER_H
#define RANGE_FILTER_H

#include <math.h>

class RangeFilter {
public:
    // alpha: EMA 평활 계수(0~1, 작을수록 매끄럽고 느림)
    // outlierThreshold_m: 직전 추정치 대비 이 값 이상 벗어나면 outlier로 간주
    RangeFilter(float alpha = 0.3f, float outlierThreshold_m = 0.5f)
        : _alpha(alpha), _threshold(outlierThreshold_m),
          _ema(0.0f), _initialized(false) {}

    // 새 측정값을 넣고 필터링된 거리를 반환.
    float update(float measured_m) {
        if (!_initialized) {
            _ema = measured_m;
            _initialized = true;
            return _ema;
        }
        // outlier면 이번 측정 무시하고 직전 추정 유지
        if (fabsf(measured_m - _ema) > _threshold) {
            return _ema;
        }
        _ema = _alpha * measured_m + (1.0f - _alpha) * _ema;
        return _ema;
    }

    void reset() { _initialized = false; _ema = 0.0f; }

private:
    float _alpha;
    float _threshold;
    float _ema;
    bool  _initialized;
};

#endif // RANGE_FILTER_H
