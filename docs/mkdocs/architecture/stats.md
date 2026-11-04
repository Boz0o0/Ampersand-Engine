# Stats

The `Stats` resource holds the metrics of the game loop: frames per second, time
of a frame, number of entities. The `Scheduler` fills it after each frame, and
the game shows it as it wants (a HUD text, a log, a message to the server).

## Concepts

| Name        | Kind     | What it does                                                          |
| ----------- | -------- | --------------------------------------------------------------------- |
| `Stats`     | resource | the metrics, in `<ampersand/core/resources/Stats.hpp>` (core module)  |
| `Scheduler` | class    | fills `Stats` at the end of each `update`, if the World has it        |

The `Scheduler` does not add `Stats` itself: without it, nothing is measured.

```cpp
world.addResource<core::Stats>();  // turns the stats on
```

## Fields

| Field      | What it holds                                                                                |
| ---------- | -------------------------------------------------------------------------------------------- |
| `frameMs`  | milliseconds between the start of the previous frame and the start of this one (0 the first time) |
| `updateMs` | milliseconds spent in the last `update`: the systems, then the `CommandQueue` and event queues |
| `fps`      | frames per second, averaged over about one second (0 during the first second)               |
| `entities` | entities alive at the end of the last `update` (`world.entityCount()`)                      |
| `history`  | the last `Stats::kHistory` (120) values of `frameMs`, oldest first, to draw a graph         |
| `values`   | metrics of the modules and of the game, by name                                              |

The times come from the **real clock**, not from the `dt` given to `update`. A
server with a fixed step (`scheduler.update(kTick)`) still sees how long its
ticks take: `updateMs` above `kTick` means the server cannot keep up.

`history.front()` is the oldest frame and `history.back()` the newest one: each
frame moves the others one slot to the front.

```cpp
for (const float ms : stats.history) {  // from the oldest to the newest
    graph.push(ms);
}
```

## Values of the game

What the engine cannot measure (the ping, the packets lost, the position of the
player) goes in `values`. The `Scheduler` never clears it: each value stays until
the game changes or erases it. Name the keys `<who>.<what>`.

```cpp
auto& stats = world.getResource<core::Stats>();
stats.values["game.ping"] = connection.pingMs();

hud.value = std::format("{:.0f} FPS  {} entities  {:.0f} ms ping", stats.fps,
                        stats.entities, stats.values["game.ping"]);
```

## Limits

| Rule      | Value                                                                      |
| --------- | -------------------------------------------------------------------------- |
| Filled    | only by the `Scheduler`, at the end of an `update` that did not throw      |
| `fps`     | updated about once per second, not every frame                             |
| `history` | fixed size (120 frames), the oldest frame is dropped                       |
| `values`  | floats, never cleared by the engine                                        |
| Scripts   | not bound to Lua yet                                                       |
