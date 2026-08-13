#include <iostream>
#include <Deck.hpp>
#include <Mixer.hpp>
#include <Equalizer.hpp>
#include <InputReader.hpp>
#include <InputInterpreter.hpp>
#include <CentralController.hpp>

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

	controller.startRun();

	for (int x = 0; x < 999; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	return 0;
}
