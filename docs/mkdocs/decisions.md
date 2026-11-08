# Design decisions

This page explains the main choices we made for the engine, and why.
For how things work, see [Architecture](architecture/index.md).

## Comparative study
The ECS, the modules and Lua are explained after the tables, and the choices of the game in the [R-Type decisions](https://boz0o0.github.io/R-Type/decisions/).

### Technologies
| Need | Choice | Alternatives | Why |
| --- | --- | --- | --- |
| Language | C++20 | C++17 | imposed C++; C++20 gives designated initializers, `std::span` and ranges, used everywhere |
| Build | CMake | Meson, Xmake | known by the team, supported by every IDE and CI |
| Dependencies | CPM | Conan, vcpkg | one file in `cmake/`, one call per dependency with its version; Conan needs Python, vcpkg its own tool |
| Architecture | our own sparse set ECS | OOP inheritance, bitset ECS, archetype ECS | data apart from the logic, nothing to keep in sync ([ECS](#ecs)) |
| Modularity | one static library per module | run-time plugins | C++ has no stable ABI across compilers; Lua covers run-time extension ([Separation of modules](#separation-of-modules)) |
| Graphics, audio, input | SFML 3 | SDL 3, Raylib | C++ API, audio built in, known by the whole team |
| Sockets | Asio standalone | POSIX sockets + Winsock, SFML Network | the same code on Linux and Windows, without Boost |
| Data files | JSON | YAML, binary | readable diffs, editable by hand; YAML breaks silently |
| Scripting | Lua 5.4 + sol2 | Python, ChaiScript | small, fast, made to be embedded ([Why Lua](#why-lua)) |
| Editor | Dear ImGui | Qt | made for engine tools, a few files; Qt is huge |
| Documentation | MkDocs Material + Doxygen | Sphinx, GitBook, a wiki | Markdown in the repository, search and diagrams, published by the CI |

### Algorithms and data structures
| Need | Choice | Alternatives | Why |
| --- | --- | --- | --- |
| Components | one packed pool per type, one row per entity | archetypes | finding a component is direct |
| Entity handles | row + version | raw index | an old handle to a reused row is seen as dead |
| Destroying during a frame | `CommandQueue` | destroying at once | no system loses an entity another one uses |
| Collisions | every pair tested, O(n²) | grid, quadtree | enough for a few hundred hitboxes |

### Security
| Need | Choice | Alternatives | Why |
| --- | --- | --- | --- |
| Malformed packets | every read checked, never throws | exceptions | a bad packet is dropped, nothing crashes |
| Scripts | only `base`, `math`, `string`, `table` | every Lua library | no access to files or the system |
| Game files | the load is undone on error | loading what can be read | never half a level |
| Dependencies | fixed versions, changed by pull request | latest versions | every update is reviewed and tested by the CI |

## ECS

### Why an ECS
For the game engine architecture, we decided to use an ECS, which we think is a good fit and is a 'norm' in game development nowadays, mostly because it separates the data from the logic. We considered other alternatives, firstly the classic C++ OOP inheritance, but it was fastly dropped, because is too rigid and couples data and behavior, which does not fit an engine that has to be focused on decoupling.

### First choice: a bitset ECS
From all the variants of ECS, we first decided to go for a bitset-based ECS, using mostly the bases set by [Austin Morlan](https://austinmorlan.com/posts/entity_component_system/). We had to choose from the different types of ECS architectures (which of course we'd have to remix). We did not consider the archetype-based one (used by UE's Mass framework and Unity DOTS) because it is way more complex and 'pays off' only at thousands of entities, which we think will not be our case. This left us with 2 choices: the sparse set ECS and the bitset ECS. The sparse set ECS keeps each component type packed in its own array, so a system only goes through the entities that have the component, and adding, removing or finding a component is direct, the cost being one lookup per component type when a system needs several of them. The bitset ECS, on the other hand, checks all the components of an entity in a single bitwise operation, but its bitset only says whether an entity has a component, not where it is, so it needs other structures next to it that all have to be kept in sync.
Lastly, one team member (Enzo) had already worked with a bitset ECS in the past (Zappy), so he was more comfortable with it, and we also thought that the sparse set ECS would be more complex to implement.

### Why we changed
When we started implementing it, we realised that Morlan's ECS is in fact already a sparse set (using hash maps), with the bitsets and the lists of each system added on top of it, which all have to be kept in sync. So instead of patching it, we decided to build our own ECS, one that we fully understand, and that is easier to understand for the whole team.

### Final choice: a sparse set with one row per entity
We went for a sparse set ECS, where each component type is packed in its own pool, but instead of having one lookup array per pool (like [EnTT](https://github.com/skypjack/entt)), each entity has a single row that holds the position of each of its components. This row works as the 'signature' of the bitset ECS and also says where the components are, so checking or reading a component is direct and there is nothing to keep in sync. The cost is a limit of 64 component types and a bit of memory per entity, which is fine for a game like R-Type. How it works in detail: [ECS](architecture/ecs.md).

## Separation of modules
We decided to separate the engine into different modules, each with its own responsibilities. The objective was to have a clear separation of concerns, which would make the engine easier to maintain and extend, and the compilation more effcient, by compiling only the necessary modules.

## Scripting

### Why Lua
We wanted the behaviour of the entities to be written outside of C++, so it can be changed without recompiling. We compared Lua, Python and ChaiScript. Python is the best known, but its interpreter is big and hard to embed and to keep apart per script. ChaiScript is made for C++, but it is slow and no longer maintained. Lua is small (a few dozen C files), fast, made to be embedded, and is the usual choice in games (World of Warcraft, Roblox, Defold), so it was the clear choice. It also lets each script get its own globals (its environment) at almost no cost.

### Why sol2
Lua's C API works with a stack that has to be handled by hand, which is easy to get wrong. [sol2](https://github.com/ThePhD/sol2) wraps it in C++ types: it calls Lua functions safely (an error becomes a value, not a crash) and turns C++ types into Lua ones by itself. It is header-only and widely used. We checked that it builds with the Lua version we use (5.4): sol2 does not support Lua 5.5 yet.

The official Lua sources have no CMake file, so CMake downloads them and builds them as the `lua_static` library.
