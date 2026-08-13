#pragma once

namespace CommandAndControl
{

// band indices, must match the order addEQbands() registers them in
inline const int EQ_BASS_IDX = 0;
inline const int EQ_MIDS_IDX = 1;
inline const int EQ_HIGH_IDX = 2;

// bass band
inline const double EQ_BASS_FREQ = 100.0;
inline const double EQ_BASS_Q = 0.7;

// mids band
inline const double EQ_MIDS_FREQ = 1000.0;
inline const double EQ_MIDS_Q = 0.7;

// high band
inline const double EQ_HIGH_FREQ = 8000.0;
inline const double EQ_HIGH_Q = 0.7;

// gain every band starts at before any control has touched it
inline const double EQ_INITIAL_GAIN_DB = 0.0;

}
