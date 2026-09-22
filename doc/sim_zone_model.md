# Simulation Zone Model

Status: the discrete half is built and `game_system_reference.md` describes it; the continuous half
is declared and not written
Date: 2026-08-25, revised 2026-09-02

Companion to `game_system_principles.md`, which owns the architecture and the phased plan. This
covers what that document defers: what a `Zone` is made of, and what the `Layout` interface
over it looks like across both the discrete and the continuous case.

Every section is marked **Settled**, **Leaning**, or **Open**, and what is deferred rather than
undecided is marked **Future**. Names are working names throughout.

## Layers, and what each has access to

**Settled.**

```
World      the set of Zones, and the relocators between them
Zone       one coordinate space: its Layout, and the entities in it
Layout     geometry and topology, and the queries over them   has positions, exposes none
entities   ids and composed components                        has placements, not coordinates
rules      behaviour over entities                            has commands, events, queries
```

A `Layout` is unapologetically geometric --- it has positions, normals, and extents, and pretending
otherwise would be a fiction maintained by not looking. What matters is that geometry travels
*outward* through it toward the view, and never inward into the rules. A rule asks the `Layout`
where a move ends up; it does not compute a destination.

The one deliberate leak is `Layout::position()`, which resolves a placement to a point so a view can
draw it. It is one-way. The reverse --- a point or an offset resolving to a location --- is what would
put geometry back into the rules path, and is not offered.

Movement is asked of `World` rather than of a `Layout` directly, because what blocks a mover comes
from two places and only `World` has both. See The Layout interface.

## World, Zones, and Levels

**Settled.** `World` is the set of `Zone`s --- dungeons and outdoor areas alike --- together with the
relocators between them. A `Zone` is loaded on entry, and residency is a bounded working set rather
than everything ever visited: keeping a continent and all its locations in memory is not practical,
so unloading is routine rather than exceptional.

**A `Zone` is one coordinate space, and it is 3D.** `Zone`s share no coordinate system and have
no spatial relationship to one another. Surface to dungeon is always a teleport: the scale
difference precludes any vertical correspondence between them.

**A `Zone` is one slab,** bounded by its two `Flat`s above and below. The levels of a
dungeon are separate zones, and a staircase between them is an entity with a relocator.

Within a `Zone` there is one coordinate system, so what lies above or below a point is a geometric
question. Across `Zone`s there are relocators with explicit targets. **No partner ever crosses a `Zone`
boundary**, so a partner is always resolvable once its own `Zone` is loaded.

**Future: a `Zone` spanning several levels.** One coordinate space over stacked levels, with a stairway
as ordinary geometry rather than a relocator. A pair of height fields cannot bound it, so it needs more
than a `Flat` above and below. Not designed, and nothing depends on it.

**Relocators are the only cross-`Zone` link, and they are components of entities rather than
structure.** A relocator is what a staircase, a portal or a waypoint has: it references a target, a
destination not yet bound or the relocator arrived at by its persistent id (`addressing.md`,
`### Relocators and access points`), and its traversal behaviour, whether walking, falling, levitating, or
blinking, is script over an engine default. Being on entities they are enumerable, which is what tells
`World` which `Zone`s are reachable from where.

### Level

**Settled.** A level is a name for a `Zone` rather than a structure of its own: numeric depth,
difficulty, name, generation recipe, encounter and loot tables, palette. The zone is the concrete
structure it points at, in the same way a `Room` points at an area within one.

It is a property rather than derived from a height band, so a spiral ramp, a mezzanine,
or a shortcut vault can be assigned deliberately instead of being sorted by whatever height it
occupies.

Numeric depth and other properties are **stored on the `Level`, not derived from z**.

Nothing structural consults a `Level`. Movement never does. Generation, the automap, and the UI do.

### Entity ownership and migration

**Settled.** Entities are partitioned by `Zone` rather than pooled. With flecs this is one
`flecs::world` in which each `Zone` is an entity and its contents are that entity's children;
unloading a `Zone` is serialising and then deleting that subtree, which is what makes a large world
affordable at all. The partition is a subtree, not a separate registry.

**A `Zone`'s contents are its subtree rather than its children.** An entity with a parent of its own,
such as a door facing whose parent is its door, belongs to the `Zone` through that parent, so a query
for the `Zone`'s children finds what is placed directly in it --- which is what occupancy and the view
ask for --- while anything covering the `Zone` entirely walks the subtree. Deletion already reaches all
of it (`addressing.md`, `### Children`).

**Ownership follows current location, not origin.** A monster that chases the player through a
relocator migrates into the destination's store, so the `Zone` it came from can unload without taking
it along. The player is the exception: always in a resident `Zone` by definition, but owned by the
session rather than by that `Zone`, so unloading or saving the `Zone` leaves it out (`addressing.md`,
`### The player`).

**Entity ids are global even though entities are partitioned by `Zone`.** Otherwise migration
renumbers, and every reference to that monster --- a quest, a leash, a summoner --- breaks on the way
through the door. Between loaded zones one flecs world supplies this inherently, and reparenting leaves
the runtime entity untouched, which `test/flecs_test` verifies. Across an unload, a save and a session
it is the entity's persistent id, which no zone is part of (`addressing.md`, `### Ids`).

**A monster's home is data, not ownership.** Origin `Zone` and leash point are fields on the
entity, which is what lets it belong to the crypt while standing in the corridor outside.

What remains genuinely global is therefore not entities at all, but world state: quest flags, game
time, faction standing, RNG streams.

### Residency, simulation, and persistence

**Settled.** *Persistence* is that a `Zone`'s state survives being left; *residency* is that its
data is in memory now. They are different properties, and with a bounded working set the distinction
bites immediately: persistence is load-bearing from the first unload, not deferred until the first
save. Unloading is serialise-and-discard, loading is read-it-back, and a save writes every visited
`Zone` regardless of what happens to be resident.

**Two tiers, for now.**

| Tier | State |
|---|---|
| The player's `Zone` | resident, simulated |
| Everything else | unloaded, frozen, on disk |

The resident set is a *set* whose size is policy, currently one, rather than a structural limit of
one. That distinction is the whole reason to state it: growing the set later should be a policy
change, not a redesign.

Were it to span more than one, "near the player" could only mean relocator-graph distance --- `Zone`s
share no coordinate system, so spatial proximity between them is undefined, and the relocator graph is
the only metric available. It is enumerable precisely because relocators are on entities.

A single resident `Zone` costs nothing that residency was originally wanted for. That concern was
that state should survive being left, and it is delivered by serialisation at the persistence
boundary rather than by keeping every zone in memory. This is also what players expect.

**Elapsed-time advance on re-entry.** A frozen `Zone` is not merely resumed where it stopped.
Effects expressible as a function of elapsed time are advanced on load: corpses age, growth
continues, stores restock.

This is a per-component capability, not a global mode. A component that can state itself as a
function of elapsed time is asked to advance; one that cannot simply freezes. Rot, growth,
restocking, and schedules fall in the first group. Anything needing pathfinding or combat resolution
falls in the second and stays frozen --- which is the line Oblivion's Radiant AI crossed, where
unconstrained off-screen simulation produced outcomes players read as absurd and which was
substantially scaled back before release.

### Naming

**Settled.**

- **`Zone`** is an independently loadable area --- a dungeon, a town, a moor. Taken from the MMO
  lineage, where *zoning* also labels the transition, which is a description for what relocators do.
- **`Layout`** is how a `Zone` is arranged, and the queries over it.
- **`World`** is the set of `Zone`s, which is exactly one `flecs::world` with each `Zone` an entity
  inside it.
- **`Flat`** is a horizontal surface, floor or ceiling. Doom's word for exactly that distinction,
  which survived that lineage acquiring sloped sectors.
- **`Scheduler`** orders actors in abstract units, the driver deciding what advances them.
- **`Level`** is metadata describing a `Zone`, not geometry. `quetzal::dungeon::Level` is the entire
  structure and becomes a `Zone`.
- **`Room`** is a named area within a `Zone`, for the player rather than for anything structural.

## Layout

### Zones, rooms and cells

**Settled.** A `Zone` is what a zone file loads: a floor and a ceiling `Flat`, the walls between
its cells, and the cells between the two flats. A surface zone has one flat, having no ceiling.
A file is one zone whatever it contains.

`Zone` is the more inclusive word for what a `Level` is: a level is the dungeon reading of it, and a
zone is equally a section of an outdoor area, or anything else bounded above and below. That is what
the `stratum_` prefix names.

A zone spans rooms. The rooms, halls and corridors within a dungeon level are labels over parts of
that one zone --- nothing about a room bounds a zone, and a zone is not divided by having several.
Where a zone's parts need unrelated heights, they are separate zones.

**A room is a label.** It is `room_id` on the cells it covers, and an entity with its components.
There is no structural room and nothing structural consults it. `Level` and `Room` are both names
with a referent, and what they refer to is a zone and part of a zone.

**Structure is stored per cell in a discrete `Zone`.** A wall is stored where one exists and nowhere
else, so the interior of an unbroken open area costs nothing and only its boundary is stored. In the
block format a wall exists only where open space meets a solid cell, nothing there being able to
separate two open cells. In the wall format one exists wherever the format put it, as often between
two open cells as against a solid one.

**A door is an attachment with a facing on each wall it is on.** The door specifies whether it is
open, closed, locked or trapped, and whether a mover may cross is what it decides. Each facing is on
the wall of one side and has what is reached from that side, so one door is one entity however many
sides see it.

In the block format a doorway is a cell rather than an edge, so a door there is on the walls the
floor either side has toward that cell --- the cell being a block of wall a mover may stand in, with no
wall of its own toward floor, so from inside it there is nothing to open on the way out.
What it is does not change with the format; only what it hangs on does.

Loading a zone creates it: the zone file's door glyphs and its entities become the door and its door
facings (`discrete_layout_rework.md`).

### Walls

**Settled.** An edge is what a zone file writes as one glyph for both sides, and a wall is one side
of an edge, stored as a bit on the cell it bounds. A wall's id encodes that cell and side, and
everything else about it follows from the grid: its ends, its normal, and the wall on the other side
of its edge. Walls are not linked to one another.

**Each side is separate.** A mover can always determine which wall it can interact with, and the wall
on the other side of the edge is out of its reach until it is on that side.

Everything is per side --- surface treatment, attachments, and a door's facing. No orientation
convention is needed to decide which side a piece of data belongs to, because each wall already faces
one cell. One-way and secret doors need no special case: they are a door whose facings differ, so
nothing is required to be symmetric.

**Settled, and done: `Wall::passable` came off the record.** A wall is there or it is not, and whether a
mover may cross it is decided by what is attached to it. A flag on the geometry is mutable state on level data,
which is the line the entity model is drawn on, and a lever toggling a `bool` inside an array is the
case it was drawn to prevent.

A closed door blocks because it is closed, a secret door because it has not been found, and an
invisible wall because that is what it is: an attachment that blocks and renders as nothing.

The predicate is the caller's to supply --- the layout asks rather than looks, staying ignorant of
entities --- so `move`, `neighbors` and `visible` each take one and `World`, which has the layout and
the entities both, binds it for the zone. Passed rather than held because the answer depends on who is
crossing as much as on the wall, which is what an actor-specific rule needs.

The layout has one wall question, `crossable`, asked of a step where the entity side's is asked of a wall:
no wall is a way through, a wall is the predicate's to answer for. The grid boundary is not a case of its
own --- floor encloses itself, so a wall stops the step there, and where there is no wall it asserts that
the cell beyond is on the grid.

Sight is still the case that makes it more than one question: a portcullis passes sight and not a
mover, a curtain the reverse. One predicate suffices today because nothing yet is one and not the
other.

- **An opening is the absence of a wall,** not a flag on one. Non-rectangular rooms have no
  predefined wall slots to flag, so absence has to mean absence.
- **A wall with no floor behind it is a border wall** --- nothing is on the other side of its edge.
  Since the zone's extent is finite and bounded by solid, this is also effectively the map border;
  the two coincide rather than being alternatives.
- **A minable block is a destructible container entity.** It occupies its cell, blocks while it is
  there, and drops its contents when destroyed. Mining destroys the entity; the layout is not
  touched, so no wall is created, removed, or renumbered and every attachment id stays where it
  was.
- **Nothing has to be reconstructed.** The faces of a minable block belong to the block rather than
  to a boundary in the layout, so the neighbours of a mined-out block already have their own and
  are simply exposed. That is also where a mineral vein lives, authored or generated per face.
- **Minability is therefore not a property of the geometry at all.** It is whether a cell has one
  of these, and an indestructible one is the same entity refusing to be destroyed.
- The cost to watch is count: a large digging volume is a large number of entities, which is the
  same argument that keeps walls out of the entity store. A layout's size stays a direct function
  of how much digging is permitted.

### Something reached from more than one side

**Settled.** A block is reached from every face that is exposed: a door from either side, a minable
block on a corner from two. What differs by side and what does not are separated rather than
duplicated.

- **One entity owns the shared fact.** Open, closed and locked for a door; contents and
  destructibility for a block. One place, so there is nothing to keep in agreement.
- **Per-side entities own what genuinely differs,** and reference that one. A door that swings
  opens outward on one side and inward on the other while being open on both, which is per-side
  appearance over one shared state. A corner block is two faces and two veins over one block that
  owns the ore and is destroyed once.

Two walls referring to one entity is not how it is done, and neither are two walls that mirror
each other: mirroring puts the same fact in two places and needs something to reconcile them when
they disagree. A side is a child of the entity it belongs to, and that entity links to each of its
sides.

A wall never consults stored cell data, which would make cells a second kind of attachment
target. The block entity is that data, and being an entity it is referenced directly rather than
found by position.

**A face earns its own entity only when something must target it.** A lever attached to it, or a
face that takes damage of its own. Otherwise per-face data on the block records the same information
for a fraction of the count, which matters against a large digging volume. A lever needs no face
entity either: it attaches to that side's wall and references the door it operates, so which side it
is on is implied by what it is attached to.

**Settled: a wall is a record, not an entity.** The count decides it: a wall per cell edge puts
thousands in a zone, and since unloading a zone serialises its subtree, every save would include them.
A wall with components or with something attached is the exception the count allows, an entity its
zone finds by wall id, sparse because what is written on walls runs to hundreds rather than thousands.

What targets a wall is a link both ways: a door facing, or whatever else is attached to the wall, links
to the wall's entity, which the zone finds by wall id, and the wall's entity links back to it. Nothing
is found by searching.

Everything on a wall or a flat is an attachment --- an alcove, a basin, a door, a button, a torch --- so
one mechanism covers all of them and a wall stays a bit on the cell it bounds.

This depends on wall ids being stable. A wall id encodes its cell and side, and a minable cell is a
phantom cell carved at generation, so mining reveals geometry rather than creating or destroying walls
and nothing renumbers. Anything that changed the grid a wall id encodes against would silently
retarget every link.

Containment is the same mechanism: an item in an alcove is attached to the alcove, `contained_in`,
the target being an entity rather than a wall or a flat cell (`addressing.md`, `### Entities`).

### Flats

**Settled.** A zone is bounded by two `Flat`s, one below and one above, each a height field over the
whole zone rather than a grid of per-sample surfaces. A surface zone has the floor alone. Floor
and ceiling differ by role and
nothing else --- stalactites and stalagmites are one mechanism inverted, which is a field rather than
a type.

A constant height is the ordinary case and not a special one. Where a zone's parts need unrelated
heights, they are separate zones: that is what dividing into zones is for, rather than a single flat
being asked to describe a discontinuity.

The name is Doom's, for exactly this distinction: wall surfaces were textures, and the horizontal
surfaces of a sector were flats. It survived that lineage acquiring sloped sectors, so it reads as
structural rather than descriptive, which is what is wanted here.

A `Flat` has:

- **A height field, not a single plane** --- sloped floors, arched ceilings, steps. It is a constant
  until something is sampled into it, and the constant applies everywhere, so the ordinary case
  costs one value and no table.
- **A sampling grid of its own, and where within it the surface is absent.** A pit, a shaft, and a
  hole dug through the floor are all somewhere the surface is absent rather than somewhere it is
  low, so a sample is an optional height rather than a height with a reserved value. The grid is
  the surface's own: a height map may be finer than the cells without anything else noticing.
- **Its role,** floor or ceiling, explicit rather than inferred from the normal. Deriving
  walkable-from-blocking by normal puts a "how steep is too steep" threshold inside the simulation,
  which is geometry the layout layer is meant to stay out of, and thresholds like that get fudged
  forever after. The normal stays available to the view.
- **`height`,** which is the height field itself. A `Wall`'s height is determined by the floor and
  ceiling it spans rather than being stated separately.
- **Attachments,** any number, each at its own position on the surface. Not written: an attachment
  is one mechanism over walls and flats alike, and neither half of it exists yet.

What is below a point is determined by asking what occupies that point in the `Zone`'s single
coordinate system.

### The three grids

**Settled.** A block crawler makes these look like one concept because all three are the same
number. They are independent:

| Grid | Purpose | Exists in continuous? |
|---|---|---|
| Surface sampling | height map and material resolution | yes |
| Movement quantization | the step size a discrete `Zone` rounds to | no |
| Spatial index | bucket size for proximity queries | yes |

Pulling them apart is what lets a continuous mover reuse the same layout. The height map may be
finer than the movement grid without anything else noticing, and the first two are apart already:
a `Flat` keeps a `Grid` of its own for sampling, which the layout's movement grid does not touch.

## Placement

**Settled.** An entity's placement is a position and a facing, uniformly, in both layout models. The
position is 3D, since a `Zone` is.

A placement is meaningful only within its own `Zone`, coordinate systems being unshared. Since
entities are stored per `Zone`, which `Zone` a placement belongs to is implied by which store the
entity is in, rather than being a field on the placement.

A discrete layout does not accumulate: it recomputes the position from integer cell coordinates on
every move, so the value is regenerated rather than added to and error never builds up. This is what
makes a shared representation safe under principle 6.

Rules never decompose a placement --- they hand it back to `Zone`. That is what makes a cached
location handle addable later as an optimisation without touching a single rule.

**Attachment.** A placement may be *derived* rather than absolute: a `Wall` or a `Flat`, plus a
position on that surface, resolved to a point by `Zone::position()` using the surface's geometry. A
basin stays on its wall whatever angle the wall is at. A `Wall` or `Flat` may have any number of
attachments --- a wall's position is along-and-up, a flat's is xy, the same shape read differently by
role.

**Extent** is a separate component: how much space the attachment occupies. A pressure plate has one, so
does a boulder, so does a monster. Only the values differ. This removes "spans more than one cell" as
a separate kind of entity --- a trigger noticing a mover standing half on it works the same way
collision does.

## The Layout interface

A `Zone` is a place; its `Layout` is how that place is arranged and what can be asked of it.

**Settled: the layouts split on how movement is resolved, and on nothing else.** A class is earned
where two cases differ in behaviour rather than in data.

- **`Layout`** is the interface every layout satisfies: where a mover ends up, what it faces, what
  is adjacent, what is visible, and how far one action of movement travels.
- **`DiscreteLayout`** is movement quantized to a grid of cells. It is the whole of the discrete
  side: nothing lies between it and `Layout`.
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

`quetzal::stratum_sim::DiscreteLayout` has no provision for continuous movement, which is
`ContinuousLayout`'s, and none for a wall spanning more than one cell edge.

**Cells and walls belong to the discrete half.** A cell is a coordinate on a grid, and one wall lies
on one cell edge so that the two run end for end. That is what lets a partner be found by exact
match, and it is a property of the grid rather than of geometry in general --- arbitrary polygons put
one area's single edge against a neighbour's several, which no exact match pairs. A continuous
layout resolves the same questions of its geometry and shares none of this machinery.

**A `GridPosition` is a position, and a displacement is a discrete vector.** The two are separate
types: a position moves only by adding a vector to it, subtracting two gives the vector between
them, and a grid's extent is a vector because a count of cells along each axis is a displacement.
A query about what adjoins a cell takes that cell and a direction, never two cells, so the
neighbour need not exist. `Grid::contains` reports whether a position is on the grid at all and is
asked only in assertions: the loader walls every boundary cell, so a step off the grid cannot be
taken. `decision_rationale.md` argues why the components stay signed.

**Settled**, and written: `Layout.hpp` is the interface, and what it must not contain is what it
does not contain --- no cells, no walls, and no suggestion that any of them exist.

```cpp
    //--------------------------------------------------------------------------
    class Layout
    {
    public:

        virtual ~Layout() = default;

        // Where the mover ended up, and what stopped it if anything did. A discrete layout
        // quantizes and a continuous one integrates; both may return short of the requested
        // distance, or unmoved. Rounding is the layout's business, not the caller's
        virtual MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance,
                                const crossable_type& crossable_wall, const blocking_type& blocking) const = 0;

        virtual placement_type turn(const placement_type& placement, Rotation rotation) const = 0;
        virtual placement_type face(const placement_type& placement, const vector_type& direction) const = 0;

        virtual bool same_location(const placement_type& a, const placement_type& b) const = 0;

        // Adjacency for AI. Both models express it as a discrete graph -- cells, or navmesh nodes
        virtual placements_type neighbors(const placement_type& placement, const crossable_type& crossable_wall) const = 0;

        virtual bool visible(const placement_type& from, const placement_type& to, const crossable_type& crossable_wall) const = 0;

        virtual value_type step_distance() const = 0;
    };
```

Two departures from the shape sketched before it was written. `position(placement)` is absent: a
placement is a `DirectedPoint` and has its own point, so there is nothing for the layout to
resolve. And `turn` divided in two, `turn` relative and `face` absolute, so that the quantization
stays with the layout in both cases rather than a host storing a rotation.

`MoveResult` is a placement and what stopped the mover, the wall or the entity, neither set meaning
nothing did. It reports "the door is locked" without the rule having to ask a second question.

Three distinctions this shape collapses:

**Relative and absolute movement are one call.** Relative is passing the entity's own facing as the
direction; absolute is passing a direction the view chose, leaving the entity's facing untouched. A
top-down view maps west to a direction; a roguelike mover therefore does not rotate when it steps
west, which turn-then-step would have caused.

**Direction is a vector, not an axis delta.** No magnitude, so granularity stays with the layout, and
hex tilings and angled walls work --- the layout snaps to whatever directions it owns. Vectors are
permitted at the *command* boundary, where a view genuinely has an absolute direction. A rule passes
one through to `Zone` and never computes with it.

**Blocked is not binary.** `move` returns where you ended up, not a success flag. A discrete layout
returns the next cell or where you started; a continuous one returns wherever the collision left
you, including partway or slid along a wall.

**Blocking has two sources:** level topology, and blocking entities. A pushed boulder blocks the way
a monster standing in a doorway does, which is what stops the two layout models from diverging over
pushable blocks.

**Settled: the two compose on `World`, not on `Layout`.** `Layout` is geometry and topology, and
giving it a view of the entities to consult is the one change that would make it something else. Nor
does the rule compose them, which would have it re-walking a path the layout had just walked.
`World` owns the layouts and the entities both, so it is the one place that can resolve it without
either cost, and movement is asked of it rather than of the `Layout` underneath.

What the two sources must not become is a sequence. Continuous movement is one solve: a blocker
partway along a swept path changes where the slide against the wall happens, so resolving topology
first and occupancy afterwards gives results that are wrong rather than approximate --- sliding along
a wall into an entity, or stopping at an entity where the wall geometry does not allow it. The
discrete case tolerates the sequence because a cell is atomic, which is exactly why it is not the
case to design against.

## Entities and level data

**Settled.** The line is **identity and mutable state**, not appearance versus object, and not
interactive versus scenery.

**Level data** is topology, height maps, and surface appearance --- materials, patterns, the scorch
mark nobody touches. There are orders of magnitude more surface samples than attachments, and
none of them have state worth an id.

**Entities** are everything with identity or physical presence, from authoring time. A basin you can
drink from, an alcove that is itself a container, a pushable boulder, and a fixed statue jutting out
far enough to change where you can walk --- the statue has no behaviour but it has presence, and
anything that changes where you can walk must be findable and removable.

Nothing is promoted from level data to entity at runtime. Promotion means two representations of the
same data, a conversion step, and a class of bugs where something was referenced in its old form.

Sparse attachments --- alcoves, basins, plates, statues --- are dozens per zone, not thousands, so entity
representation costs nothing. The scale argument only ever applied to per-sample appearance.

### Relationships

**Settled: flecs.** Level content produced two relationships in its first concrete description:

- **attached-to** --- a wall or a flat, plus an offset across it.
- **contained-in** --- an alcove *is* a container.

Both need to be traversable from either end. Mining through a wall containing an alcove has to find
that alcove and decide what happens to its contents, which means the wall must be able to find what
is attached to it, not only the reverse.

`game_system_principles.md` scheduled this choice for Phase 4 --- flecs if relationships prove
central, EnTT if the conventional minimum suffices. They proved central before Phase 1, three times
over: attachment, containment, and `Zone` partitioning. EnTT models none of them natively;
attachment there would be ids in components plus reverse indexes maintained by hand, which is what
the hand-rolled storage already did.

`test/flecs_test` verified the fit empirically rather than by argument: `Zone`s as entities with
contents as children, migration by reparenting with the entity id unchanged, unload as a subtree
delete, per-`Zone` serialisation, and whole-world JSON round trip preserving `ChildOf` and
multi-member components.

A `Parent` hierarchy is the exception, and it is what a door's facings use: flecs writes one and
cannot read it back, `from_json` aborting on a world that holds one (Flecs 4.1.6). So a zone is written
out in the zone file format, where a child states its own parent, rather than through flecs JSON
(`addressing_plan.md`, `## Flecs`).

Attachment and containment are not relationship pairs. A pair per target puts each distinct target in
a table of its own, which at a world's scale splits storage into a table per wall or cell with
something on it, so an attachment is a component naming its target, and the target has a component
listing what is attached to it (`decision_rationale.md`, "Attachments and walls link both ways"). That
is ids in components with a reverse index kept by hand, which was EnTT's shortcoming above; it is a
choice made for the count, and flecs still supplies the zones as subtrees, the hierarchy, prefabs,
queries and reflection.

Two obligations come with it. Component reflection has to be registered member by member or `to_json`
emits names without values. And iteration is deterministic but **not ordered** --- entities are visited
grouped by archetype, so anything wanting sorted output, such as Phase 5's checked-in reference
files, must sort explicitly rather than assume iteration provides it.

### Behaviour

**Open.** Pressure plates, levers, timed doors, and pits are what a dungeon crawler is made of, not
advanced content. Trigger behaviour is precisely the case that turns a data format into a bad
programming language, so the scripting escape hatch is due around Phase 4 rather than
open-endedly later.

**Settled.** What happens when a statue is attacked, a wall is mined, or an alcove's wall is
destroyed is script-determined, over a default the engine supplies, with specific entities
overriding that default. So the alcove question is not a structural one --- the structure only has to
guarantee that the wall can find what is attached to it, and script decides the outcome.

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

- **What creates a door, and what places a room.** `load_layout` returns a layout and nothing else,
  so nothing in it can place an entity or a named grouping of cells. Both formats define them and
  neither survives the read. A door no longer does: the loader reports the walls one attaches to and
  a caller with a world builds it, which is the shape the rest will take.
- **How a height field is stored and sampled,** given a constant one is the ordinary case and needs
  to cost nothing.
- The scripting language --- above, and earlier than the phased plan assumes.
