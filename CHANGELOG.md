# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- Added `GameConfig` as the shared source for canvas dimensions, initial window scale,
  target frame rate, and window title.
- Added the RAII-based `GameWindow` class to manage raylib window creation and cleanup.
- Added regression coverage for non-finite animation timing and oversized tile-layer
  indexing.

### Changed

- Renamed the game resolution settings to `CanvasWidth`, `CanvasHeight`, and
  `InitialWindowScale` for clearer intent.
- Moved window configuration out of `Game` and into `GameWindow`.
- Made `PixelCanvas` automatically release its render texture on destruction.
- Simplified `Game` cleanup to rely on the RAII destructors of the world, canvas, and
  window in their safe declaration order.
- Prevented game-world and graphics initialization when window creation fails.
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
