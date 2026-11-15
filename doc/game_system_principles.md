# Game System --- Core Principles and Plan

Status: Phase 3 complete but for presentation tables; real time is done and the layout hierarchy is named
Date: 2026-08-23, revised 2026-09-22

The work queue is `work_plan.md`, and what is built is described in `game_system_reference.md`. `modding_plan.md` is the
design goal of principle 8. `sim_zone_model.md` has the model beneath the code and
`decision_rationale.md` the argument behind the decisions, so neither is repeated here.

## Synopsis

A general, data-driven game system built as a headless C++ simulation core with pluggable simulation modules and
pluggable views. The first game built on it is a block-movement dungeon crawler playable in multiple presentations ---
ASCII, 2D tile, first-person 3D, orthographic --- over one shared simulation.

The architecture leaves room for continuous-space genres (FPS, third-person action-RPG) without implementing them. The
front end is a replaceable configuration rather than a foundation (Front end configurations, below).

Subsystem names and the project name itself are placeholders throughout this document.

## Core principles

These are the decisions that are expensive to reverse.

1. **The core never links an engine.** It builds, runs, and is tested standalone and headless.
2. **Layout is abstracted from the start.** Discrete cells never appear in core interfaces.
3. **Scheduling is abstracted from the start.** A `Scheduler` over abstract units; the driver decides whether they
   advance on player command (turn-based) or on real time.
4. **Views keep their own animation state.** The simulation is discrete and has no frame concept; interpolation and easing
   belong to presentation.
5. **Every view can rebuild from a full snapshot,** not only from incremental events, so one view can be swapped for
   another at runtime, and a separate process can attach to a running simulation at an arbitrary point and be correct
   immediately.
6. **Generation is deterministic,** with explicit RNG and hierarchical seeding.
7. **Abstract on the second implementation, not the first.** Interface boundaries are designed early; interfaces are finalized only
   once two real cases exist to derive them from. This bounds principles 2 and 3.
8. **Everything a game adds is data or script.** Every game is moddable. Types, components, styles, geometry and
   behavior are definitions loaded at runtime, from the game and from mods alike, and C++ supplies the mechanisms and
   the inner loops. Nothing a mod might add is assumed to be known in advance (`modding_plan.md`).

## Architecture

The system has three layers, with a hard rule at each boundary.

**Simulation core** --- the world, entities, scheduler, and rules. It has no notion of pixels, cameras, frames, or
real time. Its outputs are a queryable world state and an event stream (`entity_moved`, `door_opened`,
`attack_resolved`). Events are plain data and never refer into simulation internals.

**View layer** --- consumes the event stream and renders. Each view keeps its own animation state, interpolating toward
the discrete simulation state.

**Input and command layer** --- each view supplies its own input mapping, all producing the same command vocabulary. A
top-down view sends `move(west)`; a first-person view sends `turn(left)` or `step(strafe_left)` relative to the facing
direction. Both resolve to the same simulation commands.

A view responds to three occasions:

```
on_world_reset(world_snapshot)   // full rebuild; enables hot swap
on_event(event)                  // incremental update
on_frame(delta)                  // animation only; never mutates simulation state
```

## Front end configurations

A front end hosts the view layer within one process. Views that share a host can be swapped for one another at runtime;
a view in another process is launched instead. Several processes over one simulation are supported; compositing two
hosts into a single window is not.

**A --- Godot front end.** All views are Godot scenes. Godot supplies the renderer, UI toolkit, audio, input, text and
font rendering, animation, particles, asset importers, platform packaging, and `SubViewport` compositing for the Dungeon
Master layout. `quetzal::vulkan` and `scene_graph` are not used by this project and remain in service to toot1, gallery
and cubes.

**B --- quetzal-native front end.** Views build on `scene_graph` over `vulkan::IRenderer`, hosted by SDL3, without
Godot, and with UI, audio, text rendering and packaging written or vendored (RmlUi, miniaudio, and similar).

**Configuration A is the plan.** B is a documented future option: a second front end on different infrastructure,
running the same simulation in place of A.

## The ASCII view is permanent

The ASCII view is not scaffolding. It exists in two hosts sharing one engine-neutral renderer, which turns a world
snapshot and a presentation table into a character grid; each host only displays that grid.

**Engine-free terminal application.** No GPU, no window, no display, and so the view that can be regression tested:
drive a fixed command sequence, render to text, and compare against checked-in reference output, in the style of the
mesh_operations reference files.

**ASCII as a Godot scene.** A monospace glyph grid, swappable at runtime with the other Godot views: a wizard view, a
debug overlay, an accessibility mode. It arrives with Phase 3 or later, once there is a presentation table for it to
read.

Every new event type needs an ASCII rendering, which the tests check.

## Module layout

```
library/quetzal/
    stratum_sim/    entities, components, zones, layout, scheduling, commands, events
    content/        JSON loading, prototypes, component schemas, mods, registry, validation, hot reload
    dungen/         the zone generator for the block notation
    catagen/        the zone generator for the wall notation
    procgen/        what the generators have in common, plus vaults and seeding
    stratum_view/   the Console character buffer, the reductions, the ASCII mapping and frame
                    differencing (engine-neutral); presentation tables to come
    godot/          the GDExtension support; the only place godot-cpp appears
    scene_graph/    used only by configuration B; see Follow-on work
    vulkan/         unchanged
    brep/           later, the back end for 3D zone geometry
library3/
    json/           nlohmann/json
    godot-cpp/      the GDExtension bindings, governed by KML_DEV_GODOT
    flecs/          the ECS; see sim_zone_model.md
application/
    stratum_terminal/ the engine-free host: input mapping, turn loop, terminal output
    stratum_godot/    the Godot front end: registration, node classes, and its project
assets/
    level/          shared data, used by every host and located through a configured root
```

## Key abstractions

**Zone and Layout.** A `Zone` is an independently loadable area --- a dungeon, a town, a moor. Its `LayoutBase` is how that
area is arranged and the interface for querying it: where an actor ends up, which wall is ahead, which placements are
adjacent, and which are visible. `DiscreteLayout` is the implementation and `ContinuousLayout` is declared and not written. Occupancy is on
neither: it composes with topology on `World`, which contains both. `sim_zone_model.md` works out both, and how the
discrete and continuous cases share them.

**Scheduler.** The scheduler orders actors' turns over abstract units, with a pluggable driver. Turn-based and real time are drivers, not separate
schedulers. The core has no conversion from units to seconds.

**Entity and component.** Entities are ids; behavior comes from composed components. There is no class hierarchy of
entity types --- `goblin_archer` is a data key, never a C++ type. flecs supplies the storage (`sim_zone_model.md`).

**Registry.** One mechanism maps a name to a factory. Plugin modules, component types, generation passes, and view
types all register through it, from C++ at compile time and from data at runtime: a mod's component schemas, types,
styles and scripts register the same way the game's do.

**Presentation table.** Each view has its own mapping from simulation asset id to its own representation --- glyph and
color, tile index, or mesh and material. The simulation refers only to ids such as `wall_stone_rough`. A style is such
an id with a parameter map (`modding_plan.md`).

## Decisions

The reasoning for each is in `decision_rationale.md`.

- **The simulation core is C++, outside the engine**, for reuse rather than speed.
- **Godot is the engine**, committed to at Phase 3 rather than before.
- **scene_graph is one view implementation's internal structure**, not a layer every view passes through. The
  renderer-agnostic boundary is at the simulation/view line. `vulkan::IRenderer` stays in `quetzal::vulkan`.
- **Nothing from Godot's data or scripting layer is used by the core.** The core has its own serialization format and
  its own scripting choice; Godot's editor can author content through an export step to the neutral format.
- **JSON for content**, parsed with `ignore_comments`, and written by tools as well as read. JSON is the file format
  only: loading decodes it into typed components, and nothing in the world is JSON.
- **Per-edge walls, one per side.** What lies on an edge is up to two walls, each separate from the other: an actor
  interacts with the wall on its own side, and the other side is out of reach until it is there. Decoration, a door
  facing and passability are all per side. An opening is the absence of a wall.
- **Data-level inheritance only.** A prototype refers to a parent, and the chain is resolved by merge at load time and
  flattened; the parent relationship does not exist at runtime. Multiple parents and mixins are allowed. A child's
  component merges over its parent's property by property: what the child states replaces the parent's value, and what
  it does not state keeps it. How lists combine, and a deterministic flattening order with multiple parents, are to be
  specified.

  An object in a zone refers to its type rather than copying it, and a property is resolved when queried, field by
  field: the object's own value, then its type's, then that of the room that contains it, then the layout's. A field the type sets
  is the object's, so only what neither states comes from the room or the layout.
- **Data describes what exists; behavior is script wherever practical,** C++ keeping the inner loops that scripts call.
  Scripts are Lua.
- **Compiled extensions register at compile time**, through static libraries with registration functions; dynamic C++
  plugins are not supported. **Everything a mod adds registers at runtime from data**: component schemas as flecs
  runtime components, types as prefabs, styles and scripts.

## Phased plan

Each phase ends with something demonstrable and testable. Nothing speculative is built ahead of a phase that needs it.

**Phase 0 --- Foundation and de-risking spike**

New library directories and CMake, and the simulation skeleton: entity ids on `quetzal::id_type`/`nullid`, a command
queue, an event queue, and a headless test harness with per-case logging. flecs supplies component storage.

Separately, a throwaway **GDExtension spike**: build godot-cpp, register one C++ class, call it from GDScript, confirm
the rebuild loop. Godot does not enter the build until Phase 3.

*Proves:* the core runs and is testable with no engine present.

**Phase 1 --- The two core interfaces**

`LayoutBase` and a grid implementation (cells, neighbors, per-edge walls), `Scheduler` and a turn-based energy
implementation, and movement, turning and interaction commands. Occupancy is on `World` rather than on `LayoutBase`. Zones
are read from both generators' output into the one structure.

*Proves:* a walkable dungeon exists, verified entirely in headless tests. No rendering.

**Phase 2 --- First view: ASCII, engine-free**

A terminal application hosting the renderer, written directly against the simulation, with input mapping producing
simulation commands. `stratum_view::render` turns a world and a zone into a `Console` character buffer, and
`stratum_view::difference` reports the runs in which one frame differs from the last, so the host writes only the cells
that changed. Fixed command sequences are rendered to text and compared byte for byte by `FileValidator` against
`test/stratum_view_test/reference/`.

*Proves:* the simulation/presentation boundary holds under a real consumer, and the game is playable.

**Phase 3 --- Godot**

Godot enters the build, committing to configuration A. This phase adds the GDExtension adapter, a 2D tile view, and
per-view presentation tables.

*Proves:* renderer-agnosticism against two real backends, and the Godot path end to end.

**Phase 3b --- A minimal first-person 3D view**

This view comes ahead of content and procgen. It is the second genuinely different view, and it validates the wall
model before procgen generates against it. The wall-aware ASCII view prepares it. The rest of the Godot 3D view --- the
`SubViewport` Dungeon Master layout, camera interpolation, orthographic --- stays in Phase 6.

*Proves:* the per-edge wall model against a view that has to draw it.

**Phase 4 --- Data-driven content**

JSON loading with `ignore_comments`. Prototype inheritance with explicit merge semantics. Component factory registry,
with component schemas from data as well as C++. Schema validation reporting every error with file and line, not only
the first. Hot reload, which requires entities to refer to prototype ids rather than to loaded data. Hardcoded entity
types moved into data. The scripting language embedded, with hooks on types. Mods loaded, in order, with patches.

*Proves:* a new monster ships without a recompile, and a mod adds one the game has never seen.

**Phase 5 --- Procedural generation**

Generation as an ordered pipeline of registered passes, configured by zone recipes in data. Hierarchical seeding (the
world seed derives a zone seed, which derives a room seed). A layout pass, connectivity validation with reject-and-retry
on a fresh sub-seed, population. Then vaults with anchor points, placement constraints, and palette substitution so
authored content adopts the surrounding biome.

*Proves:* reproducible, validated dungeons, with checked-in reference outputs.

**Phase 6 --- First-person 3D, in full**

The rest of the Godot 3D view: `SubViewport` for the Dungeon Master layout, smooth camera interpolation over discrete
moves. Orthographic follows as a camera projection flag.

*Proves:* four presentations over one simulation.

**Phase 7 --- Generalization checkpoint**

Write `ContinuousLayout`, the second implementation of `LayoutBase`, already declared with every member pure. This is
where FPS and third-person become possible, the abstraction corrected against a second real case. Continuous time is
done; continuous space is this phase.

**Deferred, explicitly**

Dynamic plugin loading. Netcode. Third-person view. A Godot editor plugin exporting authored zones to
the neutral format. Brep-driven 3D zone geometry.

## Follow-on work

This work is not part of the phased plan, and is not blocked by it.

**scene_graph renderer independence**, for the existing applications and as the prerequisite for configuration B:

- `GeometryNode` refers to an asset id rather than a `vulkan::GeometryHandle`, so a graph can be built without a live
  renderer having uploaded resources.
- `Scene` stops owning `vulkan::IRenderer&` and `draw()` becomes external, so one scene can be drawn by more than one
  renderer, as a 3D viewport with a 2D automap beside it requires.

**Configuration B front end.** An SDL3 host, `scene_graph` over `IRenderer`, with UI, audio, and text rendering written
or vendored.

## Open decisions

- **Project name.** `stratum_pattern` is the game's working title, `pattern` a placeholder. It designates no target;
  the hosts are named separately.
- **Godot binding location.** `library/quetzal/godot`, or under `application/`.
- **Front end configuration.** A is the plan; the Phase 3 boundary is the last cheap point to choose B instead.
- **Save format.** JSON initially for debuggability; binary later if load time requires it.

## Risks

**Scope versus a finished game.** The generality ambition and a playable crawler pull in different directions. Every
phase producing something playable mitigates it; it remains the risk most likely to materialize.

**Premature abstraction.** Principle 7 counters building the general case up front, and Phase 7 is last for this reason.

**GDExtension friction.** ABI churn across Godot versions and awkward hot reload of extension libraries. godot-cpp
defaults to the static release CRT even in a debug build, and its only alternative is `/MDd`, neither matching this
solution's `/MTd`, so `CMAKE_MSVC_RUNTIME_LIBRARY` is set in the cache before godot-cpp is added.

**Authoring gap.** Until the editor-export plugin exists, zone authoring is hand-written files. Acceptable while procgen
supplies most content, but real unbuilt work.

**Data-driven drift.** Content formats grow conditionals and become bad programming languages. Mitigated by Lua scripts for
what data cannot express.

**Mod compatibility.** Once mods exist, component names and fields, type names and the script API are a public
interface, and renaming one breaks every mod that uses it.
