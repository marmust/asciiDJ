#pragma once

#include <X11/keysym.h>

namespace Input
{

// keys are ints rather than chars so non-ASCII keys (arrows, modifiers) can be represented -
// values are X11 KeySyms, which equal plain ASCII for printable keys (XK_a == 'a', etc) so every
// existing binding below keeps its literal char value unchanged

// left deck (deck1)
inline const int DECK1_SPEED_UP = 'w';
inline const int DECK1_SPEED_DOWN = 'q';
inline const int DECK1_SHIFT_FWD = 's';
inline const int DECK1_SHIFT_BACK = 'a';
inline const int DECK1_PAUSE = 'x';
inline const int DECK1_VOLUME_UP = 't';
inline const int DECK1_VOLUME_DOWN = 'g';
inline const int DECK1_EQ_BASS_UP = 'v';
inline const int DECK1_EQ_BASS_DOWN = 'c';
inline const int DECK1_EQ_MIDS_UP = 'f';
inline const int DECK1_EQ_MIDS_DOWN = 'd';
inline const int DECK1_EQ_HIGH_UP = 'r';
inline const int DECK1_EQ_HIGH_DOWN = 'e';

// right deck (deck2)
inline const int DECK2_SPEED_UP = 'p';
inline const int DECK2_SPEED_DOWN = 'o';
inline const int DECK2_SHIFT_FWD = ';';
inline const int DECK2_SHIFT_BACK = 'l';
inline const int DECK2_PAUSE = '.';
inline const int DECK2_VOLUME_UP = 'y';
inline const int DECK2_VOLUME_DOWN = 'h';
inline const int DECK2_EQ_BASS_UP = ',';
inline const int DECK2_EQ_BASS_DOWN = 'm';
inline const int DECK2_EQ_MIDS_UP = 'k';
inline const int DECK2_EQ_MIDS_DOWN = 'j';
inline const int DECK2_EQ_HIGH_UP = 'i';
inline const int DECK2_EQ_HIGH_DOWN = 'u';

// mixer
inline const int XFADER_RIGHT = 'n';
inline const int XFADER_LEFT = 'b';

// file browser: up/down move the selection, left/right load it onto deck1/deck2 (while
// FILE_SELECT_TOGGLE has switched into file-select mode)
inline const int ARROW_UP = XK_Up;
inline const int ARROW_DOWN = XK_Down;
inline const int ARROW_LEFT = XK_Left;
inline const int ARROW_RIGHT = XK_Right;
inline const int FILE_SELECT_TOGGLE = '`';

}
