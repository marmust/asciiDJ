#pragma once

namespace Input
{

// deck speed control (precision mover)
inline const double SPEED_START_VAL = 1.0;
inline const double SPEED_MOVE_SPEED = 0.01;
inline const double SPEED_MIN = 0.0;
inline const double SPEED_MAX = 2.0;

// deck turntable shift / scratch control (momentum mover)
inline const double SHIFT_START_VAL = 0.0;
inline const double SHIFT_MOVE_SPEED = 0.02;
inline const double SHIFT_DECAY_RATE = 0.9;
inline const double SHIFT_MIN = -2.0;
inline const double SHIFT_MAX = 2.0;

// deck volume control (precision mover)
inline const double VOLUME_START_VAL = 1.0;
inline const double VOLUME_MOVE_SPEED = 0.01;
inline const double VOLUME_MIN = 0.0;
inline const double VOLUME_MAX = 1.0;

// EQ band gain control (precision mover)
inline const double EQ_START_VAL = 0.0;
inline const double EQ_MOVE_SPEED = 0.5;
inline const double EQ_MIN = -24.0;
inline const double EQ_MAX = 24.0;

// mixer crossfader control (precision mover)
inline const double XFADER_START_VAL = 0.0;
inline const double XFADER_MOVE_SPEED = 0.02;
inline const double XFADER_MIN = -1.0;
inline const double XFADER_MAX = 1.0;

}
