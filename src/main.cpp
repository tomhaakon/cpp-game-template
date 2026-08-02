#include "game/Game.h"

#include <exception>
#include <game_core/Log.h>
#include <iostream>

int main() {
    try {
        Log::configureDefaults();
        Log::initializeFileLogging("logs", "C++ Game Template");
        Log::applyRaylibTraceLevel();
        Log::info("Startup", "Game initialized");

        {
            Game game(1280, 720, "C++ Game Template");
            game.run();
        }

        Log::info("Shutdown", "Game closed normally");
        Log::shutdownFileLogging();
        return 0;
    } catch (const std::exception &error) {
        Log::error("Fatal", error.what());
        Log::shutdownFileLogging();
        return 1;
    }
}
