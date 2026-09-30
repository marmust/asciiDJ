#include <Deck.hpp>
#include <Mixer.hpp>
#include <Equalizer.hpp>
#include <InputReader.hpp>
#include <InputInterpreter.hpp>
#include <CentralController.hpp>
#include <FileLoadManager.hpp>
#include <DirReader.hpp>
#include <GraphicsComposer.hpp>
#include <TUIdisplay.hpp>

#include <unistd.h>

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

	// the EQ sample rate is the pipeline's fixed output rate, independent of any loaded track
	eq1.reportSampleRate((int)Playback::Deck::kOutputSampleRate);
	eq2.reportSampleRate((int)Playback::Deck::kOutputSampleRate);

	mixer.addDeck(&deck1, &eq1);
	mixer.addDeck(&deck2, &eq2);

	Files::FileLoadManager fileManager;
	fileManager.addDeck(&deck1);
	fileManager.addDeck(&deck2);

	// scans and caches the working directory
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

	// wires every control into interpreter
	CommandAndControl::CentralController controller(&deck1, &deck2, &eq1, &eq2, &mixer, &iReader, &interpreter, &fileManager, &display, &dirReader);

	controller.startRun();
	display.startUpdateLoop();

	// everything runs on its own threads from here, main sleeps until a signal (ctrl+c kills the process)
	pause();

	display.stopUpdateLoop();

	return 0;
}
