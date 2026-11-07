# Comparative study

Why we use each technology of the engine, compared with its alternatives.
The full story of the ECS and Lua is in [Design decisions](decisions.md).
The game side (protocol, accounts, passwords) is in the
[R-Type decisions](https://boz0o0.github.io/R-Type/decisions/).

## Technologies

| Need | Chosen | Alternatives | Why |
| ---- | ------ | ------------ | --- |
| Language | **C++20** | C++17, C++23 | designated initializers, `std::span`, `requires`; C++23 is still partial in MSVC |
| Dependencies | **CMake + CPM** | Conan, vcpkg | one file, no extra tool, versions written in `CMakeLists.txt` |
| Architecture | **sparse set ECS** | inheritance, Unity-like components, archetypes | decoupled data and logic, simple; archetypes only pay off with many thousands of entities |
| Modularity | **compile time + static libraries** | run-time plugins | component ids are global to the program and C++ has no stable ABI across compilers; Lua covers run-time extension |
| Graphics, audio, input | **SFML 3** | SDL 3, Raylib, OpenGL | C++ API, audio built in, known by the whole team; hidden behind `IRenderer` / `IAudioEngine` |
| Network | **Asio standalone** | BSD sockets + Winsock, Boost.Asio, ENet | same code on Linux and Windows, no Boost; ENet would do the work for us |
| Packets | **field by field, big-endian** | `memcpy` of structs, Protobuf | same bytes on every machine, every read checked, no code generator |
| Data files | **JSON (nlohmann/json)** | YAML, TOML, binary | see [Storage](#storage) |
| Scripting | **Lua 5.4 + sol2** | Python, ChaiScript, AngelScript | small, fast, made to be embedded, per-script globals |
| Editor | **Dear ImGui** | Qt, our own UI | made for engine tools, a few files; Qt is huge |
| Tests, quality | **GoogleTest, clang-format, clang-tidy** | Catch2, cppcheck | CTest integration, run by the CI on Linux and Windows |
| Docs | **MkDocs Material + Doxygen** | Sphinx, GitBook | Markdown, search, diagrams, published by the CI |

## Algorithms and data structures

| Problem | Choice | Why |
| ------- | ------ | --- |
| Components | one packed array per type, **one row per entity** (our own) | the row is both the signature and the position: nothing to keep in sync |
| Removing a component | swap-and-pop | O(1), no holes |
| Entity handles | index + version | an old handle to a reused row is seen as dead |
| Destroying during a frame | command queue, applied at the end of the frame | no system loses an entity another one uses |
| Events | `EventQueue`, emptied each frame | no callbacks, events are data |
| Time | `dt` capped at 0.1 s, or a fixed step | same speed on every computer |
| Collisions | every pair tested, O(n²) | enough for a few hundred hitboxes; a grid can replace it without changing games |
| Prefab overrides | merge and diff (our own) | the editor writes only the fields that differ from the prefab |
| Scene loading | undone on error (our own) | a broken scene never leaves half a level |

## Storage

The engine only stores game content (configuration, scenes, prefabs). Player
data belongs to the game (R-Type uses SQLite).

| Format | Verdict |
| ------ | ------- |
| **JSON** | readable diffs in Git, editable by hand, strict syntax: **chosen** |
| YAML | indentation errors and implicit types break silently |
| Binary | fast, but not readable nor diffable |
| SQLite | great for player data, overkill for level files |

Reliability: every file has a `version`, bad files give an error with the path
of the bad value, and the editor asks before losing changes.
Limits: the editor writes files in place (not atomic), and the World cannot be
saved back to JSON.

## Security

| Technology | Risk | What we do | Left to do |
| ---------- | ---- | ---------- | ---------- |
| C++ | memory errors | RAII, no raw owning pointers, `std::span`, a strict ECS API that throws, clang-tidy | sanitizers in the CI |
| UDP | malformed packets | `BinaryReader` checks every read (size, string length, NaN, enum range) and never throws | encryption and rate limits are the game's job |
| Lua | scripts touching the system | only `base`, `math`, `string` and `table` are opened: no `io`, `os`, `package`; a failing script is turned off | remove `dofile` / `loadfile`, limit script time |
| JSON | malformed files | errors are caught, the load is undone | paths are not kept inside `gameRoot` |
| Dependencies | vulnerable versions | every version is fixed in `CMakeLists.txt` | check archive hashes |

**Monitoring:** the CI builds, tests and lints every pull request on Linux and
Windows; dependency updates are pull requests noted in the
[changelog](changelog.md); script and ECS errors are reported so a server logs
them and keeps running.
