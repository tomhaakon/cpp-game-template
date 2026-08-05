cmake_minimum_required(VERSION 3.24)
project(teya_game_template LANGUAGES C CXX)
option(TEYA_ENABLE_EDITOR "Build the Teya development editor" OFF)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/teya-core/CMakeLists.txt" OR
   NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/teya-2d/CMakeLists.txt")
    message(FATAL_ERROR
        "Teya submodules are missing. Run: git submodule update --init --recursive")
endif()

set(TEYA_CORE_BUILD_TESTS OFF CACHE BOOL "Build teya-core tests" FORCE)
add_subdirectory(external/teya-core EXCLUDE_FROM_ALL)
add_subdirectory(external/teya-2d EXCLUDE_FROM_ALL)
if(TEYA_ENABLE_EDITOR AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/teya-game-editor/CMakeLists.txt")
    message(FATAL_ERROR "Editor submodule missing. Run: git submodule update --init --recursive")
endif()

add_executable(teya_game
    src/main.cpp
    src/game/Game.cpp
    src/game/Game.h
    src/game/GameConfig.h
    src/game/GameStateMachine.cpp
    src/game/GameStateMachine.h
    src/game/GameWindow.cpp
    src/game/GameWindow.h
    src/game/GameEditorHost.cpp
    src/game/GameEditorHost.h
    src/player/Player.cpp
    src/player/Player.h
    src/player/AttachmentObjects.cpp
    src/player/AttachmentObjects.h
    src/ui/MainMenu.cpp
    src/ui/MainMenu.h
    src/ui/PauseMenu.cpp
    src/ui/PauseMenu.h
    src/ui/UiStyle.h
    src/world/World.cpp
    src/world/World.h
)
target_compile_features(teya_game PRIVATE cxx_std_17)
set_target_properties(teya_game PROPERTIES
    CXX_EXTENSIONS OFF
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:teya_game>"
)
target_include_directories(teya_game PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(teya_game PRIVATE Teya::2D)
if(TEYA_ENABLE_EDITOR)
    enable_testing()
    add_subdirectory(external/teya-game-editor EXCLUDE_FROM_ALL)
    target_link_libraries(teya_game PRIVATE Teya::GameEditor)
    target_compile_definitions(teya_game PRIVATE TEYA_ENABLE_EDITOR=1)
else()
    target_compile_definitions(teya_game PRIVATE TEYA_ENABLE_EDITOR=0)
endif()
target_compile_definitions(teya_core PRIVATE
    TEYA_DEVELOPMENT_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")

include(cmake/CompilerWarnings.cmake)
teya_set_warnings(teya_game)

add_custom_command(TARGET teya_game POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:teya_game>/assets"
    COMMENT "Copying assets beside teya_game"
)
