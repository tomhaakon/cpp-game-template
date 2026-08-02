cmake_minimum_required(VERSION 3.24)

project(cpp_game_template LANGUAGES C CXX)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/raylib/CMakeLists.txt" OR
   NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/json/CMakeLists.txt")
    message(FATAL_ERROR
        "Dependency submodules are missing. Run: git submodule update --init --recursive"
    )
endif()

set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "Build raylib games" FORCE)

# Pinned dependencies can support older CMake versions than this project does.
# Keep their deprecation notices out of the project's configure diagnostics.
set(_cpp_game_warn_deprecated_was_cached FALSE)
if(DEFINED CACHE{CMAKE_WARN_DEPRECATED})
    set(_cpp_game_warn_deprecated_was_cached TRUE)
    set(_cpp_game_saved_warn_deprecated "$CACHE{CMAKE_WARN_DEPRECATED}")
endif()
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Suppress dependency deprecation warnings" FORCE)
add_subdirectory("external/raylib" EXCLUDE_FROM_ALL)
add_subdirectory("external/json" EXCLUDE_FROM_ALL)
if(_cpp_game_warn_deprecated_was_cached)
    set(CMAKE_WARN_DEPRECATED "${_cpp_game_saved_warn_deprecated}" CACHE BOOL
        "Enable CMake deprecation warnings" FORCE
    )
else()
    unset(CMAKE_WARN_DEPRECATED CACHE)
endif()
unset(_cpp_game_saved_warn_deprecated)
unset(_cpp_game_warn_deprecated_was_cached)

add_executable(cpp_game)
target_sources(cpp_game PRIVATE
    "src/main.cpp"
    "src/game/Game.cpp"
    "src/game/Game.h"
    "src/support/AssetPath.cpp"
    "src/support/AssetPath.h"
    "src/support/JsonFile.cpp"
    "src/support/JsonFile.h"
)
target_compile_features(cpp_game PRIVATE cxx_std_17)
set_target_properties(cpp_game PROPERTIES
    CXX_EXTENSIONS OFF
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:cpp_game>"
)
target_include_directories(cpp_game PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(cpp_game PRIVATE raylib nlohmann_json::nlohmann_json)
target_compile_definitions(cpp_game PRIVATE
    CPP_GAME_DEVELOPMENT_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

include("cmake/CompilerWarnings.cmake")
cpp_game_set_warnings(cpp_game)

add_custom_command(TARGET cpp_game POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:cpp_game>/assets"
    COMMENT "Copying assets beside cpp_game"
)
