# Accessibility

What the engine gives a game to be accessible. **Done** is in the engine,
**Possible** means the game adds it in a few lines, **Not yet** is planned.

## Physical and motor

| Need | Status | How |
| ---- | ------ | --- |
| Change every key | **Done** | games read named buttons and axes, set in `config.json` ([Input](architecture/input.md)) |
| One hand, mouse only, adapted controllers | **Done** | several bindings per button (key, mouse, controller); any controller works |
| Same keys on every keyboard | **Done** | keys are physical positions (AZERTY, QWERTY) |
| Slower game, pause | **Possible** | `scheduler.update(dt * speed)`, `scheduler.update(0)` |
| Toggle instead of hold | **Possible** | `getButtonDown` to flip a state |
| Remapping menu | **Not yet** | |

## Visual and hearing

| Need | Status | How |
| ---- | ------ | --- |
| Bigger image and text | **Done** | the `Camera` scales the game to the window; `Text.size` |
| Colour-blind palettes | **Possible** | colours are data in JSON; shapes and outlines so nothing relies on colour only |
| No fast flashes | **Possible** | `Animation.frameDuration` is data |
| Separate volumes | **Done** | per sound, music and master (`AudioListener.volume`) |
| See the sounds, subtitles | **Possible** | every sound is an entity with an `AudioSource`, a system can show it |
| Screen reader | **Not yet** | SFML has no screen reader support |

## Mental and cognitive

| Need | Status | How |
| ---- | ------ | --- |
| Easier game | **Possible** | rules in Lua and JSON, changed per difficulty without recompiling |
| A game that does not break | **Done** | a broken script is turned off, a broken scene is undone |
| Easy to learn | **Done** | names follow Unity; content made in the editor or JSON, without C++ |

## Editor and documentation

- **Editor**: keyboard shortcuts, asks before losing changes. Not yet:
  keyboard navigation, interface scale, icons next to the colour cues.
- **Documentation**: a website with search, a menu and light and dark themes,
  zoomable, in simple English, with text next to every diagram and `alt` on
  images.
