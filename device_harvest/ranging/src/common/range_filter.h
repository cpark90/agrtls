/*
 * range_filter.h  (chip-independent)
 *
 * Range smoothing + outlier rejection. Used by FEATURE_FILTERED variants.
 * First-line mitigation of transient jitter from multipath/NLOS (CLAUDE.md item 4).
 *
 * Note: a stronger filter trades off slower response (latency) to real motion.
 */

#ifndef RANGE_FILTER_H
#define RANGE_FILTER_H

#include <math.h>

class RangeFilter {
public:
    // alpha: EMA smoothing factor (0..1, smaller = smoother and slower)
    // outlierThreshold_m: treat as outlier if it deviates from the last estimate by >= this
    RangeFilter(float alpha = 0.3f, float outlierThreshold_m = 0.5f)
        : _alpha(alpha), _threshold(outlierThreshold_m),
          _ema(0.0f), _initialized(false) {}

    // Feed a new measurement and return the filtered range.
    float update(float measured_m) {
        if (!_initialized) {
            _ema = measured_m;
            _initialized = true;
            return _ema;
        }
        // outlier -> ignore this measurement, keep the last estimate
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
