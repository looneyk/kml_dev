# Game System --- Core Principles and Plan

Status: Phase 3 complete but for presentation tables; the run up to real time is done and the
layout hierarchy is named
Date: 2026-08-23, revised 2026-09-02

Near-term ordering is in `work_plan.md`, which holds the stages within a phase and the run up to
real time. A stage large enough to need its own detail gets a plan of its own --- `intent_plan.md`
for the input model, `layout_plan.md` for the layout hierarchy, `view_plan.md` for the view axis,
`attachment_plan.md` for what is attached to a wall --- and what comes of any of them goes to
`game_system_reference.md`. `sim_zone_model.md` holds the model beneath the code and
`decision_rationale.md` the argument behind the decisions, so neither is repeated here.

## Synopsis

A general, data-driven game system built as a headless C++ simulation core with pluggable
simulation modules and pluggable views. The first game built on it is a block-movement dungeon
crawler playable in multiple presentations --- ASCII, 2D tile, first-person 3D, orthographic --- over
one shared simulation.

The architecture leaves room for continuous-space genres (FPS, third-person action-RPG) without
implementing them. The front end is a replaceable configuration rather than a foundation --- see
Front end configurations below.

Subsystem names and the project name itself are placeholders throughout this document.

## Core principles

These are the decisions that are expensive to reverse.

1. **The core never links an engine.** It builds, runs, and is tested standalone and headless.
2. **Layout is abstracted from the start.** Discrete cells must never appear in core interfaces.
3. **Scheduling is abstracted from the start.** A `Scheduler` over abstract units; the driver
   decides whether they advance on player command (turn-based) or on wall clock (real-time).
4. **Views own their animation state.** The simulation is discrete and has no frame concept;
   interpolation and easing belong entirely to presentation.
5. **Every view can rebuild from a full snapshot,** not only from incremental events. This does two
   jobs: it lets one view be swapped for another at runtime, and it lets a separate process attach
   to a running simulation at an arbitrary point and be correct immediately.
6. **Generation is deterministic,** with explicit RNG and hierarchical seeding.
7. **Abstract on the second implementation, not the first.** Seams are designed early; interfaces
   are finalized only once two real cases exist to derive them from.

Principle 7 exists specifically to bound principles 2 and 3. Design the interface now, implement the
second case only when it is real.

## Architecture

Three layers with a hard rule at each boundary.

**Simulation core** --- owns the world, entities, scheduler, and rules. Knows nothing about pixels,
cameras, frames, or elapsed wall-clock time. Its outputs are a queryable world state and an event
stream (`entity_moved`, `door_opened`, `attack_resolved`). Events are plain data and never hold
references into simulation internals.

**View layer** --- consumes the event stream and renders. Each view keeps its own animation state,
interpolating toward the discrete simulation state.

**Input and command layer** --- each view supplies its own input mapping, all producing the same
command vocabulary. A top-down view sends `move(west)`; a first-person view sends `turn(left)` or
`step(strafe_left)` relative to facing. Both resolve to the same simulation commands.

```
class View
{
    on_world_reset(world_snapshot)   // full rebuild; enables hot swap
    on_event(event)                  // incremental update
    on_frame(delta)                  // animation only; never mutates simulation state
};
```

## Front end configurations

A front end hosts the view layer within one process. Views that share a host can be swapped for one
another at runtime; a view in another process is launched instead of swapped to. Several processes
over one simulation are expected and supported --- what is rejected is compositing two hosts into a
single window.

**A --- Godot front end.** All views are Godot scenes. Godot supplies renderer, UI toolkit, audio,
input, text and font rendering, animation, particles, asset importers, platform packaging, and
`SubViewport` compositing for the Dungeon Master layout. `quetzal::vulkan` and `scene_graph` are not
used by this project and remain in service to toot1, gallery, and cubes.

**B --- quetzal-native front end.** Views build on `scene_graph` over `vulkan::IRenderer`, hosted by
SDL3. No Godot. Requires owning UI, audio, text rendering, and packaging, whether hand-written or
vendored (RmlUi, miniaudio, and similar).

**Configuration A is the plan.** B is a documented future option, and building it would be the
strongest available proof of the view abstraction --- a second front end on entirely different
infrastructure, running the same simulation *in place of* A rather than alongside it.

Compositing A and B into one window --- Godot for 2D and UI, quetzal for the 3D viewport --- is
rejected. Two windowing stacks in a single window means two event loops and two presentation paths,
with no clean answer for input focus, z-order, or which one owns the swap chain.

This argues against mixing hosts in one process, and against nothing else. It is not an argument
against B, whose costs are its own and are listed above; nor against the engine-free ASCII
application below, which is a separate process and composites with nothing.

## The ASCII view is permanent

The ASCII view is not scaffolding to be discarded once Godot arrives. It exists in two hosts sharing
one engine-neutral renderer, which turns a world snapshot and a presentation table into a character
grid; each host does nothing but display that grid. The shared renderer is extracted when the second
host arrives, not written up front --- principle 7 applies to it as to everything else.

**Engine-free terminal application.** No GPU, no window, no display, and therefore the only view
that can be regression tested: drive a fixed command sequence, render to text, diff against
checked-in reference output in the same style as the mesh_operations reference files. The test suite
renders through it, which is what keeps it alive --- a fallback nobody runs stops compiling. It also
keeps "the core builds standalone" enforced by the build rather than by discipline, which otherwise
expires the moment Godot becomes the only consumer. And it keeps two genuinely different views
permanently in hand, so every later change to the `View` interface has to satisfy one that has no
camera, no pixels, and no frame loop. Because a terminal redraws from full state, it exercises the
snapshot rebuild path continuously rather than only on a swap.

**ASCII as a Godot scene.** A monospace glyph grid, swappable at runtime with the other Godot views:
a wizard view, a debug overlay, an accessibility mode. Arrives with Phase 3 or later, once there is
a presentation table for it to read.

The ongoing cost is that every new event type needs an ASCII rendering or it silently shows
nothing. The test suite is what catches that.

## Module layout

```
library/quetzal/
    stratum_sim/    entities, components, zones, layout, scheduling, commands, events
    content/        JSON loading, prototypes, registry, validation, hot reload
    dungen/         existing; the block format generator
    catagen/        existing; the wall format generator, sibling to dungen
    procgen/        what the two generators come to have in common, plus vaults and seeding
    stratum_view/   Console character buffer, the ASCII renderer, frame differencing, and the
                    cell reduction both views consume; the view interface and presentation
                    tables wait on a third consumer (engine-neutral)
    godot/          GDExtension adapter; the only place godot-cpp appears
    scene_graph/    existing; used only by configuration B -- see follow-on work
    vulkan/         existing; unchanged
    brep/           existing; later, the back end for 3D level geometry
library3/
    json/           nlohmann/json -- already vendored
    godot-cpp/      the GDExtension bindings, governed by KML_DEV_GODOT
    flecs/          the ECS; see sim_zone_model.md
application/
    stratum_terminal/ the engine-free host: input mapping, turn loop, terminal output
    stratum_godot/    the Godot front end: registration, node classes, and its project
assets/
    level/          shared data, owned by no host and reached through a configured root
```

## Key abstractions

**Zone and Layout.** A `Zone` is an independently loadable area --- a dungeon, a town, a moor. Its
`Layout` is how that area is arranged and the query interface over it: where a mover ends up, what
it faces, what is adjacent, what is visible. `DiscreteLayout` is the implementation and
`ContinuousLayout` is declared and not written. Occupancy is not on either: it composes with
topology on `World`, which is the only thing holding both. Both, and how the discrete and
continuous cases share them, are worked out in `sim_zone_model.md`.

**Scheduler.** Ordering over abstract units with a pluggable driver. Turn-based and real-time are
drivers, not separate schedulers. The core does not know what a unit is worth in seconds.

**Entity and component.** Entities are ids; behavior comes from composed components. There is no
class hierarchy of entity types --- `goblin_archer` is a data key, never a C++ type. flecs supplies
the storage, chosen for its relationships rather than its storage --- see `sim_zone_model.md`.

**Registry.** One mechanism maps a name to a factory. Plugin modules, component types, generation
passes, and view types all register through it. Compile-time registration only, to start.

**Presentation table.** Each view owns its own mapping from simulation asset id to its own
representation --- glyph and color, tile index, or mesh and material. The simulation refers only to
ids such as `wall_stone_rough`.

## Decisions and rationale

**Simulation core in C++, outside the engine --- for reuse, not speed.** A turn-based grid simulation
would perform fine in GDScript. The justification is that the core outlives the engine choice and
travels to any future front end, including a quetzal-native one.

**Godot is chosen, and the choice is deliberately deferred and reversible.** With simulation, data,
serialization, and entity modelling removed from it, what Godot still supplies is the whole
presentation half --- and that is the half it is genuinely good at. The cost is that GDExtension sits
exactly where most of the work happens: every core change crosses that boundary, with a rebuild and
an occasional editor restart. Since phases 0 through 2 need no engine at all, the commitment is not
made until Phase 3, by which point the view interface is understood. A wrong answer there costs one
view implementation, not the project.

**scene_graph is not the universal renderer abstraction.** It is a 3D spatial hierarchy with
inherited transforms, materials, and lights --- exactly what a first-person or orthographic view
wants, and pure overhead for an ASCII view. It is one view implementation's internal structure, not
a layer every view passes through. The renderer-agnostic boundary sits above it, at the
simulation/view line.

Under configuration A it is not used by this project at all. Making it more renderer-independent
remains worth doing on its own merits, for the existing applications and as the prerequisite for
configuration B; it is listed under follow-on work.

`vulkan::IRenderer` stays in `quetzal::vulkan`. It is a good Vulkan abstraction and was never going
to be a good ASCII one.

**Nothing from Godot's data or scripting layer travels.** `Resource`, `ResourceSaver`, and GDScript
are welded to Godot's `Object`, `ClassDB`, `Variant`, and virtual filesystem. The core therefore
owns its own serialization format and its own scripting choice. The corollary is that Godot's editor
can still serve as an authoring tool via an export step to the neutral format.

**JSON for content.** Prototypes are deeply nested trees, and JSON Schema gives validation and
editor support with no custom work. Tools write this format as well as read it --- editor export,
generated content, saves --- so it has to survive a machine round trip, and comments are recovered by
parsing with `ignore_comments`. It matches the field: Cataclysm DDA and Minecraft data packs are
JSON.

**Per-edge walls, in partnered pairs.** Decided, ahead of Phase 1. Solid cells cannot express a wall
or a door between two open cells without spending a whole cell on it, so per-edge subsumes them and
not the reverse. What lies on an edge is two walls, one per side, which are each other's partners. Everything is
therefore per side --- decoration, the door, and passability itself --- so one-way doors and secret
doors need no special case, and no orientation convention is required to say which side data belongs
to. An opening is the absence of a wall rather than a flag on one, which is what non-rectangular
rooms need, having no predefined wall slots to flag. A `nullid` partner means solid rock: nothing
behind it, and not minable. The structures in `library/quetzal/dungeon` are the starting point.

**Data-level inheritance only.** A prototype references a parent, and the chain is resolved by merge at
load time and flattened; the parent relationship does not exist at runtime. Multiple parents and mixins are
reasonable here because merging property bags is well defined. Merge semantics must be specified
explicitly --- whether a child component replaces or merges field-by-field, how lists combine, and
whether flattening order is deterministic with multiple parents.

**Data describes what exists; behavior lives in code or script.** The escape hatch to a real
scripting language is chosen before it is needed, not after conditionals have accumulated in JSON.

**Compile-time plugin registration first.** Dynamic C++ plugin ABIs are fragile across compilers and
build settings; GDExtension's own compatibility trouble is this problem in a well-resourced project.
Static libraries with registration functions give the full plugin architecture with none of the ABI
cost, and the extension point for runtime loading is already in place if it is ever needed.

## Phased plan

Each phase ends with something demonstrable and testable. Nothing speculative is built ahead of a
phase that needs it.

**Phase 0 --- Foundation and de-risking spike**

New library directories and CMake, and the simulation skeleton: entity ids on
`quetzal::id_type`/`nullid`, a command queue, an event queue, and a headless test harness with
per-case logging. Component storage was hand-rolled here and removed when flecs was adopted ahead of
Phase 1, once level content showed relationships to be central; the queues stayed, flecs' observers
being immediate-mode rather than something a view drains per step.

Separately, a throwaway **GDExtension spike**: build godot-cpp, register one C++ class, call it from
GDScript, confirm the rebuild loop. This is the riskiest integration in the plan and its friction
should be known before it is load-bearing. The spike is then discarded; Godot does not enter the
build until Phase 3.

*Proves:* the core runs and is testable with no engine present.

**Phase 1 --- The two core interfaces**

`Layout` and a grid implementation (cells, neighbors, per-edge walls), `Scheduler` and a turn-based
energy implementation, and movement, turning and interaction commands. Occupancy sits on `World`
rather than on `Layout`, a `Layout` being geometry and knowing nothing of entities. Levels are read
from dungen's block format; the thin-wall format waited until something emitted one, which `catagen`
now does, and both load into the one structure.

*Proves:* a walkable dungeon exists, verified entirely in headless tests. No rendering.

**Phase 2 --- First view: ASCII, engine-free**

The cheapest possible renderer, hosted by a terminal application rather than an engine, written
directly against the simulation, with input mapping producing simulation commands. `view::render`
turns a world and a zone into a `Console` character buffer. There is no `View` interface yet, one
implementation being too few to earn its shape. `stratum_view::difference` reports the runs in which one
frame differs from the last, and sits in `view/` rather than in the host because every view wants to
know what changed --- only the emission is device-specific.

`application/stratum_terminal` is the host: key mapping, the turn loop, a status line, and a `Terminal`
that turns those runs into cursor moves, so a turn writes the cells that moved rather than
repainting. Its `--script` option plays a key sequence in place of the keyboard, so the loop can be
driven from the command line. Fixed command sequences are rendered to text and gated byte for byte
by `FileValidator` against `test/stratum_view_test/reference/`, played in process against the two libraries
rather than through the host, whose key mapping is not what they check.

Keeping this phase engine-free turns "the core builds standalone" from a principle into something a
real application proves, and yields three consecutive phases of zero-friction core iteration.

This application is permanent rather than scaffolding --- see The ASCII view is permanent above.

*Proves:* the simulation/presentation boundary holds under a real consumer, and the game is playable.

**Phase 3 --- Godot, and the view interface earns its shape**

Godot enters the build here, and this is the point at which configuration A is actually committed
to. GDExtension adapter and a 2D tile view. With two genuinely different views in hand, extract and
finalize the `View` interface from both. Establish per-view presentation tables.

*Proves:* renderer-agnosticism against two real backends, and the Godot path end to end.

**Phase 3b --- A minimal first-person 3D view**

Pulled forward from Phase 6, ahead of content and procgen. It is the second genuinely different
view, and it validates the wall model before procgen starts generating against it --- a wall between
two open cells is the thing a cell-per-glyph view cannot show and a first-person one cannot avoid.
Stage 5 of `work_plan.md` is what makes it cheap: the wall-aware ASCII view and a `View` interface
derived from three consumers rather than two.

Minimal is the point. What stays in Phase 6 is the rest of the Godot 3D view --- the `SubViewport`
Dungeon Master layout, camera interpolation, orthographic.

*Proves:* the per-edge wall model against a view that has to draw it.

**Phase 4 --- Data-driven content**

JSON loading with `ignore_comments`. Prototype inheritance with explicit merge semantics. Component
factory registry. Schema validation reporting every error with file and line, not the first failure.
Hot reload, which requires entities to hold prototype ids rather than pointers into loaded data.
Migrate hardcoded entity types into data.

*Proves:* a new monster ships without a recompile.

**Phase 5 --- Procedural generation**

Generation as an ordered pipeline of registered passes, configured by level recipes in data.
Hierarchical seeding (world seed derives level seed derives room seed). A layout pass, connectivity
validation with reject-and-retry on a fresh sub-seed, population. Then vaults with anchor points,
placement constraints, and palette substitution so authored content adopts the surrounding biome.

*Proves:* reproducible, validated dungeons, with checked-in reference outputs in the same style as
the mesh_operations reference files.

**Phase 6 --- First-person 3D, in full**

The rest of the Godot 3D view, the minimal one having arrived as Phase 3b: `SubViewport` for the
Dungeon Master layout, smooth camera interpolation over discrete moves. Orthographic follows as a
camera projection flag.

*Proves:* four presentations over one simulation.

**Phase 7 --- Generalization checkpoint**

Write `ContinuousLayout`, the second implementation of `Layout`. It is declared already, every
member pure and nothing behind it, so the shape of what is missing is stated rather than guessed at.
This is where the FPS and third-person door actually opens, and it opens honestly because the
abstraction is corrected against a second real case rather than against imagination.

The real-time driver is no longer part of this phase, and is done: real time is the direction the
application is moving toward rather than an option held open, so it came forward to the run up to
this phase, ordered in `work_plan.md`. The two axes separate cleanly, and the first has arrived ---
continuous *time* is done and continuous *space* stays here.

**Deferred, explicitly**

Dynamic plugin loading. Embedded scripting. Netcode. Third-person view. A Godot editor plugin
exporting authored levels to the neutral format. Brep-driven 3D level geometry.

## Follow-on work

Not part of the phased plan, and not blocked by it.

**scene_graph renderer independence.** Worth doing on its own merits for the existing applications,
and the prerequisite for configuration B:

- `GeometryNode` holds an asset id rather than a `vulkan::GeometryHandle`, so a graph can be built
  without a live renderer having already uploaded resources.
- `Scene` stops owning `vulkan::IRenderer&` and `draw()` becomes external, so one scene can be drawn
  by more than one renderer --- needed for a 3D viewport with a 2D automap alongside it.

**Configuration B front end.** SDL3 host, `scene_graph` over `IRenderer`, with UI, audio, and text
rendering either hand-written or vendored. A second front end on entirely different infrastructure
would be the strongest available proof that the view abstraction is real.

## Open decisions

- **Project name.** `stratum_pattern` is the game's working title, `pattern` a placeholder for
  something more colourful. It designates no target; the hosts are named separately.
- **Godot binding location.** `library/quetzal/godot` as proposed, or under `application/`.
- **Front end configuration.** A is the plan; the Phase 3 boundary is the last cheap moment to
  choose B instead.
- **Save format.** JSON initially for debuggability; binary later if load time demands it.
- **Scripting language,** if and when behavior outgrows data.

## Risks

**Scope versus a finished game.** The generality ambition and a playable crawler pull in different
directions. The phasing mitigates this by making every phase produce something playable; it remains
the risk most likely to bite.

**Premature abstraction.** Principle 7 counters the instinct to build the general case up front.
Phase 7 is deliberately last for this reason.

**GDExtension friction.** ABI churn across Godot versions and awkward hot reload of extension
libraries. The Phase 0 spike found the loop cheaper than feared: 3.1 seconds from a one-file change
to a relinked extension, and a running editor did not block the relink --- though that was measured
against the headless editor, and the GUI editor is untested. The real friction was the MSVC runtime.
godot-cpp defaults to the static *release* CRT even in a debug build, and its only alternative is
`/MDd`, so neither of its options matches this solution's `/MTd`. `CMAKE_MSVC_RUNTIME_LIBRARY` has
to be seeded in the cache before godot-cpp is added, or no quetzal static library will link into an
extension.

**Authoring gap.** Until the editor-export plugin exists, level authoring is hand-written files.
Acceptable while procgen supplies most content, but it is real unbuilt work.

**Data-driven drift.** Content formats grow conditionals and become bad programming languages.
Mitigated by choosing the escape hatch to real scripting before it is needed.
