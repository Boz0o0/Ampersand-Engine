# Render

The render module draws the entities in a window. The game only adds
components: the `RenderSystem` draws them through the `IRenderer` stored in the
`Renderer` resource (`SfmlRenderer` here), so the game never calls SFML itself.
The server does not need this module.

## Concepts

| Name                    | Kind      | What it does                                                                                                                |
| ----------------------- | --------- | --------------------------------------------------------------------------------------------------------------------------- |
| `Sprite`                | component | image to draw: `texture`, `source` (part of the texture), `color` (tint), `layer` (draw order)                              |
| `Text`                  | component | text to draw: `font`, `value`, `size`, `color`, `layer` (draw order)                                                        |
| `Rectangle`             | component | filled rectangle from the local (0, 0): `size`, `fill`, `outline`, `outlineThickness`, `layer`                              |
| `Circle`                | component | filled circle in the box of its diameter: `radius`, `fill`, `outline`, `outlineThickness`, `layer`                          |
| `Line`                  | component | segment between two local points: `from`, `to`, `color`, `layer`                                                            |
| `Animation`             | component | list of frames played on the `Sprite`, `isFinished`, `framesFromGrid` (see [Animation](#animation))                         |
| `AnimationStateMachine` | component | picks the clip of the `Animation` from named states, transitions and one-shot actions (see [State machine](#state-machine)) |
| `Renderer`              | resource  | the `IRenderer` and its window: `size`, `title`, `isOpen` (see [Window](#window))                                           |
| `Clock`                 | resource  | `dt` of the frame in seconds, filled by the game (from `core`)                                                              |
| `Camera`                | resource  | part of the world shown in the window (see [Camera](#camera)), optional                                                     |
| `ClearColor`            | resource  | background color of the window, optional (black without it)                                                                 |
| `AnimationSystem`       | system    | runs every `AnimationStateMachine`, then moves every `Animation` forward and writes its frame in `Sprite::source`           |
| `RenderSystem`          | system    | draws one whole frame: clear, camera, every `Sprite`, `Text`, `Rectangle`, `Circle` and `Line`, display                     |
| `IRenderer`             | interface | the window: open, events, load textures and fonts, draw (`SfmlRenderer`)                                                    |

The systems only use the entities that also have a `Transform` (and a `Sprite`
for the `AnimationSystem`). The `RenderSystem` draws each entity at its world
transform: a child follows its parent (see
[World transform](ecs.md#world-transform)). The `RenderSystem` throws `ResourceNotFound`
without a `Renderer`, and the `AnimationSystem` without a `Clock`.

The `layer` of each drawable (`Sprite`, `Text`, `Rectangle`, `Circle`, `Line`)
is the draw order: the bigger layer is drawn on top (default 0), and the `z` of
the `Transform` is ignored.

## Example

```cpp
namespace render = ampersand::render;

render::SfmlRenderer sfml;  // the only line that names the backend
auto& window = world.addResource<render::Renderer>(
    sfml, render::WindowConfig{.size = {1280, 720}, .title = "R-Type"});
const auto ships = window.renderer.loadTexture("assets/ships.png");
const auto font = window.renderer.loadFont("assets/font.ttf");

world.addResource<ampersand::core::Clock>();
world.addResource<render::ClearColor>().value = {.r = 8, .g = 8, .b = 24};

const ecs::Entity ship = world.create();
world.add(ship, ampersand::core::Transform{});
world.add(ship, render::Sprite{.texture = ships, .color = {.r = 255, .g = 80, .b = 80}});
world.add(ship, render::Animation{.frames = {{0, 0, 33, 17}, {33, 0, 33, 17}}});

const ecs::Entity score = world.create();
world.add(score, ampersand::core::Transform{});
world.add(score, render::Text{.font = font, .value = "0000"});

render::AnimationSystem animation;
render::RenderSystem draw;  // finds the renderer in the Renderer resource

while (window.isOpen) {
    while (const auto event = window.renderer.pollEvent()) {
        if (std::holds_alternative<render::Closed>(*event)) window.isOpen = false;
    }
    world.getResource<ampersand::core::Clock>().dt = frameTime();
    animation.update(world);
    draw.update(world);  // always last: it displays the frame
}
```

## Window

The `Renderer` resource is the one place where the backend is given: adding it
opens the window with the `WindowConfig`, and every system then finds the
renderer and the window in the World, without holding a reference itself.

```cpp
const auto& window = world.getResource<render::Renderer>();
window.renderer;  // IRenderer: load textures and fonts, poll events, draw by hand
window.size;      // size of the window in pixels
window.title;     // title given when it was opened
window.isOpen;    // false once the window is closed
```

At the start of each update, the `RenderSystem` keeps the resource up to date:

- `isOpen = false` (set by the game) closes the window, and nothing is drawn.
- A window closed another way (`window.renderer.close()`) sets `isOpen` back to
  `false`.
- `size` is copied from the window, so it follows the user resizing it.

Only `isOpen` acts on the window: writing `size` or `title` changes nothing
(the next update overwrites `size`). If the window cannot be opened, `isOpen`
is `false` from the start.

The backend (`sfml` in the example) is not copied: it must stay alive as long
as the World, so declare it before the World.

## Sprites

A texture is loaded once and gets an id (`TextureId`, `FontId` for fonts).
Components only keep this id, so many entities share the same image. A failed
load returns `0`, and an entity with an unknown id is not drawn.

`source` is the part of the texture to draw, in pixels. It lets one image (a
spritesheet) hold many sprites. An empty `source` draws the whole texture.

```
ships.png
x: 0     33    66
   +-----+-----+-----+  y: 0
   |  ▶  | ▶▶  | ▶▶▶ |   blue ship
   +-----+-----+-----+  y: 17
   |  ▶  | ▶▶  | ▶▶▶ |   red ship
   +-----+-----+-----+

Sprite{.texture = ships, .source = {33, 17, 33, 17}}  → red ship, 2nd frame
```

`color` is multiplied with the image: white (the default) keeps the image as is.
With a white or grey image, one texture gives a ship of each color, one per
player.

## Animation

`frames` is the list of the `source` rects to show, one after the other, each
during `frameDuration` seconds. They can be anywhere on the texture, in any
order.

- `loop = true` (default): starts again after the last frame.
- `loop = false`: stays on the last frame (an explosion that the game destroys
  after).
- Set `elapsed` to `0` to start again from the first frame.

When every frame of a spritesheet has the same size, `framesFromGrid` builds the
list instead of writing each rect. Frames are read left to right, then top to
bottom:

```cpp
// 8 frames of 33x17, 4 per row, 1 pixel between them, starting at (0, 34)
world.add(ship, render::Animation{.frames = render::framesFromGrid({
    .frameSize = {33, 17}, .columns = 4, .count = 8, .gap = {1, 1}, .offset = {0, 34}})});
```

`gap` and `offset` default to `{0, 0}`. It throws `std::invalid_argument` if
`columns`, `count` or a side of `frameSize` is not positive.

Alone, an `Animation` only plays: what comes next is decided by the game (or by a [state machine](#state-machine)). `isFinished(animation)` is true once a non-looping animation was fully played,
so a system of the game can switch back to another one:

```cpp
// when the ship is hit
world.get<render::Animation>(ship) = hitAnimation();  // loop = false
world.add(ship, Hit{});                               // a component of the game

// a system of the game, every frame
std::vector<ecs::Entity> done;
for (const ecs::Entity entity : world.owners<Hit>()) {
    if (render::isFinished(world.get<render::Animation>(entity))) done.push_back(entity);
}
for (const ecs::Entity entity : done) {  // not inside the loop: owners() is the real list
    world.get<render::Animation>(entity) = idleAnimation();
    world.remove<Hit>(entity);
}
```

## State machine

An `AnimationStateMachine` holds every clip of an entity by name, and the
`AnimationSystem` copies the one to play into the entity's `Animation` (added
if missing). The game only writes `params`, and the machine switches the clips:

```cpp
render::AnimationStateMachine machine{{{"idle", idleClip()},
                                       {"run", runClip()},
                                       {"jump", jumpClip()},
                                       {"attack", attackClip()}},
                                      "idle"};  // initial base state
machine.transitions = {
    {.from = "*", .to = "jump",
     .condition = [](const render::AnimationParams& p) { return p.bools.at("jumping"); }},
    {.from = "idle", .to = "run",
     .condition = [](const render::AnimationParams& p) { return p.floats.at("speed") > 0.0F; }},
    {.from = "run", .to = "idle",
     .condition = [](const render::AnimationParams& p) { return p.floats.at("speed") == 0.0F; }},
};
machine.params.bools["jumping"] = false;
machine.params.floats["speed"] = 0.0F;
world.add(player, std::move(machine));

// gameplay, every frame
auto& animator = world.get<render::AnimationStateMachine>(player);
animator.params.floats["speed"] = std::abs(velocity.x);
if (attackPressed) animator.playOnce("attack");
```

- **Base state** (`current()`): follows the transitions. They are checked in
  order and the first one whose `from` is the current state (or `"*"` for any
  state) and whose `condition` returns true wins, even if it leads to the
  current state: the order of the list is the priority. At most one transition
  per update. `setCurrent(state)` moves there directly.
- **Action** (`playOnce(state)`): plays a state once, without looping, on top
  of the base state, then gives it back. The transitions are paused while it
  plays. A new action replaces the playing one, and the same action again
  restarts it. `isPlayingAction()` is true until its last frame was shown for
  its whole duration.
- Each time the clip to play changes, it starts again from its first frame.
  After editing `states` while the game runs, call `reload()` to load the
  playing clip again.
- The entity's `Animation` belongs to the machine: do not write it by hand, it is replaced at the next clip change. If it is removed, the `AnimationSystem` adds it back and loads the playing clip again.

The constructor, `setCurrent` and `playOnce` throw `std::invalid_argument` for
a state that is not in `states`, and so does the `AnimationSystem` for a
transition to an unknown state (the base state is then left as it was).

## Camera

The `Camera` is the rectangle of the world shown in the window: `center` is the
world point in the middle of the window, `size` the width and height shown.
Without it, one world unit is one pixel of the window.

For R-Type, set it once to a fixed size. Every client then shows the same part
of the world, whatever the size of its window:

```cpp
world.addResource<render::Camera>() = {.center = {960.0F, 540.0F},
                                       .size = {1920.0F, 1080.0F}};
```

To follow a player, change `center` every frame. To zoom, change `size`. If the
window does not have the same ratio as `size`, the image is stretched.

## Shapes

`Rectangle`, `Circle` and `Line` draw plain shapes without a texture. They are placed by the `Transform` like a `Sprite`: the shape starts at the local (0, 0), the `origin` is its pivot, and the `scale` stretches it (and its outline). Give the size in pixels with `size` or `radius` and keep a scale of 1, so the children of the entity are not stretched with it.

```cpp
const ecs::Entity button = world.create();
world.add(button, core::Transform{.position = {100.0F, 50.0F, 0.0F}});
world.add(button, render::Rectangle{.size = {160.0F, 48.0F},
                                    .fill = {58, 66, 80, 255},
                                    .outline = {255, 255, 255, 70},
                                    .outlineThickness = 2.0F,
                                    .layer = 10});

const ecs::Entity range = world.create();
constexpr float kRadius = 150.0F;
world.add(range, core::Transform{.origin = {kRadius, kRadius, 0.0F}});  // centered
world.add(range, render::Circle{.radius = kRadius, .fill = {255, 255, 255, 40}});
```

## Drawing by hand

`IRenderer` (`window.renderer`) can also draw without the ECS: `beginFrame`,
then `draw` (with a `Sprite`, a `Text` or a `Rectangle`, `Circle` or `Line`),
then `endFrame`. The `RenderSystem` already does `beginFrame` and `endFrame`,
so use one or the other, not both in the same frame.

## Limits

| Rule       | Value                                                                                            |
| ---------- | ------------------------------------------------------------------------------------------------ |
| Dimensions | 2D only: the `z` of the `Transform` is ignored, `layer` is the draw order                        |
| Shapes     | `Rectangle`, `Circle` and `Line` only, without texture; a `Line` is always 1 pixel thick          |
| Animation  | one clip at a time per entity: no blending between two clips, a switch shows the new one at once |
| Camera     | one for the whole window, stretched if the window does not have the ratio of its `size`          |
| Assets     | a failed load returns `0`, and an entity with an unknown id is not drawn                         |
| Window     | one per `Renderer`; only `isOpen` acts on it, `size` and `title` are read-only                   |
