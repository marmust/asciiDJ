#pragma once

namespace Input
{

// MOVE_SPEED values are units of travel PER SECOND (they used to be per poll pass, back when
// the loop ran at a fixed rate). converted by multiplying the old per-pass figures by the
// ~814Hz the throttled loop actually achieved, so the feel carries over unchanged; the full
// travel time each one implies is noted next to it, tune from there

// held to snap whichever control is being touched back to its default instead of nudging it
inline const char RESET_MODIFIER_KEY = ' ';

// deck speed control (precision mover)
inline const double SPEED_START_VAL = 1.0;
inline const double SPEED_MOVE_SPEED = 2.0;      // 0..2 in 0.25s
inline const double SPEED_MIN = -3.0;
inline const double SPEED_MAX = 3.0;

// deck turntable shift / scratch control (momentum mover)
inline const double SHIFT_START_VAL = 0.0;
inline const double SHIFT_MOVE_SPEED = 64.0;     // -2..2 in 0.25s
// seconds for a released scratch to fall halfway back to 0. the old 0.9-per-pass decay worked
// out to a 6.58 pass half-life, ie ~8ms at the rate the loop was running - near instant snap back
inline const double SHIFT_DECAY_HALFLIFE = 0.02;
inline const double SHIFT_MIN = -3.0;
inline const double SHIFT_MAX = 3.0;

// deck volume control (precision mover)
inline const double VOLUME_START_VAL = 1.0;
inline const double VOLUME_MOVE_SPEED = 2.0;     // 0..1 in 0.13s
inline const double VOLUME_MIN = 0.0;
inline const double VOLUME_MAX = 1.0;

// EQ band gain control (precision mover)
inline const double EQ_START_VAL = 0.0;
inline const double EQ_MOVE_SPEED = 10.0;       // -24..24 in 0.12s
inline const double EQ_MIN = -24.0;
inline const double EQ_MAX = 24.0;

// mixer crossfader control (precision mover)
inline const double XFADER_START_VAL = 0.0;
inline const double XFADER_MOVE_SPEED = 2.0;    // -1..1 in 0.13s
inline const double XFADER_MIN = -1.0;
inline const double XFADER_MAX = 1.0;

}
