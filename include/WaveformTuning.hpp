#pragma once

#include <cstddef>

namespace Waveform
{

// seconds of *replay* time the trace spans either side of the playhead. the track content that
// covers scales with playback speed (see Deck::extractExpectedWaveform), so faster decks squish
inline constexpr double WINDOW_SECONDS = 1.0;

// smallest per-sample step extractExpectedWaveform takes through the track, ie the most a
// slow/stopped deck zooms in - stops a (near-)zero speed collapsing the window onto one frame
inline constexpr double MIN_STRIDE = 0.1;

// samples each trace char averages over, centered on the char. wider than one char's hop on
// purpose so neighbouring chars overlap and the trace reads smooth. also the block size the
// per-track loudness reference is measured over, so the reference lands on the same scale
inline constexpr std::size_t AVERAGING_FRAMES = 15000;

// which block-loudness percentile of the track renders as full scale: high enough that loud
// passages fill the trace, low enough that one freak transient doesn't dim the whole track
inline constexpr double REFERENCE_PERCENTILE = 1.0;

// seconds for the trace's normalizer (a trailing average of each frame's loudest visible slice)
// to close half the gap to a new window max. time-based rather than a per-frame weight so it
// feels the same at any frame rate - a 0.1 per-frame weight at 60fps works out to ~0.11s.
// shorter follows the music tighter but pumps; longer is steadier but slower to adapt
inline constexpr double NORMALIZER_HALFLIFE = 0.5;

// multiplies the trailing normalizer at render time (the trailing value itself is left alone),
// so the trace brightens up instead of settling dark once the normalizer catches up. relative
// rather than a subtracted offset so it brightens loud and quiet masters alike. below 1.0
// brightens (anything within this fraction of the window max pins at full scale), above 1.0
// darkens; keep it > 0
inline constexpr float NORMALIZER_SCALE = 0.8f;

}
