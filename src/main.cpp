#include <iostream>
#include <Deck.hpp>
#include <Mixer.hpp>

int main()
{
	std::unique_ptr<Playback::Deck> deck1 = std::make_unique<Playback::Deck>();
	std::unique_ptr<Playback::Deck> deck2 = std::make_unique<Playback::Deck>();

	Playback::Mixer mixer = Playback::Mixer();

	deck1->loadTrack("./music/your_touch.mp3");
	deck2->loadTrack("./music/darts_n_vines.mp3");

	mixer.addDeck(deck1.get());
	mixer.addDeck(deck2.get());

	mixer.play();

	std::cout << "playing simultaniously..." << std::endl;

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	std::cout << "spinning deck1..." << std::endl;

        for (int x = 0; x <= 300; x++)
        {
                deck1->setTurntableShift((double)x / 100.0);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

	std::this_thread::sleep_for(std::chrono::seconds(3));

        for (int x = 300; x >= 0; x--)
        {
                deck1->setTurntableShift((double)x / 100.0);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

	for (int x = 0; x < 5; x++) { std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << x << std::endl; }

	return 0;
}
