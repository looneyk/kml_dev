# Game System --- Reference

This document describes what the system is and how it is put together, as implemented. The reasoning behind its
larger decisions is in `decision_rationale.md`.

The system has three layers, and none of them uses an engine. `quetzal::stratum_sim` simulates, `quetzal::stratum_view`
turns a simulation state into characters or place codes, and a host puts those on a device. `stratum_sim` links
`quetzal_common`, `quetzal_stratum_format` and flecs; `stratum_view` links `stratum_sim`; each host links
`stratum_view`. `quetzal::stratum_gen` adapts the generators to `stratum_sim`'s registries, so `stratum_sim` links no
generator.

The `stratum_` prefix names the layout class they share: open ground extruded vertically between one floor and one
ceiling.

| Directory | Target |
| --- | --- |
| `assets/` | shared data, owned by no host |
| `library/quetzal/stratum_sim/` | `quetzal_stratum_sim` |
| `library/quetzal/stratum_view/` | `quetzal_stratum_view` |
| `library/quetzal/stratum_format/` | `quetzal_stratum_format` --- the zone file format |
| `library/quetzal/stratum_gen/` | `quetzal_stratum_gen` --- the generators, adapted and registered by name |
| `application/stratum_terminal/` | `stratum_terminal` |
| `application/stratum_godot/` | `stratum_godot` |
| `library/quetzal/godot/` | `quetzal_godot` |
| `tool/dungen/`, `tool/catagen/` | `dungen`, `catagen` --- the generator drivers |
| `test/stratum_sim_test/`, `test/stratum_view_test/`, `test/stratum_format_test/`, `test/dungen_test/`, `test/catagen_test/`, `test/flecs_test/` | test executables |

## Simulation

A `World` contains zones. A zone is one independently loadable 3D coordinate space. Zones share no coordinate system,
so a placement is meaningful only within its own zone, and an entity's zone is the zone among its ancestors. Each zone
has a layout, which is its geometry and topology, and the entities in it.

A caller asks the layout where a move ends; it does not compute a destination and hand it back.

### Types

`value_type` is `float` and `traits_type` is `math::CartesianTraits<value_type, 3>`, which gives `point_type`,
`vector_type`, `points_type` and `polygon_type`. `placement_type` is `geometry::DirectedPoint<traits_type>`, a position
and a facing direction.

`coord_type` is `std::int32_t`. `discrete_vector_type` is `math::DiscreteVector<coord_type, 2>`, a displacement on
the grid, and `discrete_point_type` is `geometry::DiscretePoint<coord_type, 2>`, a cell position. A cell position is
moved by adding a `discrete_vector_type`, the difference of two cell positions is the displacement between them, and
adding two does not compile. `cardinal_directions` are the four unit vectors `East`, `North`, `West` and `South`, in
`CardinalDirection` order, and `cardinal_direction(d)` returns one of them.

`point_projection_type` and `vector_projection_type` are `geometry::MinProjection` along the z axis onto the xy plane.
`grid_step(vector)` returns the cardinal direction nearest a direction.

### Layout

`Layout` is an interface to one zone's geometry and topology. Its queries --- `move`, `turn`, `face`,
`same_location`, `neighbors`, `visible`, `ahead` and `step_distance` --- are all pure virtual, and none of them
mentions a cell or a wall. `Rotation` (`CCW`, `CW`, `Half`), `MoveResult`, `crossable_function_type` and
`blocker_function_type` are declared with it.

A second, non-virtual `move` takes neither predicate, so no wall is crossable and no entity blocks. A derived layout
brings it back into scope with `using Layout::move`.

The destructor is public and virtual, since a zone owns its layout through a `std::unique_ptr<Layout>`. The other
special members are protected and defaulted, so a concrete layout copies and moves as itself and cannot be sliced
through the base.

`layout_as<L>(layout)` returns the layout as its concrete class, through a cast that asserts the class. The loader, the
view reductions, `Session`'s initial placement, `World::entities_at` by cell position and the host status readouts use
it. `commands.cpp` refers to no cell and no concrete class.

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

`DiscreteLayout` is the static topology of one zone --- its walls, flats, and rooms --- stored as a table of cells over
the grid. Entities refer into it by id.

| Type | |
| --- | --- |
| `Cell` | One square of the grid. `open` is true if an actor may stand there, `walls` is a `DirectionMask` of the sides that have a wall facing into the cell, and `room_id` is the room that contains the cell, or `nullid`. |
| `DirectionMask` | A set of the four cardinal directions, one bit each in `CardinalDirection` order, with `direction_mask(direction)`. The view uses it for the directions in which a wall line continues. |
| `Flat` | The floor or the ceiling, which differ only by `ceiling`. Its height is given for the whole flat, per room (`room_height`) and per cell (`cell_height`), each overriding the one before; the per-room and per-cell tables are sparse. |
| `GridMapping` | `origin`, the corner of cell `(0, 0)`; `scale`; and `extent`, the cell count along each axis as a `discrete_vector_type`. It has `contains(cell_position)`, `cell_position_at(point)` and `position(cell_position)`, which returns the cell center. |
| `MoveResult` | The placement reached, possibly short of the distance asked for, and the `blocking_wall_id` or `blocking_entity_id` that stopped it. Only one of them is set, and both are `nullid` if nothing stopped it. Declared in `Layout.hpp`. |

A layout is constructed with its origin, scale and extent, and sizes its cell table from them. Cells run from `(0, 0)`
to the extent. `GridMapping::contains` is used in assertions only: the loader guarantees a walled boundary, so a move
off the grid cannot happen. The cell accessors assert that the cell position is on the grid; `cell_position_at` floors
any point to a cell position without a bounds check.

A wall is one side of one cell edge, and the two sides of an edge are independent. A wall notation wall between two
rooms is a wall on each side, and a doorway block's wall toward floor is on the floor's side alone. An opening is the
absence of a wall.

A zone with no height data loads a constant floor into the same class; only the data a game style loads differs. The
layout makes no provision for continuous movement.

Walls are set per cell with `set_cell_walls` and read with `walls`. The room count is read with `room_count` and set
with `set_room_count`, and a cell's room is read with `room_at` and set with `set_cell_room`; everything else about a
room belongs to its entity. Openness is read with `open` and set with `set_cell_open`, separately from the room, which
has no effect on passage.

`wall_toward(cell_position, direction)` returns the id of the wall on that side of the cell, or `nullid` if there is
none; it is a bit test. The id encodes the cell's slot and the direction, which `wall_cell_position(wall_id)` and
`wall_direction(wall_id)` decode. `crossable(cell_position, direction, is_wall_crossable)` is true if there is no wall,
and otherwise returns the result of `is_wall_crossable` for the wall. Both read only the one cell, so a cell on the
boundary returns the wall that closes the layout.

`set_flats(height_floor, height_ceiling)` creates the two flats, once; `floor_id` and `ceiling_id` identify them.
`height(flat_id, cell_position)` returns the cell's own height if it has one, otherwise its room's, and otherwise the
flat's. `position(cell_position)` returns the cell center with z at the floor height there, or on the grid's plane
before the flats are set.

`turn` is relative and `face` absolute. A view with a relative turn asks `turn` for the resulting direction and sends
it as a facing direction, so the layout does the quantizing in both cases. `face` snaps to a cardinal direction and
leaves the facing direction unchanged for a zero direction.

`move` snaps the direction with `grid_step`, rounds the distance to whole cells, and moves cell by cell until
`crossable` rejects a wall or the cell ahead is occupied. It then sets the position from the cell reached, so rounding
error never accumulates. `blocker_at` returns the entity occupying a placement; the layout calls it before entering
each cell and reports the result as `blocking_entity_id`. `World::move` supplies it.

`step_distance` returns the grid scale, the distance one action of movement travels.

`neighbors` returns the placements reachable in one move. `visible` walks the cells that the ray between two
placements passes through, one edge at a time, and fails at the first wall that cannot be crossed. `field_of_view` is
declared and not implemented. `ahead(placement)` returns the wall of the placement's cell in its facing direction,
snapped to a cardinal direction, or `nullid` if there is none.

### load_layout

`load_layout(zone, document)` fills an empty zone from a `Document` that `stratum_format::read` returned. It builds the
layout through the same `stratum_format` functions that validated it. Each floor/door cell is open, with a wall on
each side for which `has_wall` is true. Each room's cells, from `room_cells`, get its id, and `room_count` is the
number of rooms. The flats get the file's heights, the floor defaulting to 0 and the ceiling to 1. A stair cell is a
block, closed like solid, and the floor beside it is walled toward it.

A doorway block is a cell an actor may stand in, walled toward solid on its own side and toward floor on the floor's
side alone. Two areas therefore never share an edge in the block notation. In the wall notation, two rooms that share
a wall are walled from both sides, which the block notation cannot express, so a block layout is a subset of a wall
layout.

It sets the zone's `ZoneLayout` to the `DiscreteLayout`, since both notations are grids of cells, and its
`ZoneNotation` to the notation the rows were written in, and its `ZoneDocument` to a copy of the document without its
entities, whose geometry a saved zone is written with. It then builds every entity in the file in two passes, in the
file's order. The first pass creates each entity in the zone, `IsA` its type, so that each has its persistent id ---
the file's own in a saved zone --- before anything refers to it. A type that is not loaded is made with no
components, so the entity keeps its type. The second sets each entity's parent; its site, with the address resolved to
layout ids and the position and orientation to a placement, and a site in a cell of the zone made a placement at that
cell with no address; and its `Relocator`. A door is the file's `door` entity, and its door facings are the `door_facing` children sited on the
walls of its doorway. Each entity that `layout.components.access_points` refers to is made an access point, with the
registered access point type that its relocator names, its `order`, and its `placement`.

Last, it decodes each component object --- of the layout, of each room, wall, flat and flat override, and of each
entity --- into the component registered under its name (`register_file_component`), and sets it on the zone's
entity, the layout entity, or the entity. The decoder walks the object's parsed values and sets each field through
flecs' reflection cursor, so no JSON reaches flecs. A component that inherits is decoded into a new value, so the
entity's own value has only the fields its object sets, and the chain supplies the rest. Any other component is
decoded over the entity's current value, which is a copy of its type's value if it has a type. A `std::optional` field
is set through its `OptionalField`. A reference in a component object becomes an `EntityReference`. In a template it
refers to the entity of the same file, with that entity's persistent id; in a saved zone it has the persistent id
written, and the entity if that entity is in the zone. `load_layout` returns the entities that a template's references
refer to, which `World::load_zone` adds to the world index.

`build_error(world, document)` returns the first reason the world cannot build a zone from a document, with the path
to it. The reason is an access point type that is not registered, or a component object that has no component
registered under its name, has a field the component does not have, or has a value its field cannot take. It decodes
every component object into a value that it then discards, with flecs' own error logging silenced, so
`World::load_zone` checks a file completely before making anything. `load_layout` asserts the conditions that
`build_error` checks.

`register_file_component(entities, name, component)`, or `register_file_component<T>(entities, name)`, registers a
component, with its reflection as a struct or an opaque type, as the one that a component object of that name decodes
into. A tag is registered too, and a file writes it as an empty object. The name is an entity under `components`.
`file_component(entities, name)` returns that component, or 0 if none is registered; `file_components(entities)`
returns every registered one with its name, in name order; and `is_tag` returns true for a component with no value.
`register_vector<T>` reflects a `std::vector<T>` as a list, which a file writes as an array.

`World` registers `name` as `DisplayName`, what an entity is called for a player to read, such as a zone's
`layout.components.name`. It identifies nothing. `register_display_name` reflects it as a string, so the file's
`"name": "..."` decodes into it. `World` also registers the state of an entity that a saved zone writes back: `access`
(`Access`), `detection` (`Detection`), `speed` (`Speed`), `pursue` (`Pursue`), and the tags `blocking` (`Blocking`)
and `dead` (`Dead`).

`make_document(zone)` returns a loaded zone as a saved zone's `Document`, the reverse of `load_layout`. The geometry is
its `ZoneDocument`'s, which does not change during play, and the rest is the zone as it is now: its persistent id and
level; the component objects of the layout and of each room, wall, flat and flat override, each of its own file
components, with a wall or override that gained components during play added; its access points; and every entity in
its subtree but the layout entities, in persistent id order. The character, which the game owns, is in no zone's
subtree. An entity's file components are those it owns, so one it reads from its type through inheritance is the type's.
Each value is encoded through flecs reflection: an unset optional field is left out, an `EntityReference` is written as a
reference to its persistent id, and a struct field by field. A site with no address on an entity in the zone is written
in a cell of the zone, at the cell its placement is in. A bound relocator's target is the persistent id of the relocator
it is bound to. `rebuild_document(zone)`, which would rebuild the geometry from the layout as well, for a zone that has
no `ZoneDocument`, is a skeleton with an empty body.

### Generation

`Generation` has the registries that `World` creates regions and zones from (`world_generation_plan.md`), each a map
from a name: `m_generator_selectors`, `m_generators` and `m_region_specs`. `RegionSpec` and `ZoneSpec` are both
`KeyValueStore`. A `RegionSpec`'s `generator_selector` names the region's selector, and a `ZoneSpec`'s `level` is the
zone's level; `generator_selector_key` and `level_key` are the two keys.

`specs.hpp` has two functions over a spec. `find_unknown_keys(spec, known_keys)` returns the spec's keys that are not
among the known keys, in key order, so a consumer reports a key it does not read. `select_prefixed(spec, prefix)` returns
the entries whose keys begin with the prefix, with the prefix removed, so one part of a spec, such as its `zone.` keys,
becomes a spec of its own.

A generator selector, `generator_selector_type`, is given the world, the region and the `ZoneSpec`, and returns a
`GeneratorSelection`, which sets exactly one of a generator's name, a zone template's tag, and a region spec's name,
with the `ZoneSpec` the zone is created from. A generator, `generator_type`, returns the `stratum_format::Document` a
zone is built from, and reads nothing from the world. `Generation.hpp` declares `Document` without including its
header, so nlohmann reaches no file that includes `World.hpp`.

### World

`World` contains the zones and everything in them. One flecs world contains the entities, and a zone is an entity
whose contents are its children, so migration and unloading are subtree operations, and migration keeps each entity's
id. A zone is an entity with the `Zone` tag, a `ZoneLayout` component that owns its layout as a
`std::unique_ptr<Layout>`, and a `ZoneNotation`. Deleting the entity deletes the layout. There is one `World` in a
game, owned by `Game`, and it cannot be copied or moved.

A region groups contiguous zones: a surface area, a dungeon, a crypt, a tower or a house (`region_plan.md`). It is an
entity with a `Region` component and a `PersistentId`, and it is the parent of its zones. It has no parent of its own.
`Region` has the region's `RegionSpec` and `zone_count`, the number of zones created in
it, which counts zones that have since unloaded. A zone is counted when it is generated, loaded from a template, or
created with a layout. A saved zone read back was counted when it was created, so it is not counted again. `create_region(region_spec)` makes one
with the spec. `create_region_zone(region_spec, zone_spec)` makes a region, then its
first zone with `create_zone`, and returns the zone.
`region_of(zone)` returns a zone's region, its parent. `zone_of` is unchanged by the regions above a zone: it returns a
 null entity for a zone itself. `unload_region(region)` unloads every zone of the region, then deletes the region, which deletes all their zones; each zone's unload
completes as `unload_zone`'s does.

Every zone is made in a region. `create_zone(region, zone_id)` makes an empty zone in the region, with the `Zone` tag
and that `PersistentId`, for a generator or the loader to fill. `create_zone(region, layout)` makes one with a new id,
that layout and nothing else, and asserts that it is given a layout. A zone may have a `DisplayName`, but each is identified by its persistent id. `zone_count` and `layout` access the zones, with
`layout` returning the interface; `create_entity`, `move` and `relocate` operate on their contents; and `unload_zone`
discards a zone, its contents and its layout.

`World` takes the game's `Generation` by reference in its constructor (`### Generation`). `create_zone(region,
zone_spec)` creates a zone in a region that has a spec, as the region's generator selector chooses. The selector, named
by the spec's `generator_selector`, is given the world, the region and the `ZoneSpec`, and returns a
`GeneratorSelection`. For a generator, `create_zone` calls it with the selection's `ZoneSpec`, and builds the zone from
the `Document` it returns as `load_zone` builds one from a file; it then sets the zone's `ZoneLevel` from the spec's
`level`, if it has one. For a region spec, it returns `create_region_zone` with that spec and the `ZoneSpec` it was
given, so the zone is the first of a new region. A zone template is not built: it throws `std::runtime_error`. So does a
selector, generator or region spec name that is not registered, and a spec with no `generator_selector`, before anything
is created; what a generator throws passes on, with nothing created.

A zone's level is its `ZoneLevel`, a `level_type` (`std::int32_t`): 0 at the surface, negative going down and positive
going up. It is a label, used when making connections between zones, and nothing is looked up by it. The loader sets it
from the file's `level`, and a zone whose file has none has no `ZoneLevel`.

`resolve_relocator_target(relocator)` returns the access point at which an actor arrives through a relocator whose
unbound target is a level with no position: relative, `+`*k* or `-`*k*, or absolute, `=`*n*. The relocator's zone must
have a level, *L*; a named zone and a position are not resolved. The target level is *L* + *k*, or *n*, which is *k*
levels from *L*.

A target with a region creates a new region with `create_region_zone`, from the region spec of that name and a
`ZoneSpec` of the target level, without searching the loaded zones. A region spec that is not registered throws `std::runtime_error`.

With the region omitted, it looks in the loaded zones of the relocator's region at the target level, other than the
relocator's own, for an unbound access point of the type at which the relocator's type arrives that leads back: one
whose relocator targets level *L*, -*k* levels from its own zone (`targets_relocator_level`). Of several, it returns the
one in the zone with the lowest persistent id, and then with the lowest order. If none is found, it makes a zone in the
relocator's region with `create_zone(region, zone_spec)`, with a `ZoneSpec` of the target level.

It returns the new zone's `entry_point` for the arrival type and the offset -*k*, the access point that leads back, or
a null entity if the new zone has no such access point. It binds nothing.

`relocate_through(actor, relocator)` moves the actor through the relocator and returns the access point it arrives at.
A bound target is resolved to the relocator it refers to. An unbound target is resolved by `resolve_relocator_target`,
and the relocator and the access point arrived at are then bound to each other, each target becoming an
`EntityReference` to the other, so going back arrives at the relocator gone through, and `entry_point` no longer
chooses either. The actor is relocated to the access point's zone, at its `placement`. It returns a null entity and
moves nothing if a bound target's relocator is not loaded, or an unbound target has a named zone or a position, or
resolves to nothing.

`World` takes the game's random engine by reference in its constructor, as it takes the id allocator: a `random_type`,
`math::Random<>`, which `Game` owns.

`load_zone(region, filepath)` reads the file and builds the zone from its document, which it checks with `build_error` before making the zone. The zone has the file's persistent id if the file has
a `zone` section, and a new one otherwise. `load_zone` then has `load_layout` fill the zone and populates it. It throws
`std::runtime_error`, having made nothing, if the reader rejects the file, `build_error` finds a reason, or the file is
a saved zone that is already loaded; a template makes a new zone each time it is loaded. `World` handles no doors itself, and neither function unloads anything.

`populate(zone)` places the zone's denizens: one pursuer at the last open cell in reverse row order, facing west,
`Blocking`, with a `Speed` and a `Pursue` that has no target.

`Speed` is the rate at which an actor gains energy in the turn order.

`zones()` returns the world's zones in id order, queried on each call, and `zone_count` returns how many there are. A
zone that has begun unloading is excluded, even if flecs has deferred its deletion.

`trim_zones(zone_entered)` has the world's `ZoneEntryIndex` mark `zone_entered` as the most recently entered zone and
choose the zones to unload, then unloads them. `zone_count_max`, a constructor argument, is the maximum number of zones
the world trims to, and `set_zone_count_max` changes it. `Game` passes its config's `zone_count_max`.

`unload_zone(zone)` begins an unload: it adds the zone's id to the zones unloading and deletes the entity. Flecs deletes
a zone's subtree before the zone itself, and if it is deferring, it runs the whole deletion when the deferred
operations run. The unload is complete in the `PersistentId` observer on the zone's own removal, which takes the zone
out of the zones unloading. An unload is therefore correct whether the deletion runs at once or later.

An entity with no parent is owned by the game rather than a zone; the character is one. The exclusive `(InZone, zone)`
relationship refers to the zone the entity is in, and flecs removes it when that zone is deleted, so unloading the
zone leaves the entity, and everything below it, in no zone. `with_in_zone(builder, zone)` adds the terms that match
the entities in a zone: its children, and the game's entities that are in it. Every query for a zone's entities uses
it: occupancy, the turn order, and the view's actors.

`zone_of(e)` follows an entity's parents to the zone among them, or to the zone of the game's entity among them, and
returns a null entity if there is neither. Entities nest, like coins in a pouch that an actor carries.

`reparent(e, parent, site)` sets an entity's parent and site together: taking something, putting it down and moving
it between zones are all this one operation. `relocate(e, zone, placement)` is `reparent` with the zone as the parent
for an entity that has a parent; for an entity owned by the game, it sets its `InZone` and site, so it stays the
game's. The entity's children move with it and its id is kept, so references to it stay valid, and the zone it left
can unload without it. Both assert that nothing below an entity moving to another zone is sited on a wall or a flat
cell, since those addresses belong to the zone it left. `set_parent(e, parent)` sets only the parent, for an entity that has no site. A zone contains
its entities through `ChildOf`, and an entity contains its children through flecs' non-fragmenting `Parent`.
`parent()`, `children()` and a `ChildOf` query term work with both, and `has(ChildOf, ...)` is true for a `Parent`
child as well.

Occupancy is `entities_at` and `entities_within`, over the entities in the zone that have no site address, sorted by
id, since flecs iterates by archetype rather than by id. `entities_at` takes either a placement, compared with the
layout's `same_location`, or a cell position, through `DiscreteLayout`.

`move`, `neighbors` and `visible` run the layout's queries from an entity's zone and placement. A wall is crossable if
the entities sited on it let that entity through. In `move`, every `Blocking` entity other than the one moving blocks
the way. Topology and occupancy are resolved in one sweep.

`Blocking` is the tag for an entity that blocks movement.

### Entity types, layout entities and properties

A type is a prefab under `types`. `create_entity_type(entities, name)` makes one, and `entity_type(entities, name)`
finds one, returning a null entity if none is loaded; the caller sets the type's components. An entity that `IsA` its
type reads each component that inherits --- one with optional fields --- from the type, with nothing copied, because
`register_file_component` registers such a component with flecs' `(OnInstantiate, Inherit)` through
`register_inheritance`. Flecs copies any other component onto the entity when it becomes an instance, so each door has
its own `Access`. `register_game_types(world)`, declared beside `Game`, which calls it, registers the game's
types until they load from data: `door`, with a closed `Access`. The suites call it too.

A layout entity is the entity of a room, a wall, a flat, a flat's override for a room, or a flat cell. Its address is
its component, and it is found by that address: a `RoomAddress`, `WallAddress`, `FlatAddress`, `FlatRoomAddress` or
`FlatCellAddress`, the five types that `for_each_layout_address` visits. It has no persistent id. A flat's override for a
room or a cell is a child of the flat's entity through `Parent`, and every other layout entity is a child of its zone.
The loader gives every room and each flat an entity. A wall, a flat's override for a room, and a flat cell have one
only while they have components of their own or something sited on them.

`layout_entities.hpp` has templates over the address type that take the zone. `ensure_layout_entity(zone, address)`
makes a layout entity once, and `layout_entity(zone, address)` finds it, returning a null entity if there is none.
`room_entity`, `wall_entity`, `flat_entity`, `flat_room_entity` and `flat_cell_entity`, and an `ensure_` form of each,
do the same by id. A layout entity has the `LayoutEntity` tag, which `ensure_layout_entity` adds; the observer that gives
entities in a zone their persistent id skips an entity with it. For each address type the zone has
an `AddressIndex<Address>` component, a map from address to entity. `for_each_layout_address` calls a function with
each address type, through a fold over the five types, and `World` uses it to register the address and index components
and its observers for all five.

A wall's or flat cell's entity lists the entities sited on it in `AttachedEntities`, kept current by observers on
`Site`. Setting a site with an address creates the layout entity if there is none and lists the sited entity once;
removing the site takes the entity off the list. The wall's or cell's entity is deleted when its list is empty and it
has no components of its own. Deleting a layout entity removes the sites of the entities on it. `reparent` removes
the site before changing the parent, so an entity leaves what it was sited on while it is still in that zone. Deleting
a zone or the world unlinks nothing.

`property<T>(e)` resolves a component along a chain: the entity's own, its type's and its type's type's through
`IsA`, that of the room that contains it, and last the layout's, on the zone entity. The first level that has the
component supplies it, and each later level fills the `std::optional` fields that are still unset, a struct member
field by field; a field that is not optional comes from the first level. The result is empty if no level has the
component. The room is the one that contains the cell the entity is placed in, or the cell of the wall or flat cell it
is sited on, or, if it is sited on its parent, its parent's room.

A layout entity's chain has no type, and there is an overload for each address type. A room's chain and a flat's chain
are the entity itself, then the layout. A wall's chain is the wall, its cell's room, then the layout. A flat's override
for a room has the override, the room, the flat, then the layout, and a flat cell's chain is the cell, the flat's
override for its room, its room, the flat, then the layout. So at the room level and at the layout level, the flat's
components win over the room's or the layout's, as `discrete_layout.md` (`### Inheritance`) specifies. The loader
decodes the component objects of each room, wall, flat and flat override onto its entity.

A component that inherits registers the type of each optional field with `register_optional<T>` (`properties.hpp`).
It describes `std::optional<T>` to flecs reflection as a `T` and sets its `OptionalField`: the reflection type of `T`,
a function that sets a value and returns it, and a function that returns the value, or null if it is unset.
`merge_unset` fills the unset optional fields of one value from another through that reflection, with no code per
component, and the loader decodes an optional field through it.

### ZoneEntryIndex

`ZoneEntryIndex` chooses the zones that `World` unloads when it trims: the zones entered longest ago, once there are
more than `zone_count_max`. It keeps a map from its count of entries to the persistent id of the zone entered at that
count. `select_unloads(zones, zone_entered)` drops the earlier entry of `zone_entered` and the entries of any zone no
longer among `zones`, adds one for `zone_entered` under the next count, and then returns zones from the first entry on
until the count left is within the maximum. It never chooses `zone_entered`, whose entry is the last, or a zone that
was never entered, which has no entry, and two trims in one deferred block choose no more than the excess. `clear`
forgets every entry and keeps the maximum.

### Persistent ids

`PersistentId` is an entity's identity in a world, a `std::uint64_t` (`PersistentId::type`) that it keeps for as long
as it exists, through unloading and loading again. It compares and hashes as its value, so it is used directly as a
map key and as a reference's id. An entity gets one from `World::next_id` when it is placed in a zone: the observers on
`ChildOf`, `Parent` and `InZone` give one to any entity in a zone that has none, whatever created it, except a layout
entity. A zone, which has no parent, gets one from `create_zone`, and the character gets one from `Character::create`. A
`World` is constructed with a reference to an `IdAllocator`, the source of its ids, whose one function is `next`.
`CountingIdAllocator` counts up from the id it is constructed with. A world is saved with an id from `next` and
restored with an allocator constructed from that id.

`entity(persistent_id)` returns the loaded entity with a persistent id, or a null entity if none is loaded. The world
index stores the zone of each entity that something refers to. `make_reference(target)` returns an `EntityReference`,
with the target's persistent id and runtime entity, and adds the target to the index. `zone_of(persistent_id)` returns
the persistent id of the zone that an entity in the index is in, whether or not it is loaded, and asserts that the
entity is in the index. `resolve(reference)` returns the reference's entity, looking it up again by persistent id if
the one last resolved is no longer alive, and returns a null entity if it is not loaded.

Observers keep both current. Setting a `PersistentId` adds the entity to the loaded entities, and removing it or
deleting the entity removes it from them. It also removes the entity from the world index, unless the entity's zone,
or the entity itself as a zone, is being unloaded. A new parent or `InZone` moves the index entries of the entity and
everything below it to the new zone, so `relocate` and `reparent` keep the index current. The flecs world,
`m_entities`, is the last member of `World`, because its teardown runs the observers.

### Sites and doors

This section covers where every entity is, and the entities sited on walls and flat cells.

`Site` is where an entity is: a `Relation`, `In` or `On`; an optional address, which is a `WallAddress` of one wall id
or a `FlatCellAddress` of a flat id and a cell position; and a `placement_type` relative to the address, or to the
parent if there is no address. An actor is `In` its zone with a placement and no address, and so is an entity resting
on a floor. `wall_id_of(site)` returns the site's wall id, if it has one. `placement_of(e)` and
`set_placement(e, placement)` read and write the placement and leave the rest of the site unchanged; `set_placement`
on an entity with no site gives it one.

A door is an entity with `Access`, so both of its sides are open or shut together. It has a door facing child on each
of its walls, sited `On` a `WallAddress`; no component marks a door facing as one. A door's walls are its sides: in
the wall notation, the two sides of one cell edge, and in the block notation, the walls the floor has toward the
doorway block. A door with one wall is a doorway into a dead end. The loader builds a door from the file's entities,
with the `door` type giving it a closed `Access`, and a generator builds one the same way.

`Access` is the state of a way through --- `Open`, `Closed` or `Locked` --- and nothing else. A door off its hinges is
open, and one smashed into rubble that blocks the doorway is closed. `access_entity(e)` returns the entity whose
`Access` applies to `e`: `e` itself, or its nearest ancestor that has one. `access(e)` returns that state, so a door
facing has its door's state. `Detection` is how actors perceive an entity: a general `DetectionState` (`Visible`,
`Concealed` or `Revealed`) and a list of `ActorDetection`, each an actor's persistent id and its own state, for each
actor whose own state differs.
`detection(e, actor)` reads it from `e` or its nearest ancestor that has one, and returns the actor's own state if it
has one and the general state otherwise. A null actor gets the general state, and an entity with no `Detection` in its
chain is visible. One side of a secret door is concealed by a `Detection` on that door facing.

An access point is an entity at which a zone is entered, sited `On` a surface --- a wall, or a floor or ceiling cell ---
with an `AccessPoint`. The `AccessPoint` has its `order` among the zone's access points of its type, counting from 0,
and its `placement`, the placement given to an arriving actor. Its type is a relationship, `(AccessType, <type>)`, so
the access points of one type are found by a query.

An access point type is an entity registered once in the world, under `access_types`, with an `Arrival` that refers
to the type at which an actor arrives through a relocator of this type. `World` registers `staircase`, `door`,
`portal`, and `waypoint`, each arriving at its own type. The type is what the access point is; whether it goes up,
down, or across is in its relocator's target. A mod registers more types with `create_access_type`. `access_type(entities, name)` finds one by name, returning a null
entity if there is none.

`set_access_point(e, type, order, placement)` makes an entity sited on a surface an access point, as the loader does
with the file's access points, and `create_access_point(zone, type, site, order, placement)` creates one.
`targets_relocator_level(e, offset)` returns true if the entity's relocator is unbound and its target is the level
`offset` levels from the entity's zone: a relative target with that offset, or an absolute target at the zone's level
plus the offset, which only a zone with a level matches. `entry_point(zone, type, offset)` returns the zone's
lowest-ordered access point of the type whose relocator targets the level `offset` levels away, or a null entity if the
zone has none. A staircase leading up is `entry_point(zone, staircase, 1)`, and an actor arriving from another level
passes the offset that leads back.

A `Relocator` has the target to which an entity moves an actor that enters it: a `stratum_format::RelocatorTarget`,
unbound, or an `EntityReference` to the relocator it is bound to. A door facing may have a relocator and be an access
point.

`way_through(zone, wall_id)` returns an entity sited on the wall that has access, found through the wall's entity and
its attached entities, or a null entity if there is none. `crossable(zone, wall_id, actor)` returns true if the actor
can get through the wall: an entity sited on it has open access, and the actor does not find it concealed. A wall with
no such entity is solid. `World::move`, `neighbors` and `visible` bind it to the entity moving or looking, so each
actor crosses a wall as it perceives the entities on it.

`Interact` opens the access of the way through on the wall ahead if it is closed, or closes it if it is open, and
reports `Interaction::Opened`, `Closed` or `Nothing`. A locked way through stays locked. If the wall ahead has a
relocator on it that the controlled actor `can_enter`, `Interact` goes through it instead, and reports `Interaction::NoDestination` if it leads nowhere the world can resolve. `Interacted` has the entity acted
on, the way through or the relocator, and `access_type_name(e)` returns the name of an access point's type, such as
`staircase`, for the hosts' messages.

`find_relocator(zone, address)` returns an entity with a `Relocator` sited on a wall or a flat cell, found through
the layout entity's attached entities, or a null entity if there is none. `can_enter(e, actor)` returns true if the
actor can go through the entity's relocator: the entity's access, if it has any, is open, and the actor does not find
it concealed. A staircase has no `Access`, so it can always be entered, and a door with a relocator only while it is
open.

A wall is solid unless something sited on it makes it crossable, and a query given no predicate treats every wall as
solid. Every boundary between floor and solid, and every cell side on the border of the grid, has a wall, so where
there is no wall there is a cell beyond it, which `crossable` asserts. A doorway block has walls toward solid cells and
the border of the grid, and none toward floor.

### Scheduler

`Scheduler` orders actors' turns in abstract units. Each actor accumulates energy at its own speed and acts once it
has `action_cost`, which is 100. `spend` charges an actor, and `elapsed` returns the units passed. Speeds and energies
are ordered maps, so ties break the same way every run.

`advance(units)` grants every actor the energy its speed earns over the units. `ready` returns the actor that can act
now --- the one with the most energy, with the lowest id breaking ties --- or `nullid` if none can. `next` advances to
the moment the soonest actor can act and returns it, which is the turn-based policy; a real-time caller converts
elapsed time to units and calls `advance`.

Granting four units to actors of speed 100 and 25 gives the fast one four actions' worth of energy by the time the
slow one reaches its first; `next` would have stopped at the first.

### Clock

`Clock` advances a `World` through its `Scheduler`: it grants time, then runs each actor that became ready.
`grant_turn` grants enough time for the soonest actor to become ready, and `grant(units)` grants units, after which no
actor need be ready.

The `Driver` chooses between them in `advance`, which a host calls. Turn-based skips ahead and repeats, returning only
when the controlled actor needs a command or no actor will ever act. Real time grants the units that the elapsed time
earns and returns whether or not an actor became ready. Turn-based ignores the elapsed time, so a host passes it either
way.

`units_per_second` converts real time to units, one unit being one action for an actor of speed `action_cost`. The
clock keeps the fraction of a unit, so frames too short to earn one still add up, and `clear` discards it. A host sets
the driver through `Session::set_driver`.

`run(events, actor_controlled)` runs each ready actor on its `PendingCommand` if it has one, removing it, and
otherwise on its `Intent`, charging the action's cost. It returns the controlled actor once that actor is ready with
nothing to do, or `nullid` if no actor is ready. Any other actor with nothing to do waits.

A move toward the facing direction asked for is one faced step. A facing direction other than the movement direction
is a separate action, and the turn is taken first.

Turn-based clears the intent after one action, so a key pressed once moves once. Real time keeps it, so a held
direction keeps the actor moving, and a host writes an intent only when the input changes. Both clear an intent whose
action achieved nothing, which ends the pass; such an action costs no time, so walking into a wall is free.

Real time over a discrete layout makes the timing continuous and leaves the space discrete: an actor moves a whole cell
at a time, as often as the real time earns. Movement within a cell needs a continuous layout.

### Intent

`Intent` is a component with the movement and facing direction that an actor is trying to take, and it persists until
changed. `direction` is the movement asked for and `facing` the facing direction asked for, each zero if none is asked
for, and `distance` is how far to move. Both directions are absolute, in world space, and a discrete layout snaps them
to its own. `Intent` registers no reflection, so it is not serialized.

### ai

`think` sets an `Intent` for every actor that has a behavior, except the controlled one. It gathers the entities first,
since setting a component during iteration can move an entity to another table.

`Pursue` is the one behavior: the actor moves toward a target while the target is visible and in the pursuer's zone,
then toward where it was last seen, and stops there if it finds nothing. The position last seen is stored on the
entity, so it unloads, migrates and saves with it. Its `target` is an `EntityReference`, resolved each time, and a
`Pursue` with no `target` pursues the controlled actor that `think` is given.

The move taken is to the reachable neighbor nearest the goal, if one is nearer than the current cell. `neighbors`
returns placements in a fixed order, so ties break the same way every run. Only topology is consulted, so a pursuer is
stopped by an occupied cell in `World::move`.

### Game and Character

`Game` is the game in its entirety, the one object a host owns, for as long as the host runs. It is constructed with a
`Config`, the program's configuration, which is the same for every game: `saves_directory`, the directory in which
each saved game is a directory, and `zone_count_max`, the most zones the world keeps loaded at once. `config()` returns
it. Each host passes a temporary config, with the saves directory in the temp directory's `KML/Stratum/saves` and a
maximum of 8, until the game's config file is read. It owns the structures that survive outside play: the config, the
settings, a `CountingIdAllocator` counting from zero, a `random_type` seeded from
`std::random_device`, the `Generation`, the `World` and the `Character`, and the `Session` that plays them, constructed in
that order, each referring to those before it. `generation()` returns the `Generation`, with which a host registers
its generators, selectors and region specs. Only the `Generation` can be copied or moved. The constructor makes no
character, region or zone; until a game is created, there is none.

`Game` has the menu and lifecycle operations a host calls: `menu_open`, `menu_close`, `clear`, `create`, `open`,
`save`, `quit` and `saved_games`. They call `clear`, `create`, `open` and `save` on `World` and `Character`, and
`clear`, `start`, `pause` and `resume` on `Session`. `saved_games` returns no games.

`Game::create(name, region_spec, character_spec)` clears, resets the id allocator to count from zero, and reseeds the
engine from `std::random_device`, so nothing passes from one game to the next. It then creates the game's directory,
`name` in the config's saves directory, and throws `std::runtime_error` if that directory exists. It has the character
created from the `CharacterSpec`, and the world's starting region and its first zone from the `RegionSpec`, and starts
the session at the entry point `World::create` returns. If either throws, it clears again and removes the directory
before passing the exception on, so a failure leaves no game in progress.

`World::create(region_spec)` reads the spec's `entry_access`: a registered access point type, a comma, and a level
offset, such as `staircase,1`. It creates the starting region from the spec and its first zone from the spec's `zone.`
keys, through `create_region_zone` and `select_prefixed`, and returns the zone's `entry_point` for that type and offset.
It throws `std::runtime_error` if `entry_access` is not a type and an offset, or the zone has no such entry point.
`entry_access_key` and `zone_prefix` are the key and the prefix.

`Game::clear` clears the session, the character and the world, in that order, in place. `Session::clear` ends play:
no zone is played, and the turn order and the clock's fraction of a unit are emptied, while the driver remains.
`Character::clear` deletes the character's entity, with what it carries and its pending command. `World::clear` unloads
every region and empties the world index and the `ZoneEntryIndex`, and keeps the registrations: the entity types, the
access point types, the file components, and the `Generation` it was given. `menu_open` pauses the session and
`menu_close` resumes it, and `quit` clears the game. `open`, `save` and `remove`, and the functions on `World` and
`Character` that they call, are skeletons with empty bodies. `CharacterSpec`, which `create` takes, is the choices a new game makes: a `name`; `stats`, a map
from a name to a value; `skills`, which include feats and abilities, and `aspects`, such as the spells of a
spellcasting skill, each a map from a name to its properties; an `inventory` of `InventoryItem`, each an item's `type`, its
`InventoryLocation` (a `relation` and a `place`), and its `properties`; and the game's other choices, such as race and
class, as `properties`.
`session_plan.md` (`## Games`) has their design.

`Character` is the entity a host controls. `create(character_spec)` makes it, with `Blocking`, the `ControlledActor`
tag, a `Speed`, a new `PersistentId`, and the spec's `name` as its `DisplayName`, and it is in no zone until one is
entered. `entity()` is null before `create` and after `clear`. It is owned by the game and has no parent, so it outlives every zone it moves through. A host
acts through it: `set_intent(intent)` sets the character's `Intent`, and `submit(action)` sets a `PendingCommand` of the
action for the character's entity, which the clock applies the next time the character is ready. `entity()` returns the
entity, for queries and drawing.

### Session

`Session` has the zone being played: references to the game's `World` and `Character`, a `Scheduler`, a `Clock`, and the
zone. A host supplies input mapping and drawing.

`Session` makes no zone. Each host creates a game with `Game::create`, which starts the session in the generated first
zone. The library has no default entry type or offset.

Each host registers `stratum_gen`'s generators, dropping dungen's warnings, and a temporary generator selector named
`dungen`, until the generators are integrated. It creates the game from a temporary `RegionSpec`, whose
`generator_selector` is `dungen`, whose `entry_access` is `staircase,1`, and whose `zone.level` is -1, and an empty
`CharacterSpec`. It names the game by the local time it began, in ISO 8601 basic format, with a suffix of `-2`, `-3`, and
so on if a game of that name exists. The selector chooses the `dungen` generator for every zone, and names the zone
"Dungeon Level N", where N is the absolute value of the level. So the character begins at the staircase leading up on
the first level down, and a staircase leads to a new zone the first time it is taken, and back through the bound
staircase after that.

`stratum_gen::register_generators(generation, warning)` registers `generate_dungen` as `dungen`. It runs dungen with
its default parameters at the `ZoneSpec`'s `level`, if it has one, with a seed from `dungen::random_seed`, names the
zone with the `ZoneSpec`'s `name`, and returns the document `dungen::make_document` builds in memory. It passes dungen's
warnings to `warning`.

`start(access_point)` relocates the character to the access point's placement in its zone, then has the world
`trim_zones` with the zone entered, so no zone the character is in can unload. It makes the zone the one being played,
removes the character's pending command, and rebuilds the turn order from each entity in the zone that has a `Speed`,
the character included. `enter(zone, type, offset)` starts at the zone's `entry_point(zone, type, offset)`; if the zone
has no such entry point, it throws `std::runtime_error`, and the character stays where it was. `notation()` returns the entered zone's `ZoneNotation`.

`set_driver(driver, units_per_second)` sets the clock's driver, which lasts until it is set again; it is turn-based by
default.

`pause()` stops play while a host's menu is open, and `resume()` restarts it, discarding the fraction of a unit the
clock had earned; `paused()` returns whether play is stopped. Neither does anything while no game is loaded, and `clear`
ends a pause. Calling `advance` while play is paused is an error, which an assertion catches. A real-time host measures
the elapsed time it next passes from the resume, so the time the menu was open earns nothing.

`character()` returns the `Character`, so a host writes `session.character().set_intent(intent)` and
`session.character().submit(Interact {})`.

`advance(events, elapsed)` calls `think`, then advances the clock, returning the controlled entity when it needs a
command, or `nullid` if no actor will ever be ready. `elapsed` is the real time since the last call; a turn-based host
omits it. If the character went through a relocator into another zone, it then makes that zone the one being played, as
`enter` does once it has placed the character: the world trims its zones, the character's pending command is dropped, and
the turn order is rebuilt from the new zone. The actors of the zone left finish the advance in which the character left.

`Dead` is the tag for an actor that has died. `over()` returns true once the character of the game being played has it.
The clock then runs no actor, stopping within an advance if the character dies during it, and turn-based skipping ahead
stops too. Every `advance` once the game is over runs no actor, reports `Died` with the character in its events, and
returns `nullid`. Each host shows the death with the game's last state until a key is pressed, then calls
`Game::quit` and returns to its menu.

Each host maps `<` and `>` to `Enter`. The hosts report a `Relocated` event with the level arrived on, shown as its
distance from the surface, and a step `Blocked` by a wall with a relocator on it as "There is a staircase before you",
naming the relocator's access point type.

### Commands and events

`Command` and `Event` are type erased over `std::any`, so a module can add either. A command has the id of the entity
it acts for. `CommandQueue` and `EventQueue` are deques in submission and occurrence order.

The commands are `Step`, `TurnTo`, `Interact`, and `Enter`. `Step` has a direction, zero for the actor's facing
direction; a distance; and `face`, which turns toward the direction within the same action. `TurnTo` has an absolute
facing direction, and `Interact` acts on the wall `ahead`. `Enter` enters the relocator on the floor of the actor's
cell, in a discrete layout. They produce `Moved`, `Blocked`, `Turned`, `Interacted`, and `Relocated` events, and `Session::advance` produces
`Died`.

The controlled actor enters a relocator on the wall ahead with `Interact`, and one on the floor of its cell with
`Enter`, through `World::relocate_through`; any other actor does not enter a relocator. A `Step` into a wall with a
relocator on it is `Blocked`, like a step into any wall, so a held movement key never takes the stairs. Entering by
moving into the wall is kept in `commands.cpp` behind `enter_relocator_by_moving`, which is off. A relocator on the
floor does not impede movement. `Relocated` has the actor, the relocator, and the access point arrived at.

The clock builds `Step` and `TurnTo` from an actor's `Intent`. A one-shot command, such as `Interact`, waits on the
actor as a `PendingCommand` component, which the clock applies before the actor's `Intent` the next time the actor is
ready; a host submits one through `Character::submit`.

`apply(world, command, events)` returns the command's cost, or zero if it did nothing; the `CommandQueue` overload
drains the queue. A move costs in proportion to the distance traveled against `step_distance`, so a move stopped short
is charged for the distance reached and a fraction of a cell costs a fraction of an action. A turn and an interaction
cost one action; a `TurnTo` to the current facing direction and an `Interact` with no wall ahead cost nothing. Going
through a relocator costs one action, and an `Enter` with nothing to enter costs nothing.

## Zone file format

`quetzal::stratum_format` reads and writes the JSON zone file that `discrete_layout.md` specifies, version `0.4`. It
depends on `quetzal_common` and nlohmann/json. `load_layout` fills a zone from what `read` returns, and both generators
write through it.

`grid.hpp` has the geometry the rows define --- `on_grid`, `floor_cell`, `floor_door_cell`, `stair_cell`, `has_wall`,
`room_cells` and `door_walls` --- which `read` uses to validate a file and `load_layout` uses to build a layout.

`references.hpp` finds and translates the references a file writes as `{ "entity": id }`. `make_reference` writes one,
`reference_id` returns the id if a value is a reference, `references` returns every reference within an object of file
components in the order written, and `translate_references` replaces each with the id a function returns for it.

`Document` is the file as data structures. The geometry is decoded into types: the grid and its rows, expanded from
runs; the barriers; each room's perimeter or `RoomLocator`; wall addresses; and each flat's heights. So are each
entity's `site`, `parent` and `relocator` file components, which the reader validates. A `Site`'s address is a
`WallAddress`, a `FlatCell`, or a `ZoneCell`, a cell of the zone that an entity such as an actor is in, with the
relation `in` and an orientation required. A `Relocator` has its target,
which is a `RelocatorTarget` or the id of the relocator it is bound to, and, for an access point, its type, order and
placement. The placement is a `discrete_directed_point_type` (`geometry::DiscreteDirectedPoint`): the cell position and
orientation given to an arriving actor. The layout's `access_points` are `Document::access_points`, a list of entity
ids, and its optional `level` is `Document::level`. The generator and every other file component are `raw_values_type`, nlohmann's `ordered_json`, as the file wrote
them. A room is kept as the file identified it.

A `RelocatorTarget` is an unbound target, `[region/]zone[@position]`: an optional region, a zone that is a
`RelativeZone` (an offset), an `AbsoluteZone` (a level), or a name, and an optional position. The region, a zone name,
and the position are kept as written. `parse_relocator_target(text)` returns the target, or nothing if the text does
not match the grammar, and `format_relocator_target(target)` writes it back as the same text. They and `level_type` are
in `RelocatorTarget.hpp`. That header and `Notation.hpp`, which has `Notation` and its names, are the only format
headers that `stratum_sim`'s headers include, so nlohmann reaches only the files that read or write a document. A saved zone's `zone.id` is `Document::zone_id`, which makes every id in the file persistent; it is
empty in a zone template.

`read` validates the file, throwing `std::runtime_error` with the path within the document and what was wrong there.
Beyond each section's structure, it checks what the rows determine: in the wall notation, a wall between every floor
cell and a blank cell, a stair cell or the edge of the grid; a wall in the grid at every wall address, whose cell is a
floor/door cell; a room's `RoomLocator` cell on a floor cell; a perimeter within the grid and along cell edges that
never meets itself, each of whose cell edges is a wall toward a floor/door cell or a virtual wall between two; and no
cell in two rooms, compared only for rooms whose bounding boxes intersect.

It then checks the entities. Ids must be whole, non-negative and unique. Every entity must be sited on the geometry, be
a child, or be a parent, with an entity sited on the geometry in every hierarchy and a parent for every entity sited on
one, and no `parent` may lead back to its own entity. In a template, every reference must be an entity's id; a saved
zone's references may refer to another zone and are not checked. Each door glyph requires one entity with a child
sited on each wall of its doorway and on no other wall. `layout.components.access_points` must refer to at least one
entity with a `relocator` sited on a surface, one on the floor or the ceiling with its site's `orientation`, and each
relocator must have a `type`, an `order` counting from 0 among the access points of its type, and a `placement` on a
cell an actor can stand in. Each relocator's target must be a
reference or a string that matches `[region/]zone[@position]`. The stair glyphs are visual aids, so a stair glyph with
no `relocator` on the walls around it, or with a second one, and an access point on a wall that is neither a stair
cell's open side nor a door's wall, are warnings. So is an access point whose relocator type is neither its entity's
type nor its parent's, the parent being a door facing's door. They are passed to `read`'s warning callback, `log_warning` by default, which
writes them to `std::clog`. Keys the format does not define are not tracked.

`write` emits the written form: two-space indentation, a grid row per line, an entry per line with each of a flat's
per-room and per-cell entries on a line of its own, and floats as the shortest digits that read back as the same
value, without an exponent. Top-level sections follow the format's key table. Within an entry, its address or
identification comes first, then the rest in its section's table order, and file components stay in the order read.
An entity's `parent` and `site` are written into its `components` ahead of the rest. In runs, a count is written for a
run of three glyphs or more. A document that `read` produces is written back equivalent to it.

## Presentation

The presentation layer is engine-neutral, depending on `stratum_sim` and on no output device.

A **reduction** takes a world and a zone to a `Buffer<Place>` --- `cells` and `lattice` --- and a **mapping** takes
that to a `Buffer<T>` that a device can emit --- `render` and `render_lattice`. The Godot tile view uses a reduction,
and the terminal uses a mapping. There is no view interface over them.

`cells.hpp` declares the reductions, `ascii.hpp` the terminal's mapping, and `read_glyphs.hpp` the reader that fills a
mapping from a file. `Buffer.hpp` defines the grid both produce.

A position reduces to a `Place`: a `PlaceType`, its `State`, its `mask`, and whether an actor is on it. `PlaceType`
(`None`, `Solid`, `Floor`, `Wall`, `Door`, `StairsUp`, `StairsDown`) is the fixed structure, whatever its state, so a
door that can be shut, locked or concealed is one type. `None` is an edge with no wall or a corner that no wall line reaches, which only
`lattice` produces. `actor` is a flag over the structure, so the floor under an actor is still there to draw.

`State` is a set of flags named after the component data they are read from. `Open` is set if the way through's
`Access` is open, and `Concealed` if its `Detection` conceals it from actors in general though not from the viewer,
which a glyph set may draw outlined. Only a state that changes the drawing is a flag. `applies(type, state)` returns
true if a place type can be in a state. The reduction is the only code that reads a component on behalf of a view.

Both reductions draw the zone as a viewer perceives it, and the hosts pass the character. A way through that the viewer
finds concealed is drawn as though there were none, as a wall. A null viewer gets the general state.

`cells(world, zone, viewer)` returns one `Place` per grid cell: `Solid`, `Floor`, or `Door` for a door that occupies a
cell, as a doorway does in the block notation. A wall or door on the edge between two cells has no position there, so
it reduces to `Floor` on both sides.

In both reductions, stairs are drawn on a floor cell with a relocator on it, and on the closed cell beyond a wall with a
relocator on it, which is a stair block. They are `StairsDown` if the relocator's target is known to be below the zone,
and `StairsUp` otherwise: an unbound relative target below zero or absolute target below the zone's level, or
a bound target whose relocator is loaded in a zone at a lower level. The site's orientation is not drawn. The glyph
sets draw `StairsUp` as `<`, the entrance glyph, and `StairsDown` as `>`, and `read_glyphs` names them `stairs_up`
and `stairs_down`.

`lattice(world, zone, viewer)` reduces the zone onto the wall notation's lattice: `(2W+1)` by `(2H+1)`, with cell
`(i, j)` at `(2i+1, 2j+1)`, the wall west of it at `(2i, 2j+1)`, the wall south of it at `(2i+1, 2j)`, and corners at
the positions that are even in both. It emits cells, `Wall` or `Door` on edges, and `Wall` at a corner if any of its
four edges has a wall or a door.

A door occupies the cell beyond it if that cell is open and has no wall back toward it, as in a doorway block, and is
then placed once in that cell; otherwise it is on the edge. Each wall is visited from the open cell it faces, which
reaches every wall.

`mask` is the set of directions in which the wall line continues from a position: a direction is set if a wall or a
door is on the edge or corner that way. A cell is never part of a wall line, so a door that occupies one is a gap in
it. There are sixteen masks, and a glyph set with fewer glyphs maps several of them to one.

A position is `Solid` if every cell it touches is closed. A cell touches itself, an edge touches the two cells it lies
between, and a corner touches the four that meet at it; positions beyond the grid count as closed. A wall faces open
floor, so no wall is at a solid position, which the reduction asserts.

Both reductions set the actor flags last, from `each_actor_cell`.

`Buffer<T>` is the grid, with `extent`, `contains`, `at`, `set` and `fill`, and row 0 lowest as in the simulation. Its
position and extent are `stratum_view`'s discrete point and vector types, the same as the simulation's grid. A private
`offset` converts a position to a subscript and asserts the bounds. `Console` is `Buffer<char>`, and `Cells` and
`Lattice` are both `Buffer<Place>`. `to_string`, for characters only, writes the highest row first.

`difference(a, b)` returns the runs in which `b` differs from `a`, each a `Span` of a position and a count along its
row, lowest row first. The result is empty if they are the same, and the whole of `b` if they differ in size.

`render` takes cells to a `Console`, and an overload first reduces a world's zone as a viewer perceives it.
`render_lattice` does the same for the lattice; it has its own name because `Cells` and `Lattice` are the same type. A
mapping is one lookup per position.

A `Glyph` is one line of a glyph set: a place type, the state flags it requires, optionally a mask, whether it
requires an actor, and the character drawn. `Glyphs` returns the character of the most specific matching line, where
each state flag, a mask, and an actor add one to the specificity. Ties go to the earlier line, and a space is returned
if no line matches. A set with a `door open` line draws an open door differently from a shut one, and a set with only
`door` draws both the same.

`cell_glyphs()` covers the three types a cell can be, a door shut and open, and an actor on each type it can be on.
Its actor lines come first, so an actor in an open doorway is drawn. `lattice_glyphs()` draws in the wall notation's
glyphs, with floor as a space and a line for each of the sixteen wall masks.

`read_glyphs` reads a glyph set from a file; `assets/glyphs/` has the two built-in sets. The first line gives the
format and version. A glyph line is a place type, the states it requires in any order (`open`, `concealed`), `actor`
if it requires an actor, `mask=` and the directions it requires, then the character in single quotes, with `\\` and
`\'` as the escapes. Omitting the states, `actor` or `mask=` matches any; `mask=` with no directions requires that no
wall line reach the position. A line beginning with `#` is a comment. It throws, reporting the line, including for a
state that its type can never be in. The shipped files are compared against the built-in sets.

## Terminal host

`Terminal` writes a `Console` to a character terminal, emitting only the cells changed since the last frame. It uses
the ECMA-48 escapes CUP, ED and EL, the DEC private mode DECTCEM to hide the cursor, the alternate screen buffer, and
the xterm extension XTWINOPS to resize.

- It enters the alternate screen buffer on construction and leaves it on destruction, so the shell's screen and
  scrollback are untouched.
- On Windows it enables `ENABLE_VIRTUAL_TERMINAL_PROCESSING`, which conhost does not enable by default.
- `terminal_size` reads the text area and `set_terminal_size` requests one, in columns by rows. The request is
  `CSI 8 ; rows ; columns t`, which Windows Terminal honors and which keeps the scrollback.
- The window is grown on the first frame that would not fit, and is never shrunk. The original size is restored on
  exit only if the window is still the size it was set to, so a manual resize is kept.
- The pseudoconsole reports a new size before the terminal has resized, so a settling delay precedes the first paint
  and follows a detected resize.
- A frame is the map rows, a blank line, the status and the help. The help line has no newline, so a frame the height
  of the window does not scroll it.
- A frame that does not fit is replaced by the terminal's size and the size needed.

`read_key` returns one key without waiting for a newline: `_getch` on Windows, termios raw mode elsewhere. Arrow keys
become `Key::Up`, `Down`, `Left` or `Right`.

The host starts at its menu, drawn in place of the map, with `n` for a new game, `c` to continue the game in progress,
which is offered only while there is one, and `q` to quit, which clears the game and ends the run. Escape on the menu
also continues. A new game is created from the host's temporary specs, and a failure to create one is shown on the
menu's status line. During play, Escape opens the menu, through `Game::menu_open`, and no other key quits. When the
character dies, the map stays with "You have died." on the status line until a key is pressed, and the host then quits
the game and opens the menu.

The loop advances the session to the actor it is waiting on, draws, reads a key, and submits it: movement and facing
direction as the actor's intent, and a one-shot action as a command. Arrows or `hjkl` move, space interacts, `.`
waits, and `^L` redraws. The view does not show the facing direction, so a move also turns toward the way it goes. The
host charges a wait. `--script` plays a key sequence instead of the keyboard, beginning at the menu, so a script that
plays starts with `n`; an ESC character in it is the Escape key, and the run quits once the script is exhausted.
`--incremental` uses the cursor-addressed output within a scripted run.

`--cell` and `--wall` choose the reduction drawn: a glyph per cell, or the lattice, which also draws walls and doors on
edges. Each uses its own glyph set. With neither, each new game chooses by its first zone's notation: the cell view for
the block notation and the wall view for the wall notation; `Session::notation` returns the notation. The host takes no
zone file.

## Godot host

Godot owns `main()` and the frame loop, and loads the simulation as a shared library. `stratum_godot` builds that
library, a GDExtension, and `application/stratum_godot/project/` is the Godot project that loads it, through
`stratum.gdextension`, which refers to `res://bin/`, where the build writes it.

`StratumWorld` is the one registered class, a `RefCounted` that owns a `Game`, with only Godot types in its
interface: `create_game`, `has_game`, `game_over`, `menu_open`, `menu_close`, `quit_game`, `extent`, `cell`, `render`, `cells`,
`actors`, `step`, `interact`,
`enter`, `turn`, and `message`. `step` sets the character's intent, turning toward the direction moved as the view does not show the
facing direction, and advances through the turn-based driver; `interact` submits `Interact` and `enter` submits
`Enter`, and each advances. `message` returns what the last of them did that the map does not show, such as a door
opening, a staircase before the character or leading nowhere, or the level arrived on, and the tile view shows it in its status line. A step ends with the next turn already granted,
so `turn` is the turn awaiting input. `create_game` creates a new game as the terminal host does, `has_game` returns true
while one is in progress, `game_over` returns true once the character is dead, and `message` then reports the death,
`menu_open` and `menu_close` pause and resume play, and `quit_game` ends the game in progress.
Exceptions do not cross into Godot: a failed creation pushes the reason to Godot's error list and returns false, and
leaves no game in progress.

`cells` is one code per cell from `stratum_view::cells`, row major from row zero; its `PlaceType` constants repeat
`stratum_view::PlaceType` for Godot, followed by `PlaceTypeDoorOpen` for a door whose access is open. `actors` is one
`Occupant` per cell in the same order: `OccupantNone`, `OccupantCharacter`, or `OccupantOther` for any other actor.

`game.tscn` is the tile view: a `TileMapLayer` fed from `cells`, a second layer for actors, a camera, and arrow or
`hjkl` keys, with space calling `interact` and `<` or `>` calling `enter`. A held movement key repeats, and any other
key acts once per press. Its tileset is built at run time from flat colors: stairs in two blues, an open door lighter than a closed one, the
character in yellow, and any other actor in red. It repaints from the new extent when the character enters a zone of another
size. Row zero is the lowest in the simulation and y grows
downward in Godot, so the view inverts j when it paints.

The tile view starts at its menu, a dark screen over the view with the same choices and keys as the terminal host's:
`N` for a new game, `C` or Escape to continue the game in progress, and `Q` to quit the game and Godot. During play,
Escape opens the menu, and no other key quits. When `game_over` returns true, the status line asks for a key, which
quits the game and shows the menu. `-- --new-game` on Godot's command line starts a new game without showing the menu.

`quetzal_godot` provides what any Godot host needs: `gdextension.cmake` and its `quetzal_configure_gdextension`, which
names the library as Godot expects and writes it into the project rather than a per-configuration subdirectory.

The root forces `CMAKE_MSVC_RUNTIME_LIBRARY` into the cache, because godot-cpp caches the static release CRT even for
a debug build, and quetzal's static libraries then do not link. godot-cpp's headers are SYSTEM includes, as flecs' are,
so they do not report `/W4` warnings in the extension's sources. `KML_DEV_GODOT` governs all of it; when it is
configured off, godot-cpp is not processed and neither the library nor the host exists.

Two headless checks need no display: `--path project --script main.gd` exercises the extension directly, and
`--path project --quit-after 5 -- --new-game` runs the tile scene in a new game, which reports the cells it painted;
without `-- --new-game` it reports that the menu is shown. A project has to be imported
once (`--headless --path project --import`) before Godot sees the extension; otherwise the class is undeclared, with
no indication why.

Visual Studio launches Godot on the project when the extension is run, so breakpoints in the extension are live. Godot
must not already have the project open, or the library is locked and the build fails.

## Tests

`stratum_sim_test` covers the layout hierarchy, layout structure, rooms, the grid, flats, blocking, the scheduler, the
clock, movement, adjacency and sight, loading, doors, the world, persistent ids, access points, residency, commands and
occupancy. It and `stratum_view_test` each check that every zone file in their `data/` has one `door` entity per door
glyph, with a `door_facing` child on each wall that `stratum_format::door_walls` finds for that glyph and no door
facing anywhere else.

`stratum_view_test` covers the console buffer, frame differencing, glyph sets and their files, the cell and lattice
reductions, both renderings and both round trips, and the reference frames. The reference frames are fixed command
sequences rendered to text and compared byte for byte by `FileValidator` against `test/stratum_view_test/reference/`.
The sequences are played against the two libraries, not through a host. Three scenarios are block notation zones drawn
through the cell view. Two use the wall view: `walls_door`, a wall notation zone, moves through a door, back, and into
the wall two rooms share, which blocks, then interacts with it; `door_cell`, a block notation zone, moves into a
doorway, where the door is drawn in its cell, and out the far side.

`dungen_test` and `catagen_test` compare reference zone files written now against those checked in, which must be
equivalent as parsed JSON, and check determinism, parameter validation, grid shape, and that the reader accepts every
generated zone. For their scenarios and over a sweep of seeds, both check that each door glyph has one `door` entity
whose door facings are on that doorway's walls in the order `door_walls` returns them, and that no door facing is
anywhere else. `dungen_test` also reads back the rooms of zones with every door, half of them, and none, over a sweep
of seeds, and checks that each room contains exactly the cells generated as it, which fails if a barrier is missing at
an open doorway.

Over another sweep with ranges of staircases each way, both check the written zone. Neither count may exceed its
maximum, and a count leading up may fall short of its minimum only if no staircase leads down. Every floor cell must be
reachable from every other with the stair blocks in place, except in maze1, which can leave corridor that no door
reaches. No staircase may block a doorway if a cell beside it would not. Every staircase must be sited on exactly one
of the walls around its block, with the target `-1` or `+1` as its glyph indicates, and must open onto a side with floor
beyond, or a door if there is none. `access_points` must refer to every staircase. The suites take what the written
zone does not record about a stair cell --- its surroundings in dungen, and which of its sides were open in catagen ---
from the zone generated again. `catagen_test` also checks that every edge between a stair cell and floor is written as
a wall. Validation rejects a staircase up count whose minimum is zero, including when only staircases down are asked
for. Each suite also reads a reference zone's generator parameters back and regenerates from them, since a zone file
has every parameter of its run. A parameter that the writer emits and the suite does not read fails, as does one that
the suite reads and the file lacks.

`tool/dungen` and `tool/catagen` build the drivers `dungen` and `catagen`; the library targets have the `quetzal_`
prefix and the suites the `_test` suffix.

`stratum_format_test` reads zone files and checks what they decode to, and rejects every error the reader raises from a
valid file with one change made to it. It checks that references are found and translated wherever they are written,
and writes each file through both `write` overloads and reads it back, comparing the two documents as maps.

`flecs_test` checks that the ECS supports zone partitioning, relationships, migration, unload, serialization and
deterministic iteration.

Each test runs from its own directory, since its data and reference paths are relative.

No test executable covers a host. The terminal host is checked by running it with `--script` and reading the frames
back, and the Godot host by its two headless invocations.

## Build

The build is CMake, with the solution at `build/kml_dev.slnx`. `/std:c++latest /bigobj /W4` is used throughout, with
third-party sources at `/W1` and flecs and godot-cpp included as SYSTEM. `quetzal_stratum_sim` links `quetzal_common`,
`quetzal_stratum_format` and `flecs_static`; `quetzal_stratum_view` links `quetzal_stratum_sim`;
`quetzal_stratum_gen` links `quetzal_dungen` and `quetzal_stratum_sim`; both hosts link `quetzal_stratum_gen` and
`quetzal_stratum_view`. `quetzal_stratum_format` links `quetzal_common` and `nlohmann_json`, vendored at
`library3/json` with its headers SYSTEM.

`KML_DEV_GODOT` governs the Godot targets and defaults on. When it is configured off, godot-cpp is not processed and
neither `quetzal_godot` nor `stratum_godot` exists.

`KML_DEV_ASSETS_ROOT` is a per-target compile definition giving the shared data root, so no source file has an
absolute path and no program depends on where it was started from. `KML_DEV_GODOT_VERSION` is the Godot release the
host targets, and `KML_DEV_GODOT_EXECUTABLE`, found from it, is what Visual Studio launches.

`application/stratum_godot` is added after `library3`, because configuring a GDExtension reads a property of the
godot-cpp target.

---

*Current stage: Phase 3, all but presentation tables. `Glyphs` is a table that a file supplies; the Godot view's color
array is still hardcoded, and making it data requires asset ids, which is Phase 4's work.*
