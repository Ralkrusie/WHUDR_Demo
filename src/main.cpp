#include <iostream>
#include <SFML/Graphics.hpp>
#include "Game/SaveManager.h"












int main() {
    std::cout << "Hello, WHUDR!" << std::endl;
    int testSlotId = 0;
    SaveManager::saveGame(testSlotId);
    bool loadSuccess = SaveManager::loadGame(testSlotId);
    if (loadSuccess) {
        std::cout << "Game loaded successfully from slot " << testSlotId << std::endl;
    } else {
        std::cout << "Failed to load game from slot " << testSlotId << std::endl;
    }

    sf::RenderWindow window(sf::VideoMode({640, 480}), "WHUDR Test");
    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                SaveManager::saveGame(testSlotId);
            }
        }
    }
    return 0;
}