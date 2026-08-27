#pragma once

#include <string>
#include <vector>
#include <utility>

namespace RenderElements
{
	inline std::string horizontalSliderLeftCap = "[";
	inline std::string horizontalSliderRightCap = "]";

	inline std::string horizontalSliderRail = "-";
	inline std::string horizontalSliderHolder = "|";

	inline std::string verticalSliderTopCap = "#\n";
	inline std::string verticalSliderBottomCap = "#";

	inline std::string verticalSliderRail = "|\n";
	inline std::string verticalSliderHolder = "=\n";


	inline std::string horizontalFillLeftCap = "[";
	inline std::string horizontalFillRightCap = "]";
	inline std::string horizontalFillFull = "|";
	inline std::string horizontalFillEmpty = " ";

	inline std::string verticalFillTopCap = "#\n";
	inline std::string verticalFillBottomCap = "#";
	inline std::string verticalFillFull = "-\n";
	inline std::string verticalFillEmpty = " \n";

	// knob rotation frames, index 0 = max (+1) position, smoothly interpolated
	// (evenly spaced) down to index (size-1) = min (-1) position
	inline std::vector<std::string> knobFrames = {
		R"(  ___
 /   \
|  |  |
 \_|_/)",
		R"(  ___
 /   \
|  \  |
 \__\/)",
		R"(  ___
 /   \
|  ---|
 \___/)",
		R"(  ___
 /  /\
|  /  |
 \___/)",
		R"(  ___
 / | \
|  |  |
 \___/)",
		R"(  ___
 /\  \
|  \  |
 \___/)",
		R"(  ___
 /   \
|---  |
 \___/)",
		R"(  ___
 /   \
|  /  |
 \/__/)",
		R"(  ___
 /   \
|  |  |
 \_|_/)"
	};

	// deck turntable spin frames, indexed directly (no interpolation - the caller picks the frame)
	inline std::vector<std::string> turntableFrames = {
		R"(      __-----------__
     /  ###########  \
   /                   \
  /                     \
 |                       |
|            |            |
|           ...           |
|           ...           |
|            |            |
 |                       |
  \                     /
   \                   /
     \__###########__/
        -----------)",
		R"(      __-----------__
     /            ###\
   /                  #\
  /                    #\
 |                      #|
|              /          |
|           ...           |
|           ...           |
|          /              |
 |#                      |
  \#                    /
   \#                  /
     \###          __/
        -----------)",
		R"(      __-----------__
     /               \
   /                   \
  /                     \
 |#                     #|
|#                       #|
|#        __...__        #|
|#          ...          #|
|#                       #|
 |#                     #|
  \                     /
   \                   /
     \__           __/
        -----------)",
		R"(      __-----------__
     /###            \
   /#                  \
  /#                    \
 |#                      |
|          \              |
|           ...           |
|           ...           |
|              \          |
 |                      #|
  \                    #/
   \                  #/
     \__          ###/
        -----------)"
	};

	// static baked frame: outer border, header (asciiDJ/OSRCS), and the two "DECKn =="
	// waveform-row labels; every other row's interior is left blank for dynamic
	// elements (turntables, waveform bars, EQ assembly, speed/xfader/time rows) to
	// draw over
	inline std::string frameImage = R"(asciiDJ ----------------------------------------------------------------------------------------- OSRCS
|                                                                                                     |
|  DECK1 ==                                                                                           |
|  DECK2 ==                                                                                           |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
|                                                                                                     |
+-----------------------------------------------------------------------------------------------------+)";

	// static baked "][" EQ divider column: band labels (HI:/MID:/LO:/DB) and the decorative
	// bracket pattern between the two knob stacks; the knob art and numeric dB readouts are
	// blanked out here and drawn as separate dynamic elements. Does NOT include the outer
	// "#" capped "|" strut columns flanking this block - those are real volume sliders
	inline std::string middleDividerImage = R"(                                   
    HI:          ]          HI:    
                 [                 
     DB          ]          DB     
                 [                 
                 ]                 
   MID:          [          MID:   
                 ]                 
     DB          [          DB     
                 ]                 
                 [                 
    LO:          ]          LO:    
                 [                 
     DB          ]          DB     
                                   )";

	// static playhead marker, drawn one row above the waveform traces at their center column
	// to mark where the cursor sits in the +-waveformWindowSeconds window each trace spans
	inline std::string playheadMarkerImage = "v";

	// spectrum density ramp, index 0 (BASS) = densest/loudest down to index size()-1 (HI) = sparsest/quietest
	inline char spectrumBlank = ' ';

	inline std::vector<char> spectrumChars = {
		'@', '%', '=', '*', '|', ';', '.', '`', ' '
	};

	// [low, high) frequency bands in Hz, log-spaced across the audible range (20Hz-20kHz),
	// index-aligned with spectrumChars (band i's char is spectrumChars[i]); non-overlapping
	// and strictly rising, so a linear scan finds the band containing any given frequency
	inline std::vector<std::pair<int, int>> frequencyRanges = {
		{20, 29}, {29, 43}, {43, 63}, {63, 93}, {93, 136}, {136, 200},
		{200, 294}, {294, 431}, {431, 633}, {633, 929}, {929, 1363}, {1363, 2001},
		{2001, 2937}, {2937, 4310}, {4310, 6326}, {6326, 9286}, {9286, 13630}, {13630, 20000}
	};
}
