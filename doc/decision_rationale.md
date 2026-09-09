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
structure a sampled height field lives in, as a block's faces and a thin wall's two sides are both
`Wall` records. A level with height and flat attachments and one with neither load the same
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

Walls are held in the layout's arrays and addressed by `id_type`. They are not flecs entities.

The argument for making them entities was that attachments, doors and mining all need to target a
wall, and a flecs relationship can only target an entity. That is real, and it is addressed by
addressing a wall by id instead --- see below.

The count is what decides it. A wall per cell edge puts thousands in a zone, and unloading a zone
serialises its subtree, so walls as entities would take all of them into every save. Nothing about
a wall needs identity in the ECS sense: it has no behaviour, no lifetime of its own, and nothing to
relate to that an id cannot express.

## An attachment holds an id, and the reverse is a query

An attachment --- anything on a wall or a flat, from an alcove to a door to a torch --- holds the id of
what it is attached to. Going the other way is a query over the zone's attachments filtered by that
id.

The alternative was a flecs relationship, which would have required walls to be entities and lost
the argument above. The scan is affordable because the two counts are nothing alike: attachments run
to hundreds per level where walls run to thousands, and the reverse lookup happens on a player
action rather than in a loop, so it needs no index behind it.

It depends on those ids being stable. Nothing may remove a record from the arrays or renumber one,
or every attachment silently retargets. That constraint is what shaped the mining decision below.

## Topology and occupancy compose on `World`

`World::move` resolves where a mover ends up, composing the layout's geometry with whatever is
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

## `passable` comes off the record

A wall is there or it is not. Whether a mover may cross it is decided by what is attached to it, not a field on the
geometry.

A flag on the record is mutable state on level data, which is the line the entity model is drawn on:
level data is topology and appearance, entities are identity and mutable state. A lever toggling a
`bool` inside the layout's arrays is precisely what that line was drawn to prevent.

The consequence to note is that `Layout::blocked` feeds `move`, `neighbors` *and* `visible`. Once
openness lives on an attachment, none of the three can decide alone and all three compose on `World`
the way movement already composes occupancy. Sight through an open door is the case that makes it
three call sites rather than one.

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

## Walls decide collision alone, and the loops decide nothing

Background for removing the outlines and indexing the walls.

**Collision reads the wall store and nothing else.** `Layout::move` steps cell to cell calling
`wall_toward`, which searches for the wall lying on that cell edge and then asks whether it may be
crossed. `blocked`, `neighbors` and `visible` all route through the same call, and `World::move`
adds the occupancy predicate above it. None of that path touches the room store, the cell-to-room
table, or anything a room says. Rooms are metadata, and movement, sight and adjacency are untouched
by them.

**The search is the cost.** `wall_toward` scans every wall, so a cell step is O(walls) and a
sight line --- which walks the grid's extent in cells --- is O(cells x walls). `think` runs one
per pursuing actor per turn.

**A flat wall id per cell edge replaces the scan**, in the shape `catagen::Level` already stores:
`(nx + 1)` by `ny` vertical and `nx` by `(ny + 1)` horizontal. One lookup, no hashing, built once
beside the cell table. One wall lies on one cell edge, so an edge holds one id per side. For a 48 by
32 level that is about 3,100 ids, some 25KB.
`load_layout::build` already constructs an equivalent index keyed by directed edge and discards it
as a local when it returns.

**Nothing needs to walk a loop.** Outline traversal has three callers and each is replaceable:

- `connect_walls` searches the walls of two areas for opposed pairs, at O(n x m). With the edge
  index a partner is one lookup --- take a wall's own directed edge and look up its reverse. That is
  already how `load_layout` pairs passages.
- `load_layout` walked an outline only to line its traced ring up with wall ids. It need not:
  `create_walls` pushes walls contiguously, so the wall for outline vertex *i* is `id0 + i`, and the
  loader was asking `Layout` for ids it holds the means to compute.
- Containment by point is a room query, and a `Room` holds its own outline rather than walking one.

What local connectivity is still for is the two things that are one hop rather than a walk: a
wall's far endpoint, and `opposed`, which compares one wall's endpoints against another's.

**A wall stores one vertex and takes its far end from `next`.** Adjacent walls share that vertex, so
holding both ends on each would store it twice and let the two drift apart. `wall_toward` and
`opposed` reading `wall(next_id).position` is the link doing what it is for, not two jobs to be
separated. `prev_id` is unread today and is the backward half of the same connectivity.

The link stays. What goes is the enumeration over it --- a start id per outline, and the walks that
follow one around.

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

## The boundary holds a mover in, and the bounds test is an assertion

`Grid::contains` is called from assertions only. Nothing branches on it.

**The loader builds the guarantee.** `trace_outlines` treats out-of-grid as outside the component,
so every edge cell is given a an impassable border wall. Passages are set passable
only between two components.

**So a runtime bounds test would be dead code.** `move`, `neighbors`, `visible` and `Interact`
briefly had one; it never fired on a loaded layout, and the four tests written to exercise it
had to build a grid with no walls at all --- a layout the loader cannot produce. Asserting instead
states the invariant where it is relied on and costs nothing in a release build.

**The queries take a direction, not a second cell.** `wall_toward(position, direction)` replaced
`wall_between(cell_a, cell_b)`, which only ever used the difference of its two arguments. The old
signature let a caller name a cell that need not exist purely to express a direction, and made the
adjacency of the two an assertion instead of a property of the type.
