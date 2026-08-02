#include "game/Game.h"
#include <exception>
#include <iostream>

int main() {
    try {
        Game game(1280, 720, "C++ Game Template");
        game.run();
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
