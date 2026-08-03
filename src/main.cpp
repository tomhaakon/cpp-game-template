#include "game/Game.h"

#include <exception>
#include <teya/core/Log.h>
#include <iostream>

int main() {
    try {
        if (!teya::core::Log::initialize("logs/teya_game.log")) {
            teya::core::Log::warning("Startup", "File logging is unavailable; continuing with console logging");
        }
        teya::core::Log::info("Startup", "Game initialized");

        {
            Game game(1280, 720, "Teya Game Template");
            game.run();
        }

        teya::core::Log::info("Shutdown", "Game closed normally");
        teya::core::Log::shutdown();
        return 0;
    } catch (const std::exception &error) {
        teya::core::Log::error("Fatal", error.what());
        teya::core::Log::shutdown();
        return 1;
    }
}
