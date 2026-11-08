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

Or from the `resources` of the [configuration](scene.md#files) (not of a scene,
so the history is not reset by each scene):

```json
"resources": { "Stats": {} }
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

## Network traffic

A `UdpSocket` counts its own [traffic](net.md#traffic). The `NetStatsSystem` of
the net module writes it in `values`, if the World has a `Stats`:

```cpp
#include <ampersand/net/systems/NetStatsSystem.hpp>

net::UdpSocket socket;
scheduler.add<net::NetStatsSystem>(socket);  // the socket must outlive it
```

| Key                         | What it holds                                         |
| --------------------------- | ----------------------------------------------------- |
| `net.kbSentPerSec`          | kilobytes (1000 bytes) sent per second                |
| `net.kbReceivedPerSec`      | kilobytes received per second                         |
| `net.packetsSentPerSec`     | datagrams sent per second                             |
| `net.packetsReceivedPerSec` | datagrams received per second                         |
| `net.packetsDropped`        | datagrams dropped by `send` since the socket was opened |

Like `fps`, the rates are measured with the real clock over about one second,
and stay at 0 during the first one. The totals of bytes are not written:
`values` holds floats, which lose precision past about 16 MB. A second argument
changes the interval (`scheduler.add<net::NetStatsSystem>(socket, 500ms)`).

```cpp
hud.value = std::format("{:.0f} FPS  up {:.1f} kB/s  down {:.1f} kB/s", stats.fps,
                        stats.values["net.kbSentPerSec"],
                        stats.values["net.kbReceivedPerSec"]);
```

## Limits

| Rule      | Value                                                                      |
| --------- | -------------------------------------------------------------------------- |
| Filled    | only by the `Scheduler`, at the end of an `update` that did not throw      |
| `fps`     | updated about once per second, not every frame                             |
| `history` | fixed size (120 frames), the oldest frame is dropped                       |
| `values`  | floats, never cleared by the engine                                        |
| Scripts   | not bound to Lua yet                                                       |
