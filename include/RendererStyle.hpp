#pragma once

#include <string>
#include <vector>

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

	inline std::string verticalSelectorBackground = "\n";
	inline std::string verticalSelectorPointer = ">\n";


	inline std::string horizontalFillLeftCap = "[";
	inline std::string horizontalFillRightCap = "]";
	inline std::string horizontalFillFull = "|";
	inline std::string horizontalFillEmpty = " ";

	inline std::string verticalFillTopCap = "#\n";
	inline std::string verticalFillBottomCap = "#";
	inline std::string verticalFillFull = "-\n";
	inline std::string verticalFillEmpty = " \n";

	// knob rotation frames, index 0 = max (+1) evenly down to the last = min (-1)
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

	// static frame: border, header and the DECKn waveform labels; dynamic elements draw over it
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

	// static EQ divider column: band labels and bracket pattern, knobs and dB readouts draw over it
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

	// playhead marker, drawn above the waveform traces at their center column
	inline std::string playheadMarkerImage = "v";

	// fill for a waveform trace with no track loaded
	inline char waveformBlank = ' ';

	// waveform loudness ramp, index 0 = loudest down to index size()-1 = silent
	inline std::vector<char> waveformChars = {
		//'@', '%', '=', '*', '|', ';', '.', '`', ' '
		//'@', '%', '@', '%', '=', '*', '=', '*', '|', ';', '|', ';', '.', '`', '.', '`', ' ', '.', ' ', '.'
		'@', '@', '@', '=', '-', '.', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ',', '.', '`', '.'
	};
}
