# Simulation Zone Model

Status: the discrete half is built and `game_system_reference.md` describes it; the continuous half is declared and not
written
Date: 2026-08-25, revised 2026-09-22

Companion to `game_system_principles.md`, which has the architecture and the phased plan. This covers what that document
defers: what a `Zone` is made of, and what the `Layout` interface over it is across the discrete and the continuous
case. `decision_rationale.md` has the argument for the larger decisions.

Every section is marked **Settled**, **Leaning**, or **Open**, and what is deferred rather than undecided is marked
**Future**. Names are working names throughout.

## Layers, and what each has access to

**Settled.**

```
World      the set of Zones, and the relocators between them
Zone       one coordinate space: its Layout, and the entities in it
Layout     geometry and topology, and the queries over them   has positions, exposes none
entities   ids and composed components                        have placements, not coordinates
rules      behavior over entities                             have commands, events, queries
```

A `Layout` is geometric --- it has positions, normals, and extents. Geometry travels outward through it toward the
view, and never inward into the rules: a rule asks the `Layout` where a move ends; it does not compute a destination.

Movement is asked of `World` rather than of a `Layout` directly, because only `World` has both the walls and the
entities that block an actor. See The Layout interface.

## World, Zones, and Levels

**Settled.** `World` is the set of `Zone`s --- dungeons and outdoor areas alike --- and the relocators between them. A
`Zone` is loaded on entry, and residency is a bounded working set rather than everything ever visited, so unloading is
routine.

**One `World` and one `Session` in a game.** `World` manages the zones, with the flecs world containing their
entities, and cannot be copied or moved. `StratumGame` owns the `World`, the `Session`, the `Player` and the id
allocator; the `Session` refers to the `World` rather than containing it.

**A zone's population are its denizens,** not all of them monsters.

**A `Zone` is one coordinate space, and it is 3D.** `Zone`s share no coordinate system and have no spatial relationship
to one another. Surface to dungeon is always a teleport: the scale difference precludes any vertical correspondence
between them.

**A `Zone` is one slab,** bounded by its two `Flat`s above and below. The levels of a dungeon are separate zones, and a
staircase between them is an entity with a relocator.

Within a `Zone` there is one coordinate system, so what lies above or below a point is a geometric
question. Across `Zone`s there are relocators with explicit targets. **No partner ever crosses a `Zone`
boundary**, so a partner is always resolvable once its own `Zone` is loaded.

**Future: a `Zone` spanning several levels.** One coordinate space over stacked levels, with a stairway as ordinary
geometry rather than a relocator. A pair of height fields cannot bound it, so it needs more than a `Flat` above and
below. Not designed.

**Future: `Region`, a grouping of `Zone`s.** Literal for a surface area, and for a dungeon too, whose levels are its
zones. Both are regions, with sets of properties that overlap but differ. "Region" is taken out of every other use
first; the open questions are in `work_plan.md` (`### Regions`).

- **A region is data.** Its properties are components on its entity: a name, and for a dungeon the maximum depth,
  which the probability of a staircase leading down needs, the overall or starting difficulty, and the other
  properties a whole dungeon has. It acts on nothing directly, apart from what a generator reads from it.
- **A region is its zones' parent.** It takes the place in the hierarchy the root has now, so a zone's flecs parent is
  its region. `zone_of` still stops at the first ancestor that is a zone, and an entity's region is its zone's parent.
  Unloading a zone leaves its region; deleting a region deletes its zones. A region, having no zone above it, is given
  its persistent id when it is created, as a zone is.
- **A region decides which zones unload first.** The zones of other regions than the one the player is in go first,
  left longest ago first; then the zones of the player's region, farthest first, by distance or by the difference in
  depth.

**Future: region files.** A region's metadata and generation settings, and possibly then its zones, one after another.

**Relocators are the only cross-`Zone` link, and they are components of entities rather than structure.** A relocator
is what a staircase, a portal or a waypoint has: it refers to a target, a destination not yet bound or the relocator
arrived at by its persistent id (`addressing.md`, `### Relocators and access points`), and its traversal behavior,
whether walking, falling, levitating, or blinking, is script over an engine default. Being on entities they are
enumerable, which is how `World` determines which `Zone`s are reachable from where.

### Level

**Settled.** A level is a name for a `Zone` rather than a structure of its own: numeric depth, difficulty, name,
generation recipe, encounter and loot tables, palette. The zone is the concrete structure it refers to, as a `Room`
refers to an area within one.

It is a property rather than derived from a height band, so a spiral ramp, a mezzanine, or a shortcut vault can be
assigned deliberately. Numeric depth and the other properties are stored on the `Level`, not derived from z.

Nothing structural consults a `Level`. Movement never does. Generation, the automap, and the UI do.

### Entity ownership and migration

**Settled.** Entities are partitioned by `Zone` rather than pooled. With flecs this is one `flecs::world` in which each
`Zone` is an entity and its contents are that entity's children; unloading a `Zone` is serializing and then deleting
that subtree. The partition is a subtree, not a separate registry.

**A `Zone`'s contents are its subtree rather than its children.** An entity with a parent of its own, such as a door
facing whose parent is its door, belongs to the `Zone` through that parent, so a query for the `Zone`'s children finds
what is placed directly in it --- as occupancy and the view need --- while anything over the whole `Zone` walks the
subtree. Deletion reaches all of it (`addressing.md`, `### Children`).

**Ownership follows current location, not origin.** A monster that chases the player through a relocator migrates into
the destination, so the `Zone` it came from can unload without it. The player is the exception: always in a resident
`Zone`, but owned by the game rather than by that `Zone`, so unloading or saving the `Zone` leaves it out
(`addressing.md`, `### The player`).

**Entity ids are global though entities are partitioned by `Zone`,** so migration does not renumber and references to
a denizen --- a quest, a leash, a summoner --- survive the move. Between loaded zones one flecs world supplies this, and
reparenting leaves the runtime entity unchanged, which `test/flecs_test` verifies. Across an unload, a save and a
session it is the entity's persistent id, which includes no zone (`addressing.md`, `### Ids`).

**A denizen's home is data, not ownership.** Origin `Zone` and leash point are fields on the entity, so it can belong to
the crypt while standing in the corridor outside.

What is global is therefore world state rather than entities: quest flags, game time, faction standing, RNG streams.

### Residency, simulation, and persistence

**Settled.** *Persistence* is a `Zone`'s state surviving being left; *residency* is its data being in memory now. With a
bounded working set, persistence is required from the first unload: unloading is serialize-and-discard, loading reads it
back, and a save writes every visited `Zone` whatever is resident.

**Two tiers, for now.**

| Tier | State |
|---|---|
| The player's `Zone` | resident, simulated |
| A `Zone` the player has left, until it is unloaded | resident |
| Everything else | unloaded, frozen, on disk |

**Leaving a `Zone` does not unload it.** When the player moves out of a `Zone`, before or after the next one is
loaded, the resident set is checked against a threshold --- a maximum `Zone` count, memory usage, and the like --- and
`Zone`s are unloaded only when it is exceeded. The session keeps a map from the timestamp of the last time the player left
each `Zone` to its id, and the oldest is unloaded first, more than one where a memory threshold requires it.

With more than one `Zone` resident, "near the player" can only mean relocator-graph distance, `Zone`s having no spatial
relation, and the relocator graph is enumerable because relocators are on entities.

**Elapsed-time advance on re-entry.** Effects expressible as a function of elapsed time are advanced when a frozen
`Zone` loads: corpses age, growth continues, stores restock. This is per component: a component that can state itself
as a function of elapsed time is asked to advance, and one that cannot stays frozen. Rot, growth, restocking and
schedules advance; anything needing pathfinding or combat resolution stays frozen.

### Naming

**Settled.**

- **`Zone`** is an independently loadable area --- a dungeon, a town, a moor.
- **`Layout`** is how a `Zone` is arranged, and the queries over it.
- **`World`** is the set of `Zone`s, one `flecs::world` with each `Zone` an entity in it.
- **`Flat`** is a horizontal surface, floor or ceiling.
- **`Scheduler`** orders actors in abstract units, the driver deciding what advances them.
- **`Level`** is metadata describing a `Zone`, not geometry.
- **`Room`** is a named area within a `Zone`, for the player rather than for anything structural.

## Layout

### Zones, rooms and cells

**Settled.** A `Zone` is what a zone file loads: a floor and a ceiling `Flat`, the walls between its cells, and the cells
between the two flats. A surface zone has one flat, having no ceiling. A file is one zone whatever it contains.

`Zone` is the more inclusive word for what a `Level` is: a level is the dungeon reading of it, and a zone is equally a
section of an outdoor area, or anything else bounded above and below.

A zone spans rooms. The rooms, halls and corridors of a dungeon level are labels over parts of one zone; a room does not
bound a zone, and a zone is not divided by having several. Where a zone's parts need unrelated heights, they are
separate zones.

**A room is a label.** It is `room_id` on the cells it contains, and an entity with its components. There is no
structural room and nothing structural consults it. `Level` and `Room` are both names for a referent: a zone and part of
a zone.

**Structure is stored per cell in a discrete `Zone`.** A wall is stored where one exists and nowhere else, so the
interior of an open area costs nothing and only its boundary is stored. In the block notation a wall exists only where
open space meets a solid cell; in the wall notation, wherever the file writes one, between two open cells as often as
against a solid one.

**A door is an attachment with a door facing on each wall it is on.** The door has whether it is open, closed, locked
or trapped, and decides whether an actor may cross. Each door facing is on the wall of one side and has what is reached
from that side, so one door is one entity however many sides see it.

In the block notation a doorway is a cell rather than an edge, so a door there is on the walls the floor either side has
toward that cell --- the cell being a block an actor may stand in, with no wall of its own toward floor. The door is the
same in both notations; only the walls it is on differ.

Loading a zone creates it: the zone file's door glyphs and its entities become the door and its door facings
(`discrete_layout_rework.md`).

### Walls

**Settled.** An edge is what a zone file writes as one glyph for both sides, and a wall is one side of an edge, stored as
a bit on the cell it bounds. A wall's id encodes that cell and side, and everything else about it follows from the grid:
its ends, its normal, and the wall on the other side of its edge. Walls are not linked to one another.

**Each side is separate.** An actor can always determine which wall it can interact with, and the wall on the other side
of the edge is out of its reach until it is on that side. Everything is per side --- surface treatment, what is sited on
it, and a door facing. One-way and secret doors are a door whose door facings differ.

**Passage is decided by what is sited on a wall.** A wall is there or it is not; nothing on the geometry marks it
crossable. A closed door blocks because it is closed, a secret door because it has not been found, and an invisible wall
because it is an entity on the wall that blocks and renders as nothing.

The caller supplies the predicate --- the layout is independent of entities --- so `move`, `neighbors` and `visible`
each take one, and `World`, which has the layout and the entities both, binds it for the zone. The answer depends on who
is crossing as much as on the wall, which an actor-specific rule needs.

The layout has one wall question, `crossable`, asked of a move: no wall is a way through, and a wall is the predicate's
to decide. The grid boundary is no separate case: floor is enclosed by walls, so a wall stops the move there.

**Open.** Sight may need a question of its own: a portcullis passes sight and not an actor, a curtain the reverse.

- **An opening is the absence of a wall,** not a flag on one.
- **A wall with no floor behind it is a border wall** --- nothing is on the other side of its edge. The zone's extent is
  finite and bounded by solid, so this is also the map border.
- **A minable block is a destructible container entity.** It occupies its cell, blocks while it is there, and drops its
  contents when destroyed. Mining destroys the entity; the layout is not touched, so no wall is created, removed, or
  renumbered, and every attachment id is unchanged.
- **The faces of a minable block belong to the block,** so the neighbors of a mined-out block already have their own and
  are exposed. A mineral vein is stored there too, authored or generated per face.
- **Minability is not a property of the geometry.** It is whether a cell has one of these, and an indestructible one is
  the same entity refusing to be destroyed.
- The cost to watch is count: a large digging volume is a large number of entities. A layout's size stays a direct
  function of how much digging is permitted.

### Something reached from more than one side

**Settled.** A block is reached from every exposed face: a door from either side, a minable block on a corner from two.
What differs by side and what does not are separated rather than duplicated.

- **One entity has the shared state.** Open, closed and locked for a door; contents and destructibility for a block.
- **Per-side entities have what differs,** and refer to that one. A swinging door opens outward on one side and inward
  on the other while open on both, which is per-side appearance over one shared state. A corner block is two faces and
  two veins over one block that has the ore and is destroyed once.

A side is a child of the entity it belongs to, and that entity links to each of its sides. Two walls do not refer to one
entity, and two walls do not mirror each other.

A wall never consults stored cell data. The block entity is that data, referred to directly rather than found by
position.

**A face has its own entity only when something must target it,** such as a lever sited on it or a face that takes
damage of its own. Otherwise per-face data on the block does, for a fraction of the count. A lever needs no face entity
either: it is sited on that side's wall and refers to the door it operates, so its side is implied by its site.

**Settled: a wall is not an entity.** A wall with components or with something sited on it is an entity its zone finds by
wall id, sparse because what is on walls numbers hundreds rather than thousands.

What targets a wall links both ways: a door facing, or anything else attached to the wall, links to the wall's entity,
which the zone finds by wall id, and the wall's entity links back to it. Nothing is found by searching.

Everything on a wall or a flat is an attachment --- an alcove, a basin, a door, a button, a torch --- one mechanism for
all of them, and a wall stays a bit on the cell it bounds.

This depends on wall ids being stable. A wall id encodes its cell and side, and a minable cell is a phantom cell carved
at generation, so mining reveals geometry rather than creating or destroying walls, and nothing renumbers.

Containment is the same mechanism: an item in an alcove is attached to the alcove, `contained_in`, the target being an
entity rather than a wall or a flat cell (`addressing.md`, `### Entities`).

### Flats

**Settled.** A zone is bounded by two `Flat`s, one below and one above, each a height field over the whole zone. A
surface zone has the floor alone. Floor and ceiling differ by role and nothing else --- stalactites and stalagmites are
one mechanism inverted.

A constant height is the ordinary case. Where a zone's parts need unrelated heights, they are separate zones, rather than
one flat describing a discontinuity.

A `Flat` has:

- **A height field, not a single plane** --- sloped floors, arched ceilings, steps. It is a constant until something is
  sampled into it, so the ordinary case costs one value.
- **Where the surface is absent.** A pit, a shaft, and a hole dug through the floor are where the surface is absent
  rather than low, so a sample is an optional height. The sampling may be finer than the cells.
- **Its role,** floor or ceiling, explicit rather than inferred from the normal, so no steepness threshold is in the
  simulation. The normal stays available to the view.
- **`height`,** the height field itself. A wall's height is determined by the floor and ceiling it spans.
- **Attachments,** any number, each at its own position on the surface.

What is below a point is determined by what occupies that point in the `Zone`'s one coordinate system.

### The three grids

**Settled.** A block crawler makes these look like one concept because all three have the same spacing. They are
independent:

| Grid | Purpose | Exists in continuous? |
|---|---|---|
| Surface sampling | height map and material resolution | yes |
| Movement quantization | the move size a discrete `Zone` rounds to | no |
| Spatial index | bucket size for proximity queries | yes |

Keeping them apart lets a continuous actor reuse the same layout, and lets the height map be finer than the movement
grid.

## Placement

**Settled.** An entity's placement is a position and a facing direction, in both layout models. The position is 3D, as a
`Zone` is.

A placement is meaningful only within its own `Zone`, and which `Zone` that is follows from the entity's ancestors rather
than from a field on the placement.

A discrete layout does not accumulate: it recomputes the position from the cell reached on every move, so error never
builds up, which makes a shared representation safe under principle 6.

Rules never decompose a placement --- they hand it back to the layout, so a cached location handle can be added later
without changing a rule.

**Attachment.** A placement may be relative rather than absolute: a wall or a flat, and a position on that surface,
resolved using the surface's geometry. A basin stays on its wall whatever angle the wall is at. A wall or flat may have
any number of attachments --- a wall's position is along-and-up, a flat's is xy.

**Extent** is a separate component: how much space the attachment occupies. A pressure plate has one, as do a boulder
and a monster. An entity spanning more than one cell is therefore no separate kind: a trigger noticing an actor half on
it works as collision does.

## The Layout interface

A `Zone` is a place; its `Layout` is how that place is arranged and what can be asked of it.

**Settled: the layouts split on how movement is resolved, and on nothing else.** A class is earned where two cases differ
in behavior rather than in data.

- **`Layout`** is the interface every layout satisfies: where an actor ends up, what is ahead, what is adjacent, what is
  visible, and how far one action of movement travels. It declares no cells and no walls.
- **`DiscreteLayout`** is movement quantized to a grid of cells. It is the whole of the discrete side: nothing lies
  between it and `Layout`.
- **`ContinuousLayout`** is movement that integrates, through geometry that is ordinary 3D models.
  Declared and not written: every member is pure, so nothing constructs one.

**Geometry is not a second axis of classes.** A zone with no height data has a floor at a constant
height, which is a `Flat` with fewer of its features used --- the same structure the sampled case
lives in, as one wall represents both a block's face and one side of an edge. A zone with
height and entities on its flat cells and one with neither load the same class from different data, so a
game style chooses by what it loads rather than by which class is built. That is why neither name
mentions `Flat`: it is implied for every discrete layout, and for a continuous one the zone is ordinary
3D models, where being level is no more a category than any other shape.
`decision_rationale.md` makes the argument.

`quetzal::stratum_sim::DiscreteLayout` has no provision for continuous movement, which is `ContinuousLayout`'s, and none
for a wall spanning more than one cell edge.

**Cells and walls belong to the discrete half.** A cell is a coordinate on a grid, and one wall lies
on one cell edge so that the two run end for end. That is what lets a partner be found by exact
match, and it is a property of the grid rather than of geometry in general --- arbitrary polygons put
one area's single edge against a neighbour's several, which no exact match pairs. A continuous
layout resolves the same questions of its geometry and shares none of this machinery.

**A cell position is a position, and a displacement is a discrete vector.** A cell position moves only by adding a
vector to it, subtracting two gives the vector between them, and a grid's extent is a vector, a count of cells along
each axis being a displacement. A query about what adjoins a cell takes that cell and a direction, so the neighbor need
not exist. `GridMapping::contains` is asked only in assertions: the loader walls every boundary cell, so a move off the
grid cannot be made.

The interface as written is in `Layout.hpp`, described in `game_system_reference.md`.

`MoveResult` is a placement and what stopped the actor, the wall or the entity, neither set meaning nothing did. It
reports "the door is locked" without the rule asking a second question.

**Relative and absolute movement are one call.** Relative passes the entity's own facing direction as the direction;
absolute passes a direction the view chose, leaving the facing direction unchanged. A top-down view maps west to a
direction, so a roguelike actor does not rotate when it moves west.

**Direction is a vector, not an axis delta,** so granularity stays with the layout, and hex tilings and angled walls
work --- the layout snaps to its own directions. Vectors are permitted at the command boundary, where a view has an
absolute direction; a rule passes one through and never computes with it.

**Blocked is not binary.** `move` returns where the actor ended up, not a success flag. A discrete layout returns the
cells moved; a continuous one returns wherever the collision left the actor, including partway or slid along a wall.

**Blocking has two sources:** zone topology, and blocking entities. A pushed boulder blocks the way a monster in a
doorway does.

**Settled: the two compose on `World`, not on `Layout`,** and are resolved in one solve, not in sequence
(`decision_rationale.md`, "Topology and occupancy compose on `World`").

## Entities and zone data

**Settled.** The line is **identity and mutable state**, not appearance versus object, and not interactive versus
scenery.

**Zone data** is topology, height maps, and surface appearance --- materials, patterns, the scorch mark nobody touches.
There are orders of magnitude more surface samples than attachments, and none of them have state worth an id.

**Entities** are everything with identity or physical presence, from authoring time: a basin to drink from, an alcove
that is itself a container, a pushable boulder, and a fixed statue far enough out to change where an actor can walk ---
the statue has no behavior but it has presence, and anything that changes where an actor can walk must be findable and
removable.

Nothing is promoted from zone data to entity at runtime, which would mean two representations of the same data and a
conversion between them.

Sparse attachments --- alcoves, basins, plates, statues --- are dozens per zone, not thousands, so entity representation
costs nothing.

### Relationships

**Settled: flecs.** Zone content has two relationships:

- **attached-to** --- a wall or a flat, and an offset across it.
- **contained-in** --- an alcove is a container.

Both are traversable from either end. Mining through a wall with an alcove has to find that alcove and decide what
happens to its contents, so the wall must be able to find what is attached to it.

`test/flecs_test` verified the fit: `Zone`s as entities with contents as children, migration by reparenting with the
entity id unchanged, unload as a subtree delete, per-`Zone` serialization, and whole-world JSON round trip preserving
`ChildOf` and multi-member components.

A `Parent` hierarchy, which a door's door facings use, is the exception: flecs writes one and cannot read it back,
`from_json` aborting on a world with one (Flecs 4.1.6). So a zone is written in the zone file format, where a child
states its own parent, rather than through flecs JSON (`addressing_plan.md`, `## Flecs`).

Attachment and containment are not relationship pairs: an attachment is a component with its target, and the target has
a component listing what is attached to it (`decision_rationale.md`, "Attachments and walls link both ways"). flecs
supplies the zones as subtrees, the hierarchy, prefabs, queries and reflection.

Component reflection is registered member by member, or `to_json` emits names without values. Iteration is
deterministic but **not ordered** --- entities are visited grouped by archetype, so sorted output, such as Phase 5's
checked-in reference files, is sorted explicitly.

### Behavior

**Open.** Pressure plates, levers, timed doors, and pits are what a dungeon crawler is made of. Trigger behavior is the
case that turns a data format into a bad programming language, so the scripting language is due around Phase 4.

**Settled.** What happens when a statue is attacked, a wall is mined, or an alcove's wall is destroyed is
script-determined, over a default the engine supplies, with specific entities overriding that default. The structure
only has to guarantee that the wall can find what is attached to it.

## Discrete and continuous

**Settled.** Separate `Layout` classes, not two simulations. See The Layout interface for which
they are and what each name means.

Shared: entity and component core, command and event vocabulary, level model, generator, renderer.
Movement rules differ between a crawler and a shooter regardless; combat and inventory rules barely
touch space and stay shared.

Block-oriented is a *subset* of wall-oriented, not a sibling of it --- solid cells cannot express a
wall or a door between two open cells without spending a whole cell on it, and per-edge subsumes
them while the reverse does not hold. So the first game is not a second `Layout` implementation; it is
one generator emitting zones whose only walls are solid boundaries.

What genuinely differs between the two `Zone` implementations:

- **Determinism.** Integer cell movement is trivially reproducible. The shared placement
  representation is safe only because a discrete layout regenerates rather than accumulates.
- **Field of view.** Roguelike FOV is a grid algorithm producing a set of visible cells, and its
  characteristic shape is something players recognise. Ray casting produces a different result.
- **Adjacency and occupancy.** A spatial query in both cases, but with very different constants.
- **Constraint by type versus by discipline.** In a discrete layout the movement constraint *is* the
  type. Rounding lives in one place --- the layout --- rather than in every rule.

This makes Phase 7 smaller than it first looked. It stops being "add a continuous layout" and
becomes "add a second mover over the layout already in place", and `game_system_principles.md`
states it that way now that `ContinuousLayout` is declared.

## Open questions

- **What builds a zone's entities.** `load_layout` returns a layout; it reports each door's walls, and the caller
  creates the door. Loading a zone builds the file's entities (`addressing_plan.md`).
- **How a height field is stored and sampled,** a constant one being the ordinary case, which must cost nothing.
- The scripting language, above, and earlier than the phased plan assumes.
