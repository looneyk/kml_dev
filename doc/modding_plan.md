# Modding and Data-Driven Design --- Plan

Date: 2026-09-22, revised 2026-10-17

Every game built on the system is moddable, and custom content --- objects, geometry, visual effects and behavior ---
is added as data and script without recompiling. Mod additions are unpredictable, so the system is built from
definitions loaded at runtime, with C++ supplying the mechanisms and the inner loops. Principle 8 of
`game_system_principles.md` states the goal, and `decision_rationale.md` ("Everything a game adds is data or script")
argues for it.

A room whose walls are fun-house mirrors is the test case: a mod adds it with a style asset, a mesh, a shader and at
most a script, and nothing in C++ changes.

## Mod contents

A mod is a directory that the game loads beside its own data, containing its manifest and any of:

- component schemas;
- types;
- styles, with the meshes, materials and shaders they refer to;
- zone files and generators;
- scripts;
- patches to types and styles loaded before it.

The game's own content is loaded the same way, as the first mod.

## A mod's identity

- **Each mod's directory has a manifest, `mod.json`,** with the mod's id and version.
- **A mod's settings are in the section `mod.<id>`** (`settings_plan.md`, `### Mods' settings`).
- **A preset that a mod adds to another's setting group is named `mod.<id>.<name>`,** so two mods cannot add presets
  of the same name to one group.
- **The mod list, `mods/mods.json`, lists each mod the user has installed and whether it is enabled.** The mod menu writes it whenever the user installs,
  enables, disables or removes a mod.

## A missing mod

A missing mod produces at least a warning, and play continues without it if the user chooses. A mod removed
deliberately never makes a game that used it unplayable. A mod that is missing without having been removed is damage
to the installation, and the user is told.

- **A mod that is missing and not enabled in the mod list was removed deliberately.**
- **A mod that the mod list has as enabled, but whose directory is gone or whose files fail to load, is damage.** It is
  reported to the user at startup, by name and with what failed, not only logged. A directory deleted by hand counts as
  damage, since the program cannot tell the difference, and the report lets the user confirm the removal.
- **`game.json` lists each mod the game uses,** with its id and version, in load order, and is read before the
  settings.
- **Opening a game with a missing mod warns the user,** who chooses to cancel, to continue without the mod, or to remove
  it from the game.
  - **Continuing** keeps the mod's saved setting values and selected presets as text, untouched, and writes them back on
    save, so reinstalling the mod restores them. A group whose selected preset is the missing mod's has its default
    preset selected meanwhile. The warning appears again the next time the game is opened.
  - **Removing it from the game** drops the mod's saved values and its entry in the game's mods, and the warning stops.
  - Either way, the game plays, whatever the mod declared is skipped, and its patches are not applied.
- **A key that a loaded mod no longer defines** is dropped from a saved file with a warning. Any other key that nothing
  defines is an error.

## Components

- **A component is typed in the world.** Loading a zone decodes each component object in the file into the component
  registered under its name, through flecs reflection, and keeps no JSON. `load_layout` decodes them from the
  `Document` as it builds the zone (`game_system_reference.md`, `### load_layout`).
- **Schemas come from C++ or from data.** The game registers its components in C++. A mod declares its components and
  their fields in a schema file, which becomes a flecs runtime component with reflection that is stored, decoded,
  saved, and accessed from script.
- **An unknown name is an error,** except for a component declared by a mod that is not loaded, which is skipped with
  a warning.
- **Saving writes components through the same reflection,** so mod data persists with the zone.

## Types

- **A type is a prefab defined in data:** its components, its model, and its child entities. An entity is `IsA` its
  type, so the type's components are its defaults and nothing is copied. A door is a `door` type with its door facings
  as children.
- **Types inherit from types** in data, and the chain is flattened when loaded (`game_system_principles.md`,
  Decisions).
- **A mod patches a type or a style** that it did not define, adding, replacing or removing components and fields.

## Styles and visuals

- **A style is an asset name and a parameter map.** The asset refers to a material, a shader and meshes; the parameters are
  named numbers, vectors, colors and asset references, passed to the shader by name.
- **A style inherits along the zone's chain** --- entity, room, layout and type --- with its parameters merging by
  name, so a zone overrides part of a style by stating only that part.
- **A style may replace a surface's mesh.** A wall, floor or ceiling is drawn from the kit piece its style refers to
  instead of as a flat face, so a curved mirror is a mesh. Collision is unchanged: a wall is still a bit on its cell.
- **A view resolves a style,** as it resolves glyphs and tiles from its presentation table. The simulation refers only
  to the name.

## Geometry

- **A zone's geometry is its file**, written by a tool or a generator, including a mod's.
- **A generator is a script** or a recipe of registered passes, configured in data (`game_system_principles.md`,
  Phase 5).
- **A surface's form comes from its style,** as above.
- **A new type of layout is C++.** A hex grid or a different collision model is a `Layout` subclass, which is an engine
  extension rather than a mod.

## Scripting

- **Scripts are Lua 5.5** (`decision_rationale.md`, "Lua for scripts and patterns"), embedded as
  `scripting_language_plan.md`, `## Lua 5.5 prototype`, describes.
- **Behavior is script wherever practical:** event hooks on types (interacting, entering, or a turn passing), AI
  writing `Intent`, rules and commands, and generators and placement.
- **Inner loops stay in C++:** movement queries, sight, pathfinding and rendering. Scripts call them.
- **Script state is stored in components.** A script keeps nothing between calls, so saving a zone saves everything.
- **Scripts use the simulation's seeded random source and time,** never the wall clock, so generation and replays
  reproduce.
- **Bindings come from reflection.** A script reads and writes any registered component, including a mod's, through
  flecs' meta cursor, with no binding written per component.
- **Mod scripts are sandboxed,** with no file system or process access beyond what the game exposes.

## JSON

JSON is used only as the file format. `stratum_format::Document` has component objects as parsed JSON, since it is
the file as data structures, and nlohmann does not appear in any `stratum_sim` header. Nothing in the world is JSON.

## Current blockers

- **`stratum_view::PlaceType` is a closed enum** --- none, solid, floor, wall and door. A modded type cannot have a
  place type or glyph of its own; the glyph sets load from a file, but the categories they map from are fixed in C++.
- **The `door` type is registered in C++,** with its `Access`, until types load from data.
- **Registration is compile-time only** (`game_system_principles.md`, Key abstractions).

## Open

The open items are tracked in `work_plan.md`, under Modding.
