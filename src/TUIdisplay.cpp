#include <TUIdisplay.hpp>
#include <EQsetup.hpp>

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
	this->startPos = this->queryCursorPos();
}

/// @brief dtor, makes sure the update loop is stopped
TUIdisplay::~TUIdisplay()
{
	this->stopUpdateLoop();
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

/// @brief starts the TUIupdateLoop func as a thread and detaches
void TUIdisplay::startUpdateLoop()
{
	this->composer->reportAudioEngineTelemetry(this->gatherTUItelemetry());
	this->composer->composeFrame();
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
	this->composer->composeFrame();

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

	if (this->deck1)
	{
		if (Playback::Track* track = this->deck1->getLoadedTrack())
		{
			telemetry.d1speed = track->params.playbackSpeed;
			telemetry.d1shift = track->params.turntableShift;
			telemetry.d1name = track->name;
			telemetry.d1sampleRate = (int)track->sampleRate;
		}

		telemetry.d1waveform = this->deck1->extractExpectedWaveform(waveformWindowSeconds);
		telemetry.d1progress = this->deck1->getTrackProgress();
		telemetry.d1timeRemaining = this->deck1->getTimeRemainingSeconds();
	}

	if (this->deck2)
	{
		if (Playback::Track* track = this->deck2->getLoadedTrack())
		{
			telemetry.d2speed = track->params.playbackSpeed;
			telemetry.d2shift = track->params.turntableShift;
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
	while (self->running)
	{
		TUItelemetry telemetry = self->gatherTUItelemetry();
		self->composer->reportAudioEngineTelemetry(telemetry);
		self->composer->composeFrame();
		self->flushBufferToConsole(self->composer->getFrameBuffer());
	}
}

}
