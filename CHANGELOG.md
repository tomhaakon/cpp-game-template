# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- Added a dockable animation-authoring workflow with independent working copies,
  clip/frame timeline editing, runtime metadata inspectors, pixel and smooth
  previews, bounded undo/redo and clipboard state, validation, save/reload, and
  safe live or temporary application through the generic editor host.
- Added versioned animation assets with grid/atlas frames, pixel and smooth render
  policies, validation, events, sockets, hitboxes, markers, mirroring, and safe playback.
- Added asset-driven Player animation, socket-following weapon diagnostics, slash
  events, and detailed read-only animation Inspector properties.

- Added the opt-in Teya development editor with a dockable Game View, runtime
  hierarchy and inspector, performance metrics, simulation controls, and input gating.

- Added an opt-in Tracy profiling preset with frame markers, timing plots, and
  instrumentation across the game loop and major systems.
- Added `GameConfig` as the shared source for canvas dimensions, initial window scale,
  target frame rate, and window title.
- Added the RAII-based `GameWindow` class to manage raylib window creation and cleanup.
- Added a game-specific pixel-canvas main menu with Start and Quit actions, keyboard
  navigation, and shared UI styling under `src/ui`.
- Added an in-game pause menu opened with Escape, including mouse-clickable Resume and
  Exit actions.
- Added regression coverage for non-finite animation timing and oversized tile-layer
  indexing.

### Changed

- Replaced hard-coded Player sprite rows and timing with
  `assets/animations/player.animation.json`.

- Renamed the game resolution settings to `CanvasWidth`, `CanvasHeight`, and
  `InitialWindowScale` for clearer intent.
- Moved window configuration out of `Game` and into `GameWindow`.
- Disabled raylib's default Escape-to-close behavior so Escape is reserved for game UI.
- Added mouse selection and activation to the main-menu buttons.
- Split `Game` update and drawing coordination into descriptive state-specific methods,
  renamed its state member to `gameState_`, and moved menu sizing into the menu classes.
- Moved pointer queries into `teya::core::Input` and canvas coordinate conversion into
  the menus, reducing menu updates in `Game` to parameterless calls.
- Added a lightweight `GameStateMachine` that owns gameplay and menus, handles their
  transitions, and keeps `Game` focused on startup and the main render loop.
- Made `PixelCanvas` automatically release its render texture on destruction.
- Simplified `Game` cleanup to rely on the RAII destructors of the world, canvas, and
  window in their safe declaration order.
- Changed essential game startup to fail fast when the window, canvas, or world cannot
  initialize, with automatic RAII cleanup and a non-zero process exit.
- Made tile indexing use unsigned size arithmetic to avoid signed integer overflow.
- Made animation updates ignore non-finite delta times and reject non-finite frame
  durations.

### Fixed

- Fixed render-target leaks when an initialized `PixelCanvas` leaves scope without an
  explicit `shutdown()` call.
- Fixed texture leaks caused by duplicate tileset or tile IDs.
- Fixed texture leaks when inserting a loaded texture into a container throws.
- Fixed window cleanup when game construction fails.
- Fixed a possible infinite animation loop caused by an infinite delta time.
