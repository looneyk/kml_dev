# Decision Rationale

Date: 2026-08-29, revised 2026-09-26

This document explains why the significant decisions went the way they did and what was rejected, with the background
that a pending change depends on. The working documents state the model; this one gives the argument behind it, and only where the
decision or design element is big enough to be worth revisiting. `sim_zone_model.md` and `game_system_reference.md`
state what is true without repeating any of this.

## Layouts split on discrete versus continuous, and on nothing else

`LayoutBase` is the interface, and `DiscreteLayout` and `ContinuousLayout` are the two classes beneath it with nothing in
between. Geometry is not a second axis of classes.

**Flat is the default, not a restriction.** A discrete layout with no height data has a floor at a constant height,
and a constant height is a `Flat` using fewer of its features. A zone with heights and entities on flat cells and one
with neither load the same class from different data, and game styles differ in which data they load. So neither class
name has `Flat` in it: it is implied for every discrete layout, and a continuous layout's geometry is ordinary 3D
models, in which being level is no more a category than any other form.

**Two classes differing in no behavior are one class.** An intermediate class splitting the grid off from the
geometry was written and taken back out. The only candidate difference was the flats, and movement, walls, blocking
and the grid were the same on both sides, so the pair would have been one body of code twice over, with the flats
loaded or not. Nothing outside the layout reads a flat.

**Continuous is different in kind.** Movement that integrates cannot substitute for movement that
quantizes: a swept path collides partway rather than cell by cell, partners cannot be found by
exact edge match, and topology and occupancy have to be solved together rather than in sequence.
That is behaviour rather than data, so `ContinuousLayout` is a class of its own.

## A wall is a record, not an entity

A wall is a bit on the cell it bounds, addressed by an `id_type` that encodes the cell and the side. Walls are not
flecs entities.

The argument for making them entities was that sited entities, doors and mining all need to target a wall, and a flecs
relationship can only target an entity. That need is real, and addressing a wall by id meets it --- see below.

The count decides it. A wall per cell edge puts thousands in a zone, and unloading a zone serializes its subtree, so
walls as entities would put all of them into every save. A bare wall needs no identity in the ECS sense: it has no
behavior, no lifetime of its own, and no relation an id cannot express.

A wall with components or with something sited on it is the exception the count allows: it has an entity, which its
zone finds by wall id. Those number what is written on walls, hundreds rather than thousands.

## Sites and places link both ways

An entity sited on a wall links to the wall's entity, and the wall's entity links back to it. A door is on two walls
through its children, a door facing sited on each, and the door links to its door facings. An entity sited on a flat
cell and that cell's entity link the same way. Nothing is found by searching.

The alternative was a query over the zone's sites filtered by wall id, which `crossable` would run per move per
actor per turn and per cell along every sight line. With the wall's entity found by id, the question is a lookup and a
link.

The link is a component with the target as data, `Site` on the sited entity and `AttachedEntities` on the wall's or
flat cell's entity, rather than a Flecs pair such as `(LyingOn, cell)`. Flecs stores each distinct pair in an archetype table of
its own, so objects on thousands of different cells split storage into thousands of small tables, and a world several
orders of magnitude past the test zones multiplies that. A pair marked `DontFragment` avoids the split at a cost to
iteration, and a component with the target and no list on the target leaves finding what is on a cell a scan of every
object. Reverse links make both directions a lookup, the zone's location map reaching a wall's or a cell's entity from
its id. The cost is keeping both ends in sync when an entity moves or is destroyed (`archived/addressing_plan.md`).

It depends on wall ids being stable. An id encodes the cell and the side, so it is stable with no store behind it;
what must not change is the grid it encodes against, or every link by wall id silently retargets. That constraint
shaped the mining decision below.

## An entity's site and parent are file components

A zone file lists its entities once, and what an entity is in or on and its parent are its `site` and `parent` file
components (`archived/addressing.md`, `### Entities`, in parts 1 and 2).

**A site is not an entity type.** There are places, which are geometry, and entities, which have identity; a site is
the link from one to the other, and in the world it is a component on the entity. A list of attachments beside the
entities made the link a record of its own, and a door, being on two walls, needed two of them --- which then needed
file components of their own for each side, ids so that a keyhole could be attached to one, and an entity on one place
treated differently from one on several.

**A door's sides are entities.** In the world a door facing is a child of its door with components of its own, so the
file writes it as one: a child, with a `parent` and its own `site`. Every entity then has at most one site, and a site
has no second case.

**Children are derived rather than listed.** A parent listing its children would state each link twice, once on each
end, and the two could disagree. The child's `parent` states it once, and Flecs builds each parent's children from
those when the zone loads, as a wall's or flat cell's list of what is sited on it is built from `site`.

**The relation is `In` or `On`.** An open vocabulary of relationships --- `contained_in`, `part_of`, `carried_by` ---
mostly restated the parent link, and pushed toward names such as `fills`, which is wrong for a door that rarely fills
its opening. Once the parent is a link of its own, what remains is whether an entity is inside what it is sited on or on
its surface.

**The file's arrangement costs nothing at run time.** A file is read once when its zone loads, and the same links
and location maps are built however it is arranged. What is on a wall is a lookup because an entity is sited by
address, which is the key to the zone's map of places, not because the file is grouped by place. Zone files are
written by generators and tools, so grouping them by place for a reader would gain nothing either.

## An entity has three ids

An entity has a local id in a template, a persistent id in a world, and a runtime entity while loaded (`archived/addressing.md`,
`### Ids`).

**The persistent id cannot include the zone.** A key taken from one zone to another, or an item dropped in a zone that
persists, would change id on the way or keep referring to the zone it left.

**It is not the Flecs id.** Flecs recycles ids, so the same entity is a different runtime entity once its zone has been
unloaded and loaded again, and nothing written can depend on one. Flecs can create an entity with a chosen id, but it
divides the 64 bits between an index and a generation count, and its entity index is paged over the index range, so
ids spread across a whole game's history would allocate pages for ids that no longer exist.

**A template has local ids rather than persistent ones.** A template is made into one zone or several: the same file
made twice gives two sets of entities, which need different ids, so making a zone allocates them either way. Local ids
keep a generated file the same for the same parameters, which is what the generator tests compare.

## Persistent ids are a 64-bit count, not UUIDs

A UUID's advantage is that it can be created with no coordination. Templates with local ids remove the one place that
needed it, a generator run as a tool with no world to query, and once a world exists it is the one place ids are made,
so a count saved with the world suffices.

A UUID is 16 bytes where a count is 8, in the component on every persistent entity, in every reference that may reach
another zone, and in the world index, which has an entry for every entity something refers to, loaded or not, and
whose keys hash more cheaply at 8. None of it is on a path a loaded zone runs every turn, since that uses runtime
entities, but the index grows with the world.

Multiplayer does not reverse it. One server allocating is the same count, and a client that predicts a spawn uses a
temporary id, which the server replaces. Several servers divide the bits between which one allocated and its own
count, as Twitter's Snowflake ids and Bethesda's form ids do. Only creation with no coordination at all --- peer to
peer, merging worlds saved separately, moving an entity from one save to another --- needs UUIDs, and then as the alias
throughout or at the boundary between worlds alone, which the rules in `archived/addressing.md` keep a change to the allocator
and the alias.

## The world index has only the entities something refers to

The index is there so that a reference can find the zone its entity is in when that zone is not loaded. An entity that
nothing refers to is never looked up that way. A dropped object is indexed in its zone, by the place it is sited on,
and is accessible while the actor is in that zone; from another zone it is not accessible at all. Indexing every
entity in every zone ever made would grow with the world for lookups nobody makes. An entity enters the index when the
first reference to it is made, so a reference finds it even after the entity has moved to another zone.

## Topology and occupancy compose on `World`

`World::move` resolves where an actor ends up, composing the layout's geometry with whatever occupies the cell ahead.
`LayoutBase` takes a predicate and stays independent of entities.

Two other places were considered. Putting it on `LayoutBase`, with `entities_within` on the interface, means giving the
layout a view of the entities, which would stop it being geometry and topology alone. Putting it in the rule means the
rule repeats a path the layout has just walked. `World` contains the layouts and the entities both, so it is the only
place that pays neither cost.

Continuous movement is what forces a single solve rather than a sequence. A blocker partway along a swept path changes
where the slide against a wall happens, so resolving topology first and occupancy afterwards gives results that are
wrong rather than approximate --- sliding along a wall into an entity, or stopping at an entity where the wall geometry
does not allow it. A discrete zone would tolerate the sequence, a cell being atomic, which is why it is not the case to
design against.

It also settles that a pushed boulder blocks the way a monster in a doorway does, which would otherwise diverge
between the two layout models.

## Actors are found through a zone proximity index

An actor's site has a placement and no address, and each zone has an index of the entities placed in it by place,
derived from the placements (`archived/addressing.md`, `### Actors`).

**Not a scan.** Checking every placed entity in the zone costs more as the zone fills, and `World::move` queries what is
at a cell for every cell it moves into.

**Not an address.** Siting an actor on the cell it stands on would make what is at a cell one lookup for actors as for
everything else. But every move would rewrite the address, and could create or delete a cell's entity. An actor's
place would be stated twice, in its placement and its address, which could disagree. A `ContinuousLayout` has no
cells to attach to. And attachment would mean two different relations: a torch stays on its wall, while an actor is
only standing somewhere this turn.

**An index.** A move costs two map updates and no entities. The placement remains the one statement of where an actor
is, the index being rebuilt from it, and each type of layout indexes in its own way.

## Passage is not a field on the wall

A wall is there or it is not. The entities sited on it decide whether an actor may cross it: `crossable` checks
whether the properties of an entity sited on it let the actor through, and has no special case for doors or concealment.
Nothing on the wall or the cell marks it crossable, so no state is kept twice.

A flag on the wall would be mutable state in layout data, which is the line the entity model is drawn on: layout data
is topology and appearance, entities are identity and mutable state. A lever toggling a `bool` inside the layout's
arrays is what that line was drawn to prevent.

The consequence is that `DiscreteLayout::crossable` feeds `move`, `neighbors` and `visible`. With openness on an
entity sited on the wall, none of the three can decide alone, and all three compose on `World` the way movement composes occupancy.
Sight through an open door is the case that makes it three call sites rather than one.

## A staircase is a block

A stair cell is a block with a wall on each side, and the staircase entity is sited on the wall on its open side
(`archived/addressing.md`, `### Places`). A staircase sited on the floor had to be given an orientation of its own and kept
consistent with the walls around it.

**Its direction is its wall's.** The normal of the wall on the open side is the direction of travel from the staircase
into the room, so there is no orientation to write, choose or check.

**Each side is a wall.** The sides the staircase does not open onto are walls, so each can look
different.

**Passage is a door's.** An actor goes through the open side as through an open door, so moving into a staircase needs
nothing beyond what `crossable` already checks.

## A minable block is a destructible container entity

A minable block occupies its cell as an entity, blocks while it is there, and drops its contents when destroyed.

Mining then destroys an entity and does not touch the layout. No wall is created, removed or renumbered, so the id
stability the attachment decision depends on is preserved by construction rather than by discipline.

The faces of a block belong to the block rather than to a boundary in the layout, so the neighbors of a mined-out block
already have their own faces and are exposed --- nothing has to be reconstructed at the moment of mining. A mineral
vein is stored there too, authored or generated per face ahead of any digging.

The cost is count, the same argument that keeps walls out of the entity store: a large digging volume is a large number
of entities. A layout's size stays a direct function of how much digging is permitted.

## Walls decide collision alone, as bits on the cells

**Collision reads only the walls.** `DiscreteLayout::move` moves cell to cell calling `wall_toward`, and checks
whether the wall there may be crossed. `move`, `neighbors` and `visible` all go through the layout's `crossable`,
and `World::move` adds the occupancy predicate above it. None of that path reads a room. Rooms are metadata, and
movement, sight and adjacency are independent of them.

**A wall is a bit on the cell it bounds.** Every wall comes from one glyph and its two neighbors, so nothing has to be
grouped, traced or linked to be built, and `wall_toward` is a bit test rather than a search. A wall's id encodes the
cell and the side. Everything a wall record once stored follows from that address: whether the wall is there, the wall
on the other side of its edge, its two ends, and its id. `discrete_layout_plan.md` has the table.

**Nothing walks an outline.** Outlines existed to give each area a boundary to trace, and the loader built them for
nothing else to read. A room's cells are one sweep over `room_id`, and a polygon for a caller that needs one is traced
from them.

## A cell position is a position and a discrete vector is a displacement

This is the background for splitting the two types, and for `math::DiscreteVector`.

**One type had three roles.** `Cell` was a coordinate, a move (`Cell {-1, 0}` in `move` and `neighbors`), and
a count (the grid extent). Only the first is a position, and the confusion showed: a cell was moved by writing out
`{cell.i + step.i, cell.j + step.j}` at four sites, and the extent as a position let a count be compared against a
coordinate without anything objecting.

**The split is by type rather than by discipline.** A cell position, `geometry::DiscretePoint` as
`discrete_point_type`, is moved only by adding a `discrete_vector_type` to it; subtracting two yields the displacement
between them; adding two does not compile. `GridMapping::extent` is a displacement, which is what a count of cells
along each axis is. Nothing spells cell arithmetic out componentwise. Variables are named `cell_position`, a cell being
the square and the cell position where it is.

**Negative components remain expressible, deliberately.** A cell position should never have one, but
`cell_position_at` maps any point to a cell position, and a move off the edge has to be representable long enough to
be rejected. Unsigned components would turn both into wraparound.

**`math::DiscreteVector<T, N>` rather than a type local to the layout.** `dungen` already had a 2D integer vector named
`Vector`, aliased as its `position_type`, `dimensions_type` and `vector_type`, and a second copy in `stratum_sim` would
have been the same class twice. Templating it on a signed integral type and a dimension, alongside `math::Vector`,
makes it one class. Signed because a displacement runs both ways along every axis.

**Quantizing is a choice of axis, not a rounding.** `direction` takes the largest component and returns a unit vector
along that axis alone; reducing each component instead turns a direction that was nearly east into a diagonal. Its continuous form requires the source and result to have the same dimension, so a caller taking a
direction through space to a flat grid reduces to the plane first --- `stratum_sim::grid_step` does that with
`geometry::MinProjection`, the dropped height staying in the layer in which the layout is flat.

## The boundary keeps an actor on the grid, and the bounds test is an assertion

`GridMapping::contains` is called from assertions only. Nothing branches on it.

**The loader builds the guarantee.** `stratum_format::has_wall` treats beyond the grid as solid, so every cell on the
border of the grid has a wall on its border side; in the wall notation, where the rows write that wall, the reader
rejects a file that leaves one out.

**So a runtime bounds test would be dead code.** `move`, `neighbors`, `visible` and `Interact` briefly had one; it
never fired on a loaded layout, and the four tests written to exercise it had to build a grid with no walls at all ---
a layout the loader cannot produce. Asserting instead states the invariant where it is relied on and costs nothing in a
release build.

**The queries take a direction, not a second cell.** `wall_toward(cell_position, direction)` replaced
`wall_between(cell_a, cell_b)`, which only ever used the difference of its two arguments. The old signature let a
caller pass a cell that need not exist purely to express a direction, and made the adjacency of the two an assertion
instead of a property of the type.

## Zone entries share one construction

A zone is constructed whole when it is encountered, geometry and contents, so `World::create_zone` from a generator
is the general entry, and a zone file is a snapshot of part of the same construction, loaded by the narrower
`load_zone`. A generator and the zone file reader each return a `ZoneDocument`, and both build the zone from it through one
function, which checks the document, makes the zone, has the loader fill it, and populates it, so neither route drifts
from the other.

A generator is a function registered by name in `Generation`, so `stratum_sim` links no generator, and the generator
reads the parameters that apply to it from the `ZoneSpec` it is given.

## The driver belongs to the zone

Turn-based or real time is chosen when a zone is loaded, not by the host, so a host implements neither and passes the
elapsed time either way. Before the clock existed, both hosts advanced the world only as a side effect of a keypress,
and a non-player actor had nowhere to act.

Real time over a discrete layout makes the timing continuous and leaves the space discrete, which is the Dungeon Master
arrangement rather than a compromise on the way to one. Movement within a cell is a continuous layout.

## An intent persists on the actor; a command is one-shot

An `Intent` is a level and a `Command` an edge: "north is where I am trying to go" against "north was pressed". An
intent is a component on the actor, so it survives unload and migration, which an actor pursuing a goal over many turns
needs and a decision asked for per action has nowhere to put. `Pursue` demonstrates it, storing the position last
seen on the entity. The clock builds moves and turns from intents, so a host sends only the one-shot half,
`Interact`, as a command.

Both movement and facing direction are absolute direction vectors, one representation for a discrete layout and a
continuous one: the discrete case snaps and the continuous case will not.

## The layout does all turning

`turn` is relative and `face` absolute. A view with a relative turn calls `turn` for the resulting direction and sends
it as an absolute facing direction, so the quantizing is the layout's in both cases, and neither the command nor the
host stores a rotation.

## A move costs the distance it travels

A move costs in proportion to the distance traveled against `step_distance`, not a flat action: four cells cost four
actions, a move stopped short is charged for the distance reached, and a real-time move of a fraction of a cell costs a
fraction of an action.

`MoveResult` reports a blocking wall and a blocking entity in two fields, a wall id encoding a side of a cell and an
entity id an entity, so the two ids are not interchangeable.

## Presentation is a reduction and a mapping, with no view interface

A reduction takes a zone to places, and a mapping takes places to what a device emits. Neither host calls a view
object to draw; each is written against the stage it uses, so a view interface would be a vtable with no call site.
The first-person view, which produces mesh instances rather than a buffer, is what will settle whether `Buffer<T>` is
the shared form.

`lattice` exists beside `cells` rather than replacing it: the two are not the same picture at a different scale, and
the tile view has no use for edges.

**An actor is a flag over a place.** An actor is on a place rather than a type of place. A `PlaceType` value for it
overwrote the floor, state and mask beneath, leaving a tile view nothing to draw under an actor. `Place::actor` is
layered over the structure instead, and a glyph line requires it as it requires a state.

**`PlaceType` has no value per state.** A door that can be shut, locked or concealed is one place type, its state
beside it, so nothing enumerates the cross product. A state is added only when it changes the drawing.

## flecs for the ECS

`game_system_principles.md` first scheduled the choice for Phase 4 --- flecs if relationships proved central, EnTT if
the conventional minimum sufficed. They proved central before Phase 1, three times over: sites, containment, and `Zone`
partitioning. EnTT models none of them natively; a site there would be ids in components with reverse indexes
maintained by hand.

A site is nonetheless ids in components with a reverse index kept by hand, chosen for the count ("Sites and places
link both ways"); flecs still supplies the zones as subtrees, the hierarchy, prefabs, queries
and reflection.

## One resident zone, and a frozen zone advanced by elapsed time

State surviving being left is what residency was first wanted for, and serialization at the persistence boundary
delivers it without keeping every zone in memory, which is also what players expect. A single resident zone therefore
costs nothing.

Off-screen effects advance only where a component can state itself as a function of elapsed time. Anything needing
pathfinding or combat resolution stays frozen: that is the line Oblivion's Radiant AI crossed, where unconstrained
off-screen simulation produced outcomes players read as absurd, and which was substantially scaled back before release.

## Zone data is never promoted to an entity

Promoting zone data to an entity at runtime means two representations of the same data, a conversion step, and a class
of bugs where something was referred to in its old form. Everything with identity or presence is an entity from
authoring time instead.

A flat's role is explicit rather than inferred from its normal: inferring walkable from blocking by the normal puts a
steepness threshold inside the simulation, which is geometry the layout layer stays out of, and such thresholds are
adjusted forever after.

## Names

`Zone` is from the MMO lineage, where zoning also labels the transition, which describes what relocators do. `Flat` is
Doom's word for the horizontal surfaces of a sector, as against wall textures; it survived that lineage acquiring
sloped sectors, so it reads as structural rather than descriptive.

## The core is C++, outside the engine, for reuse

A turn-based grid simulation would perform well enough in GDScript. The core is C++ because it outlives the engine
choice and goes to any future front end, including a quetzal-native one.

Component storage was first written by hand and replaced by flecs ahead of Phase 1, once zone content showed
relationships to be central. The command and event queues stayed, flecs' observers being immediate rather than
something a view drains per move.

## Godot, committed to at Phase 3

With simulation, data, serialization and entity modeling removed from it, what Godot supplies is the presentation half,
which it does well. The cost is that GDExtension is where most of the work happens: every core change crosses that
boundary, with a rebuild and an occasional editor restart. Phases 0 to 2 need no engine, so the commitment waits until
Phase 3, when the view interface is understood; a wrong answer then costs one view implementation, not the project.

Nothing from Godot's data or scripting layer is used by the core: `Resource`, `ResourceSaver` and GDScript are tied to
Godot's `Object`, `ClassDB`, `Variant` and virtual filesystem.

## One host per window

Compositing configurations A and B into one window --- Godot for 2D and UI, quetzal for the 3D viewport --- means two
windowing stacks in one window, two event loops and two presentation paths, with no clean answer for input focus,
z-order, or which one owns the swap chain. That argues only against mixing hosts in one process: not
against B, whose costs are its own, and not against the terminal application, a separate process.

Building B would be the strongest proof of the view abstraction: a second front end on different infrastructure,
running the same simulation in place of A.

## The ASCII view is permanent

The terminal application is the only view that can be regression tested, and the tests render through it, which
keeps it compiling. It keeps "the core builds standalone" enforced by the build rather than by discipline, which would
lapse once Godot became the only consumer. It keeps two different views in hand, so a later change for views has to
suit one with no camera, no pixels and no frame loop. And because a terminal redraws from full state, it exercises the
snapshot rebuild path continuously rather than only on a swap.

The shared renderer was extracted when the second host arrived rather than written up front, per principle 7.

## scene_graph is not the universal renderer abstraction

`scene_graph` is a 3D spatial hierarchy with inherited transforms, materials and lights --- what a first-person or
orthographic view needs, and overhead for an ASCII view. It is one view implementation's structure. Making it
renderer independent remains worth doing, for the existing applications and as the prerequisite for configuration B.
`vulkan::IRenderer` is a good Vulkan abstraction and would never be a good ASCII one.

## JSON for content

Prototypes are deeply nested trees, and JSON Schema gives validation and editor support with no custom work. Tools write
the format as well as read it --- editor export, generated content, saves --- so it has to survive a machine round trip,
and comments are recovered by parsing with `ignore_comments`. Cataclysm DDA and Minecraft data packs are JSON.

The zone file is read with nlohmann/json: it parses to a document the reader can validate and report from,
`ordered_json` preserves key order, and it is what most open source C++ projects use. simdjson, also in the build, is a
read-only on-demand parser, suited to a hot path rather than to a reader that reports what was wrong and where.

`stratum_format` writes its own output because no library's pretty-printer produces the written form the format
specifies, and that form is what makes a zone file diff.

**JSON stops at loading.** A `ZoneDocument`'s component objects are `ordered_json`, the file being content the format does
not interpret, arbitrarily nested and in written order. A value type defined in `stratum_format`, as tinygltf's `Value`
is for glTF extras, would hide the library but remain a JSON tree under another name, with no gain in typing, so the
`ZoneDocument` keeps nlohmann's and nothing outside the file layer includes it. Keeping the tree on entities instead would
cost a string lookup on every property read, defer every error to the first read, and allocate a node per value in a
world orders of magnitude larger than the test zones; typed components are read directly, checked at load, and stored
packed.

## Everything a game adds is data or script

This is principle 8, detailed in `modding_plan.md`. Mod additions are unpredictable --- a room of fun-house mirrors is as
likely as a new monster --- so nothing is designed on the assumption that the vocabulary is known in advance.

**Typed components with runtime schemas.** Decoding into typed components is what Bevy scenes, Godot scene files and
Cataclysm DDA do, and flecs already requires reflection registered for every component so that saving works. flecs
builds a component from a reflection description at runtime as readily as from a C++ type, so a mod's schema produces
a component that is stored, decoded, saved and scripted like the game. Registering in C++ alone would make every
mod a rebuild. An unknown name failing the load catches misspelled names, as Cataclysm DDA's rejection of unvisited
members does; a component a missing mod declares is skipped, as RimWorld's `MayRequire` skips content.

**Visuals are data.** Engines treat a material as a shader asset with named parameters --- Unity's material property
overrides, Unreal's material instances --- and a style is that: an asset name and a parameter map. Merging parameters
by name gives a zone partial overrides without optional fields in every component. Surface form comes from kit pieces,
as in Legend of Grimrock's wall sets and Bethesda's architecture kits, so a curved mirror is a mesh and collision is
unchanged.

**Scripting wherever practical.** Lua is the genre's language: Factorio, Legend of Grimrock, DCSS and Noita all use it,
and Luau, Roblox's Lua, adds sandboxing and gradual typing for running other people's code. Legend of Grimrock 2 is the
closest precedent: a grid dungeon crawler whose mods define objects as component lists, materials, wall sets and
behavior in Lua. Script state in components is what keeps saving a matter of components, and the seeded random source
is what keeps generation reproducible.

**The limit.** A new type of layout is a `LayoutBase` subclass in C++. Every data-driven game places the engine line there.

## Lua for scripts and patterns

Lua 5.5 is the language of the game's and mods' scripts and of patterns (`archived/scripting_language_plan.md`). It is the
genre's language (Factorio, Legend of Grimrock, DCSS, Noita), so mod authors already know it. Its sandbox is built from
what it has: an environment for each script, a count hook, and the allocator. A hash seed derived from the world seed
and a `random` that draws from the simulation's random engine make it deterministic. Its tables give patterns arrays
and structures without adding them to `quetzal::expression`, and one userdata type binds any component registered at
runtime through the meta cursor. Compiled as C++, a Lua error unwinds C++ frames. A distinct pattern costs about 3 us
to compile and call, against 1.8 us to parse and evaluate one with `quetzal::expression`.

Luau's sandbox is built in, but its type checker and tooling need an integration of their own outside Roblox.
AngelScript is C++-like and reference-counted, with thin developer tooling.

## Entity locations in other tools

Tiled (a general 2D map editor, TMX or JSON) places objects by position in object layers and refers between them by
integer object id, keeping the next id in the map. LDtk (a 2D level editor from the studio behind Dead Cells, JSON) has
entity layers with typed fields and refers by UUID. Hammer (Valve's editor for Half-Life, Counter-Strike and Portal) has
point entities and entities tied to brushes, wired by name through inputs and outputs. IFC (buildingSMART's ISO 16739
model for building data) makes each relationship an object --- an opening voids a wall and a door fills the opening ---
and gives each object a GUID. Unity keeps an object's place as its `Transform` component, and Flecs its hierarchy and
prefabs as `ChildOf` and `IsA`, which is the form chosen here: where an entity is and what its parent is are components.
NetHack's per-cell object chains are a location index outside any entity system. Snowflake ids at Twitter and Discord
divide 64 bits between a time, a machine and a count, and Bethesda's form ids between a plugin's place in the load order
and its own count.

## Per-edge walls, one per side

Solid cells cannot express a wall or a door between two open cells without spending a whole cell, so per-edge walls
subsume them and not the reverse. With everything per side, one-way doors and secret doors need no special case, and no
orientation convention is needed for which side data belongs to. An opening being the absence of a wall is what
non-rectangular rooms need, having no predefined wall slots to flag.

## Data-level inheritance only, and compiled extensions registered at compile time

Multiple parents and mixins are reasonable in prototype inheritance because merging property bags is well defined.

Dynamic C++ plugin ABIs are fragile across compilers and build settings; GDExtension's own compatibility trouble is
this problem in a well-resourced project. Static libraries with registration functions give compiled extensions the
plugin architecture without the ABI cost. Mods need no ABI at all, adding schemas, types, styles and scripts as data
at runtime ("Everything a game adds is data or script").

## The phase order

A minimal first-person 3D view comes at Phase 3b, ahead of content and procgen, because a wall between two open cells
is what a cell-per-glyph view cannot show and a first-person one cannot avoid, and procgen should generate against a
wall model already validated.

The real-time driver came forward from Phase 7: real time is the direction the application is moving toward rather
than an option held open. Continuous time and continuous space separate cleanly, so only continuous space stays in
Phase 7.
