/*
 * tag_quality.h  (chip-independent, pure)
 *
 * Pluggable per anchor-tag LINK quality used by the synchronous TDMA scheduler (docs/ARCHITECTURE_synchronous_tdma.md).
 * Isolated so the formula can be improved later (range / NLOS / geometry / history) without touching
 * the registry, coloring, or scheduler.
 *
 *   linkQuality(rxp, range) : higher = nearer/stronger.  First-cut = rxp (dBm).
 *   θ_link (TQ_LINK_THRESH) : a link is "eligible" (good enough to range) iff quality >= θ_link.
 */

#ifndef TAG_QUALITY_H
#define TAG_QUALITY_H

#ifndef TQ_LINK_THRESH
#define TQ_LINK_THRESH (-85.0f)   // theta_link, in linkQuality units (dBm for the first-cut)
#endif
#ifndef TQ_EMA_ALPHA
#define TQ_EMA_ALPHA (0.3f)       // RXP smoothing weight (0..1); lower = smoother / slower to react
#endif
#ifndef TQ_HYSTERESIS
#define TQ_HYSTERESIS (3.0f)      // dB band around theta_link; stops eligibility flapping on a link
#endif                            // sitting near the threshold (multipath variance).

// First-cut: quality = RXP. Range is accepted for future use (kept out of the first-cut formula).
inline float linkQuality(float rxp_dBm, float range_m) {
    (void)range_m;
    return rxp_dBm;
}

// Plain (non-hysteretic) threshold check: is this link at/above theta_link right now?
// The live scheduler instead uses the smoothed + hysteretic path (qualityEma + linkEligibleHyst,
// stored per link in TagRegistry); this raw predicate is for one-shot checks / host tests.
inline bool linkEligible(float rxp_dBm, float range_m) {
    return linkQuality(rxp_dBm, range_m) >= TQ_LINK_THRESH;
}

// Temporal smoothing of a quality value (EMA). The registry keeps one smoothed value per link so a
// single noisy sample does not swing the schedule.
inline float qualityEma(float prevQuality, float sampleQuality) {
    return TQ_EMA_ALPHA * sampleQuality + (1.0f - TQ_EMA_ALPHA) * prevQuality;
}

// Hysteretic eligibility: a link must be clearly above theta_link to BECOME eligible, and only drops
// out when clearly below -> a marginal link holds its state instead of flapping (which would flip the
// coloring and desync the frame epoch). Falls back to plain linkEligible when no prior state.
inline bool linkEligibleHyst(float quality, bool prevEligible) {
    return prevEligible ? (quality >= TQ_LINK_THRESH - TQ_HYSTERESIS)
                        : (quality >= TQ_LINK_THRESH + TQ_HYSTERESIS);
}

#endif // TAG_QUALITY_H
