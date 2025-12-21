#include <iostream>
#include <SFML/Graphics.hpp>
#include "Game/Game.h"











int main() {
    std::cout << "Hello, WHUDR!" << std::endl;
    // int testSlotId = 0;
    // SaveManager::saveGame(testSlotId);
    // bool loadSuccess = SaveManager::loadGame(testSlotId);
    // if (loadSuccess) {
    //     std::cout << "Game loaded successfully from slot " << testSlotId << std::endl;
    // } else {
    //     std::cout << "Failed to load game from slot " << testSlotId << std::endl;
    // }
    Game game;
    game.run();
    return 0;
}