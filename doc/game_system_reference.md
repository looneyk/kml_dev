# Game System --- Reference

What the system is and how it is put together, as implemented. The reasoning behind its larger decisions is in
`decision_rationale.md`.

Three layers, with no engine in any of them. `quetzal::stratum_sim` simulates, `quetzal::stratum_view` turns a
simulation state into characters or place codes, and a host puts those on a device. `stratum_sim` links
`quetzal_common`, `quetzal_stratum_format` and flecs; `stratum_view` links `stratum_sim`; each host links
`stratum_view`.

The `stratum_` prefix names the layout class they share: open ground extruded vertically between one floor and one
ceiling.

| Directory | Target |
| --- | --- |
| `assets/` | shared data, owned by no host |
| `library/quetzal/stratum_sim/` | `quetzal_stratum_sim` |
| `library/quetzal/stratum_view/` | `quetzal_stratum_view` |
| `library/quetzal/stratum_format/` | `quetzal_stratum_format` --- the zone file format |
| `application/stratum_terminal/` | `stratum_terminal` |
| `application/stratum_godot/` | `stratum_godot` |
| `library/quetzal/godot/` | `quetzal_godot` |
| `tool/dungen/`, `tool/catagen/` | `dungen`, `catagen` --- the generator drivers |
| `test/stratum_sim_test/`, `test/stratum_view_test/`, `test/stratum_format_test/`, `test/dungen_test/`, `test/catagen_test/`, `test/flecs_test/` | test executables |

## Simulation

A `World` contains zones. A zone is one independently loadable 3D coordinate space. Zones share no coordinate system,
so a placement is meaningful only within its own zone, and an entity's zone is the zone among its ancestors. Each zone
has a layout, its geometry and topology, and the entities in it.

A caller asks the layout where a move ends; it does not compute a destination and hand it back.

### Types

`value_type` is `float` and `traits_type` is `math::CartesianTraits<value_type, 3>`, giving `point_type`,
`vector_type`, `points_type` and `polygon_type`. `placement_type` is `geometry::DirectedPoint<traits_type>`, a position
and a facing direction.

`coord_type` is `std::int32_t`. `discrete_vector_type` is `math::DiscreteVector<coord_type, 2>`, a displacement on
the grid, and `discrete_point_type` is `geometry::DiscretePoint<coord_type, 2>`, a cell position. A cell position is
moved by adding a `discrete_vector_type`, the difference of two is the displacement between them, and adding two does
not compile. `cardinal_directions` are the four unit vectors `East`, `North`, `West`, `South`, in `CardinalDirection`
order, and `cardinal_direction(d)` returns one.

`point_projection_type` and `vector_projection_type` are `geometry::MinProjection` along the z axis onto the xy plane.
`grid_step(vector)` is the cardinal direction nearest a direction.

### Layout

An interface over one zone's arrangement: `move`, `turn`, `face`, `same_location`, `neighbors`, `visible`, `ahead`
and `step_distance`, all pure virtual. None of them mentions a cell or a wall. `Rotation` (`CCW`, `CW`, `Half`),
`MoveResult`, `crossable_function_type` and `blocker_function_type` are declared with it.

A second, non-virtual `move` takes neither predicate, so no wall is crossable and no entity blocks. A derived layout
brings it back into scope with `using Layout::move`.

The destructor is public and virtual, a zone owning its layout through a `std::unique_ptr<Layout>`. The other special
members are protected and defaulted, so a concrete layout copies and moves as itself and cannot be sliced through the
base.

`layout_as<L>(layout)` is the layout as its concrete class, a checked cast that asserts. The loader, the view
reductions, `Session`'s initial placement, `World::entities_at` by cell position and the host status readouts use it.
`commands.cpp` refers to no cell and no concrete class.

`DiscreteLayout` and `ContinuousLayout` are the two classes beneath it, and only `DiscreteLayout` is implemented.

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

The static topology of one zone: its walls, its flats, and its rooms, stored as a table of cells over the grid.
Entities refer into it by id.

| Type | |
| --- | --- |
| `Cell` | One square of the grid: `open`, whether an actor may stand there; `walls`, a `DirectionMask` of the sides with a wall facing into the cell; and `room_id`, the room containing it, or `nullid`. |
| `DirectionMask` | Any of the four cardinal directions, a bit each in `CardinalDirection` order, with `direction_mask(direction)`. The view uses it for the directions a wall line continues in. |
| `Flat` | The floor or the ceiling, differing only by `ceiling`. Its height is given for the whole flat, per room (`room_height`) and per cell (`cell_height`), each overriding the one before; the per-room and per-cell tables are sparse. |
| `GridMapping` | `origin`, the corner of cell `(0, 0)`; `scale`; and `extent`, the cell count along each axis as a `discrete_vector_type`. `contains(cell_position)`, `cell_position_at(point)` and `position(cell_position)`, the cell center. |
| `MoveResult` | The placement reached, possibly short of the distance asked, and the `blocking_wall_id` or `blocking_entity_id` that stopped it, never both, `nullid` when nothing did. Declared in `Layout.hpp`. |

A layout is constructed with its origin, scale and extent, and sizes its cell table from them. Cells run from `(0, 0)`
to the extent. `GridMapping::contains` is used in assertions only: the loader guarantees a walled boundary, so a move
off the grid cannot arise. The cell accessors assert that the cell position is on the grid; `cell_position_at` floors
any point to a cell position without a bounds check.

A wall is one side of one cell edge, and the two sides of an edge are independent: a wall notation wall between two
rooms is a wall on each side, and a doorway block's wall toward floor is on the floor's side alone. An opening is the
absence of a wall.

A zone with no height data loads a constant floor into the same class; which data a game style loads is what
differs. The layout makes no provision for continuous movement.

Walls are set per cell with `set_cell_walls` and read with `walls`. Rooms are `room_count`/`set_room_count`, and
`room_at` and `set_cell_room` per cell; everything else about a room belongs to an entity. Openness is `open` and
`set_cell_open`, separate from the room, which decides nothing about passage.

`wall_toward(cell_position, direction)` is the id of the wall on that side of the cell, or `nullid`, a bit test. The
id encodes the cell's slot and the direction, which `wall_cell_position(wall_id)` and `wall_direction(wall_id)` decode. `crossable(cell_position, direction, is_wall_crossable)` is true where
there is no wall, and otherwise what `is_wall_crossable` returns for the wall. Both read the one cell, so a cell on
the boundary returns the wall that closes the layout.

`set_flats(height_floor, height_ceiling)` creates the two flats, once; `floor_id` and `ceiling_id` identify them.
`height(flat_id, cell_position)` is the cell's own height, else its room's, else the flat's. `position(cell_position)`
is the cell center with z the floor height there, or on the grid's plane before the flats are set.

`turn` is relative and `face` absolute. A view with a relative turn asks `turn` for the resulting direction and sends
it as a facing direction, so the layout does the quantizing in both cases. `face` snaps to a cardinal direction and
leaves the facing direction unchanged for a zero direction.

`move` snaps the direction with `grid_step`, rounds the distance to whole cells, and moves cell by cell until a wall
`crossable` rejects or the cell ahead is occupied, then sets the position from the cell reached, so rounding error
never accumulates. `blocker_at` reports the entity occupying a placement; the layout calls it before entering each
cell and reports what it returns as `blocking_entity_id`. `World::move` supplies it.

`step_distance` is the grid scale, the distance one action of movement travels.

`neighbors` is the placements reachable in one move. `visible` walks the cells the ray between two placements passes
through, one edge at a time, and fails at the first wall that cannot be crossed. `field_of_view` is declared and not
implemented. `ahead(placement)` is the wall of the placement's cell in its facing direction snapped to a cardinal
direction, or `nullid`.

### load_layout

`load_layout(zone, document)` fills an empty zone from a `Document` that `stratum_format::read` returned, building the
layout through the same `stratum_format` functions that validated it. Each floor/door cell is
open, with a wall on each side `has_wall` reports. Each room's cells, from `room_cells`, take its id, and `room_count`
is the number of rooms. The flats take the file's heights, the floor defaulting to 0 and the ceiling to 1. A stair cell
is a block, closed like solid, and the floor beside it is walled toward it.

A doorway block is a cell an actor may stand in, walled toward solid on its own side and toward floor on the floor's
side alone. Two areas therefore never share an edge in the block notation. In the wall notation, two rooms sharing a
wall are walled from both sides, which the block notation cannot express, so a block layout is a subset of a wall
layout.

It sets the zone's `ZoneLayout` to the `DiscreteLayout`, both notations being grids of cells, and its `ZoneNotation`
to the notation the rows were written in. Then it builds every entity in the file, in the file's order: first each in
the zone, `IsA` its type where one is loaded, so each has its persistent id --- the file's own in a saved zone ---
before anything refers to it; then each one's parent, its site with the address resolved to the layout's ids and its
position and orientation to a placement, and its `Relocator`. A door is the file's `door` entity, and its door facings
the `door_facing` children sited on the walls of its doorway. Each entity `layout.components.access_points` refers to
is made an access point, with the registered access point type its relocator names, its `order`, and its `entry` as
a placement.

Last, each component object of the layout, of each room, wall and flat and a flat's overrides, and of each entity is
decoded into the component registered under its name (`register_file_component`), set on the zone's entity, the
layout entity, or the entity. The decoder walks the object's parsed values and
sets each field through flecs' reflection cursor, so no JSON reaches flecs. A component that inherits is decoded into a
new value, so the entity's own has only the fields its object states and the chain supplies the rest; any other
starts from the entity's value, its type's copy where it has a type. An `std::optional` field is set through its `OptionalField`. A
reference in a component object becomes an `EntityReference`: in a template, to the entity of the same file, with its
persistent id; in a saved zone, to the persistent id written, and the entity where it is in the zone. `load_layout`
returns the entities a template's references refer to, which `World::load_zone` adds to the world index.

`build_error(world, document)` is the first reason the world cannot build a zone from a document: an access point type
not registered, or a component object with no component registered under its name, a field the component does not
have, or a value its field cannot take, with the path to it. It decodes every component object into a value it then
discards, with flecs' own error logging silenced, so `World::load_zone` checks a file completely before making anything.
`load_layout` asserts what `build_error` accepts.

`register_file_component(entities, name, component)`, or `register_file_component<T>(entities, name)`, registers a
component with its reflection as what a component object of that name decodes into, under `components`;
`file_component(entities, name)` is that component, or 0.

### World

Zones and everything in them. One flecs world contains the entities; a zone is an entity whose contents are its
children, so migration and unload are subtree operations, and migration keeps each entity's id. A zone is an
entity with the `Zone` tag, a `ZoneLayout` component owning its layout as a `std::unique_ptr<Layout>`, and a
`ZoneNotation`. Deleting the entity deletes the layout. There is one `World` in a game, owned by `StratumGame`, and it
cannot be copied or moved.

`create_zone(zone_id)` makes an empty zone, the `Zone` tag and that `PersistentId`, for a generator or the loader to
fill. `create_zone(layout)` makes one with a new id, that layout and nothing else, asserting it is given one. Zones have no names;
each is identified by its persistent id. `zone_count` and `layout` reach the zones, `layout` returning the interface;
`create_entity`, `move` and `relocate` reach their contents; and `unload_zone` discards a zone, its contents and its
layout.

`create_zone(generator)` makes an empty zone with a new id, has `Generator::generate` fill it as the loader does, and
populates it; if the generator throws, the zone is unloaded and the exception passes on.
`load_zone(filepath)` reads the file and checks its access point types before making the zone, with the file's
persistent id where it has a `zone` section and a new one otherwise, then has `load_layout` fill it and populates it.
It throws `std::runtime_error` when the reader rejects the file, an access point type is not registered, or the file's
zone is already loaded, having made nothing. `World` handles no doors itself. Neither unloads anything.
`populate(zone)` places the zone's denizens: one pursuer at the last open cell in reverse row order facing west,
`Blocking`, with a `Speed` and a `Pursue` that has no target.

`Speed` is the rate an actor gains energy at in the turn order.

`zones()` is the world's zones in id order, read live, and `zone_count` their number. A zone whose unload has begun is
not among them, though its deletion may be deferred.

`trim_zones(zone_entered)` has the world's `ZoneEntryIndex` make `zone_entered` the zone entered most recently and choose
the zones to unload, then unloads them. `zone_count_max`, a constructor argument, is the maximum the world trims to;
`set_zone_count_max` changes it. `StratumGame` passes 8, a temporary value until the game's config file supplies it.

`unload_zone(zone)` begins an unload: it adds the zone's id to the zones unloading and deletes the entity. Flecs deletes
a zone's subtree before the zone itself, and where it is deferring, runs the whole deletion when the deferred
operations do. The unload is complete in the `PersistentId` observer on the zone's own removal, which takes the zone
out of the zones unloading. So an unload is correct whether the deletion runs at once or later.

### Entity types, layout entities and properties

A type is a prefab, under `types`: `create_entity_type(entities, name)` makes it and `entity_type(entities, name)`
finds it, null where none is loaded. The caller sets its components. An entity `IsA` its type reads a component that
inherits --- one with optional fields --- from the type, with nothing copied, as `register_file_component` registers
such a component with flecs' `(OnInstantiate, Inherit)` through `register_inheritance`. Any other component flecs
copies onto the entity when it becomes one, so each door has its own `Access`.
`register_game_types(world)`, beside `StratumGame`, which calls it, registers the game's types --- `door`, with a
closed `Access` --- until the game's types load from data. The suites call it too.

A layout entity is the entity of a room, a wall, a flat, a flat's override for a room, or a flat cell. Its address is
its component, and how it is found: a `RoomAddress`, `WallAddress`, `FlatAddress`, `FlatRoomAddress` or
`FlatCellAddress`, the five `layout_addresses_type` lists. It has no persistent id. A flat's override for a room or a
cell is a child of the flat's entity through `Parent`, and every other layout entity a child of its zone. The loader
gives every room and each flat an entity; a wall, a flat's override for a room and a flat cell have one while they
have components of their own or something sited on them. `layout_entities.hpp` has templates over the address type,
taking the zone: `ensure_layout_entity(zone, address)` makes one once and `layout_entity(zone, address)` finds it,
null where there is none; `room_entity`, `wall_entity`, `flat_entity`, `flat_room_entity` and `flat_cell_entity`, and
an `ensure_` form of each, do the same by id; and `is_layout_entity(e)` tells one apart. The zone's
`ZoneAddressIndex` has a map from address to entity for each address type, and `for_each_layout_address` calls a
function with each address type, which is how `World` registers what it does for all five.

A wall's or a flat cell's entity lists what is sited on it in `AttachedEntities`, kept by observers on `Site`: setting a
site with an address makes the entity where there is none and lists the sited entity once, and removing it takes the
sited entity off the list. The wall's or cell's entity is deleted when its list is empty and it has no components of
its own. Deleting a layout entity leaves what was on it with no site. `reparent` removes the site before changing the
parent, so an entity leaves what it was sited on while still in that zone. Deleting a zone or the world unlinks
nothing.

`property<T>(e)` resolves a component along the chain: the entity's own, its type's and its type's type through `IsA`,
the room containing it, then the layout's on the zone entity. The first level that has the component supplies it, and
each later one fills the `std::optional` fields still unset, a struct member field by field; a field that is not
optional comes from the first level. It is empty where no level has the component. The room is the one containing the
cell the entity is placed in, the cell of the wall or flat cell it is sited on, or its parent's where it is sited on
its parent.

A layout entity's chain has no type, and is an overload for each address type. A room's and a flat's are themselves,
then the layout; a wall's is the wall, its cell's
room, then the layout; a flat's override for a room is itself, the room, the flat, then the layout; and a flat cell's is
the cell, the flat's override for its room, its room, the flat, then the layout. So at the room's level and at the
layout's, what the flat states wins over what the room or the layout does, as `discrete_layout.md` (`### Inheritance`)
specifies. The loader decodes each room's, wall's, flat's and flat override's component objects onto its entity.

A component that inherits registers each optional field's type with `register_optional<T>` (`properties.hpp`), which
describes `std::optional<T>` to flecs reflection as a `T` and sets its `OptionalField`: the reflection type of `T`, and
functions that set a value and return it, and return the value or null where it is unset. `merge_unset` fills the unset
optional fields of one value from another through that reflection, with no code per component, and the loader decodes
an optional field through it.

### ZoneEntryIndex

The criteria `World` trims its zones by: the zones entered longest ago, once there are more than `zone_count_max`. It
keeps a map from its count of entries to the persistent id of the zone entered then. `select_unloads(zones,
zone_entered)` drops the earlier entry of `zone_entered` and those of any zone no longer among `zones`, adds one for
`zone_entered` under the next count, then returns zones from the first entry on until the count left is within the
maximum. `zone_entered`, whose entry is the last, and a zone never entered, which has none, are never chosen, and two
trims in one deferred block choose no more than the excess.

`zone_of(e)` follows an entity's parents to the zone among them, or returns a null entity. Entities nest, as coins
in a pouch an actor carries do.

`reparent(e, parent, site)` sets an entity's parent and site together: taking something, putting it down and moving
it between zones are this one operation. `relocate` is `reparent` with a zone as the parent. The entity's children
move with it and its id is kept, so references to it stay valid, and the zone it left can unload without it.
`set_parent(e, parent)` sets the parent alone, for an entity with no site. A zone contains its entities through
`ChildOf`, and an entity its children through flecs' non-fragmenting `Parent`; `parent()`, `children()` and a
`ChildOf` query term work with both, and `has(ChildOf, ...)` reports a `Parent` child as well.

Occupancy is `entities_at` and `entities_within`, over the zone's children with no site address, sorted by id,
flecs iterating by archetype rather than id. `entities_at` takes a placement, compared with the layout's
`same_location`, or a cell position, through `DiscreteLayout`.

`move`, `neighbors` and `visible` are the layout's queries from an entity's zone and placement, with each wall
crossable as the entities sited on it allow. In `move`, every `Blocking` entity other than the one moving blocks. Topology and
occupancy are resolved in one sweep.

`Blocking` is the tag that makes an entity block movement.

### Persistent ids

`PersistentId` is an entity's identity in a world, a `std::uint64_t` (`PersistentId::type`) it keeps for as long as it
exists, through unloading and loading again. It compares and hashes as its value, so it is a map key and a reference's
id as it is. An entity gets one from `World::next_id` when it is placed in a zone: the observers on `ChildOf` and on
`Parent` give one to any entity in a zone that has none, whatever created it, a layout entity excepted. A zone, which has no parent, gets one from `create_zone`, and the player
from its constructor. A `World` is constructed with a reference to an `IdAllocator`, the source of its ids, whose one
function is `next`; `CountingIdAllocator` counts from the id it is constructed with. A world is saved with an id from
`next` and restored with an allocator constructed from it.

`entity(persistent_id)` is the loaded entity with a persistent id, or a null entity where none is loaded. The world
index is the zone of each entity something refers to. `make_reference(target)` returns an `EntityReference`, the
target's persistent id and runtime entity, and adds the target to the index. `zone_of(persistent_id)` is the persistent
id of the zone an entity in the index is in, loaded or not, and asserts the entity is in the index.
`resolve(reference)` returns the reference's entity, looked up again by persistent id when the one last resolved is no
longer alive, and a null entity where it is not loaded.

Observers keep both: setting a `PersistentId` adds the entity to the loaded entities, and removing it or deleting the
entity removes it, from the world index too unless the entity's zone, or the entity itself as a zone, is among the zones
unloading. A new parent moves the index entries of the
entity and everything below it to the parent's zone, so `relocate` and `reparent` keep the index current. The flecs
world, `m_entities`, is the last member of `World`, as its teardown runs the observers.

### Sites and doors

Where every entity is, and the entities sited on a wall or a flat cell. Doors are built; alcoves, basins, buttons and torches are planned
(`attachment_plan.md`, `site_plan.md`).

`Site` is where an entity is: a `Relation`, `In` or `On`; an optional address, a `WallAddress` of one wall id or a
`FlatCellAddress` of a flat id and a cell position; and a `placement_type` relative to the address, or to the parent
where there is none. An actor is `In` its zone with a placement and no address. An entity resting on a floor has a
placement and no address. `wall_id_of(site)` is the site's wall id, if it has one. `placement_of(e)` and
`set_placement(e, placement)` read and write the placement, leaving the rest of the site unchanged; `set_placement` on
an entity with no site gives it one.

A door is an entity with `Access`, so both of its sides are open or shut together, and a door facing child on each of
its walls, sited `On` a `WallAddress`, with no component of its own to be one. The walls are the sides a door has: in
the wall notation the two sides of one cell edge, in the block notation the walls the floor has toward the doorway
block. A door with one wall is a doorway into a dead end. The loader builds a door from the file's entities, the
`door` type giving it a closed `Access`, and a generator builds one the same way.

`Access` is a way through's state, `Open`, `Closed` or `Locked`, and nothing else: a door off its hinges is open, and
one smashed into rubble that blocks it closed. `access_entity(e)` is the entity whose `Access` applies to `e`, `e` or
the nearest of its parents with one, and `access(e)` its state, so a door facing has its door's. `Detection` is how
actors perceive an entity: a `DetectionState`, `Visible`, `Concealed` or `Revealed`, and the states of the actors
whose own differs, by persistent id. `detection(e, actor)` reads it from `e` or the nearest of its parents with one,
the actor's own state first; a null actor takes the general state, and an entity with none in its chain is visible.
One side of a secret door is concealed by a `Detection` on that door facing.

An access point is an entity a zone is entered at, sited `On` a surface --- a wall, or a floor or ceiling cell --- with
an `AccessPoint`: its `order` among the zone's access points of its type, from 0, and its `entry`, the placement an
arriving actor is given. Its type is a relationship, `(AccessType, <type>)`, so the access points of one type are a
query.

An access point type is an entity registered once in the world, under `access_types`, with an `Arrival` naming the
type an actor arrives at through a relocator of this type. `World` registers `up` and `down`, each arriving at the
other, and `waypoint` and `portal`, each arriving at its own type; a mod registers more with `create_access_type`.
`access_type(entities, name)` finds one by name, or returns a null entity.

`set_access_point(e, type, order, entry)` makes an entity sited on a surface an access point, as the loader does with
the file's; `create_access_point(zone, type, site, order, entry)` creates one. `entry_point(zone, type)` is the zone's
access point of the type with the lowest order, or a null entity where it has none.

A `Relocator` is where an entity moves an actor that enters it: its target, a destination name, or an
`EntityReference` to the relocator it is bound to.

`way_through(zone, wall_id)` is an entity sited on the wall that has access, found through the wall's entity and what
is attached to it, or a null entity. `crossable(zone, wall_id, actor)` is whether the actor gets through the wall: an
entity sited on it has open access and the actor does not find it concealed. A wall with nothing on it with access is
solid. `World::move`, `neighbors` and `visible` bind it to the entity moving or looking, so each actor crosses as it
perceives what is on the wall.

`Interact` opens the closed, or closes the open, access of the way through on the wall ahead, and reports
`Interaction::Opened`, `Closed` or `Nothing`; a locked one stays locked.

A wall is solid unless something sited on it makes it crossable, and a query given no predicate has every wall
solid. Every boundary between floor and solid, and every cell side on the border of the grid, has a wall, so where
there is no wall there is a cell beyond, which `crossable` asserts. A doorway block has walls toward solid cells and
the border of the grid, and none toward floor.

### Scheduler

Turn order in abstract units. Each actor accumulates energy at its own speed and acts once it has `action_cost`,
which is 100. `spend` charges it; `elapsed` is the units passed. Speeds and energies are ordered maps, so ties break
the same way every run.

`advance(units)` grants every actor the energy its speed earns over the units. `ready` is the actor that can act now,
the one with the most energy first and the lowest id among ties, or `nullid`. `next` advances to the moment the
soonest actor can act and returns it, which is the turn-based policy; a real-time caller converts elapsed time to units
and calls `advance`.

Granting four units to actors of speed 100 and 25 gives the fast one four actions' worth of energy while the slow one
reaches its first; `next` would have stopped at the first.

### Clock

Advances a `World` through its `Scheduler`: grants time, then runs each actor that became ready. `grant_turn` grants
enough for the soonest actor to become ready; `grant(units)` grants units, after which no actor need be ready.

The `Driver` chooses between them in `advance`, which a host calls. Turn-based skips ahead and repeats, returning only
when the controlled actor needs a command or no actor will ever act. Real time grants what the elapsed time earns and
returns whether or not an actor became ready. Turn-based ignores the elapsed time, so a host passes it either way.

`units_per_second` converts real time to units, one unit being one action for an actor of speed `action_cost`. The
clock keeps the fraction of a unit, so frames too short to earn one still add up. A host sets the driver
through `Session::set_driver`.

`run(events, actor_controlled)` runs each ready actor on its `Intent`, charging the action's cost, and returns the
controlled actor once it is ready with nothing to do, or `nullid` when no actor is ready. Any other actor with nothing
to do waits.

A move toward the facing direction asked for is one faced step. A facing direction other than the movement direction
is a separate action, the turn taken first.

Turn-based clears the intent after one action, so a key pressed once moves once. Real time keeps it, so a held
direction keeps the actor moving and a host writes an intent only when the input changes. Both clear an intent whose
action achieved nothing, which ends the pass; such an action costs no time, so walking into a wall is free.

Real time over a discrete layout makes the timing continuous and leaves the space discrete: an actor moves a whole cell
at a time, as often as the real time earns. Movement within a cell is a continuous layout.

### Intent

What an actor is trying to do, a component on the actor that persists until changed. `direction` is the movement
asked for and `facing` the facing direction asked for, each zero when none is, and `distance` is how far. Both are
absolute directions in world space, which a discrete layout snaps to its own. `Intent` registers no reflection, so it
is not serialized.

### ai

`think` sets an `Intent` for every actor with a behavior except the controlled one. It gathers the entities first, as
setting a component during iteration can move an entity to another table.

`Pursue` is the one behavior: move toward a target while it is visible, then toward where it was last seen, stopping
there if it finds nothing. The position last seen is stored on the entity, so it unloads, migrates and saves with it.
A `Pursue` with no `target_id` pursues the controlled actor `think` is given.

The move taken is to the reachable neighbor nearest the goal, if one is nearer than the current cell. `neighbors`
returns placements in a fixed order, so ties break the same way every run. Only topology is consulted, so a pursuer
is stopped by an occupied cell in `World::move`.

### StratumGame and Player

`StratumGame` is the one owner of a game's objects: a `CountingIdAllocator` counting from zero, the `World`, the
`Player` and the `Session`, constructed in that order, each referring to those before it. None of the four can be
copied or moved. The hosts each own one `StratumGame`.

`Player` is the entity a host controls, created with `Blocking`, a `Speed` and a `PersistentId`, and in no zone until
one is entered. It outlives every zone it is moved through.

### Session

The zone being played: references to the game's `World` and `Player`, and a `Scheduler`, a `Clock`, the zone, and the
player's pending one-shot command. A host supplies input mapping and drawing.

`create(generator, type)` and `load(filepath, type)` have the world make the zone, with `World::create_zone` and
`World::load_zone`, and enter it at the access point type. `load` rethrows a failure to enter with the file path. The
library has no default type; each host passes `up` from a temporary constant of its own, `entry_access_type`.

`Generator` is an interface in `stratum_sim`, implemented where a generator is chosen, so `stratum_sim` depends on no
generator. Each implementation declares its own parameters. `generate(zone)` fills an empty zone,
as the loader does: its `ZoneLayout` with its floor and ceiling, its `ZoneNotation`, its room and flat entities, and
its entities, each `IsA` its type, its doors and access points among them (`placement_plan.md`).

`enter(zone, type)` relocates the player to the entry of the zone's `entry_point` of the access point type, then has
the world `trim_zones` with the zone entered, so no zone the player is in can unload.
It makes the zone the one being played, clears the pending command,
and rebuilds the turn order from each entity in the zone with a `Speed`, the player among them. Where the zone has no
access point of the type it throws `std::runtime_error`, the player staying where it was and nothing left. `notation()` is the
entered zone's `ZoneNotation`.

`set_driver(driver, units_per_second)` sets the clock's driver, which lasts until it is set again; it is turn-based by
default. Stage 7 of `work_plan.md` covers the lifecycle.

`player()` is the player's entity. `set_intent` sets the controlled entity's movement and facing direction. `submit` leaves a one-shot command for it,
taken by the next `advance`.

`advance(events, elapsed)` calls `think`, then advances the clock, returning the controlled entity when it needs a
command, or `nullid` when no actor will ever be ready. A pending command is applied when the controlled entity is
ready, and the clock then advances again with no elapsed time, as that frame's time is already granted. `elapsed` is
the real time since the last call; a turn-based host omits it.

### Commands and events

`Command` and `Event` are type erased over `std::any`, so a module can add either. A command has the id of the entity
it acts for. `CommandQueue` and `EventQueue` are deques in submission and occurrence order.

The commands are `Step` (a direction, zero for the actor's facing direction; a distance; and `face`, to turn toward
the direction within the same action), `TurnTo` (an absolute facing direction) and `Interact`, which acts on the wall
`ahead`. They produce `Moved`, `Blocked`, `Turned` and `Interacted` events.

The clock builds `Step` and `TurnTo` from an actor's `Intent`. A host sends `Interact` as a command.

`apply(world, command, events)` returns the command's cost, zero when it did nothing; the `CommandQueue` overload
drains the queue. A move costs in proportion to the distance traveled against `step_distance`, so a move stopped short
is charged for the distance reached and a fraction of a cell costs a fraction of an action. A turn and an interaction
cost one action; a `TurnTo` to the current facing direction and an `Interact` with no wall ahead cost nothing.

## Zone file format

`quetzal::stratum_format` reads and writes the JSON zone file `discrete_layout.md` specifies, version `0.4`. It
depends on `quetzal_common` and nlohmann/json. `load_layout` fills a zone from what `read` returns, and both generators
write through it.

`grid.hpp` has the geometry the rows define --- `on_grid`, `floor_cell`, `floor_door_cell`, `stair_cell`, `has_wall`,
`room_cells` and `door_walls` --- used both by `read` to validate a file and by `load_layout` to build a layout.

`references.hpp` finds and translates the references a file writes, `{ "entity": id }`: `make_reference` writes one,
`reference_id` is the id where a value is one, `references` is every reference within an object of file components in
the order written, and `translate_references` replaces each by what a function returns for it.

`Document` is the file as data structures. The geometry is decoded into types --- the grid and its rows, expanded from
runs, the barriers, each room's perimeter or `RoomLocator`, wall addresses, each flat's heights --- and so are each
entity's `site`, `parent` and `relocator` file components, which the reader validates. A `Relocator` has its target,
a destination name or the id of the relocator it is bound to, and for an access point its type, order and entry, a
`discrete_directed_point_type` (`geometry::DiscreteDirectedPoint`): the cell position and orientation an arriving
actor is given. The layout's `access_points` are `Document::access_points`, a list of entity ids. The generator and
every other file
component are `values_type`, nlohmann's `ordered_json`, as the file wrote them. A room is kept as the file identified
it. `Notation` and its names are in `Notation.hpp` of their own, which is all `stratum_sim`'s headers include from the
format, so nlohmann reaches only the files that read or write a document. A saved zone's `zone.id` is `Document::zone_id`, which makes every id in the file persistent; it is empty in a zone
template.

`read` validates the file, throwing `std::runtime_error` with the path within the document and what was wrong there.
Beyond each section's structure it checks what the rows determine: in the wall notation, a wall between every floor
cell and a blank cell, a stair cell or the edge of the grid; a wall in the grid at every wall address, whose cell is a
floor/door cell; a room's `RoomLocator` cell on a floor cell; a perimeter within the grid and along cell edges that
never meets itself, each of its cell edges a wall toward a floor/door cell or a virtual wall between two; and no cell
in two rooms, compared only for rooms whose bounding boxes intersect.

It then checks the entities: ids whole, non-negative and unique; every entity sited on the geometry, a child or a
parent, with an entity sited on the geometry in every hierarchy, a parent for every entity sited on one, and no
`parent` leading back to its own entity; and, in a template, every reference an entity's id. A saved zone's references
may refer to another zone and are not checked. Each door glyph requires one entity with a child sited on each wall of
its doorway and on no other wall. `layout.components.access_points` must refer to at least one entity with a
`relocator` sited on a surface, each relocator with a `type`, an `order` counting from 0 among the access points of its
type, and an `entry` on a cell an actor can stand in. The stair glyphs are visual aids: a stair glyph with no
`relocator` on the walls around it, or a second one, and an access point on a wall that is not a stair cell's open
side, are warnings, passed to `read`'s warning callback, `log_warning` by default, which writes them to `std::clog`.
Keys the format does not define are not tracked.

`write` emits the written form: two-space indentation, a grid row per line, an entry per line with a flat's per-room
and per-cell entries each on a line of its own, and floats as the shortest digits that read back as the same value,
without an exponent. Top-level sections follow the format's key table; within an entry, its address or identification
comes first, then the rest in its section's table order, and file components stay in the order read. An entity's
`parent` and `site` are written into its `components` ahead of the rest. In runs, a count is written for a run of three
glyphs or more. A document `read` produces is written back equivalent to it.

## Presentation

Engine-neutral, depending on `stratum_sim` and no output device.

A **reduction** takes a world and a zone to a `Buffer<Place>` --- `cells` and `lattice` --- and a **mapping** takes
that to a `Buffer<T>` a device can emit --- `render` and `render_lattice`. The Godot tile view uses a reduction; the
terminal uses a mapping. There is no view interface over them.

`cells.hpp` declares the reductions, `ascii.hpp` the terminal's mapping, and `read_glyphs.hpp` the reader that fills a
mapping from a file. `Buffer.hpp` defines the grid both produce.

A position reduces to a `Place`: a `PlaceType`, its `State`, its `mask`, and whether an actor is on it. `PlaceType`
(`None`, `Solid`, `Floor`, `Wall`, `Door`) is the fixed structure, whatever its state, so a door that can be shut,
locked or concealed is one type. `None` is an edge with no wall or a corner no wall line reaches, which only `lattice`
produces. `actor` is a flag over the structure, so the floor under an actor is still there to draw.

`State` is a set of flags named after the component data they are read from: `Open` where the way through's `Access`
is open, and `Concealed` where its `Detection` conceals it from actors in general though not from the viewer, which a
glyph set may draw outlined. Only what changes the drawing is a state. `applies(type, state)` is whether a place type
can be in a state. The reduction is the only code that reads a component on behalf of a view.

Both reductions draw the zone as a viewer perceives it, the hosts passing the player: a way through the viewer finds
concealed is drawn as though there were none, a wall. A null viewer takes the general state.

`cells(world, zone, viewer)` is one `Place` per grid cell: `Solid`, `Floor`, or `Door` for a door occupying a cell, as a
doorway does in the block notation. A wall or door on an edge between two cells has no position there, so it reduces
to `Floor` on both sides.

`lattice(world, zone, viewer)` reduces onto the wall notation's lattice: `(2W+1)` by `(2H+1)`, cell `(i, j)` at
`(2i+1, 2j+1)`, the wall west of it at `(2i, 2j+1)`, the wall south of it at `(2i+1, 2j)`, and corners at the even
positions in both. It emits cells, `Wall` or `Door` on edges, and `Wall` at a corner any of whose four edges has a wall
or a door.

A door occupies the cell beyond it when that cell is open and has no wall back toward it, as in a doorway block, and
is then placed once in that cell; otherwise it is on the edge. Each wall is visited from the open cell it faces, which
reaches every wall.

`mask` is the set of directions in which the wall line continues from a position: a direction is set where a wall or a
door is on the edge or corner that way. A cell is never part of a wall line, so a door occupying one is a gap in it.
There are sixteen masks; a glyph set with fewer glyphs maps several of them to one.

A position is `Solid` when every cell it touches is closed --- a cell touches itself, an edge the two it lies between,
a corner the four meeting at it --- and positions beyond the grid count as closed. A wall faces open floor, so none is
at a solid position, which the reduction asserts.

Both reductions set the actor flags last, from `each_actor_cell`.

`Buffer<T>` is the grid, with `extent`, `contains`, `at`, `set` and `fill`, row 0 lowest as in the simulation. Its
position and extent are `stratum_view`'s discrete point and vector types, the same as the simulation's grid. A private
`offset` converts a position to a subscript and asserts the bounds. `Console` is `Buffer<char>`, and `Cells` and
`Lattice` are both `Buffer<Place>`. `to_string`, for characters only, writes the highest row first.

`difference(a, b)` is the runs in which `b` differs from `a`, each a `Span` of a position and a count along its row,
lowest row first; empty when they are the same, and the whole of `b` when they differ in size.

`render` takes cells to a `Console`, and an overload reduces a world's zone first, as a viewer perceives it. `render_lattice` does the same for
the lattice, named apart because `Cells` and `Lattice` are the same type. A mapping is one lookup per position.

A `Glyph` is one line of a glyph set: a place type, the state flags it requires, optionally a mask, whether it
requires an actor, and the character drawn. `Glyphs` returns the character of the most specific matching line, where
each state flag, a mask and an actor count one; the first among equals; and a space where none matches. A set with a
`door open` line draws an open door apart from a shut one, and a set with only `door` draws both alike.

`cell_glyphs()` covers the three types a cell can be, a door shut and open, and an actor on each type it can be on;
its actor lines come first, so an actor in an open doorway is drawn. `lattice_glyphs()` draws in the wall notation's
glyphs, floor a space, with a line for each of the sixteen wall masks.

`read_glyphs` reads a glyph set from a file; `assets/glyphs/` has the two built-in sets. The first line gives the
format and version. A glyph line is a place type, the states it requires in any order (`open`, `concealed`), `actor` if it requires an
actor, `mask=` and the directions it requires, then the character in single quotes, with `\\` and `\'` the escapes.
Omitting the states, `actor` or `mask=` matches any; `mask=` with no directions requires that no wall line reach the
position. A line beginning with `#` is a comment. It throws reporting the line, including for a state its type can
never be in. The shipped files are compared against the built-in sets.

## Terminal host

`Terminal` writes a `Console` to a character terminal, emitting only the cells changed since the last frame. It uses
the ECMA-48 escapes CUP, ED and EL, and the DEC private modes DECTCEM, to hide the cursor, and the alternate screen
buffer, with the xterm extension XTWINOPS to resize.

- It enters the alternate screen buffer on construction and leaves it on destruction, so the shell's screen and
  scrollback are untouched.
- On Windows it enables `ENABLE_VIRTUAL_TERMINAL_PROCESSING`, which conhost does not by default.
- `terminal_size` reads the text area and `set_terminal_size` requests one, in columns by rows. The request is
  `CSI 8 ; rows ; columns t`, which Windows Terminal honors and which keeps the scrollback.
- The window is grown on the first frame where the frame would not fit, and never shrunk. The original size is
  restored on exit only while the window is still the size it was set to, so a manual resize is kept.
- The pseudoconsole reports a new size before the terminal has resized, so a settling delay precedes the first paint
  and follows a detected resize.
- A frame is the map rows, a blank line, the status and the help. The help line has no newline, so a frame the
  height of the window does not scroll it.
- A frame that does not fit is replaced by the terminal's size and the size needed.

`read_key` returns one key without waiting for a newline: `_getch` on Windows, termios raw mode elsewhere. Arrow keys
become `Key::Up`, `Down`, `Left` or `Right`.

The loop advances the session to the actor it is waiting on, draws, reads a key, and submits it: movement and facing
direction as the actor's intent, a one-shot action as a command. Arrows or `hjkl` move, space interacts, `.` waits,
`^L` redraws, and `q` or Escape quits. The view does not show the facing direction, so a move also turns toward the
way it goes. The host charges a wait. `--script` plays a key sequence instead of the keyboard, and `--incremental`
uses the cursor-addressed output within a scripted run.

`--cell` and `--wall` choose the reduction drawn: a glyph per cell, or the lattice, which also draws walls and doors on
edges. Each uses its own glyph set. With neither, a block notation zone uses the cell view and a wall notation zone the
wall view; `Session::notation` reports which.

Zone files are in `assets/level/`. The host reaches them through `KML_DEV_ASSETS_ROOT`, a compile definition CMake
sets from the source tree root, so the working directory does not matter.

## Godot host

Godot owns `main()` and the frame loop, and loads the simulation as a shared library. `stratum_godot` builds that
library, a GDExtension, and `application/stratum_godot/project/` is the Godot project that loads it, through
`stratum.gdextension`, which refers to `res://bin/` where the build writes.

`StratumWorld` is the one registered class, a `RefCounted` owning a `StratumGame`, with Godot types only in its interface:
`default_zone_path`, `load_zone`, `extent`, `cell`, `render`, `cells`, `actors`, `step` and `turn`. `step` sets the
player's intent and advances, through the turn-based driver. A step ends with the next turn already granted, so
`turn` is the turn awaiting input. Exceptions do not cross into Godot: a failed load pushes the reason to Godot's error
list and returns false, and the previous zone stays intact.

`cells` is one code per cell from `stratum_view::cells`, row major from row zero; its `PlaceType` constants repeat
`stratum_view::PlaceType` for Godot. `actors` is one byte per cell in the same order, set where an actor is.

`game.tscn` is the tile view: a `TileMapLayer` fed from `cells`, a second layer for actors, a camera, and arrow or
`hjkl` keys. Its tileset is built at run time from flat colors. Row zero is the lowest in the simulation and y grows
downward in Godot, so the view inverts j when it paints.

`quetzal_godot` provides what any Godot host needs: `gdextension.cmake` and its `quetzal_configure_gdextension`, which
names the library as Godot expects and writes it into the project rather than a per-configuration subdirectory.

`CMAKE_MSVC_RUNTIME_LIBRARY` is forced into the cache by the root, as godot-cpp caches the static release CRT even for a
debug build and quetzal's static libraries then do not link. godot-cpp's headers are SYSTEM includes, as flecs' are, so
they do not report `/W4` warnings in the extension's sources. `KML_DEV_GODOT` governs all of it; configured off,
godot-cpp is not processed and neither the library nor the host exists.

Two headless checks need no display: `--path project --script main.gd` exercises the extension directly, and
`--path project --quit-after 5` runs the tile scene, which reports the cells it painted. A project has to be imported
once (`--headless --path project --import`) before Godot sees the extension; otherwise the class is undeclared, with
no indication why.

Visual Studio launches Godot on the project when the extension is run, so breakpoints in the extension are live. Godot
must not already have the project open, or the library is locked and the build fails.

## Tests

`stratum_sim_test` covers the layout hierarchy, layout structure, rooms, the grid, flats, blocking, the scheduler, the
clock, movement, adjacency and sight, loading, doors, the world, persistent ids, access points, residency, commands and occupancy. It and `stratum_view_test`
each check that every zone file in their `data/` has one `door` entity per door glyph, with a `door_facing` child on
each wall `stratum_format::door_walls` finds for that glyph and no door facing anywhere else.

`stratum_view_test` covers the console buffer, frame differencing, glyph sets and their files, the cell and lattice
reductions, both renderings and both round trips, and the reference frames: fixed command sequences rendered to text
and compared byte for byte by `FileValidator` against `test/stratum_view_test/reference/`. The sequences are played
against the two libraries, not through a host. Three scenarios are block notation zones drawn through the cell view.
Two use the wall view: `walls_door`, a wall notation zone, moves through a door, back, and into the wall two rooms
share, which blocks, then interacts with it; `door_cell`, a block notation zone, moves into a doorway, where the door
is drawn in its cell, and out the far side.

`dungen_test` and `catagen_test` compare reference zone files written now against those checked in, which must be
equivalent as parsed JSON, and check determinism, parameter validation, grid shape and that the reader accepts every
generated zone. Both check, for their scenarios and over a sweep of seeds, that each door glyph has one `door` entity
whose door facings are on that doorway's walls in the order `door_walls` returns them, and that no door facing is
anywhere else. `dungen_test` also reads back the rooms of zones with every door, half and none, over a sweep of seeds,
and checks each room contains exactly the cells generated as it, which fails where a barrier is missing at an open
doorway. Over another sweep with ranges of staircases each way, both check from the written zone that neither count
exceeds its maximum, that a count leading up falls short of its minimum only where no staircase leads down, that every
floor cell is reachable from every other with the stair blocks in place (maze1 excepted, which can leave corridor no
door reaches), that no staircase blocks a doorway while a cell beside it would not, that every staircase is sited on
exactly one of the walls around its block, leading `down` or `up` as its glyph indicates and opening onto a side with
floor beyond, or a door where there is none, and that `access_points` refers to every staircase. What the written zone
does not record about a stair cell, its space in dungen and which of its sides were open in catagen, they take from the
zone generated again. `catagen_test` also checks that every edge between a stair cell and floor is written as a wall.
Validation rejects a staircase up count whose minimum is zero, a staircase down alone included. Each suite also
reads a reference zone's generator parameters back and regenerates from them, a zone file having every parameter of
its run. A parameter the writer emits and the suite does not read fails, as does one the suite reads and the file lacks.

`tool/dungen` and `tool/catagen` build the drivers `dungen` and `catagen`, the library targets having the `quetzal_`
prefix and the suites the `_test` suffix.

`stratum_format_test` reads zone files and checks what they decode to, rejects every error the reader raises from a
valid file with one change made to it, checks that references are found and translated wherever they are written, and
writes each file through both `write` overloads and reads it back, comparing the two documents as maps.

`flecs_test` checks that the ECS supports zone partitioning, relationships, migration, unload, serialization and
deterministic iteration.

Each test runs from its own directory, its data and reference paths being relative.

No test executable covers a host. The terminal host is checked by running it with `--script` and reading the frames
back, and the Godot host by its two headless invocations.

## Build

CMake, with the solution at `build/kml_dev.slnx`. `/std:c++latest /bigobj /W4` throughout, with third-party sources
at `/W1` and flecs and godot-cpp included as SYSTEM. `quetzal_stratum_sim` links `quetzal_common`,
`quetzal_stratum_format` and `flecs_static`; `quetzal_stratum_view` links `quetzal_stratum_sim`; both hosts link
`quetzal_stratum_view`. `quetzal_stratum_format` links `quetzal_common` and `nlohmann_json`, vendored at
`library3/json` with its headers SYSTEM.

`KML_DEV_GODOT` governs the Godot targets and defaults on. Configured off, godot-cpp is not processed and neither
`quetzal_godot` nor `stratum_godot` exists.

`KML_DEV_ASSETS_ROOT` is a per-target compile definition giving the shared data root, so no source file has an
absolute path and no program depends on where it was started from. `KML_DEV_GODOT_VERSION` is the Godot release the
host targets, and `KML_DEV_GODOT_EXECUTABLE`, found from it, is what Visual Studio launches.

`application/stratum_godot` is added after `library3`, as configuring a GDExtension reads a property of the godot-cpp
target.

---

*Current stage: Phase 3, all but presentation tables. `Glyphs` is a table a file supplies; the Godot view's color
array is still hardcoded, and making it data requires asset ids, which is Phase 4's work.*
