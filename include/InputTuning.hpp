#pragma once

namespace Input
{

// MOVE_SPEED values are units of travel per second, full travel times noted alongside

// deck speed control (precision mover)
inline const double SPEED_START_VAL = 1.0;
inline const double SPEED_MOVE_SPEED = 0.3;      // 0..2 in 0.25s
inline const double SPEED_MIN = -3.0;
inline const double SPEED_MAX = 3.0;

// deck turntable shift / scratch control (momentum mover)
inline const double SHIFT_START_VAL = 0.0;
inline const double SHIFT_MOVE_SPEED = 3.0;
// seconds for a released scratch to fall halfway back to 0
inline const double SHIFT_DECAY_HALFLIFE = 0.2;
inline const double SHIFT_MIN = -3.0;
inline const double SHIFT_MAX = 3.0;

// deck volume control (precision mover)
inline const double VOLUME_START_VAL = 1.0;
inline const double VOLUME_MOVE_SPEED = 2.0;     // 0..1 in 0.13s
inline const double VOLUME_MIN = 0.0;
inline const double VOLUME_MAX = 1.0;

// EQ band gain control (precision mover)
inline const double EQ_START_VAL = 0.0;
inline const double EQ_MOVE_SPEED = 50.0;
inline const double EQ_MIN = -100.0;
inline const double EQ_MAX = 100.0;

// mixer crossfader control (precision mover)
inline const double XFADER_START_VAL = 0.0;
inline const double XFADER_MOVE_SPEED = 2.0;    // -1..1 in 0.13s
inline const double XFADER_MIN = -1.0;
inline const double XFADER_MAX = 1.0;

}
