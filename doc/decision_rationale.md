# Decision Rationale

Date: 2026-08-29, revised 2026-09-02

Why the significant decisions went the way they did, and what was rejected, together with the
background a pending change rests on. The working documents state the model; this one holds the
argument behind it, and only where the decision or design element is big enough to be worth
revisiting. `sim_zone_model.md` and `game_system_reference.md` say what is true, without repeating
any of this.

## Layouts split on discrete versus continuous, and on nothing else

`Layout` is the interface, and `DiscreteLayout` and `ContinuousLayout` are the two classes beneath
it with nothing in between. Geometry is not a second axis of classes.

**Flat is the default, not a restriction.** A discrete layout with no height data has a floor at a
constant height, and a constant height is a `Flat` with fewer of its features used --- the same
structure a sampled height field lives in, as a block's faces and the two sides of an edge are both
walls. A level with height and flat attachments and one with neither load the same
class from different data, and game styles differ in which data they load. That is why neither name
says `Flat`: it is implied for every discrete layout, and for a continuous one the level is ordinary
3D models, where being level is no more a category than any other shape.

**Two classes differing in no behaviour are one class.** An intermediate splitting the grid off from
the geometry was written and taken back out. The only candidate difference was the flats, and
everything else --- movement, walls, blocking, the grid --- was the same on both sides, so the pair
would have been one body of code twice over with the flats loaded or not. Nothing outside the
layout reads a flat as it is: `cell_position` takes its height from `Grid::origin`, and the
only reads anywhere are the two test cases asserting the loader created them.

**Continuous is different in kind.** Movement that integrates cannot substitute for movement that
quantizes: a swept path collides partway rather than cell by cell, partners cannot be found by
exact edge match, and topology and occupancy have to be solved together rather than in sequence.
That is behaviour rather than data, so `ContinuousLayout` is a class of its own.

## A wall is a record, not an entity

A wall is a bit on the cell it bounds, addressed by an `id_type` that encodes the cell and the side.
Walls are not flecs entities.

The argument for making them entities was that attachments, doors and mining all need to target a
wall, and a flecs relationship can only target an entity. That is real, and it is addressed by
addressing a wall by id instead --- see below.

The count is what decides it. A wall per cell edge puts thousands in a zone, and unloading a zone
serialises its subtree, so walls as entities would take all of them into every save. A bare wall
needs no identity in the ECS sense: it has no behaviour, no lifetime of its own, and nothing to
relate to that an id cannot express.

A wall with components or with something attached to it is the exception the count allows: it has an
entity, which its zone finds by wall id. Those run to what is written on walls, hundreds rather than
thousands.

## Attachments and walls link both ways

An entity attached to a wall links to the wall's entity, and the wall's entity links back to it. A
door is on two walls through its children, a door facing attached to each, and the door links to its door
facings. An entity attached to a flat cell and that cell's entity link the same way. Nothing is found
by searching.

The alternative was a query over the zone's attachments filtered by wall id, which `crossable` would
run per step per actor per turn and per cell along every sight line. With the wall's entity found by
id, the question is a lookup and a link.

The link is a component with the target as data, `Attached` on the attached entity and
`AttachedEntities` on the target, rather than a Flecs pair such as `(LyingOn, cell)`. Flecs stores
each distinct pair in an archetype table of its own, so objects on thousands of different cells split
storage into thousands of small tables, and a world several orders of magnitude past the test levels
multiplies that. A pair marked `DontFragment` avoids the split at a cost to iteration, and a
component with the target and no list on the target leaves finding what is on a cell a scan of every
object. Reverse links make both directions a lookup, with the zone's location map reaching a wall's
or a cell's entity from its id. The cost is keeping both ends in sync when an entity moves or is
destroyed (`addressing_plan.md`).

It depends on wall ids being stable. An id encodes the cell and the side, so it is stable with no store
behind it; what must not change is the grid it encodes against, or every link by wall id silently
retargets. That constraint is what shaped the mining decision below.

## Where an entity is and what its parent is are file components

A zone file lists its entities once, and what an entity is attached to and its parent are its
`attached` and `parent` file components (`addressing.md`, `### Entities`, in parts 1 and 2).

**An attachment is not a kind of thing.** There are places, which are geometry, and entities, which have
identity; an attachment is the link from one to the other, and in the world it is already a component
on the entity. A list of attachments beside the entities made the link a record of its own, and a door,
being on two walls, needed two of them --- which then needed file components of their own for what each side
has, ids so that a keyhole could be attached to one, and an entity with a single attachment treated
differently from one with several.

**A door's sides are entities.** In the world a door facing is a child of its door with components of
its own, so the file writes it as one: a child, with a `parent` and its own `attached`. Every entity then
has at most one attachment, and nothing about attachment has a second case.

**Children are derived rather than listed.** A parent listing its children would state each link twice,
once on each end, and the two could disagree. The child's `parent` states it once, and Flecs keeps each
parent's children from those once the zone loads, as a place's list of what is attached to it is built
from `attached`.

**Relationships are optional.** Most attachments need nothing beyond their target: a door facing on its
wall, a chest on a floor cell. Requiring a relationship gave every one of them a name that decided
nothing, and pushed the vocabulary toward names such as `fills`, which is wrong for a door that rarely
fills its opening. A relationship is written only where it distinguishes one way of being attached from
another, as `contained_in` does.

**How the file is arranged costs nothing at run time.** A file is read once when its zone loads, and
the same links and location maps are built whichever way it is arranged. What is on a wall is a lookup
because an entity is attached by address, which is the key to the zone's map of places, not because the
file is grouped by place. Zone files are written by generators and tools, so grouping them by place
for a reader would buy nothing either.

## An entity has three ids

A local id in a template, a persistent id in a world, and a runtime entity while loaded
(`addressing.md`, `### Ids`).

**The persistent id cannot include the zone.** A key carried from one zone to another, or an item
dropped in a zone that persists, would change id on the way or keep referring to the zone it left.

**It is not the Flecs id.** Flecs recycles ids, so the same entity is a different runtime entity once
its zone has been unloaded and loaded again, and nothing written can depend on one. Flecs can create
an entity with a chosen id, but it divides the 64 bits between an index and a generation count, and its
entity index is paged over the index range, so ids spread across a whole game's history would allocate
pages for ids that no longer exist.

**A template has local ids rather than persistent ones.** A template is made into a zone once, or into
several zones: the same file made twice gives two sets of entities, which need
different ids, so making a zone allocates them either way. Local ids keep a generated file the same
for the same parameters, which is what the generator suites compare.

## Persistent ids are a 64-bit count, not UUIDs

A UUID's advantage is that it can be created with no coordination. Templates with local ids remove
the one place that needed it, a generator run as a tool with no world to ask, and once a world exists
it is the one place ids are made, so a count saved with the world suffices.

A UUID is 16 bytes where a count is 8, in the component on every persistent entity, in every reference
that may reach another zone, and in the world index, which has an entry for every entity something
refers to, loaded or not, and whose keys hash more cheaply at 8. None of it is on a path a loaded zone
runs every turn, since that uses runtime entities, but the index grows with the world.

Multiplayer does not reverse it. One server allocating is the same count; several servers divide the
bits between which one allocated and its own count, as Twitter's Snowflake ids and Bethesda's form
ids do. Only creation with no coordination at all needs UUIDs, and then as the alias throughout or at
the boundary between worlds alone, which the rules in `addressing.md` keep a change to the allocator
and the alias.

## The world index has only the entities something refers to

The index is there so that a reference can find the zone its entity is in when that zone is not loaded.
An entity that nothing refers to is never looked up that way. A dropped object is indexed in its zone,
by the place it is attached to, and is accessible while the actor is in that zone; from another zone the
actor moves to, it is not accessible at all. Indexing every entity in every zone ever made would grow
with the world for lookups nobody makes. An entity enters the index when
the first reference to it is made, so a reference finds it even after the entity has moved to another
zone.

## Topology and occupancy compose on `World`

`World::move` resolves where an actor ends up, composing the layout's geometry with whatever is
occupying the cell ahead. `Layout` takes a predicate and stays ignorant of entities.

Two other homes were considered. Putting it on `Layout` --- which an earlier revision of
`sim_zone_model.md` had, with `entities_within` on the interface --- means giving the layout a view of
the entities, which is the one thing that would stop it being geometry and topology. Putting it in
the rule means the rule re-walks a path the layout has just walked. `World` holds the layouts and
the entities both, so it is the only place that pays neither cost.

What forces a single answer rather than a sequence is continuous movement. A blocker partway along a
swept path changes where the slide against a wall happens, so resolving topology first and occupancy
afterwards gives results that are wrong rather than approximate --- sliding along a wall into an
entity, or stopping at an entity where the wall geometry does not allow it. A discrete zone would
tolerate the sequence, a cell being atomic, which is exactly why it is not the case to design
against.

It also settles that a pushed boulder blocks the way a monster standing in a doorway does, which is
what would otherwise diverge between the two layout models.

## Actors are found through a zone actor index

An actor keeps its `placement_type`, and each zone has an index of its actors by place, derived from the
placements (`addressing.md`, `### Actors`).

**Not a scan.** Checking every placed entity in the zone costs more as the zone fills, and `World::move`
asks what is at a cell for every cell it moves into.

**Not an attachment.** Attaching an actor to the cell it stands on would make what is at a cell one
lookup for actors as for everything else. But every move would detach and attach, and could make or
delete a cell's entity. An actor's place would be stated twice, in its placement and its attachment,
which could disagree. A `ContinuousLayout` has no cells to attach to. And attachment would mean two
things: a torch stays on its wall, while an actor is only standing somewhere this turn.

**An index.** A move costs two map updates and no entities. The placement remains the one record of
where an actor is, the index being rebuilt from it, and each kind of layout indexes in its own way.

## `passable` comes off the record

A wall is there or it is not. Whether an actor may cross it is decided by what is attached to it, not a field on the
geometry: `crossable` asks each attached entity whether its properties let the actor through, and knows nothing about
doors or concealment. Nothing on the wall or the cell marks it crossable, so no state is kept twice.

A flag on the record is mutable state on level data, which is the line the entity model is drawn on:
level data is topology and appearance, entities are identity and mutable state. A lever toggling a
`bool` inside the layout's arrays is precisely what that line was drawn to prevent.

The consequence to note is that `Layout::blocked` feeds `move`, `neighbors` *and* `visible`. Once
openness lives on an attachment, none of the three can decide alone and all three compose on `World`
the way movement already composes occupancy. Sight through an open door is the case that makes it
three call sites rather than one.

## A staircase is a block

A stair cell is a block with a wall on each side, and the staircase entity is attached to the wall on its
open side (`addressing.md`, `### Places`). A staircase attached to the floor had to be given an
orientation of its own and kept consistent with the walls around it.

**Its direction is its wall's.** The normal of the wall on the open side is the direction of travel from
the staircase into the room, so there is no orientation to write, choose or check.

**Each side is a wall.** The sides the staircase does not open onto are walls like any other, so each can
look different.

**Passage is a door's.** An actor goes through the open side as through an open door, so moving into a
staircase needs nothing beyond what `crossable` already asks.

## A minable block is a destructible container entity

A minable block occupies its cell as an entity, blocks while it is there, and drops its contents
when destroyed.

Mining then destroys an entity and does not touch the layout. No wall is created, removed or
renumbered, so the id stability the attachment decision depends on is preserved by construction
rather than by discipline.

The faces of a block belong to the block rather than to a boundary in the layout, so the neighbours
of a mined-out block already have their own and are simply exposed --- nothing has to be
reconstructed at the moment of mining. That is also where a mineral vein lives, authored or
generated per face ahead of anything being dug.

The cost is count, and it is the same argument that keeps walls out of the entity store: a large
digging volume is a large number of entities. A layout's size stays a direct function of how much
digging is permitted.

## Walls decide collision alone, as bits on the cells

**Collision reads the walls and nothing else.** `Layout::move` steps cell to cell calling
`wall_toward`, and asks whether the wall there may be crossed. `blocked`, `neighbors` and `visible`
all route through the same call, and `World::move` adds the occupancy predicate above it. None of
that path touches a room. Rooms are metadata, and movement, sight and adjacency are untouched by
them.

**A wall is a bit on the cell it bounds.** Every wall comes from one glyph and its two neighbors, so
nothing has to be grouped, traced or linked to be built, and `wall_toward` is a bit test rather than
a search. A wall's id encodes the cell and the side. Everything a wall record once stored follows
from that address: whether the wall is there, the wall on the other side of its edge, its two ends,
and its id. `discrete_layout_plan.md` has the table.

**Nothing walks an outline.** Outlines existed to give each area a boundary to trace, and the loader
built them for nothing else to read. A room's cells are one sweep over `room_id`, and a polygon for a
caller that needs one is traced from them.

## A grid position is a position and a discrete vector is a displacement

Background for splitting the two types and for `math::DiscreteVector`.

**One type was serving three roles.** `Cell` was a coordinate, a step (`Cell {-1, 0}` in `move` and
`neighbors`), and a count (`Grid::extent`). Only the first is a position, and the confusion showed:
a cell was moved by writing out `{cell.i + step.i, cell.j + step.j}` at four sites, and the
extent-as-position let a count be compared against a coordinate without anything objecting.

**The split is by type rather than by discipline.** `GridPosition` holds a position and is moved
only by adding a `discrete_vector_type` to it; subtracting two yields the displacement between
them; adding two does not compile. `Grid::extent` is a displacement, which is what a count of cells
along each axis is. Nothing now spells cell arithmetic out componentwise. The name is
`GridPosition` rather than `Cell` because a cell is the square, and what the type holds is where
that square is.

**Negative components remain expressible and that is deliberate.** A position should never hold
one, but `cell_at` maps any point to a cell, and a step off the edge has to be representable long
enough to be rejected. Making the components unsigned would turn both into wraparound.

**`math::DiscreteVector<T, N>` rather than a type local to the layout.** `dungen` already had a
2D integer vector under the name `Vector`, aliased as its `position_type`, `dimensions_type` and
`vector_type`, and a second copy in `stratum_sim` would have been the same class twice. Templating
it on a signed integral type and a dimension, alongside `math::Vector`, gives one of them. Signed
because a displacement runs both ways along every axis.

**Quantizing is a choice of axis, not a rounding.** `direction` takes the largest component and
returns a unit step along that axis alone; reducing each component instead turns a direction that
was nearly east into a diagonal nobody asked for. Its continuous form requires the source and
result to have the same dimension, so a caller taking a step across a flat grid from a direction
through space reduces to the plane first --- `stratum_sim::grid_step` does that with
`geometry::MinProjection`, keeping the dropped height in the layer that knows the layout is flat.

## The boundary holds an actor in, and the bounds test is an assertion

`Grid::contains` is called from assertions only. Nothing branches on it.

**The loader builds the guarantee.** `stratum_format::has_wall` treats beyond the grid as solid, so
every cell on the border of the grid has a wall on its border side; in the wall notation, where the
rows write that wall, the reader rejects a file that leaves one out.

**So a runtime bounds test would be dead code.** `move`, `neighbors`, `visible` and `Interact`
briefly had one; it never fired on a loaded layout, and the four tests written to exercise it
had to build a grid with no walls at all --- a layout the loader cannot produce. Asserting instead
states the invariant where it is relied on and costs nothing in a release build.

**The queries take a direction, not a second cell.** `wall_toward(position, direction)` replaced
`wall_between(cell_a, cell_b)`, which only ever used the difference of its two arguments. The old
signature let a caller name a cell that need not exist purely to express a direction, and made the
adjacency of the two an assertion instead of a property of the type.
