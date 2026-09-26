# Modding and Data-Driven Design --- Plan

Status: design goal; nothing built
Date: 2026-09-22, revised 2026-09-24

Every game built on the system is moddable, and custom content --- objects, geometry, visual effects, behavior --- is
added as data and script without recompiling. What a mod may add cannot be predicted, so the system is built from
definitions loaded at runtime, with C++ supplying the mechanisms and the inner loops. Principle 8 of
`game_system_principles.md` states the goal, and `decision_rationale.md` ("Everything a game adds is data or script")
argues it.

A room whose walls are fun-house mirrors is the test case: a mod adds it with a style asset, a mesh, a shader and at
most a script, and nothing in C++ changes.

## What a mod is

A directory the game loads beside its own data, containing any of:

- component schemas;
- types;
- styles, with the meshes, materials and shaders they refer to;
- zone files and generators;
- scripts;
- patches to types and styles loaded before it.

The game's own content is loaded the same way, as the first mod.

## Components

- **A component is typed in the world.** Loading a zone decodes each component object in the file into the component
  registered under its name, through flecs reflection, and keeps no JSON. `load_layout` decodes them from the
  `Document` as it builds the zone (`discrete_layout_rework.md`).
- **Schemas come from C++ or from data.** The game registers its components in C++. A mod declares its components and
  their fields in a schema file, which becomes a flecs runtime component with reflection, stored, decoded, saved and
  reached from script like any other.
- **An unknown name is an error,** except a component declared by a mod that is not loaded, which is skipped with a
  warning.
- **Saving writes components through the same reflection,** so mod data persists with the zone.

## Types

- **A type is a prefab defined in data:** its components, its model, and its child entities. An entity is `IsA` its
  type, so the type's components are its defaults with nothing copied. A door is a `door` type with its door facings as
  children.
- **Types inherit from types** in data, the chain flattened when loaded (`game_system_principles.md`, Decisions).
- **A mod patches a type or a style** it did not define: adding, replacing or removing components and fields.

## Styles and visuals

- **A style is an asset name and a parameter map.** The asset names a material, a shader and meshes; the parameters are
  named numbers, vectors, colors and asset references, passed to the shader by name.
- **A style inherits along the zone's chain** --- entity, room, layout, type --- its parameters merging by name, so a
  zone overrides part of a style by stating only that part.
- **A style may replace a surface's mesh.** A wall, floor or ceiling is drawn from the kit piece its style names rather
  than as a flat face, so a curved mirror is a mesh. Collision is unchanged: a wall is still a bit on its cell.
- **A view resolves a style,** as it resolves glyphs and tiles from its presentation table. The simulation refers only
  to the name.

## Geometry

- **A zone's geometry is its file**, written by a tool or a generator, a mod's included.
- **A generator is a script** or a recipe of registered passes, configured in data (`game_system_principles.md`,
  Phase 5).
- **A surface's form is its style's,** as above.
- **A new kind of layout is C++.** A hex grid or a different collision model is a `Layout` subclass, an engine
  extension rather than a mod.

## Scripting

- **Behavior is script wherever practical:** event hooks on types (interacting, entering, a turn passing), AI writing
  `Intent`, rules and commands, generators and placement.
- **Inner loops stay C++:** movement queries, sight, pathfinding and rendering. Scripts call them.
- **Script state lives in components.** A script keeps nothing between calls, so saving a zone saves everything.
- **Scripts use the simulation's seeded random source and time,** never the wall clock, so generation and replays
  reproduce.
- **Bindings come from reflection.** A script reads and writes any registered component, a mod's included, through
  flecs' meta cursor, with no binding written per component.
- **Mod scripts are sandboxed:** no file system or process access beyond what the game exposes.

## JSON

JSON is the file format and nothing else. `stratum_format::Document` holds component objects as parsed JSON, being the
file as data structures, and nlohmann does not appear in any `stratum_sim` header. Nothing in the world is JSON.

## What blocks it now

- **`stratum_view::PlaceType` is a closed enum** --- none, solid, floor, wall, door. A modded type cannot have a place
  type or glyph of its own; the glyph sets load from a file, but the categories they map from are fixed in C++.
- **The `door` type is registered in C++,** with its `Access`, until types load from data.
- **Registration is compile-time only** (`game_system_principles.md`, Key abstractions).

## Steps

- [ ] **1. Place types from data.** A reduction maps an entity's type to a place type the presentation table names,
      rather than to an enum value, so a modded type has a glyph of its own.
- [x] **2. Components decoded at load.** With `addressing_plan.md` step 5: `load_layout` decodes each component object
      into its registered component, and no entity keeps JSON.
- [ ] **3. Types as prefabs from data**, doors among them, with inheritance and patches.
- [ ] **4. Component schemas from data**, as flecs runtime components.
- [ ] **5. Styles:** the asset, the parameter map, inheritance by name, and a view resolving them.
- [ ] **6. The scripting language chosen and embedded,** with hooks on types and bindings through reflection.
- [ ] **7. Mods:** the directory, load order, patches, and skipping what a missing mod declares.

## Open

Tracked in `work_plan.md`, under Modding.
