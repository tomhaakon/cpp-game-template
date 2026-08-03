#include "game/Game.h"

#include <exception>
#include <iostream>
#include <teya/core/Log.h>

int main() {
    try {
        if (!teya::core::Log::initialize("logs/teya_game.log")) {
            teya::core::Log::warning(
                "Startup", "File logging is unavailable; continuing with console logging");
        }
        teya::core::Log::info("Startup", "Game initialized");

        {
            Game game(480, 320, "Teya Game Template");
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
