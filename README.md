# C++ Game Template

A small C++17 foundation for games and prototypes using raylib, nlohmann/json, CMake, and VS Code. It deliberately contains only a game loop, centralized asset paths, and JSON file helpers.

## Prerequisites

- Git
- CMake 3.24 or newer
- A C++17 compiler
- Ninja for the included presets

## Clone and build

Clone a new copy with its dependencies:

```bash
git clone --recurse-submodules <git@github.com:tomhaakon/cpp-game-template.git>
cd cpp-game-template
```

For an existing clone, initialize the dependencies with:

```bash
git submodule update --init --recursive
```

The shared code in `external/game-core` is pinned to a specific commit. After publishing changes to the `main` branch of `cpp-game-core`, update the template to the latest core commit with:

```bash
git submodule update --remote external/game-core
git add external/game-core
git commit -m "Update cpp-game-core"
```

The final commit records the new core revision in this repository. Cloning or running `git pull` alone does not advance a pinned submodule.

Configure and build a debug version:

```bash
cmake --preset debug
cmake --build --preset debug
```

Configure and build an optimized release version:

```bash
cmake --preset release
cmake --build --preset release
```

## Continuous integration

The GitHub Actions workflow in `.github/workflows/build.yml` builds both presets on Windows, Linux, and macOS. It runs for every push and pull request, checks out the dependency submodules recursively, and gives the workflow read-only repository permissions.

### Creating a release

Push a version tag to build Windows, Linux, and macOS packages and publish them in a GitHub Release with automatically generated release notes:

```bash
git tag -a v1.0.0 -m "Release v1.0.0"
git push origin v1.0.0
```

Release tags must start with `v`. Use semantic versions such as `v1.0.0`, `v1.1.0`, or `v2.0.0`. The release is published only after every platform package builds successfully.

## Extending the template

Add new `.cpp` files explicitly to `target_sources` in `CMakeLists.txt`; do not rely on source globbing.

### Naming conventions

Keep new code consistent with the existing template:

- **Classes and structs:** `PascalCase`, for example `Game` and `PlayerState`.
- **Functions and methods:** `camelCase`, for example `run()`, `loadConfig()`, and `update(float deltaTime)`.
- **Local variables and parameters:** `camelCase`, for example `fontSize`, `relativePath`, and `deltaTime`.
- **Member variables:** `camelCase` with a trailing underscore, for example `windowOpen_` and `currentScene_`. The underscore makes object state easy to distinguish from parameters and local variables.
- **Constants:** `camelCase` when scoped locally, for example `fontSize`; use `kPascalCase` for named namespace- or class-level constants, for example `kDefaultWindowWidth`.
- **Namespaces:** lowercase `snake_case`, for example `json_file`. A single lowercase word is preferred when it is clear, such as `assets`.
- **Macros and compile definitions:** uppercase `SNAKE_CASE`, for example `CPP_GAME_DEVELOPMENT_ASSET_DIR`.
- **Class files:** match the class name and use `.h` and `.cpp`, for example `Game.h` and `Game.cpp`.
- **Other source files:** use a descriptive `PascalCase` name, for example `AssetPath.cpp` and `JsonFile.h`.
- **CMake targets:** lowercase `snake_case`, for example `cpp_game`.

Use `#pragma once` in project headers. Prefer clear names over abbreviations, and keep terminology consistent between filenames, types, and functions.

Load an asset with `assets::path("textures/player.png")`. Development builds use the repository's `assets` directory, while distributable builds fall back to the `assets` folder beside the executable/current working directory.

Load and save JSON with `json_file::load(assets::path("data/config.json"))` and `json_file::save(path, document)`. Saving creates missing parent directories and writes four-space-indented JSON.

### Logging

The template initializes `cpp-game-core` file logging at startup and shuts it down after the game window closes. Each run writes a timestamped file under `logs` relative to the executable's working directory. Messages can be recorded with calls such as:

```cpp
Log::info("Gameplay", "Level loaded");
Log::warning("Assets", "Using fallback texture");
Log::error("Save", "Could not write save file");
```

### Input actions

Game code queries logical actions instead of physical keyboard keys. WASD and the arrow keys share the same directional actions, so they can drive menus or gameplay without duplicated checks.

- `Input::isDown(action)` remains true while any bound key is held.
- `Input::isPressed(action)` is true only on the frame a bound key is pressed.
- `Input::isReleased(action)` is true only on the frame a bound key is released.

For example, a menu can move its selection once per key press:

```cpp
if (Input::isPressed(Action::MoveDown)) {
    // Select the next menu item.
}
```

Use a pressed query for one-time actions:

```cpp
if (Input::isPressed(Action::Confirm)) {
    // Confirm once.
}
```

To add an action, make and publish the change in the separate `cpp-game-core` repository, then update this template's submodule revision. The public declaration is in `include/game_core/Input.h`, and its compile-time bindings are in `src/Input.cpp` within that repository.

To start a new game, rename the CMake project and executable target in `CMakeLists.txt`, update the target references in `.vscode/tasks.json`, and replace the title passed to `Game` in `src/main.cpp`.

## Troubleshooting

If configuration reports missing raylib or nlohmann/json sources, run:

```bash
git submodule update --init --recursive
```

If MSVC reports that standard headers such as `string` or `filesystem` cannot be found in VS Code, run **CMake: Delete Cache and Reconfigure**. The shared VS Code settings activate the complete Visual Studio developer environment so Ninja can find the MSVC standard library and Windows SDK.
