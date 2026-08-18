
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
	std::cout << Graphics::Renderer::renderHorizontalSlider(10, 0) << std::endl;
	std::cout << Graphics::Renderer::renderHorizontalSlider(10, 1) << std::endl;
	std::cout << Graphics::Renderer::renderHorizontalSlider(10, 2) << std::endl;
	std::cout << Graphics::Renderer::renderHorizontalSlider(10, 3) << std::endl;
	std::cout << Graphics::Renderer::renderHorizontalSlider(10, 4) << std::endl;
	std::cout << Graphics::Renderer::renderHorizontalSlider(10, 5) << std::endl;
	std::cout << Graphics::Renderer::renderHorizontalSlider(10, 6) << std::endl;
	std::cout << Graphics::Renderer::renderHorizontalSlider(10, 7) << std::endl;
	std::cout << Graphics::Renderer::renderHorizontalSlider(10, 8) << std::endl;
	std::cout << Graphics::Renderer::renderHorizontalSlider(10, 9) << std::endl;

	std::cout << Graphics::Renderer::renderVerticalSlider(10, 2) << std::endl;

	for (int progress = 0; progress <= 10; progress++)
		std::cout << Graphics::Renderer::renderHorizontalFill(10, progress) << std::endl;

	std::cout << Graphics::Renderer::renderVerticalFill(10, 4) << std::endl;

	std::cout << Graphics::Renderer::renderKnob(-0.3);
	std::cout << Graphics::Renderer::renderKnob(-0.7);
	std::cout << Graphics::Renderer::renderKnob(0.1);
	std::cout << Graphics::Renderer::renderKnob(0.9);

	// renderValue showcase
	std::cout << "[" << Graphics::Renderer::renderValue(3.14159f, 8) << "]" << std::endl;   // float, pads/truncates to fit
	std::cout << "[" << Graphics::Renderer::renderValue(3.14159, 10) << "]" << std::endl;   // double
	std::cout << "[" << Graphics::Renderer::renderValue(-3.14159, 8) << "]" << std::endl;   // negative
	std::cout << "[" << Graphics::Renderer::renderValue(42, 6, 0) << "]" << std::endl;  // int, zero-padded (windowPos unused, fits fine)

	for (int windowPos = 0; windowPos < 20; windowPos++)
	{
		std::cout << "[" << Graphics::Renderer::renderValue(std::string("HELLO WORLD"), 10, windowPos) << "]" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// int too long to fit promiseLength - rotates via the string overload, now animated by windowPos
	for (int windowPos = 0; windowPos < 20; windowPos++)
	{
		std::cout << "[" << Graphics::Renderer::renderValue(123456789, 4, windowPos) << "]" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// FFT chirp test: a sine wave rising in frequency, put through the FFT, then classified
	// frame-by-frame into RenderElements::frequencyRanges by its peak bin
	{
		const int sampleRate = 44100;
		const int durationSamples = sampleRate * 2; // 2 seconds
		const float startFreq = 100.0f;
		const float endFreq = 8000.0f;

		std::vector<float> chirp(durationSamples);
		double phase = 0.0;

		for (int i = 0; i < durationSamples; i++)
		{
			float t = (float)i / (float)durationSamples;
			float freq = startFreq + (endFreq - startFreq) * t;

			// integrate phase from the instantaneous frequency instead of sin(freq * t)
			// directly, so the sweep doesn't jump/click as freq changes sample to sample
			phase += 2.0 * M_PI * freq / sampleRate;
			chirp[i] = (float)std::sin(phase);
		}

		int frameCount = 20;
		int binCount = 1024; // power of two, so FFTprocessor's internal fftSize is exactly binCount * 2
		int fftSize = binCount * 2;

		auto frames = WaveformMath::FFTprocessor::computeSTFT(chirp, frameCount, binCount);

		std::cout << "chirp test: " << startFreq << "Hz -> " << endFreq << "Hz over " << frameCount << " frames" << std::endl;

		for (int f = 0; f < frameCount; f++)
		{
			int peakBin = 0;
			float peakMag = 0.0f;

			for (int b = 0; b < binCount; b++)
			{
				if (frames[f][b] > peakMag)
				{
					peakMag = frames[f][b];
					peakBin = b;
				}
			}

			float peakHz = (float)peakBin * sampleRate / (float)fftSize;

			// ranges are non-overlapping and always rising, so one linear pass finds the
			// range containing peakHz (or leaves rangeIdx at -1 if it falls outside all of them)
			int rangeIdx = -1;
			for (int r = 0; r < (int)RenderElements::frequencyRanges.size(); r++)
			{
				if (peakHz >= RenderElements::frequencyRanges[r].first && peakHz < RenderElements::frequencyRanges[r].second)
				{
					rangeIdx = r;
					break;
				}
			}

			std::cout << "frame " << f << ": peak ~" << peakHz << " Hz -> range " << rangeIdx << std::endl;
		}
	}

	// real pipeline demo: load a deck, pull its expected waveform, FFT it, classify each
	// frame into a frequency range, and render the result as one spectrum line
	{
		Playback::Deck spectrumDeck = Playback::Deck();
		spectrumDeck.loadTrack("./music/your_touch.mp3");
		spectrumDeck.setPlaybackSpeed(2.0);

		// cursor starts at 0, so the first half of this +-10s window is silence (nothing
		// before the start of the track) and the second half is real audio
		std::vector<float> waveform = spectrumDeck.extractExpectedWaveform(40.0);

		int frameCount = 600;
		int binCount = 1024; // power of two, so FFTprocessor's internal fftSize is exactly binCount * 2
		int fftSize = binCount * 2;

		auto fftFrames = WaveformMath::FFTprocessor::computeSTFT(waveform, frameCount, binCount);

		if (spectrumDeck.getLoadedTrack() && !fftFrames.empty())
		{
			int sampleRate = spectrumDeck.getLoadedTrack()->sampleRate;

			std::vector<int> dominantFreqRanges;
			dominantFreqRanges.reserve(frameCount);

			for (int f = 0; f < frameCount; f++)
			{
				int peakBin = 0;
				float peakMag = 0.0f;

				for (int b = 0; b < binCount; b++)
				{
					if (fftFrames[f][b] > peakMag)
					{
						peakMag = fftFrames[f][b];
						peakBin = b;
					}
				}

				// a silent frame's FFT is all zero, so peakMag stays 0 and peakBin stays
				// at its default of 0 - without this check that would misclassify silence
				// as the lowest frequency range instead of leaving it blank
				int rangeIdx = -1;
				if (peakMag > 0.0f)
				{
					float peakHz = (float)peakBin * sampleRate / (float)fftSize;

					for (int r = 0; r < (int)RenderElements::frequencyRanges.size(); r++)
					{
						if (peakHz >= RenderElements::frequencyRanges[r].first && peakHz < RenderElements::frequencyRanges[r].second)
						{
							rangeIdx = r;
							break;
						}
					}
				}

				dominantFreqRanges.push_back(rangeIdx);
			}

			std::cout << "track spectrum: [" << Graphics::Renderer::renderSpectrum(dominantFreqRanges) << "]" << std::endl;
		}
	}

	// overlay demo: render every element on top of each other at random positions, using
	// makeTransparent() so each element's spaces don't blank out what's already drawn there
	{
		std::random_device rd;
		std::mt19937 rng(rd());
		std::uniform_int_distribution<int> rowDist(1, 20);
		std::uniform_int_distribution<int> colDist(1, 50);

		std::vector<std::string> elements = {
			Graphics::Renderer::renderHorizontalSlider(10, 4),
			Graphics::Renderer::renderVerticalSlider(10, 2),
			Graphics::Renderer::renderHorizontalFill(10, 6),
			Graphics::Renderer::renderVerticalFill(10, 4),
			Graphics::Renderer::renderKnob(0.3f),
			Graphics::Renderer::renderTurntable(1),
		};

		std::cout << "\033[2J"; // clear screen so the overlap is easy to see

		for (const std::string& element : elements)
		{
			int row = rowDist(rng);
			int col = colDist(rng);

			std::string transparent = Graphics::Renderer::makeTransparent(element);

			// reposition per line: a plain '\n' would snap back to column 1 instead of
			// staying at col, since we're placing this with absolute cursor moves
			size_t lineStart = 0;
			int lineOffset = 0;
			while (lineStart <= transparent.length())
			{
				size_t lineEnd = transparent.find('\n', lineStart);
				std::string line = transparent.substr(lineStart, lineEnd - lineStart);

				std::cout << "\033[" << (row + lineOffset) << ";" << col << "H" << line;

				if (lineEnd == std::string::npos)
					break;

				lineStart = lineEnd + 1;
				lineOffset++;
			}
		}

		std::cout << "\033[25;1H" << std::flush;
	}

	// GraphicsComposer demo: live deck1 speed/shift (own InputReader/InputInterpreter, not
	// the one further down - same keys/tuning as CentralController::configureInputs() wires
	// deck1 to, just without the rest of the deck/mixer/EQ controls) drives the composer's
	// wall-time-based turntable animation frame by frame
	{
		Input::InputReader demoIReader = Input::InputReader();
		Input::InputInterpreter demoInterpreter = Input::InputInterpreter();

		demoInterpreter.provideIReader(&demoIReader);

		demoInterpreter.addInput(Input::DECK1_SPEED_UP, Input::DECK1_SPEED_DOWN,
					  Input::SPEED_START_VAL, Input::SPEED_MOVE_SPEED,
					  Input::SPEED_MIN, Input::SPEED_MAX,
					  demoDeck1SpeedCallback);

		demoInterpreter.addInput(Input::DECK1_SHIFT_FWD, Input::DECK1_SHIFT_BACK,
					  Input::SHIFT_START_VAL, Input::SHIFT_MOVE_SPEED, Input::SHIFT_DECAY_RATE,
					  Input::SHIFT_MIN, Input::SHIFT_MAX,
					  demoDeck1ShiftCallback);

		demoInterpreter.startRefreshThread();

		Graphics::GraphicsComposer composer;
		Graphics::TUItelemetry telemetry;

		std::cout << "\033[2J";

		while (true)
		{
			telemetry.d1speed = demoDeck1Speed;
			telemetry.d1shift = demoDeck1Shift;
			composer.reportAudioEngineTelemetry(telemetry);

			std::cout << "\033[H" << composer.composeFrame() << std::flush;
			std::cout << telemetry.d1speed << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	}

	return 0;

	while (true)
	{
	for (int frame = 0; frame < (int)RenderElements::turntableFrames.size(); frame++)
	{
		std::cout << Graphics::Renderer::renderTurntable(frame) << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(70));
	}
	}

	return 0;

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
