#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <cmath>
#include <FFTprocessor.hpp>
#include <Deck.hpp>
#include <Mixer.hpp>
#include <Equalizer.hpp>
#include <InputReader.hpp>
#include <InputInterpreter.hpp>
#include <CentralController.hpp>
#include <Renderer.hpp>
#include <TUIdisplay.hpp>
#include <GraphicsComposer.hpp>
#include <InputSchema.hpp>
#include <InputTuning.hpp>

namespace
{
	// InputInterpreter callbacks are plain function pointers (no captures), so the demo loop's
	// live speed/shift have to live here rather than as locals in main()
	double demoDeck1Speed = Input::SPEED_START_VAL;
	double demoDeck1Shift = Input::SHIFT_START_VAL;

	void demoDeck1SpeedCallback(double speed) { demoDeck1Speed = speed; }
	void demoDeck1ShiftCallback(double shift) { demoDeck1Shift = shift; }
}

int main()
{
	Input::InputReader iReader = Input::InputReader();
	Input::InputInterpreter interpreter = Input::InputInterpreter();

	interpreter.provideIReader(&iReader);

	Playback::Deck deck1 = Playback::Deck();
	Playback::Deck deck2 = Playback::Deck();

	Playback::Equalizer eq1 = Playback::Equalizer();
	Playback::Equalizer eq2 = Playback::Equalizer();

	Playback::Mixer mixer = Playback::Mixer();

	deck1.loadTrack("./music/your_touch.mp3");
	deck2.loadTrack("./music/darts_n_vines.mp3");

	eq1.reportSampleRate(deck1.getLoadedTrack()->sampleRate);
	eq2.reportSampleRate(deck2.getLoadedTrack()->sampleRate);

	mixer.addDeck(&deck1, &eq1);
	mixer.addDeck(&deck2, &eq2);

	// wires every control (speed/shift/pause/volume/EQ, both decks) into interpreter itself
	CommandAndControl::CentralController controller(&deck1, &deck2, &eq1, &eq2, &mixer, &iReader, &interpreter);

	Graphics::GraphicsComposer composer;
	Graphics::TUIdisplay display;

	display.setGraphicsComposer(&composer);
	display.setDeck1(&deck1);
	display.setDeck2(&deck2);
	display.setMixer(&mixer);
	display.setEq1(&eq1);
	display.setEq2(&eq2);

	controller.startRun();
	display.startUpdateLoop();

	for (int x = 0; x < 999; x++)
		std::this_thread::sleep_for(std::chrono::seconds(1));

	display.stopUpdateLoop();

	return 0;
}
