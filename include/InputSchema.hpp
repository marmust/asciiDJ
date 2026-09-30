#pragma once

#include <linux/input-event-codes.h>

namespace Input
{

// values are evdev key codes, ie physical key positions independent of keyboard layout

// left deck (deck1)
inline const int DECK1_SPEED_UP = KEY_W;
inline const int DECK1_SPEED_DOWN = KEY_Q;
inline const int DECK1_SHIFT_FWD = KEY_S;
inline const int DECK1_SHIFT_BACK = KEY_A;
inline const int DECK1_PAUSE = KEY_X;
inline const int DECK1_VOLUME_UP = KEY_T;
inline const int DECK1_VOLUME_DOWN = KEY_G;
inline const int DECK1_EQ_BASS_UP = KEY_V;
inline const int DECK1_EQ_BASS_DOWN = KEY_C;
inline const int DECK1_EQ_MIDS_UP = KEY_F;
inline const int DECK1_EQ_MIDS_DOWN = KEY_D;
inline const int DECK1_EQ_HIGH_UP = KEY_R;
inline const int DECK1_EQ_HIGH_DOWN = KEY_E;

// right deck (deck2)
inline const int DECK2_SPEED_UP = KEY_P;
inline const int DECK2_SPEED_DOWN = KEY_O;
inline const int DECK2_SHIFT_FWD = KEY_SEMICOLON;
inline const int DECK2_SHIFT_BACK = KEY_L;
inline const int DECK2_PAUSE = KEY_DOT;
inline const int DECK2_VOLUME_UP = KEY_Y;
inline const int DECK2_VOLUME_DOWN = KEY_H;
inline const int DECK2_EQ_BASS_UP = KEY_COMMA;
inline const int DECK2_EQ_BASS_DOWN = KEY_M;
inline const int DECK2_EQ_MIDS_UP = KEY_K;
inline const int DECK2_EQ_MIDS_DOWN = KEY_J;
inline const int DECK2_EQ_HIGH_UP = KEY_I;
inline const int DECK2_EQ_HIGH_DOWN = KEY_U;

// held to snap whichever control is being touched back to its default instead of nudging it
inline const int RESET_MODIFIER_KEY = KEY_LEFTSHIFT;

// mixer
inline const int XFADER_RIGHT = KEY_N;
inline const int XFADER_LEFT = KEY_B;

// file browser: up/down move the selection, left/right load it onto deck1/deck2 in file-select mode
inline const int ARROW_UP = KEY_UP;
inline const int ARROW_DOWN = KEY_DOWN;
inline const int ARROW_LEFT = KEY_LEFT;
inline const int ARROW_RIGHT = KEY_RIGHT;
inline const int FILE_SELECT_TOGGLE = KEY_GRAVE;

}
