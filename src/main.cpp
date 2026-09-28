#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdio>
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
#include <FileLoadManager.hpp>
#include <DirReader.hpp>
#include <RendererStyle.hpp>

#include <unistd.h>
#include <termios.h>

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

	// the EQ's sample rate is a property of the pipeline's fixed output format
	// (Deck::kOutputSampleRate), not of whatever track happens to be loaded on a deck - so this
	// doesn't need to wait on (or crash on the absence of) a loaded track
	eq1.reportSampleRate((int)Playback::Deck::kOutputSampleRate);
	eq2.reportSampleRate((int)Playback::Deck::kOutputSampleRate);

	mixer.addDeck(&deck1, &eq1);
	mixer.addDeck(&deck2, &eq2);

	Files::FileLoadManager fileManager;
	fileManager.addDeck(&deck1);
	fileManager.addDeck(&deck2);

	// scans the local dir once, right here, and caches it - listLocalFiles() is a plain getter
	// from then on
	Files::DirReader dirReader;

	Graphics::GraphicsComposer composer;
	Graphics::TUIdisplay display;

	display.setGraphicsComposer(&composer);
	display.setDeck1(&deck1);
	display.setDeck2(&deck2);
	display.setMixer(&mixer);
	display.setEq1(&eq1);
	display.setEq2(&eq2);
	display.setDirReader(&dirReader);
	display.setFileManager(&fileManager);

	// wires every control (speed/shift/pause/volume/EQ, both decks, file browser) into
	// interpreter itself
	CommandAndControl::CentralController controller(&deck1, &deck2, &eq1, &eq2, &mixer, &iReader, &interpreter, &fileManager, &display, &dirReader);

	controller.startRun();
	display.startUpdateLoop();

	for (int x = 0; x < 999; x++)
		std::this_thread::sleep_for(std::chrono::seconds(1));

	display.stopUpdateLoop();

	return 0;
}
