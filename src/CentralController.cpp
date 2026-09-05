#include <CentralController.hpp>
#include <InputTuning.hpp>
#include <InputSchema.hpp>
#include <EQsetup.hpp>
#include <DirReader.hpp>

namespace CommandAndControl
{

CentralController* CentralController::self = nullptr;

/// @brief ctor
/// @param you get the picture
CentralController::CentralController(Playback::Deck* deck1, Playback::Deck* deck2,
				      Playback::Equalizer* eq1, Playback::Equalizer* eq2,
				      Playback::Mixer* mixer,
				      Input::InputReader* iReader, Input::InputInterpreter* interpreter,
				      Files::FileLoadManager* fileManager, Graphics::TUIdisplay* display,
				      Files::DirReader* dirReader)
{
	this->deck1 = deck1;
	this->deck2 = deck2;

	this->eq1 = eq1;
	this->eq2 = eq2;

	this->mixer = mixer;

	this->iReader = iReader;
	this->interpreter = interpreter;

	this->fileManager = fileManager;
	this->display = display;
	this->dirReader = dirReader;

	CentralController::self = this;

	this->configureInputs();
}

/// @brief callback from input to set deck1's playback speed
/// @param double speed the speed to set the deck to
void CentralController::deck1SpeedCallback(double speed)
{
	self->deck1->setPlaybackSpeed(speed);
}

/// @brief callback from input to set deck2's playback speed
/// @param double speed the speed to set the deck to
void CentralController::deck2SpeedCallback(double speed)
{
	self->deck2->setPlaybackSpeed(speed);
}

/// @brief callback from input to set deck1's turntable shift
/// @param double shift the shift to set the deck to
void CentralController::deck1ShiftCallback(double shift)
{
	self->deck1->setTurntableShift(shift);
}

/// @brief callback from input to set deck2's turntable shift
/// @param double shift the shift to set the deck to
void CentralController::deck2ShiftCallback(double shift)
{
	self->deck2->setTurntableShift(shift);
}

/// @brief callback from input to toggle deck1's pause state
void CentralController::deck1pauseCallback()
{
	if (self->deck1->getIsPaused())
		self->deck1->unpause();
	else
		self->deck1->pause();
}

/// @brief callback from input to toggle deck2's pause state
void CentralController::deck2pauseCallback()
{
	if (self->deck2->getIsPaused())
		self->deck2->unpause();
	else
		self->deck2->pause();
}

/// @brief callback from input to set deck1's volume
/// @param double volume the volume to set the deck to
void CentralController::deck1VolumeCallback(double volume)
{
	self->mixer->setVolume(volume, 0);
}

/// @brief callback from input to set deck2's volume
/// @param double volume the volume to set the deck to
void CentralController::deck2VolumeCallback(double volume)
{
	self->mixer->setVolume(volume, 1);
}

/// @brief callback from input to set the mixer's crossfader position
/// @param double xfaderPos where to put the crossfader
void CentralController::xfaderCallback(double xfaderPos)
{
	self->mixer->setXfader(xfaderPos);
}

/// @brief callback from input to set deck1's bass band gain
/// @param double db the gain in dB to set the band to
void CentralController::deck1EQbassCallback(double db)
{
	self->eq1->changeBand(EQ_BASS_IDX, EQ_BASS_FREQ, EQ_BASS_Q, db);
}

/// @brief callback from input to set deck2's bass band gain
/// @param double db the gain in dB to set the band to
void CentralController::deck2EQbassCallback(double db)
{
	self->eq2->changeBand(EQ_BASS_IDX, EQ_BASS_FREQ, EQ_BASS_Q, db);
}

/// @brief callback from input to set deck1's mids band gain
/// @param double db the gain in dB to set the band to
void CentralController::deck1EQmidsCallback(double db)
{
	self->eq1->changeBand(EQ_MIDS_IDX, EQ_MIDS_FREQ, EQ_MIDS_Q, db);
}

/// @brief callback from input to set deck2's mids band gain
/// @param double db the gain in dB to set the band to
void CentralController::deck2EQmidsCallback(double db)
{
	self->eq2->changeBand(EQ_MIDS_IDX, EQ_MIDS_FREQ, EQ_MIDS_Q, db);
}

/// @brief callback from input to set deck1's high band gain
/// @param double db the gain in dB to set the band to
void CentralController::deck1EQhighCallback(double db)
{
	self->eq1->changeBand(EQ_HIGH_IDX, EQ_HIGH_FREQ, EQ_HIGH_Q, db);
}

/// @brief callback from input to set deck2's high band gain
/// @param double db the gain in dB to set the band to
void CentralController::deck2EQhighCallback(double db)
{
	self->eq2->changeBand(EQ_HIGH_IDX, EQ_HIGH_FREQ, EQ_HIGH_Q, db);
}

/// @brief callback from input to toggle file-select mode; keeps this instance's own
/// fileSelectionMode and display's toggleFileDisplay in sync so both the input gating and the
/// render mode switch together on the same keypress
void CentralController::fileSelectToggleCallback()
{
	self->fileSelectionMode = !self->fileSelectionMode;
	self->display->setToggleFileDisplay(self->fileSelectionMode);

	// refresh the manager's listing and reset the selection right as the browser opens - it was
	// never being populated before, so loadToDeck() was always silently no-opping regardless of
	// what got pressed
	if (self->fileSelectionMode)
	{
		self->fileManager->reportFiles(self->dirReader->listLocalFiles());
		self->fileManager->setSelectedIdx(0);
	}
}

/// @brief callback from input to move the file selection to the previous entry
void CentralController::arrowUpCallback()
{
	self->fileManager->setSelectedIdx(self->fileManager->getSelectedIdx() - 1);
}

/// @brief callback from input to move the file selection to the next entry
void CentralController::arrowDownCallback()
{
	self->fileManager->setSelectedIdx(self->fileManager->getSelectedIdx() + 1);
}

/// @brief callback from input to load the currently selected file onto deck1 (left deck), only
/// while in file-select mode; drops back out of file-select mode once the load's triggered
void CentralController::arrowLeftCallback()
{
	if (!self->fileSelectionMode)
		return;

	self->fileManager->loadToDeck(0, self->fileManager->getSelectedIdx());

	self->fileSelectionMode = false;
	self->display->setToggleFileDisplay(false);
}

/// @brief callback from input to load the currently selected file onto deck2 (right deck), only
/// while in file-select mode; drops back out of file-select mode once the load's triggered
void CentralController::arrowRightCallback()
{
	if (!self->fileSelectionMode)
		return;

	self->fileManager->loadToDeck(1, self->fileManager->getSelectedIdx());

	self->fileSelectionMode = false;
	self->display->setToggleFileDisplay(false);
}

/// @brief registers every control with the InputInterpreter, wiring each one to its callback,
/// keybinds taken from controls.txt (left deck = deck1, right deck = deck2)
/// NOTE: speed/decay/range values live in InputTuning.hpp, still placeholders, tune to taste
void CentralController::configureInputs()
{
	// left deck (deck1)
	this->interpreter->addInput(Input::DECK1_SPEED_UP, Input::DECK1_SPEED_DOWN, Input::SPEED_START_VAL, Input::SPEED_MOVE_SPEED, Input::SPEED_MIN, Input::SPEED_MAX, deck1SpeedCallback);
	this->interpreter->addInput(Input::DECK1_SHIFT_FWD, Input::DECK1_SHIFT_BACK, Input::SHIFT_START_VAL, Input::SHIFT_MOVE_SPEED, Input::SHIFT_DECAY_HALFLIFE, Input::SHIFT_MIN, Input::SHIFT_MAX, deck1ShiftCallback);
	this->interpreter->addInput(Input::DECK1_PAUSE, deck1pauseCallback);
	this->interpreter->addInput(Input::DECK1_VOLUME_UP, Input::DECK1_VOLUME_DOWN, Input::VOLUME_START_VAL, Input::VOLUME_MOVE_SPEED, Input::VOLUME_MIN, Input::VOLUME_MAX, deck1VolumeCallback);
	this->interpreter->addInput(Input::DECK1_EQ_BASS_UP, Input::DECK1_EQ_BASS_DOWN, Input::EQ_START_VAL, Input::EQ_MOVE_SPEED, Input::EQ_MIN, Input::EQ_MAX, deck1EQbassCallback);
	this->interpreter->addInput(Input::DECK1_EQ_MIDS_UP, Input::DECK1_EQ_MIDS_DOWN, Input::EQ_START_VAL, Input::EQ_MOVE_SPEED, Input::EQ_MIN, Input::EQ_MAX, deck1EQmidsCallback);
	this->interpreter->addInput(Input::DECK1_EQ_HIGH_UP, Input::DECK1_EQ_HIGH_DOWN, Input::EQ_START_VAL, Input::EQ_MOVE_SPEED, Input::EQ_MIN, Input::EQ_MAX, deck1EQhighCallback);

	// mixer specific
	this->interpreter->addInput(Input::XFADER_RIGHT, Input::XFADER_LEFT, Input::XFADER_START_VAL, Input::XFADER_MOVE_SPEED, Input::XFADER_MIN, Input::XFADER_MAX, xfaderCallback);

	// right deck (deck2)
	this->interpreter->addInput(Input::DECK2_SPEED_UP, Input::DECK2_SPEED_DOWN, Input::SPEED_START_VAL, Input::SPEED_MOVE_SPEED, Input::SPEED_MIN, Input::SPEED_MAX, deck2SpeedCallback);
	this->interpreter->addInput(Input::DECK2_SHIFT_FWD, Input::DECK2_SHIFT_BACK, Input::SHIFT_START_VAL, Input::SHIFT_MOVE_SPEED, Input::SHIFT_DECAY_HALFLIFE, Input::SHIFT_MIN, Input::SHIFT_MAX, deck2ShiftCallback);
	this->interpreter->addInput(Input::DECK2_PAUSE, deck2pauseCallback);
	this->interpreter->addInput(Input::DECK2_VOLUME_UP, Input::DECK2_VOLUME_DOWN, Input::VOLUME_START_VAL, Input::VOLUME_MOVE_SPEED, Input::VOLUME_MIN, Input::VOLUME_MAX, deck2VolumeCallback);
	this->interpreter->addInput(Input::DECK2_EQ_BASS_UP, Input::DECK2_EQ_BASS_DOWN, Input::EQ_START_VAL, Input::EQ_MOVE_SPEED, Input::EQ_MIN, Input::EQ_MAX, deck2EQbassCallback);
	this->interpreter->addInput(Input::DECK2_EQ_MIDS_UP, Input::DECK2_EQ_MIDS_DOWN, Input::EQ_START_VAL, Input::EQ_MOVE_SPEED, Input::EQ_MIN, Input::EQ_MAX, deck2EQmidsCallback);
	this->interpreter->addInput(Input::DECK2_EQ_HIGH_UP, Input::DECK2_EQ_HIGH_DOWN, Input::EQ_START_VAL, Input::EQ_MOVE_SPEED, Input::EQ_MIN, Input::EQ_MAX, deck2EQhighCallback);

	// file browser
	this->interpreter->addInput(Input::FILE_SELECT_TOGGLE, fileSelectToggleCallback);
	this->interpreter->addInput(Input::ARROW_UP, arrowUpCallback);
	this->interpreter->addInput(Input::ARROW_DOWN, arrowDownCallback);
	this->interpreter->addInput(Input::ARROW_LEFT, arrowLeftCallback);
	this->interpreter->addInput(Input::ARROW_RIGHT, arrowRightCallback);
}

/// @brief registers the fixed bass/mid/high bands each EQ callback drives by index (order must
/// match EQ_BASS_IDX/EQ_MIDS_IDX/EQ_HIGH_IDX in EQsetup.hpp)
/// requires reportSampleRate() to have already been called on eq1/eq2 first, otherwise
/// addBand() silently no-ops and changeBand()'s unchecked bands[bandIdx] becomes UB
void CentralController::addEQbands()
{
	this->eq1->addBand(EQ_BASS_FREQ, EQ_BASS_Q, EQ_INITIAL_GAIN_DB);
	this->eq1->addBand(EQ_MIDS_FREQ, EQ_MIDS_Q, EQ_INITIAL_GAIN_DB);
	this->eq1->addBand(EQ_HIGH_FREQ, EQ_HIGH_Q, EQ_INITIAL_GAIN_DB);

	this->eq2->addBand(EQ_BASS_FREQ, EQ_BASS_Q, EQ_INITIAL_GAIN_DB);
	this->eq2->addBand(EQ_MIDS_FREQ, EQ_MIDS_Q, EQ_INITIAL_GAIN_DB);
	this->eq2->addBand(EQ_HIGH_FREQ, EQ_HIGH_Q, EQ_INITIAL_GAIN_DB);
}

/// @brief clears every band from both EQs
void CentralController::resetEQs()
{
	this->eq1->resetEQ();
	this->eq2->resetEQ();
}

/// @brief starts decks, mixer, and the InputInterpreter
void CentralController::startRun()
{
	// (re)register the bands the EQ callbacks expect to already exist
	this->addEQbands();

	// start both decks paused
	this->mixer->play();

	this->deck1->pause();
	this->deck2->pause();

	// start input loop
	this->interpreter->startRefreshThread();
}

/// @brief stops decks, mixer, and the InputInterpreter
void CentralController::stopRun()
{
	// stop the mixer, freeze decks - mixer->stop() fully tears down its ma_device (blocks
	// until its audio thread exits), so resetEQs() below can't race the EQ apply loop
	this->mixer->stop();

	this->deck1->pause();
	this->deck2->pause();

	// stop input loop
	this->interpreter->stopRefreshThread();

	// drop the bands added by startRun(), so a stray EQ callback firing between
	// stopRun() and the next startRun() finds nothing to (unsafely) index into
	this->resetEQs();
}

}
