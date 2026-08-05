# Game architecture

`Game` owns the raylib window, fixed-resolution `PixelCanvas`, state machine, and
optional editor. `game::World` owns the map, collision world, and Player. Player
owns game-specific movement, its animation asset/player, texture, collider, and
event interpretation. Generic foundations stay in `teya-core`; reusable 2D data,
validation, playback, mirroring, and rendering policy stay in `teya-2d`.

## Lifecycle

Window creation precedes canvas, world, texture, and editor initialization.
Frames update the state machine, render the world into the fixed canvas, and then
present either directly or through editor Game View. Destruction is reversed:
editor, world GPU resources, canvas, then window.

## Development editor

The opt-in `external/teya-game-editor` submodule consumes immutable host
snapshots. Debug, release, and profile remain editor-free. Game View focus and
ImGui capture feed one game-level input gate. Play, Pause, and deterministic Step
control whether world updates run.

The Inspector shows read-only Player and animation diagnostics: asset/texture
paths, source/render/filter modes, clip/frame timing and state, source rectangle,
sockets, hitboxes, markers, recent events, and facing. Animation authoring remains
out of scope until phase 3.

## Player animation flow

`assets/animations/player.animation.json` is the sole source for texture path,
clip timing, sprite indices or atlas regions, filtering, sockets, hitboxes,
markers, and events. Player validates the immutable `AnimationAsset` against the
loaded texture and drives `AnimationPlayer`. Movement selects `idle`, `walk`, or
`run`; Confirm demonstrates the non-looping `sword_attack`.

Pixel assets can use nearest filtering and rounding. Smooth assets can use linear
filtering and preserve subpixel transforms. Player applies the asset policy rather
than hard-coding either workflow. Right-facing metadata is canonical and mirrored
for left-facing rendering.

Game code consumes `attack_started`, `attack_active`, `spawn_slash`,
`attack_finished`, and `play_sound`; `teya-2d` never interprets them. A generic
debug weapon follows `weapon_hand`, honors socket position/rotation/scale and
front/behind layer, and draws a short slash diagnostic. It is not a combat system.

## Input and ownership

Gameplay reads logical `teya::core::Input` actions only when the centralized gate
allows it. Player keeps a non-owning collision-world pointer whose lifetime is
owned by World. Animation players retain `shared_ptr<const AnimationAsset>` so
editable vector changes cannot dangle runtime references; replacement is atomic
and invalid assets never displace valid data.

## Extension boundaries

- Game-specific entities, event meaning, attachments, and combat belong in the
  game template.
- Generic animation formats, playback, transforms, validation, and policies
  belong in `teya-2d`.
- Physical bindings, logging, asset paths, and JSON file primitives belong in
  `teya-core`.
- Phase 3 may add editor working copies and validated publish commands without
  exposing mutable Player or live asset vectors.
