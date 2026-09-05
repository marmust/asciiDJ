#include <TUIdisplay.hpp>
#include <EQsetup.hpp>
#include <DirReader.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <unistd.h>
#include <termios.h>
#include <algorithm>

namespace Graphics
{

namespace
{
	// how many seconds of raw track content each deck's waveform preview should span
	constexpr double waveformWindowSeconds = 2.0;
}

/// @brief ctor, saves the terminal's current cursor position so later frames get redrawn in
/// place instead of scrolling the terminal
TUIdisplay::TUIdisplay()
{
	std::ios_base::sync_with_stdio(false);
	this->originalTermios = std::make_unique<termios>();
	this->disableTerminalEcho();
	this->startPos = this->queryCursorPos();
}

/// @brief dtor, makes sure the update loop is stopped and the terminal's original settings
/// (echo/canonical mode) are put back so the shell behaves normally again after exit
TUIdisplay::~TUIdisplay()
{
	this->stopUpdateLoop();
	tcsetattr(STDIN_FILENO, TCSANOW, this->originalTermios.get());
}

/// @brief disables terminal echo/canonical mode for the rest of the process's life (restored in
/// the dtor) - physical keystrokes are read directly by InputReader (X11/evdev), not typed into
/// this program's stdin, so without this the terminal's own line discipline still echoes every
/// keypress onto the screen, garbling the TUI's carefully cursor-positioned output
void TUIdisplay::disableTerminalEcho()
{
	tcgetattr(STDIN_FILENO, this->originalTermios.get());

	termios rawTermios = *this->originalTermios;
	rawTermios.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &rawTermios);
}

/// @brief setter to pass which composer to use to render
/// @param GraphicsComposer* composer composer ptr
void TUIdisplay::setGraphicsComposer(GraphicsComposer* composer)
{
	this->composer = composer;
}

/// @brief setter for the deck1 ptr to gather telemetry from
void TUIdisplay::setDeck1(Playback::Deck* deck1)
{
	this->deck1 = deck1;
}

/// @brief setter for the deck2 ptr to gather telemetry from
void TUIdisplay::setDeck2(Playback::Deck* deck2)
{
	this->deck2 = deck2;
}

/// @brief setter for the mixer ptr to gather telemetry from
void TUIdisplay::setMixer(Playback::Mixer* mixer)
{
	this->mixer = mixer;
}

/// @brief setter for deck1's eq ptr to gather telemetry from
void TUIdisplay::setEq1(Playback::Equalizer* eq1)
{
	this->eq1 = eq1;
}

/// @brief setter for deck2's eq ptr to gather telemetry from
void TUIdisplay::setEq2(Playback::Equalizer* eq2)
{
	this->eq2 = eq2;
}

/// @brief setter for which compose function composeCurrentFrame() calls - pushed here by
/// CentralController's file-select toggle callback, so both bools change together
void TUIdisplay::setToggleFileDisplay(bool toggleFileDisplay)
{
	this->toggleFileDisplay = toggleFileDisplay;
}

/// @brief setter for the dirReader ptr to gather the file listing from
void TUIdisplay::setDirReader(Files::DirReader* dirReader)
{
	this->dirReader = dirReader;
}

/// @brief setter for the fileManager ptr to gather the current file selection from
void TUIdisplay::setFileManager(Files::FileLoadManager* fileManager)
{
	this->fileManager = fileManager;
}

/// @brief starts the TUIupdateLoop func as a thread and detaches
void TUIdisplay::startUpdateLoop()
{
	this->composer->reportAudioEngineTelemetry(this->gatherTUItelemetry());
	this->composeCurrentFrame();
	this->reserveRenderingSpace(this->calcFrameHeight(this->composer->getFrameBuffer()));

	this->running = true;

	std::thread TUIloop(TUIdisplay::TUIupdateLoop, this);
	TUIloop.detach();
}

/// @brief haults the update loop if running
void TUIdisplay::stopUpdateLoop()
{
	this->running = false;

	this->composer->reportAudioEngineTelemetry(this->gatherTUItelemetry());
	this->composeCurrentFrame();

	// unlike startUpdateLoop, don't cursor back up to the top afterward - this is the last
	// thing printed, so the cursor should stay past the ascii for whatever prints next (eg
	// the shell prompt), not land back in the middle of it
	this->reserveRenderingSpace(this->calcFrameHeight(this->composer->getFrameBuffer()), false);
}

/// @brief queries the terminal for the cursor's current position
/// @returns CursorPos the cursor's current row/col
CursorPos TUIdisplay::queryCursorPos()
{
	termios oldTermios;
	tcgetattr(STDIN_FILENO, &oldTermios);

	termios rawTermios = oldTermios;
	rawTermios.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &rawTermios);

	std::cout << "\033[6n" << std::flush;

	std::string reply;
	char nextChar = 0;

	while (read(STDIN_FILENO, &nextChar, 1) == 1 && nextChar != 'R')
		reply += nextChar;

	tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);

	CursorPos pos;
	std::sscanf(reply.c_str(), "\033[%d;%d", &pos.y, &pos.x);

	return pos;
}

/// @brief moves the terminal cursor to the given position via a CUP escape sequence
/// @param CursorPos pos the row/col to move the cursor to
void TUIdisplay::moveCursorToPos(CursorPos pos)
{
	std::cout << "\033[" << pos.y << ";" << pos.x << "H";
}

/// @brief gathers a fresh telemetry snapshot from all reporting components (2 decks, mixer,
/// their eqs), null/unloaded components are left at TUItelemetry's defaults
/// @returns TUItelemetry the gathered snapshot
TUItelemetry TUIdisplay::gatherTUItelemetry()
{
	TUItelemetry telemetry;

	// dirReader caches its listing at construction, so this is a plain in-memory read now, no
	// disk I/O - safe to pull unconditionally every tick same as everything else below
	if (this->dirReader)
	{
		telemetry.fileList = this->dirReader->listLocalFiles();
		telemetry.localDir = this->dirReader->getLocalDir();
	}

	if (this->fileManager)
	{
		telemetry.fileSelectedIdx = this->fileManager->getSelectedIdx();
		telemetry.fileSelectOpen = this->toggleFileDisplay;
	}

	if (this->deck1)
	{
		// off deckParams directly (not the loaded track's ridealong copy) so an unloaded deck
		// correctly reports 0/0 (paused, same as startRun() leaves every deck) instead of
		// falling back to TUItelemetry's struct defaults (1.0/0.0 - "playing") when there's no
		// track to read real params from
		telemetry.d1speed = this->deck1->getPlaybackSpeed();
		telemetry.d1shift = this->deck1->getTurntableShift();

		if (Playback::Track* track = this->deck1->getLoadedTrack())
		{
			telemetry.d1name = track->name;
			telemetry.d1sampleRate = (int)track->sampleRate;
		}

		telemetry.d1waveform = this->deck1->extractExpectedWaveform(waveformWindowSeconds);
		telemetry.d1progress = this->deck1->getTrackProgress();
		telemetry.d1timeRemaining = this->deck1->getTimeRemainingSeconds();
	}

	if (this->deck2)
	{
		telemetry.d2speed = this->deck2->getPlaybackSpeed();
		telemetry.d2shift = this->deck2->getTurntableShift();

		if (Playback::Track* track = this->deck2->getLoadedTrack())
		{
			telemetry.d2name = track->name;
			telemetry.d2sampleRate = (int)track->sampleRate;
		}

		telemetry.d2waveform = this->deck2->extractExpectedWaveform(waveformWindowSeconds);
		telemetry.d2progress = this->deck2->getTrackProgress();
		telemetry.d2timeRemaining = this->deck2->getTimeRemainingSeconds();
	}

	if (this->mixer)
	{
		telemetry.d1volume = this->mixer->getVolume(0);
		telemetry.d2volume = this->mixer->getVolume(1);
		telemetry.xfader = this->mixer->getXfader();
	}

	if (this->eq1)
	{
		telemetry.deck1EQlow = this->eq1->getBandGain(CommandAndControl::EQ_BASS_IDX);
		telemetry.deck1EQmid = this->eq1->getBandGain(CommandAndControl::EQ_MIDS_IDX);
		telemetry.deck1EQhigh = this->eq1->getBandGain(CommandAndControl::EQ_HIGH_IDX);
	}

	if (this->eq2)
	{
		telemetry.deck2EQlow = this->eq2->getBandGain(CommandAndControl::EQ_BASS_IDX);
		telemetry.deck2EQmid = this->eq2->getBandGain(CommandAndControl::EQ_MIDS_IDX);
		telemetry.deck2EQhigh = this->eq2->getBandGain(CommandAndControl::EQ_HIGH_IDX);
	}

	return telemetry;
}

/// @brief composes whichever frame is currently selected - the normal deck/mixer frame, or the
/// file-select frame while toggleFileDisplay is on
void TUIdisplay::composeCurrentFrame()
{
	if (this->toggleFileDisplay)
		this->composer->composeFileSelectFrame();
	else
		this->composer->composeFrame();
}

/// @brief prints a given frame to console (overrides previous one with cursor movements)
/// @param const std::vector<std::string>* buffer the frame buffer as generated by the composer to be printed
void TUIdisplay::flushBufferToConsole(const std::vector<std::string>* buffer)
{
	for (auto it = buffer->begin(); it != buffer->end(); it++)
	{
		this->moveCursorToPos(this->startPos);
		std::cout << *it;
	}

	std::cout << std::flush;
}

/// @brief calculates a frame's height by counting the lowest \n found in any of its strings
/// @param const std::vector<std::string>* buffer the frame buffer as generated by the composer
int TUIdisplay::calcFrameHeight(const std::vector<std::string>* buffer)
{
	int frameHeight = 0;

	for (const std::string& element : *buffer)
		frameHeight = std::max(frameHeight, (int)std::count(element.begin(), element.end(), '\n'));

	return frameHeight;
}

/// @brief prints a given amount of newlines to reserve terminal space for a frame; when
/// returnToTop, moves back up and re-syncs startPos (needed before further rendering), otherwise
/// leaves the cursor sitting past the reserved block (needed when this is the last thing printed)
/// @param int height how many newlines to print
/// @param bool returnToTop whether to cursor back up to the top of the reserved block afterward
void TUIdisplay::reserveRenderingSpace(int height, bool returnToTop)
{
	std::cout << std::string(height, '\n');

	if (!returnToTop)
	{
		std::cout << std::flush;
		return;
	}

	if (height > 0)
		std::cout << "\033[" << height << "A";

	std::cout << std::flush;

	this->startPos = this->queryCursorPos();
}

/// @brief main TUI loop, queries for the telemetry, composes frame, flushes frame to console
/// @param TUIdisplay* self a this pointer to cheat around the static func only for threads requirement
void TUIdisplay::TUIupdateLoop(TUIdisplay* self)
{
	bool previousToggleFileDisplay = self->toggleFileDisplay;

	while (self->running)
	{
		// composeFrame() and composeFileSelectFrame() draw entirely different sets of
		// elements covering different parts of the shared frame border, and every dynamic
		// element is positioned via cursor-forward escapes (which skip columns rather than
		// overwriting them) - so switching modes can leave whatever the previous mode drew
		// sitting in a spot the new mode's elements never touch. Hard-clear once, right on
		// the transition frame, so nothing carries over
		if (self->toggleFileDisplay != previousToggleFileDisplay)
		{
			self->moveCursorToPos(self->startPos);
			std::cout << "\033[0J" << std::flush;
			previousToggleFileDisplay = self->toggleFileDisplay;
		}

		TUItelemetry telemetry = self->gatherTUItelemetry();
		self->composer->reportAudioEngineTelemetry(telemetry);
		self->composeCurrentFrame();
		self->flushBufferToConsole(self->composer->getFrameBuffer());

		// governor: this loop had no throttle at all before, so it span as fast as the CPU
		// allowed - usually masked somewhat by the mixer's audio thread contending for the
		// same CPU time, but fully exposed (extreme flicker) whenever that thread had less to
		// do, eg with a deck unloaded. 60fps is far more than a terminal needs to look smooth
		std::this_thread::sleep_for(std::chrono::microseconds(tuiFrameIntervalMicroseconds));
	}
}

}
