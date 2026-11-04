# Editor

The editor is a desktop app (made with [Dear ImGui](https://github.com/ocornut/imgui)
and [ImGui-SFML](https://github.com/SFML/imgui-sfml)) to edit the JSON files of
a [game folder](architecture/scene.md#game-folder) without writing them by
hand: the configuration, the scenes, their entities and their components.

It edits the **files**, not a running game: there is no game view yet, and the
changes are only written to the disk on **Build**.

## Build and run

The editor is off by default. It needs the render and scene modules:

```sh
cmake -B build -DAMPERSAND_BUILD_EDITOR=ON
cmake --build build --target ampersand_editor_app --parallel
./build/editor/src/ampersand_editor [game folder]
```

ImGui (docking branch), ImGui-SFML and
[nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended)
are downloaded by CMake. Without an argument, the editor starts empty: open or
create a project from the **File** menu.

## Projects

A project is a game folder with a readable `world/config.json` holding a JSON
object. The editor shows its `assets/`, `scripts/` and `world/` folders.

| Action         | Shortcut | What it does                                                                                           |
| -------------- | -------- | ------------------------------------------------------------------------------------------------------ |
| Open           | `Ctrl+O` | picks a game folder with the system dialog                                                             |
| Create project | -        | picks a parent folder, asks a name, and creates the folders, a `config.json` and a `defaultScene.json` |
| New Scene      | `Ctrl+N` | creates `world/scenes/<name>.json`, empty                                                              |
| Build          | `Ctrl+B` | writes the changed files to the disk                                                                   |
| Quit           | -        | closes the editor                                                                                      |

A new scene is **not** added to the `scenes` of `config.json`: a folder can
hold scenes the game does not use. Add it in the **World** inspector for
`SceneManager::loadScene` to find it (see [Inspector](#inspector)).

The layout of the panels is saved per project, in `<game folder>/imgui.ini`.

## Panels

| Panel     | What it shows                                                                                       |
| --------- | --------------------------------------------------------------------------------------------------- |
| Files     | the `assets/`, `scripts/` and `world/` folders; **Refresh** lists them again from the disk          |
| Hierarchy | the opened scene: the **World** (`config.json`), the **Scene resources** and the tree of entities  |
| Inspector | what is selected in the Hierarchy                                                                   |
| Viewport  | nothing yet                                                                                         |

### Files

Double-click a `.json` file to open it as the scene. The file must have the
shape of a scene (`"version": 2`, an `entities` array, and no `components`,
`children` or `prefab` key): a prefab or the configuration is refused.

Drag a file onto a field that holds a path (`texture`, `font`, `sound`,
`music`, `path`, or an item of `scripts`) to set it to the path of the file,
relative to the game folder.

### Hierarchy

Right-click an entity for **Create Empty**, **Create Child**, **Duplicate**,
**Delete** and **Rename**, or the empty space below the tree to create an
entity at the root. Drag an entity onto another one to make it its child, or
onto the empty space to move it to the root. An entity cannot be moved into
itself or one of its children.

A new entity is named `Entity` and gets a default `Transform`. An entity
without a `name` is shown with the name of its prefab, or as `Entity N`.

Prefab instances are shown in blue. The children they get from their prefab
are greyed out: they are not in the scene file, so they cannot be selected.

### Inspector

| Selection       | What can be edited                                                                     |
| --------------- | -------------------------------------------------------------------------------------- |
| World           | the `startScene`, `scenes` and `resources` of `config.json`, including the resources only allowed there (`Renderer`...) |
| Scene resources | the `resources` of the scene                                                           |
| Entity          | its name, `persistent` (root entities only) and its components                         |

**Add component** and **Add resource** list the types of the modules that are
built, with their default values (read from the engine
[serializers](architecture/scene.md#engine-types)). Each field gets the widget
of its shape: drag fields for numbers, vectors and rects, a colour picker for
`{r, g, b, a}`, a list for the names a field accepts (input bindings, joystick axes), and a
tree for the other objects and arrays.

On a prefab instance, the components are shown merged with the prefab. The
fields that differ from the prefab are shown in orange, with a button to go
back to the value of the prefab, and only these fields are written to the
scene (like the overrides of a Unity prefab).

In the **World**, **Scenes** lists the scenes of the game, by name: **+**
adds one, **-** removes one, and each has a name and a file path (drop a scene
file from Files on it). **Start scene** picks the scene loaded first among
them; it is shown in red when it is not one of the scenes, since the engine
refuses to load the configuration then. Renaming the start scene in
**Scenes** renames it in **Start scene** too.

## Build

The editor keeps the files in memory. A changed file is marked with `*` in the
Hierarchy (the scene name, or **World** for `config.json`).

**Build** writes each changed file (the configuration and the opened scene)
with an indentation of 4 spaces, in the order of its keys. The files that did
not change are not written. If a file cannot be written, an error is shown and
the file stays marked as changed.

Prefabs are only read: the editor never changes them.

## Unsaved changes

Before an action that would lose changes, the editor asks to **Build**,
**Discard** them or **Cancel** the action:

| Action                                           | Asks when                                 |
| ------------------------------------------------ | ----------------------------------------- |
| Quit, or closing the window                      | a file is changed                         |
| Open, Create project                             | a file is changed                         |
| Open another scene (double-click in Files)       | the opened scene is changed               |

The changes of `config.json` are kept when another scene is opened, so they do
not need a build first. If the build fails, the action is not done.

## Limits

- One scene is opened at a time.
- No undo.
- No game view: the Viewport is empty, and the game must be run on its own.
- Prefab files cannot be edited.
- `gameRoot` cannot be edited.
