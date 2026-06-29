/*
 * tag_quality.h  (chip-independent, pure)
 *
 * Pluggable per anchor-tag LINK quality used by the window-TDMA scheduler (docs/ARCHITECTURE_window_tdma.md).
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

// First-cut: quality = RXP. Range is accepted for future use (kept out of the first-cut formula).
inline float linkQuality(float rxp_dBm, float range_m) {
    (void)range_m;
    return rxp_dBm;
}

// Anchor selection: is this anchor-tag link good enough for the anchor to range the tag?
inline bool linkEligible(float rxp_dBm, float range_m) {
    return linkQuality(rxp_dBm, range_m) >= TQ_LINK_THRESH;
}

#endif // TAG_QUALITY_H
