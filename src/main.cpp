#include <iostream>
#include <Deck.hpp>

int main()
{
	Playback::Deck deck = Playback::Deck();
	deck.loadTrack("./music/your_touch.mp3");
	deck.play();

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	std::cout << "pausing track..." << std::endl;
	deck.pause();

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	std::cout << "unpausing track..." << std::endl;
	deck.unpause();

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	std::cout << "stopping track..." << std::endl;
	deck.stop();

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	std::cout << "playing track..." << std::endl;
	deck.play();

	std::cout << "skipping forward..." << std::endl;

	for (int x = 0; x < 400; x++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		deck.setTurntableShift((double)x / 50);
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));

	for (int x = 400; x >= 0; x--)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		deck.setTurntableShift((double)x / 50);
	}

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	std::cout << "speed 1.5x..." << std::endl;
	deck.setPlaybackSpeed(1.5);

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	std::cout << "speed -1.0x..." << std::endl;
	deck.setPlaybackSpeed(-1.0);

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	deck.setPlaybackSpeed(1.0);

	std::cout << "simulating turntable rotation..." << std::endl;

	for (int x = 0; x < 300; x++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5));

		deck.setTurntableShift((double)x / 50);
	}

	for (int x = 300; x >= 0; x--)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5));

		deck.setTurntableShift((double)x / 50);
	}

	for (int x = 0; x < 300; x++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5));

		deck.setTurntableShift((double)x / -50);
	}



	for (int x = 300; x >= 0; x--)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		deck.setTurntableShift((double)x / -50);
	}

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	std::cout << "simulating turntable scratch..." << std::endl;

	deck.setPlaybackSpeed(0.0);

	for (int y = 0; y <= 10; y++)
	{
		for (int x = 0; x < 25; x++)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			deck.setTurntableShift((double)x / 5);
		}

		for (int x = 25; x >= -25; x--)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			deck.setTurntableShift((double)x / 5);
		}

		for (int x = -25; x < 25; x++)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			deck.setTurntableShift((double)x / -5);
		}

		for (int x = 25; x >= 0; x--)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			deck.setTurntableShift((double)x / -5);
		}
	}

	deck.setPlaybackSpeed(1.0);

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	std::cout << "smooth volume lowering to 0%..." << std::endl;

	for (int x = 100; x >= 0; x--)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		deck.setVolume((double)x / 100);
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));
	std::cout << "smooth volume raising to 200%..." << std::endl;

	for (int x = 0; x <=200; x++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		deck.setVolume((double)x / 100);
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));
	std::cout << "smooth volume lowering to 100%..." << std::endl;

	for (int x = 200; x >= 100; x--)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		deck.setVolume((double)x / 100);
	}

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

    	return 0;
}
