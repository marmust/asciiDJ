#include <iostream>
#include <Deck.hpp>
#include <Mixer.hpp>
#include <Equalizer.hpp>
#include <InputReader.hpp>
#include <InputInterpreter.hpp>

// addInput's callback is a raw function pointer (void (*)(double)), so it can't capture
// deck1 as a closure - route it through this file-scope pointer instead
static Playback::Deck* g_deck1 = nullptr;

static void onScratchMove(double value)
{
	g_deck1->setTurntableShift(value);
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
	//mixer.addDeck(&deck2, &eq2);

	g_deck1 = &deck1;
	interpreter.addInput('d', 'a', 0.0, 0.1, 0.1, -2.0, 2.0, onScratchMove);
	interpreter.startRefreshThread();

	mixer.play();

	std::cout << "playing simultaniously..." << std::endl;

	for (int x = 0; x < 999; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	return 0;

	std::cout << "spinning deck1..." << std::endl;

        for (int x = 0; x <= 150; x++)
        {
                deck1.setTurntableShift((double)x / 100.0);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

	std::this_thread::sleep_for(std::chrono::seconds(3));

        for (int x = 150; x >= 0; x--)
        {
                deck1.setTurntableShift((double)x / 100.0);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	std::cout << "lowering highpass..." << std::endl;

	eq1.addBand(700.0, 0.1, 0.0);

	for (int x = 0; x >= -64; x--)
	{
		eq1.changeBand(0, 700.0, 0.1, x);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	std::cout << "raising highpass..." << std::endl;

	for (int x = -64; x <= 0; x++)
	{
		eq1.changeBand(0, 700.0, 0.1, x);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	return 0;
}
