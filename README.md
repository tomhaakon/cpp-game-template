# Teya Game Template

A C++17 starting point for Teya games and prototypes. The template consumes
`Teya::2D`; that module brings `Teya::Tiled`, while `Teya::Core` owns raylib,
nlohmann/json, logging, input, and asset paths.

## Current architecture

The application is split into three game-level responsibilities:

- `Game` owns the window, fixed-resolution canvas, and main loop.
- `game::World` owns the tile map, collision world, and player.
- `Player` owns player input, movement, animation, texture, and collider.

See [Game architecture](docs/architecture.md) for ownership diagrams, the
update/draw sequence, lifecycle rules, module boundaries, and the exact player
sprite-sheet layout.

## Prerequisites

- Git
- CMake 3.24 or newer
- A C++17 compiler
- Ninja for the included presets

## Clone and build

Clone a new copy with its dependencies:

```bash
git clone --recurse-submodules git@github.com:tomhaakon/teya-game-template.git
cd teya-game-template
```

For an existing clone, initialize the dependencies with:

```bash
git submodule update --init --recursive
```

The shared modules in `external/teya-core` and `external/teya-2d` are pinned to
specific commits. After publishing module changes, update them with:

```bash
git submodule update --remote external/teya-core
git submodule update --remote external/teya-2d
git add external/teya-core external/teya-2d
git commit -m "Update Teya modules"
```

The final commit records the new Core and 2D module revisions in this repository.
Cloning or running `git pull` alone does not advance pinned submodules.

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

### Profiling with Tracy

The profiling preset builds optimized code with debug symbols and enables Tracy
instrumentation across the game and Teya modules:

```bash
cmake --preset profile
cmake --build --preset profile
```

Start the Tracy profiler application, connect to `teya_game`, and exercise the
gameplay you want to measure. Capture begins only while Tracy is connected. The
timeline includes complete frames, update and drawing stages, game states, UI,
world and player work, animation, collision, map loading/rendering, input, JSON,
logging, and pixel-canvas presentation. It also plots frame time and collider
count.

Normal debug and release presets leave profiling disabled. Their markers compile
away and Tracy is not downloaded or linked.

### Visual Studio presets on Windows

The portable `debug` and `release` presets use Ninja. VS Code installations that do not propagate the complete MSVC developer environment should select **Windows Debug (Visual Studio)** instead. It uses Visual Studio's CMake generator, which discovers the compiler, standard library, and Windows SDK without environment-variable setup:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
```

Use `windows-release` for the corresponding release build. These Windows-only presets do not contain compiler installation paths.

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
- **Macros and compile definitions:** uppercase `SNAKE_CASE`, for example `TEYA_DEVELOPMENT_ASSET_DIR`.
- **Class files:** match the class name and use `.h` and `.cpp`, for example `Game.h` and `Game.cpp`.
- **Other source files:** use a descriptive `PascalCase` name, for example `AssetPath.cpp` and `JsonFile.h`.
- **CMake targets:** lowercase `snake_case`, for example `teya_game`.

Use `#pragma once` in project headers. Prefer clear names over abbreviations, and keep terminology consistent between filenames, types, and functions.

Load an asset with `teya::core::assets::path("textures/player.png")`. Development builds use the repository's `assets` directory, while distributable builds fall back to the `assets` folder beside the executable/current working directory.

Load and save JSON with `teya::core::json_file::load(...)` and
`teya::core::json_file::save(...)`. Saving creates missing parent directories
and writes four-space-indented JSON.

### Logging

The template initializes `teya-core` file logging at startup and shuts it down
after the game window closes. Each run appends a new session to
`logs/teya_game.log` relative to the executable's working directory. Messages
can be recorded with calls such as:

```cpp
teya::core::Log::info("Gameplay", "Level loaded");
teya::core::Log::warning("Assets", "Using fallback texture");
teya::core::Log::error("Save", "Could not write save file");
```

Pass a `teya::core::Log::LogConfig` as the second argument to
`teya::core::Log::initialize()` when the default `Info` level and flushing
behavior are not suitable.

### Input actions

Game code queries logical actions instead of physical keyboard keys. WASD and the arrow keys share the same directional actions, so they can drive menus or gameplay without duplicated checks.

- `teya::core::Input::isDown(action)` remains true while any bound key is held.
- `teya::core::Input::isPressed(action)` is true only on the frame a bound key is pressed.
- `teya::core::Input::isReleased(action)` is true only on the frame a bound key is released.

For example, a menu can move its selection once per key press:

```cpp
if (teya::core::Input::isPressed(teya::core::Action::MoveDown)) {
    // Select the next menu item.
}
```

Use a pressed query for one-time actions:

```cpp
if (teya::core::Input::isPressed(teya::core::Action::Confirm)) {
    // Confirm once.
}
```

To add an action, make and publish the change in `teya-core`, then update this
template's submodule revision. The public declaration is in
`include/teya/core/Input.h`, and its bindings are in `src/Input.cpp`.

To start a new game, rename the CMake project and executable target in `CMakeLists.txt`, update the target references in `.vscode/tasks.json`, and replace the title passed to `Game` in `src/main.cpp`.

## Troubleshooting

If configuration reports missing raylib or nlohmann/json sources, run:

```bash
git submodule update --init --recursive
```

If MSVC reports that standard headers such as `string` or `filesystem` cannot be found in VS Code, run **CMake: Delete Cache and Reconfigure**. The shared VS Code settings activate the complete Visual Studio developer environment so Ninja can find the MSVC standard library and Windows SDK.
