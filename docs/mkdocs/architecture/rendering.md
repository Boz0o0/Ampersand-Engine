# 2D rendering

The render module exposes an SFML-backed renderer through `IRenderer`. It
supports textured sprites, basic shapes, text, a 2D view, and 3D transforms.

## Frame flow

Open a renderer, load textures and fonts, then submit draw calls between
`beginFrame` and `endFrame`. Draw commands are collected for that frame and
rendered by ascending Z position; commands at the same Z keep their submission
order. The default view uses window pixel coordinates. Set a custom
`View` for world-space camera movement or call `resetView` to restore the
window-sized view. Set the view before submitting that frame's draw commands.
Every draw call takes an `ampersand::core::Transform`. Its X and Y position,
scale, rotation around Z, and origin affect 2D geometry; Z position determines
draw order and must be finite. The renderer currently draws 2D content.

```cpp
ampersand::render::SfmlRenderer renderer;
renderer.open({{1280, 720}, "Demo"});

const auto texture = renderer.loadTexture("assets/ship.png");
const auto font = renderer.loadFont("assets/font.ttf");
const ampersand::render::Sprite ship{
    texture, {0, 0, 32, 32}};

ampersand::core::Transform transform;
transform.position = {200.0F, 180.0F, 10.0F};
transform.scale = {2.0F, 2.0F, 1.0F};
transform.rotationDegrees = {0.0F, 0.0F, 15.0F};
transform.origin = {16.0F, 16.0F, 0.0F};

renderer.beginFrame({12, 16, 28});
renderer.draw(ship, transform);
ampersand::core::Transform backgroundTransform;
renderer.draw(ampersand::render::Rectangle{{100.0F, 40.0F}, {40, 180, 80}},
              backgroundTransform);
renderer.drawText(font, "Ready", 24, {255, 255, 255},
                  ampersand::core::Transform{});
renderer.endFrame();
```

For a camera, `View` specifies its world-space center and visible size:

```cpp
renderer.setView({{320.0F, 180.0F}, {640.0F, 360.0F}});
```

`Rectangle`, `Circle`, and `Line` provide basic primitives. Rectangles and
circles support an optional outline through their `outline` and
`outlineThickness` fields.

## Sprite animation

`SpriteAnimation` advances a linear sequence of sprite frames when `update` is
called with elapsed seconds. It loops continuously; retrieve `currentFrame()`
and pass it to the renderer as an ordinary sprite.

```cpp
using ampersand::core::math::Rect;
using ampersand::render::Sprite;
using ampersand::render::SpriteAnimation;

SpriteAnimation animation(
    {Sprite{texture, Rect<std::int32_t>{0, 0, 32, 32}},
     Sprite{texture, Rect<std::int32_t>{32, 0, 32, 32}}},
    0.12F);

animation.update(deltaSeconds);
renderer.draw(animation.currentFrame(), transform);
```

The frame list must be non-empty and the frame duration must be finite and
positive. Negative or non-finite elapsed times are rejected.
