# Game System --- Reference

What the system is and how it is put together, as implemented.

Three layers, with no engine in any of them. `quetzal::stratum_sim` simulates,
`quetzal::stratum_view` turns a simulation state into characters, and `stratum_terminal` puts those
characters on a terminal. `stratum_sim` links `quetzal_common` and flecs; `stratum_view` links
`stratum_sim`; the host links `stratum_view`.

The `stratum_` prefix names the layout class the two share: open ground extruded vertically between
one floor and one ceiling. `sim` and `view` are accurate as roles and would claim too much on their
own at `quetzal::`.

| Directory | Target |
| --- | --- |
| `assets/` | shared data, owned by no host |
| `library/quetzal/stratum_sim/` | `quetzal_stratum_sim` |
| `library/quetzal/stratum_view/` | `quetzal_stratum_view` |
| `application/stratum_terminal/` | `stratum_terminal` |
| `application/stratum_godot/` | `stratum_godot` |
| `library/quetzal/godot/` | `quetzal_godot` |
| `tool/dungen/`, `tool/catagen/` | `dungen`, `catagen` --- the generator drivers |
| `test/stratum_sim_test/`, `test/stratum_view_test/`, `test/dungen_test/`, `test/catagen_test/`, `test/flecs_test/` | test executables |

## Simulation

A `World` holds `Zone`s. A zone is one independently loadable coordinate space, and it is 3D. Zones
share no coordinate system and bear no spatial relationship to one another, so a placement means
something only within its own zone --- and which zone that is comes from the entity's parent rather
than from a field on the placement. Each zone has a layout, which is its geometry and topology, and
the entities occupying it.

Geometry travels outward through the layout toward the view, and never inward. A caller asks the
layout where a move ends up; it does not compute a destination and hand it back.

### Types

`value_type` is `float` and `traits_type` is `math::CartesianTraits<value_type, 3>`, giving
`point_type`, `vector_type`, `points_type` and `polygon_type`. `placement_type` is
`geometry::DirectedPoint<traits_type>` --- a position and a facing.

`discrete_vector_type` is `math::DiscreteVector<long, 2>`, a displacement in the grid, exact where
`Vector` is approximate. `GridPosition` is a position and never a displacement: the two are
separate types, a position is moved only by `position + discrete_vector_type`, and `b - a` over two
of them gives the displacement between. Adding two positions does not compile. `Direction` names
the four steps a flat grid can take as `discrete_vector_type` constants --- `East`, `North`, `West`,
`South`.

`point_projection_type` and `vector_projection_type` are `geometry::MinProjection` over
`point_type` and `vector_type`. Default constructed they project along z onto the xy-plane, which
is the only form a flat layout uses; the axis-choosing constructor takes a normal.
`grid_step(vector_type)` composes the vector one with `math::direction` to give the cell step a
direction most nearly asks for.

### Layout

How one zone is arranged and what can be asked of it, as an interface: `move`, `turn`, `face`,
`same_location`, `neighbors`, `visible`, `ahead` and `step_distance`, all pure virtual. Nothing on it mentions a
cell or a wall, and nothing on it implies either exists --- a layout that quantizes movement and one
that integrates it satisfy all of it alike, rounding being the layout's business rather than the
caller's. `Rotation`, `MoveResult` and `obstructed_type` are its vocabulary and live with it.

`move` has a second, non-virtual overload taking no `obstructed_type`, for the layout's own
topology alone. It is non-virtual so that no default argument is declared on a virtual function, where the
default would be taken from the static type; a derived layout brings it back into scope with
`using Layout::move`.

The destructor is public and virtual, since a zone owns its layout through a
`std::unique_ptr<Layout>`. The other special members are protected and defaulted, so a concrete
layout stays a value that can be copied and moved as itself while nothing can slice one through the
base.

`layout_as<L>(layout)` is the way to the concrete class, a checked cast that asserts. A caller
writing one has said it needs more than the interface offers --- the loader, the cell reduction both
views consume, `Session`'s opening placement, `World::entities_at(cell)`, and the two host status
readouts. No rule is among them: `commands.cpp` refers to no cell and depends on no concrete class.

`DiscreteLayout` and `ContinuousLayout` are the two beneath it, and `DiscreteLayout` is the only one
with an implementation. Neither name mentions `Flat`: it is implied for every discrete layout, and a
continuous one takes ordinary 3D models where being level is no more a category than any other
shape. `sim_zone_model.md` sets out the model, `decision_rationale.md` why geometry is not a second
axis of classes, and `layout_plan.md` the order they arrived in.

### ContinuousLayout

Movement that integrates rather than quantizing, through geometry that is ordinary 3D models.
Declared and not written: every member is pure, so nothing constructs one and nothing has to be
defined, and the header exists to state the case rather than leave it inferred from
`DiscreteLayout`, whose name does not mention what it omits.

The comments on it are the content --- what each query would compute, and why the discrete layout
cannot substitute for it. A mover is anywhere in the zone rather than in one cell or the next, so a position
is not regenerated from a cell and error accumulates; collision is a swept path ending partway or
slid along a wall, with topology and occupancy solved together; walls are arbitrary polygons, so one
area's single edge meets a neighbour's several and `partner_id`, `next_id` and `prev_id` do not
apply; proximity needs a spatial index, there being no movement grid; and `value_type` needs
settling first, every vertex being grid derived and exactly representable today.

`stratum_sim_test` compiles the header and checks the two properties a skeleton promises: that it is a
`Layout`, and that it is abstract.

### DiscreteLayout

The static topology of one zone: its walls, its flats, and the rooms defined over them. Held as
plain arrays indexed by `quetzal::id_type` rather than as entities; entities refer into it by id.

Arrays rather than entities because of the count. A wall per cell edge puts thousands in a zone,
and a zone serialises its whole subtree when it unloads, so entities would take all of them into
every save. An attachment --- anything on a wall or a flat, from an alcove to a door to a torch ---
stores the id instead, and the reverse direction is a query filtered by that id rather than a
relationship, which is affordable because attachments run to hundreds where walls run to
thousands. It rests on wall ids being stable, nothing removing records from the arrays.

| Type | |
| --- | --- |
| `Wall` | One side of the barrier on a cell edge. `partner_id`, `next_id`, `prev_id`, `position` (its start vertex, the far end being `next_id`'s position), and a `deleted` flag with the same interface `Flags` gives a halfedge. It settles nothing about crossing: whether a mover may is decided by what is *attached to* the wall, which is an attachment. A `nullid` partner makes it a **border wall**, with nothing behind it, which is also the zone's outer boundary. A wall refers to no area: what a room covers is the room's to decide. |
| `Flat` | A horizontal surface, one floor and one ceiling per zone, their ids held by the layout, differing by `ceiling` and nothing else. A height field rather than a plane: a constant until something is sampled into it, which is what a level with no height data --- every level today --- loads, and it costs one value and no table. `set_sampling` gives it a `Grid` of its own and starts every sample at the constant; `sample` and `set_sample` reach one. A sample is `std::optional<value_type>`, absent meaning the surface is not there --- a pit, a shaft, a hole dug through the floor --- rather than low. `height(position)` is what a caller asks, giving the constant when unsampled, the sample when not, and nothing off the sampling grid. |
| `Room` | A named area within a zone, in `Room.hpp`: an outline of `{position, wall_id, type}` vertices, the cells it covers, the zone it is in, and an entity id for its name, description and appearance. Metadata --- nothing in movement, sight or adjacency consults one. `type` is `Wall`, `Door` or `Open`, `Open` being where the room gives onto a corridor with nothing between them. |
| `GridPosition` | The coordinates of a cell in the grid: `long i`, `long j`. Not a stored record. `operator+=` and `operator+` take a `discrete_vector_type`, and subtracting two gives one. |
| `Grid` | `origin`, `spacing`, `extent` (a `discrete_vector_type`, cell counts being a displacement), `contains(GridPosition)`, and `cell_at`/`cell_position` for the arithmetic both ways. A `Flat` samples over one of its own, so the arithmetic belongs here rather than on the layout, which delegates to it. |
| `Rotation` | `CCW`, `CW`, `Half`. In `Layout.hpp`, being interface vocabulary. |
| `MoveResult` | The placement reached, which may be short of what was asked, and the wall or the entity that stopped it --- `nullid` when nothing did. In `Layout.hpp` with `Rotation`. |

A `Grid` is set at load from the extent the file covers, rather than derived from the wall
geometry afterwards. Cells run from `(0, 0)` to the extent, so a negative component is always off
the grid. `Grid::contains` is the one place that comparison is written, and it is used in
assertions only: the loader guarantees a walled boundary, so a step off the grid cannot arise and
is not a case to be handled at run time.

An opening is the absence of a wall rather than a flag on one, a non-rectangular area having no
predefined wall slots to flag. Where a wall run stops at an opening the boundary folds: it runs
along one side, around the tip, and back along the other, both walls lying on the same outline.
That is what `connect_folds` pairs, and it is why nothing within a thin wall is ever `nullid`.

This is the whole of the discrete side, with no interface between it and `Layout`. A level with no
height data loads a constant floor rather than a different class, so which data a game style
loads is what differs rather than which class is built; `decision_rationale.md` argues why. It has no
provision for continuous movement, which is `ContinuousLayout`'s, and none for a wall spanning more
than one cell edge --- one wall lies on one cell edge, which is what lets a partner be found by exact
match.

Walls are created an outline set at a time with `create_walls`, which returns the wall each outline
starts at. An outline's walls run contiguously from there, so a caller given an outline's
length can derive which walls came from it --- which is why nothing needs to walk the loop to enumerate
them. Walls are joined with `connect`, `disconnect`, `connect_walls` and `connect_folds`, and tested
with `opposed`.

Rooms are built with `create_room`, which takes a `Room` the caller has filled in and stores the
cells it covers. `room`, `room_store` and `room_store_count` read them back; `room_at` locates one
by point or by cell, and `set_cell_room` stores the latter. A cell no room covers gives `nullid`,
which every corridor cell does.

The grid side is `set_grid`, `grid`, `open`, `set_cell_open`, `cell_at` and `cell_position`, and
`wall_toward` and `blocked` read what is attached to it. `move`, `turn`, `face`, `same_location`,
`neighbors`, `visible` and `step_distance` are what it overrides from `Layout`.

`wall_toward(position, direction)` returns the wall on that cell's side of the edge it presents in
that direction, and `blocked(position, direction)` whether a mover stepping that way is stopped.
Both read the cell alone, so the neighbour need not exist and a boundary cell returns the wall
that closes the layout.

`cell_at` is total: it floors any position into a cell without bounds check or clamp, so a position
off the grid maps to a cell off the grid rather than failing. The cell-indexed accessors are the
other way about --- `open`, `room_at(GridPosition)`, `set_cell_open` and `set_cell_room` assert that
the position is on the grid, an off-grid one being the caller's error rather than a question with
an answer.

`cell_position` is the one place the layout reads a flat. It takes x and y from the grid and z from
the floor, sampled at that point, so a cell sits on the ground rather than on the grid's datum. A
layout with no floor yet, and a cell the floor is absent under, both fall back to the grid's own
plane --- the first is a layout under construction and the second is a hole rather than a floor to
walk on, and neither has a ground height to give.

`turn` is relative and `face` is absolute, and the two divide the work of turning: a view with a
relative turn asks `turn` what lies that way and sends the answer as a facing, so the quantization
belongs to the layout in both cases and neither the command nor the host stores a rotation. `face`
snaps a direction as `move` snaps the one it steps in, and leaves the facing alone when asked for
nothing.

`move` snaps the direction it is given to whichever of the layout's own directions was most nearly
asked for, steps cell by cell until an impassable wall stops it, and recomputes the position from
the cell it reached rather than accumulating offsets --- so the position is regenerated on every move
and rounding error never builds up. Rounding is the layout's business, not the
caller's, and blocking is reported as where the mover ended up rather than as a success flag.

A mover is held on the grid by the boundary itself. `load_layout` traces the outline of every
component treating out-of-grid as outside it, so an edge cell always has a wall outward, with no
partner and impassable --- a step off the grid cannot be taken and is not tested for. `move`,
`neighbors`, `visible` and `ahead` assert `Grid::contains` on the cell they reach, stating that
the wall test above is what kept it on.

`ahead(placement)` is the wall directly in front, `nullid` when there is none. It snaps the facing
to one of the grid's own directions as `move` snaps the direction it travels in, and returns
the wall on the near side of the edge that way --- so a rule asking what an actor faces steps no cell
of its own and references no wall record to find it.

A layout is independent of entities and resolves only its own geometry. It takes a predicate
reporting whether something occupies a placement, calls it before entering each step, and
reports whatever it returned as the `obstruction_entity_id` --- so what counts as an obstruction
stays with the caller while the sweep stays with the layout. `World::move` is what supplies it.

A `MoveResult` reports what stopped a mover as `obstruction_wall_id` or `obstruction_entity_id`, one
or the other and never both. `Blocked` has the same pair. Two fields rather than one because a
wall is a record in the layout's arrays and an entity is an entity, so the ids are not
interchangeable.

Two flat tables sized by the grid extent are stored alongside the arrays. One stores whether each cell is
open, which is what `open` reads and the only question a mover or a view asks; the other stores which
room covers a cell and backs `room_at(GridPosition)`, the outline test being too dear to run per cell.
Openness is its own table because a room is a label and decides nothing about passage.

### load_layout

Reads either format a generator writes, the first line being the highest row and the grid origin
lower left in both. A header row names the generator, the version of the format, and the parameters
of the run; a grid row never begins with a letter, so its absence marks a file written before
headers existed, which is dungen's block format. `catagen_reference.md` describes the wall format.

The version is a label rather than a number --- `0.2` today, both generators writing the same one and
the reader accepting only that. A leading zero marks pre-release: nothing older is kept readable, so
a change to the glyphs or the shape regenerates every level rather than adding a case to the reader.

dungen's block format is one character per cell, its symbols from `dungen::cell_types` --- `#` solid
and `0` null are both solid, ` ` room and `.` corridor are open floor, `|` is a doorway and `>` a
stairway. A doorway or a stairway is a **block**: a cell of wall a mover may stand in. It belongs to
the area around it, so the area's boundary encloses the sides the block presents to solid cells, and
a mover leaving one sideways meets that wall end wrapping around into the next area. The faces it presents to floor
are interior to the area and have no outline wall, so the block is ringed with four walls of its
own, each wound about the cell outside it. A door hangs on the faces toward floor, whose winding
matches the area's own run; from inside the block those are backs, which `wall_toward` does not
report at all, so one opening passes the whole doorway and leaving is free. Room floor and
corridor floor are laid out alike and differ only in what the level calls them.

Two areas therefore never share an edge in this format, a block always lying between them, so no
wall of a block layout is partnered. Partnering is what the wall format has and this one does not.

catagen's wall format puts the walls between the cells, so its grid is two characters across per
cell plus one. Cells are joined into one area only across a wall that is absent, so a door separates
two of them, and two rooms sharing a solid wall are partnered along its
whole length and passable nowhere. That last is what the block format cannot express --- a wall with a
room behind it rather than a solid cell --- and a block layout is therefore a subset of a wall layout
rather than a different model, which is why both load into the same structure.

Both reduce to the same intermediate before anything is built: which cells are open, which of them
belong to an area, where two areas meet, which cells are blocks, and which the format called room
floor. Each
group's boundary is traced with the open space on the left and chained into closed outlines, all of
them alike. Two walls are matched by the directed boundary edge they lie on, from an index built as
the walls are created. Openness comes from what the file said of each cell, both formats stating it.

Rooms come from the last of those. dungen writes room floor and corridor floor as different symbols,
so one `Room` is built per contiguous run of its room cells, with the outline traced the same way
and each outline edge referencing the wall that lies on it --- or `nullid` where the room opens onto a
corridor with nothing between them. catagen has one open-cell symbol and so defines no rooms, and a
level of either format that defines none loads with none rather than having them guessed at.

Returns the layout it built, as a `std::unique_ptr<DiscreteLayout>` rather than as the
interface: both formats are grids of cells, so both build that class and the signature declares it. A
format describing something else is a reader of its own rather than another return from this one.
`World::create_zone` takes what comes back, so what class a zone's layout is comes from what was
loaded.

It builds geometry and no entities, having no world to build them in, so where the file specifies a door
goes it *reports* rather than places: an optional `door_type` is called with the pair of walls, and
a caller with a world turns that into a door. That is what `Session::populate` does, and it is the
answer to what creates an attachment at load --- the loader never learns what a door is.

Throws `std::runtime_error` when the file cannot be opened, has no rows, names a generator or
version it does not read, or does not match its own header. Nothing is built until every one of
those has passed, so a throw leaves no half-built layout behind.

### World

Zones and everything in them. One flecs world holds the entities; a zone is an entity whose contents
are its children, so migration and unload are subtree operations and entity ids survive both.
Layouts are kept alongside in a map keyed by zone, being static topology rather than game objects. They
are held as `std::unique_ptr<Layout>` because a layout is polymorphic and the map cannot store an
abstract class by value; `World` itself is a plain value, a member of `Session` that is moved once.

`create_zone` takes the layout the zone is to have and asserts it was given one --- there is no
default, a default being what would put one concrete class back inside `World`. `zone`, `zone_count`
and `layout` reach the zones, `layout` returning the interface; `create_entity`, `zone_of`,
`move` and `relocate` reach their contents; `unload_zone` discards a zone, its contents and its
layout, leaving persistence to a caller.

Occupancy is `entities_at` and `entities_within`, both returning entities ordered by id so that a
run gives the same result every time --- flecs iteration is deterministic but visits entities grouped
by archetype rather than in id order, so the sort is explicit. `entities_at` takes a placement or a
cell: the placement form asks the layout `same_location` and so involves no cells at all, and it is
what `move` uses; the cell form is for a caller that genuinely counts cells and goes to
`DiscreteLayout` to do it.

Ownership follows current location rather than origin: `relocate` reparents an entity into its
destination zone, so the zone it came from can unload without taking it along. Entity ids are
allocated by the one flecs world and reparenting leaves them untouched, so references to a migrated
entity survive the move.

`move` is where topology and occupancy meet. What stops a mover comes from two places --- the zone's
walls and whatever occupies the cell ahead --- and only `World` has both, so neither
the layout beneath it nor the rule above it has to reach into the other. It hands the layout a
predicate over its own entities and the layout does the sweep. Rules ask this rather than the
layout directly.

The two must not become a sequence. Continuous movement is one solve: a blocker partway along a
swept path changes where the slide against a wall happens, so resolving topology first and occupancy
afterwards gives results that are wrong rather than approximate. A discrete zone tolerates the
sequence because a cell is atomic, which is why it is not the case the shape was designed against.

`Blocking` is the tag that makes an entity obstruct a mover. Presence rather than identity: a
dropped sword has a placement and no more, while a monster and a pushable boulder stop a mover the
same way. A mover is never stopped by itself.

### Attachments

What is attached to a wall --- a door for now, an alcove, a basin, a button or a torch later. One mechanism
for all of them is what keeps `Wall` a small dense record. `attachment_plan.md` gives the shape and
the argument.

An attachment is an entity, so mutable state is allowed on it in a way it is not on a wall record.
`Attachment` is where it sits on the surface, along and up; `OnWall` references the wall by id, a wall
being a record rather than an entity, and the reverse direction is a query filtered by that id.

A door is two facings and what they share. The shared part has `Door` and `Openable` --- one door
cannot be open from one side and shut from the other --- and each facing has `DoorFacing`,
`OnWall` and its own per-side data. A facing is a child of its door, so which door it belongs to is
its parent rather than a field, the other facing is its sibling, and the whole assembly is one
subtree: it unloads with the zone and one delete removes it.

The two walls need not be a partnered pair. In the wall format they are the two sides of one cell
edge; in the block format a doorway is a *cell*, so they are the two walls that cell presents to its
neighbours, and entering the cell is passing through the door. `create_door` is told which two, and
nothing else notices the difference.

`create_doors(world, zone, doors)` builds one on each set of walls the loader reported, which is
what both formats' doors and doorways load as, and `Session::populate` calls it. They are generic and
**closed**, the formats having no properties of their own yet, so a level's doors all have to be
opened.

`facing_on(zone, wall_id)` is the facing attached to a wall, a wall id identifying a wall in one zone's layout so
the zone is what the search is over; `door_of` is its parent and `other_facing` its sibling.
`crossable(zone, wall_id)` is the passage question: a wall with nothing attached is solid, an
`Openable` attachment reports whether it is open, anything else whether it has `Passable` --- which is
what a door smashed to a hole leaves and blocking debris does not. A facing `Concealed` and not yet
found refuses, a secret door being a wall until it is found, and from that side alone.

`Interact` opens and shuts one: the facing on the wall ahead, its door, `Openable` flipped. It
reports which of `Interaction::Opened`, `Closed` or `Nothing` happened, so a view can show it rather
than name a wall id at a player.

**Passage is this, and nothing else.** `blocked` calls `Layout`'s `crossable_type`, which
`World` sets to `crossable` when it takes a layout, so a wall is solid unless an attachment on it
makes it otherwise. `Wall` has no flag, and a layout never given a predicate has every wall solid.

A solid cell is never met in any way. Every boundary between floor and solid has a wall, so `blocked`
decides from the wall alone and refuses only a step off the grid. A doorway block is no exception:
it belongs to the area around it, so that area's outline encloses the sides the block presents to
solid cells, and a mover leaving one sideways meets the wall end wrapping around into the next area.

### Scheduler

Turn order in abstract units. Each actor accumulates energy at its own speed and acts once it has
banked `action_cost`, which is 100. `spend` charges it; `elapsed` reports the units passed. Speeds
and energies are kept in ordered maps, so ties break identically on every run.

Ordering and timing are separate, and only the first is the scheduler's. `advance(units)` grants
every actor the energy its speed earns and does nothing else; `ready` reports who can act now,
readiest first and lowest id of those tied. `next` is those two composed --- advance to the moment
the soonest actor can act, then take it --- which is the turn-based policy rather than anything the
ordering requires. A scheme advancing on the wall clock converts elapsed time to units, calls
`advance`, and never calls `next`, since skipping past the wait is exactly what it must not do.

The distinction is visible in what banking looks like: grant four units to actors of speed 100 and
25 and the fast one banks four turns' worth while the slow one banks its first, so it stays the
readier of the two. `next` would have stopped at the first.

### Clock

Advancing a `World` through its `Scheduler`: grant time, then run whoever that made ready. Which
grant is used is the whole of what makes a world turn-based or real time, and everything after the
grant is identical either way. `grant_turn` skips to the moment the soonest actor can act;
`grant(units)` credits what those units earn, leaving actors possibly still short.

A `Driver` decides which, and `advance` is what a host calls. Turn-based skips ahead and goes again,
so it returns only when a decision is wanted or nobody will ever act. Real time grants what the
elapsed time earned and returns whether or not that made anyone ready, skipping the wait being
exactly what it must not do --- and it ignores the elapsed time a turn-based driver is handed, which
is why a host passes one without having to check which it is running.

`units_per_second` converts wall clock to units, one unit being one action for an actor of speed
`action_cost`. The clock keeps the fractional remainder, so a run of frames each too short to earn a
whole unit still advances rather than every one of them rounding away to nothing --- which is the
accumulation every host would otherwise have to get right separately.

`set_driver` is called by `Session::populate`, so the choice belongs to the zone rather than to the
host.

`run(events, actor_controlled)` runs each ready actor on whatever its `Intent` asks for, charging
what the action cost, and returns the controlled actor once it is up asking for nothing. `nullid`
means none is ready at all.

Asked to face the way it is going, an actor cannot do one without the other, so the two are a single
faced step. Asked to face elsewhere while moving --- a strafe that turns --- they are two actions: the
turn is taken first and the movement is still owed, one action each.

What becomes of the intent afterwards is the driver's. Turn-based drops what it took --- one action to
an intent, so a key pressed once moves once. Real time keeps it, so a direction held goes on
producing steps as the actor earns them and a host writes an intent when the input changes rather
than every frame.

Either driver drops an intent whose action achieved nothing. That is what bounds a pass: an actor
held against a wall under real time would otherwise be asked the same impossible step for as long as
it had the time to try.

Real time over a discrete layout makes the *timing* continuous, not the space. An actor still steps
a whole cell at a time, and how often is what the wall clock decides --- which is the Dungeon Master
arrangement rather than a compromise on the way to one. Sub-cell movement is a continuous layout,
and that is Phase 7.

An action that achieved nothing costs no time, since achieving nothing does not end a turn --- walking
into a wall is free. What it asked for is spent either way, so the next pass finds nothing asked for
rather than trying the same action again.

Which actor is controlled is the whole of what tells one apart from the rest: it hands control out
when it has nothing to do, and every other actor in that position waits instead. That is what keeps
an actor with no intent from stopping the turn order, and it is an explicit binding rather than an
actor recognised by returning nothing.

This is the simulation running rather than a view of it, so a host supplies only the decision it
alone can make and none of the ordering. That matters for more than tidiness --- before it existed,
both hosts advanced the world only as a side effect of a keypress, and a non-player actor had
nowhere to act at all.

### Intent

What an actor is trying to do, held on the actor rather than asked for when it comes to act. A
`Command` is an edge and an `Intent` is a level: "north was pressed" against "north is where I am
trying to go".

`direction` is the movement asked for and `facing` the facing asked for, both zero when nothing is,
and `distance` is how far --- one action's worth in turn-based, and what a real-time driver sets per
tick. Both parts are directions in world space, absolute rather than relative, which the layout
snaps where it is discrete, so one representation works for a discrete layout and a continuous one
alike.

Being a component it belongs to the entity, so it survives unload and migration with everything else
the entity has. That is what an actor pursuing a goal across many turns needs and what a decision
asked for per action has nowhere to put. It registers no reflection, so it does not serialise --- an
action half issued is not state worth restoring.

### Deciding for itself

`think` writes an `Intent` for everything but the entity a host controls. An actor with no
behaviour is left alone, and one that asks for nothing waits rather than stopping the turn order.
It gathers the entities before writing any, a component set during iteration being able to move the
entity it belongs to.

`Pursue` is the one behaviour: go toward a target while it can be seen, toward where it was last
seen when it cannot, and give that up on arriving there having found nothing. Where it was last seen
is what makes it more than a reaction, and it lives on the entity, so it unloads, migrates and
saves with everything else the entity has --- which a decision asked for per action would have
nowhere to keep. That is the argument `Intent` was introduced on, demonstrated rather than asserted.

The step taken is the reachable neighbour that closes the most distance, so a wall is gone around
rather than walked into, and none is taken when none is nearer. `neighbors` returns in a fixed
order, so ties break the same way on every run. It consults topology only, so a pursuer walks up to
whatever occupies the cell ahead and is stopped there by `World::move` rather than by having avoided
it.

### Session

A loaded world and what is needed to run it: a `World`, a `Scheduler`, a `Clock`, the entity a host
controls, and that entity's pending one-shot command. A host is left with input mapping and drawing.

**A zone is constructed whole when it is encountered**, geometry and every content of it, and
`create` is that: it takes a `Generator` and builds the lot. The level files are snapshots of part of
the same construction, taken where only geometry existed, so `load` is the narrower entry rather than
the general one. `populate` is the third --- geometry already built, populated as a constructed zone
is --- and it is what the other two are written in terms of, so no route can drift from another.

`Generator` is an interface here and an implementation wherever a generator was chosen: `stratum_sim`
links `quetzal_common` and flecs and nothing else, reading both file formats with readers of its own
rather than depending on either generator. Which one runs decides which parameters apply, so the
concrete generator declares its own. It returns the geometry, reports the walls each door is attached
to exactly as the loader does, and gives the `Format` a constructed zone has no file to supply.
`placement_plan.md` gives the shape.

`populate` builds the doors, places the controlled entity in the lowest open cell facing east, and
sets the clock's driver, throwing `std::runtime_error` where the layout has no open cell; `load`
throws the same reporting the file, which is the context it has and `populate` does not. The scan for a
cell returns `std::optional<GridPosition>`, an empty grid having no cell to name rather than a
reserved one. Where the player starts is asked in cells, so this is one of the callers that reaches
past `Session::layout`'s interface with `layout_as`. The driver arrives here rather than from a host
so that turn-based and real time are a property of the zone; it will come from the zone's own data
once zones have any.

`create_player` and `create_monsters` are what put a zone's contents in it --- the player in the lowest
open cell, and one pursuer in the highest so that something other than the host decides for itself.
They are separate because they are where a zone's starting contents go when a level specifies them:
selection and placement over a zone's contents, rather than the two fixed entities they are today.

The lifecycle around these entries is in progress rather than built. A session constructs one zone
and its player together and has no route to a second, so the player cannot outlive the zone it was
made in and there is nothing to clear on a new game. Stage 7 of `work_plan.md` states what it
becomes.

`advance` calls `think` before the clock reads what anyone is asking for, so everything decides
before anything acts. One decision per call: an actor fast enough to act twice between calls acts
twice on the one decision it made, which is where that hook moves if it ever matters. It builds
alongside and moves in only once the whole of it has succeeded, so a failure leaves the previous zone
intact rather than half of a new one. The clock's references are to the session's own members
rather than to what is moved from, so they survive that.

`set_intent` specifies what the controlled entity is trying to do, movement and facing being kept on the
entity until something changes them. `submit` leaves a one-shot action for it instead, interacting
being an edge rather than a level.

`advance` hands out time and runs, returning the controlled entity when it needs a decision that was
not available. `nullid` means no actor will ever be ready rather than none being ready now, passing
time being what the grant already tried. A pending one-shot is applied at that point, an `Intent`
limited to movement and facing, which is why the session rather than the clock owns it --- and
the pass that follows is given no elapsed time, a real-time driver otherwise being paid twice for
one frame.

The elapsed time is what a real-time host measured since it last called. A turn-based host passes
none, and both hosts do exactly that today: neither specifies a driver, and neither would change if the
level it loaded specified the other.

Both hosts had their own copy of all of this. What remains different between them is only what
surrounds `advance`.

### Commands and events

`Command` and `Event` are type-erased over `std::any`, so a module can introduce either without the
core being compiled against the type. A command also references the entity it acts for. `CommandQueue` and
`EventQueue` are deques drained in submission and occurrence order.

The vocabulary is `Step` (a direction and distance, a zero direction meaning along the actor's own
facing, and `face` to turn to that direction as part of the same action rather than at the cost of
one of its own), `TurnTo` (the facing asked for, absolute rather than relative) and `Interact`, which asks
the layout what is `ahead` and acts on that. They produce `Moved`, `Blocked`, `Turned` and
`Interacted`.

`Step` and `TurnTo` are what the clock builds from an actor's `Intent` rather than anything a host
sends. What a host still sends as a command is the one-shot half of the vocabulary --- `Interact` ---
which is why `Command` narrowed rather than disappearing when intent arrived.

Both movement and facing are therefore direction vectors, which is one representation for a discrete
layout and a continuous one alike: the discrete case snaps and the continuous case will not. A
`TurnTo` asking for the facing already held costs nothing and reports nothing, as a `Step` into a
wall does.

`apply(World&, const Command&, EventQueue&)` returns the energy the command cost, zero when nothing
happened; the `CommandQueue` overload drains the queue.

What a move costs is what it covered rather than a flat action: `step_distance` is what one
action of movement is worth, so crossing one cell costs one action and crossing four costs four, and
a move stopped short is charged for the distance actually reached. That is what makes a cheap
per-tick step possible, a fraction of a step costing a fraction of an action. Turning and
interacting cost one action, the result they derive from being whether they happened at all: a
`TurnTo` asking for the facing already held and an `Interact` with nothing in front both cost nothing.

## Presentation

Engine-neutral. It depends on `stratum_sim` and on no output device at all.

Two stages, and no interface over them. A **reduction** takes a world and a zone to a
`Buffer<Place>` --- `cells` and `lattice` --- and a **mapping** takes that to a `Buffer<T>` a device can
emit, which is `render` and `render_lattice`. The Godot tile view consumes a reduction and stops;
the terminal consumes a mapping. Nothing dispatches: neither host owns a view and asks it to draw,
each being written against the one it needs, so a `View` interface would be a vtable with no call
site. The consumer that would settle whether `Buffer<T>` is the shared shape is the first-person 3D
view, which produces mesh instances rather than a buffer.

`cells(world, zone)` reduces a zone to what is at each cell --- `PlaceType::Solid`, `Floor` or
`Door` --- and that reduction is as far as the simulation is taken before any view is involved. What a
consumer turns it into is its own business: `Glyphs` is a table taking a place to a character for
the terminal, and the Godot tile view maps the same place types to tiles.

The files divide on that line. `cells.hpp` declares the reductions and is what a view includes to get
one; `ascii.hpp` declares only the terminal's mapping, so the tile view has no reason to include it,
and `read_glyphs.hpp` only the reader that fills one of those mappings from a file. `Buffer.hpp`
is under both, defining the grid a reduction and a mapping each produce.

It is worth being exact about what `cells` is, because it looks more general than it is. It is the
reduction one cell per grid position offers and no more: what lies on the edges between them is
not in it, so a wall or a door between two open cells reduces to `Floor` on both sides.

`lattice(world, zone)` is the reduction that produces them, onto the same shape the wall format
writes: `(2W+1)` by `(2H+1)`, cell `(i, j)` at `(2i+1, 2j+1)`, the wall west of it at `(2i, 2j+1)`,
the wall south of it at `(2i+1, 2j)`, corners at the even positions in both. It exists alongside
`cells` rather than replacing it --- the tile view has no use for edges yet, and the two are not the
same picture at a different scale.

A position reduces to a `Place` --- a `PlaceType`, the `State` it is in, the `Mask` of what reaches
it, and whether an actor stands on it. `PlaceType` is one entry per type of structure and none per state
it can be in, so a door that can be shut, locked or concealed stays one `PlaceType` and nothing has
to enumerate the cross product. `cells` emits `Solid`, `Floor` and the `Door` that occupies a cell of
its own; `lattice` emits those at cell positions, `Wall` at the others, and `Door` at whichever of
the two the door is.

`PlaceType` covers only fixed structure. An actor is not one of its values but a flag beside them,
because an actor is not a kind of place --- it is on one --- and giving it a value of its own overwrote
the floor, the state and the mask underneath, which left a tile view with nothing to draw beneath an
actor. `Place::actor` layers instead, and a glyph line asks for it the way it asks for a state:
`floor actor '@'` matches an actor on floor, and a line that does not name `actor` matches whether or
not one is there. An `actor` line and a one-state line are equally specific, so the actor lines come
first in both shipped sets, which is what makes an actor in an open doorway draw as the actor.
`None` is nothing at all, which only `lattice` produces: an edge or corner with no wall on it and an
open cell beside it, such as the corner interior to a room. `cells` never emits it, assigning every
position `Solid`, `Floor` or `Door`.

A door has a position in `cells` only where the door *is* a cell, which is what a doorway is in the
block format. A door on an edge has nowhere to go there, so the wall format's doors are what a glyph
per cell cannot show --- the same rule that decides where the lattice draws one, asked once and shared
by both reductions.

`State` is a set of flags named after the component data they are read from --- `open` from
`Openable::open`, `passable` from the `Passable` tag, `concealed` from `Concealed`, whose `found`
field is its inverse. Nothing is coined, and only what changes the drawing belongs there: a lock
enters when a locked door is drawn differently and not before. The reduction is the one place either
side is aware of the other, nothing downstream of it seeing a component. `applies(type, state)` reports
whether a state can apply to a type at all, which is what rejects a glyph set specifying one that
never can rather than leaving it a line that never matches.

Every flag in `State` comes from a component, so nothing about the shape of the map is in it. The
two absences are told apart by the type instead: `Solid` where every cell the position touches is
closed, read from the layout, and `None` where one of them is open and nothing is on the position.

Where the door goes is derived rather than told. A door the cell beyond cannot see back across is
one hanging on a doorway block's ring, so the door is that cell and is placed in it, once however
many ways in it has, leaving its faces as the openings they are. A door both cells see is an edge
between them and is drawn on the edge. That is one rule reading the two formats apart from their
own geometry, without either being mentioned.

A corner is `Wall` where any of the four edges touching it has a wall or a door on it. That is a
derivation rather than a second store --- the walls determine it, which is why `load_layout` reads
none --- but it is structure and not decoration: a first-person view puts a column on the joint, and
what each view makes of a corner is its own.

`Mask` is what reaches a position, one bit per direction, set where a wall or a door lies on the
edge or corner that way. A cell is never part of a wall line, so a door occupying one is a gap in
the line rather than a continuation of it, and an edge is left with only the corners at its own two
ends --- which is what tells a vertical wall from a horizontal one without asking whether the position
is a cell, an edge or a corner. Sixteen patterns is the whole of a box drawing set; a set with less
to express maps many of them onto one glyph, and that duplication is the shape of the set rather
than of the key.

`Buffer<T>` is the grid those live in, `at`/`set`/`fill`, row 0 lowest to match the simulation.
`Console` is `Buffer<char>`, and `Cells` and `Lattice` are both `Buffer<Place>`, named apart for what
they are indexed by rather than for any difference in type; the payload is a parameter because a
terminal needs characters and a tile view has no use for them. `to_string` is constrained to the
character case, there being nothing to write for a cell without one, and it emits the
highest row first so the result reads as a map.

`difference(a, b)` returns the runs in which `b` differs from `a`, as `Span {i, j, count}`, in row
order lowest first. It is empty when the two are the same, and the whole of `b` when they differ in
size --- which a caller repainting a device takes as its cue to clear first. One implementation
handles any payload, so a tile view differences places exactly as the terminal differences characters.

`render` takes cells to a `Console`, and an overload composes the two steps for a caller that needs
characters from a world directly.

`render_lattice` is the same for the lattice, with a name of its own rather than an overload because
`Cells` and `Lattice` are the same type.

Both reductions place actors last, through one `each_actor_cell`, so that an actor takes the cell it
occupies rather than being drawn over by the floor under it.

An edge is asked of the side that faces it, which for a map is the open cell: a wall exists only on
the side facing open space, so a closed cell has nothing to report. Asking a fixed side instead is
what left every wall with its closed cell to the east or north undrawn.

Where both cells are open the edge has a partnered pair, and a file writes the two the same
character --- one character per edge is all the wall format can express. So either will do, and a pair made
to differ from code, a one-way door, is the case a single `PlaceType` cannot express.

`Solid` is decided in the reduction and is one rule over every position: a position is solid when
every cell it touches is closed --- a cell touches itself, an edge the two it lies between, a corner
the four that meet at it. A position off the grid touches no open cell, so the outer boundary comes
out solid without anything beyond the grid being treated as a cell. A wall faces open floor on the
side it belongs to, so none is ever at a solid position, and the reduction asserts it rather than
deciding what to do about it.

Rendering is then one lookup per position and no rule of its own. What a position is, what state it
is in and what reaches it are all in the reduction, so cells, edges and corners resolve through the
same table rather than through a branch on which of the three a position is.

A `Glyph` is one line of that table: a type, the state flags it asks for, optionally a mask, and the
character to draw. `Glyphs` keeps the lines and returns the most specific one matching --- a
named state flag and a named mask counting one each, the first of them where two are equally
specific. So a set naming `door open` draws an open door apart, and a set naming only `door` draws
both alike, without the renderer being aware which it has. That is what makes a set's expressive
range a property of the set.

`cell_glyphs()` is six lines: the three types a cell can be, a door shut and open, and an actor
over each of the two types it can stand on.
`lattice_glyphs()` is what the wall format writes: floor a space rather than a `.`, which is what
makes a wall format level render back to the file it was loaded from, checked by `stratum_view_test`
the same way the block format's round trip is --- the corner shaping and where solid shows agree with
`catagen`'s writer without either depending on the other. Its wall is sixteen lines, ASCII having
three glyphs for the joints the mask tells apart.

`read_glyphs` reads a set from a file, `assets/glyphs/` shipping the two the code builds in. A line
is a type, the state names it asks for in any order, `mask=` and the directions it asks for, then
the character in single quotes --- quoted so that a space is a glyph like any other, with `\\` and
`\'` the two escapes. Naming no state matches whatever state a place is in, and leaving out `mask=`
matches whatever reaches it, `mask=` alone asking for nothing reaching it at all. A line beginning
with `#` is a comment, a glyph never beginning one. It throws reporting the line, a set being written
by hand, and a state a type can never be in is one of the errors it throws on. The two shipped files
are compared against the built-in sets, so neither can drift from the other.

## Terminal host

`Terminal` writes a `Console` to a character terminal, emitting only the cells that changed since
the last frame. The escapes are CUP, ED and EL of ECMA-48, plus three DEC private modes: DECTCEM to
hide the cursor, the alternate screen buffer, and XTWINOPS to resize.

- It enters the alternate screen buffer on construction and leaves it on destruction, so the
  shell's own screen and scrollback come back untouched.
- On Windows it sets `ENABLE_VIRTUAL_TERMINAL_PROCESSING` explicitly rather than relying on the
  host's default, which conhost does not turn on.
- `terminal_size` reads the text area; `set_terminal_size` asks for one with
  `CSI 8 ; rows ; columns t`, honoured by Windows Terminal and, unlike the Windows console API,
  leaving the scrollback intact.
- The window is grown on the first frame where the frame would not otherwise fit, and never shrunk.
  The original size is restored on exit, but only while the window is still the size it was put at,
  so a resize by hand is left alone.
- The pseudoconsole reports a new size before the terminal has finished resizing. A settling delay
  therefore precedes the first paint and follows a detected resize; painting too early scrolls the
  frame off the top.
- A frame is the map rows, a blank, the status and the help. The help line is not newline
  terminated, so a frame exactly the height of the window does not scroll it.
- The map rows are `render_lattice`, so the host draws what lies between cells as well as what
  occupies them, with `lattice_glyphs` --- open is a space, a room's interior being what the walls
  around it enclose rather than something to fill. That makes a frame twice the grid's size plus
  one in each direction, which is what the window growing below is sized against.
- A frame that will not fit is replaced by the terminal's size and the size needed.

`read_key` returns one key without waiting for a newline: `_getch` on Windows, termios raw mode
elsewhere. Arrows arrive as a lead byte and code on Windows and as `ESC [ code` elsewhere, and
become `Key::Up`, `Down`, `Left` or `Right` either way.

The loop advances the session to whoever acts, draws, reads a key, and routes it: movement and
facing become the actor's intent, and a one-shot action a command. Arrows or `hjkl` step, space
interacts, `.` waits, `^L` redraws, and `q` or Escape quits. Nothing in this view shows which way
the actor faces, so there is no key that only turns: a step is asked for facing the way it goes,
which is the same vector as its direction and its facing. Waiting is charged by the host, the
simulation having no wait command. `--script` plays a key sequence in place of the keyboard, and
`--incremental` forces the cursor-addressed path within a scripted run.

`--cell` and `--wall` choose which reduction is drawn. The cell view is a glyph per cell, which is
what a roguelike shows; the wall view also draws what lies on the edges between cells, so a door
or a wall two rooms share is only in that one. Each takes the glyph set built for it, and the choice
is made once rather than per frame.

Asked for neither, the zone decides: a block format zone draws through the cell view, which loses
nothing of it, and a wall format zone through the wall view, the cell view having nowhere to put
the walls between its open cells. `load_layout` reports which format it read through an optional
`Format*` and `Session` keeps it, so the host asks the session rather than reading the file again.
Both formats still load into the same structure --- the format is what a caller decides *by*, not
a property of the layout.

Level files live in `assets/level/`, which belongs to no host. The host reaches them through
`KML_DEV_ASSETS_ROOT`, a compile definition CMake sets from the source tree root, so the working
directory a program is started from does not matter.

## Godot host

The inversion the other host does not have: Godot owns `main()` and the frame loop, and the
simulation is a shared library Godot loads. `stratum_godot` builds that library --- a GDExtension ---
and `application/stratum_godot/project/` is the Godot project that loads it, named by
`stratum.gdextension`, which points at `res://bin/` where the build writes.

`StratumWorld` is the one registered class, a `RefCounted` owning a `Session`. `step` sets the
player's intent and advances, so the two hosts differ only in what surrounds the advance: the
terminal asks for a decision and then advances, Godot is handed one and then advances. Both advance
through `grant_turn`, which is what makes both turn-based; a real-time driver would grant units from
a per-frame callback instead, and nothing else about either host would change.

Because the session runs on until something needs a decision it does not have, a step ends with the
next turn already granted. So `turn` reports the turn now awaiting input, which is what the terminal
host's status line has always meant --- the terminal reports it before the action and Godot after. It returns Godot types only, so nothing above it sees a quetzal type:
`default_zone_path`, `load_zone`, `extent`, `cell`, `render`, `cells`, `step` and `turn`. Exceptions stop at that
boundary --- `Session::load` throws, and letting that cross into Godot's own frame is not an option, so
a failed load pushes the reason to Godot's error list and returns false. The previous zone is left
intact by the session rather than by anything the host does.

`game.tscn` is the tile view: a `TileMapLayer` fed from `cells`, a camera, and arrow or `hjkl` keys
producing the same steps the terminal host sends. Its tileset is built at run time from three flat
colours rather than imported, so no binary art is checked in for what a real tileset will replace
without anything else changing. Row zero is the lowest in the simulation and y grows downward in
Godot, so the view inverts j when it paints.

`cells` hands GDScript one code per cell, straight from `stratum_view::cells` --- the same reduction
the terminal host renders characters from, taken one step earlier. The `PlaceType` constants are
repeated on the class only because Godot has to be told the names of the constants it can use.

`quetzal_godot` provides what any Godot host would need rather than anything about this game:
`gdextension.cmake` and its `quetzal_configure_gdextension`, which supplies the naming Godot looks
for and writes the library into the project rather than into a per-configuration subdirectory.

Two settings the build has to get right, both of which fail confusingly rather than loudly.
`CMAKE_MSVC_RUNTIME_LIBRARY` is forced into the cache by the root, because godot-cpp otherwise
caches the static release CRT even for a debug build and quetzal's static libraries then will not
link. And godot-cpp's headers are marked as SYSTEM includes, as flecs' are, since they compile as
part of whatever includes them and would otherwise report at `/W4` against the extension's sources.

`KML_DEV_GODOT` governs all of it. Configured off, godot-cpp is not processed and neither the
library nor the host exists.

Two headless checks, neither needing a display: `--path project --script main.gd` exercises the
extension directly and prints what it found, and `--path project --quit-after 5` runs the tile
scene, which reports the cells it painted. A project has to be imported once
(`--headless --path project --import`) before Godot will see the extension at all --- without it the
class is simply undeclared, with no indication why.

Visual Studio launches Godot on the project when the extension is run, so breakpoints in the
extension are live in a running game. Godot must not already have the project open, or the library is
locked and the build fails.

## Tests

`stratum_sim_test`, 396 cases, covers the layout hierarchy, layout structure, partner linking, wall
folds, point location, the grid, flats, blocking, the scheduler, the clock, movement, adjacency and
sight, loading, doors, world, commands and occupancy.

`stratum_view_test`, 96 cases, covers the console buffer, frame differencing, glyph sets and the
files they load from, the cell and lattice
reductions, both renderings and both round trips, and owns the reference
frames: fixed command sequences rendered to text and compared byte for byte by `FileValidator` against
`test/stratum_view_test/reference/`. Those sequences are played in process against the two libraries rather
than through the host, whose key mapping is not what they check.

A scenario selects which view it is played through, so the two are checked separately. Three are block
levels through the cell view, where a wall exists only against a solid cell and a glyph per cell
loses nothing. Two are the wall view, and they differ in where the door is. `walls_door` is a wall format
level: through a door, back, and into the wall two rooms share, which reports blocking it and then
interacts with it while it is drawn beside the actor --- the whole of what a glyph per cell cannot
show. `door_cell` is a block format level, where the door is a cell rather than an edge and is drawn
in it, the actor stepping into the doorway and out the far side.

`dungen_test`, 34 cases, and `catagen_test`, 19 cases, check the generators the same way: reference
levels written now against levels checked in, byte for byte, plus determinism, parameter validation
and grid shape. Each also reads a reference level's own header back into parameters and regenerates
from them, which is the claim the format rests on --- a level names what produced it --- and the one
nothing was checking. A parameter a writer emits and the reader does not recognise fails there rather
than passing silently.

The two suites are what the drivers never were. `tool/dungen` and `tool/catagen` take the plain
names, the `quetzal_` prefix on the library targets being the distinction, and the `_test` names
belong to the suites under `test/`.

`flecs_test` confirms the ECS supports zone partitioning, relationships, migration, unload,
serialisation and determinism.

Each test runs from its own directory, their data and reference paths being relative.

Neither host is covered by a test executable. The terminal host is checked by running it with
`--script` and reading the frames back; the Godot host by its two headless invocations. Both are
manual, and the reference frames do not reach either of them.

## Build

CMake, with the solution at `build/kml_dev.slnx`. `/std:c++latest /bigobj /W4` throughout, with
third-party sources at `/W1`, and both flecs and godot-cpp included as SYSTEM so their headers stay
quiet in whatever compiles them. `quetzal_stratum_sim` links `quetzal_common` and `flecs_static`,
`quetzal_stratum_view` links `quetzal_stratum_sim`, and both hosts link `quetzal_stratum_view`.

`KML_DEV_GODOT` governs the Godot half and defaults on. Configured off, godot-cpp is not processed
at all and neither `quetzal_godot` nor `stratum_godot` exists --- 844 projects rather than 866.

`CMAKE_MSVC_RUNTIME_LIBRARY` is forced into the cache by the root rather than merely set, because
godot-cpp caches the static release CRT even for a debug build and quetzal's static libraries then
refuse to link into the extension.

`KML_DEV_ASSETS_ROOT` is a per-target compile definition giving the shared data root, so no absolute
path appears in any source file and no program depends on where it was started from.
`KML_DEV_GODOT_VERSION` gives the Godot release the host targets and `KML_DEV_GODOT_EXECUTABLE` is
found from it, which is what Visual Studio launches.

`application/stratum_godot` is added after `library3` rather than beside the other applications,
since configuring a GDExtension reads a property from the godot-cpp target and that target has to
exist by then.

---

*Current stage: Phase 3, all but presentation tables. `Glyphs` is a table a file supplies; the Godot
view's colour array is still hardcoded, and making it data requires asset ids, which is Phase 4's work.*
