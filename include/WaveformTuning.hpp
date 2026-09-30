#pragma once

#include <cstddef>

namespace Waveform
{

// seconds of playback time the trace spans either side of the playhead
inline constexpr double WINDOW_SECONDS = 1.0;

// minimum per-sample step through the track, keeps a stopped deck's window from collapsing
inline constexpr double MIN_STRIDE = 0.5;

// samples each trace char averages over, also the block size of the loudness reference
inline constexpr std::size_t AVERAGING_FRAMES = 15000;

// block-loudness percentile of the track that renders as full scale
inline constexpr double REFERENCE_PERCENTILE = 0.95;

// seconds for the trace normalizer to close half the gap to a new window max
inline constexpr double NORMALIZER_HALFLIFE = 0.1;

// multiplier on the normalizer at render time, below 1.0 brightens, above darkens, keep > 0
inline constexpr float NORMALIZER_SCALE = 1.0f;

}
