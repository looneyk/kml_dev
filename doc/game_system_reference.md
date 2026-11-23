# Game System --- Reference

This document describes what the system is and how it is put together, as implemented. The reasoning behind its
larger decisions is in `decision_rationale.md`.

The system has three layers, and none of them uses an engine. `quetzal::stratum_sim` simulates, `quetzal::stratum_view`
turns a simulation state into characters or place codes, and a host puts those on a device. `stratum_sim` links
`quetzal_common`, `quetzal_settings`, `quetzal_stratum_core`, `quetzal_stratum_format` and flecs; `stratum_view` links `stratum_sim`; each host links
`stratum_view`. `quetzal::stratum_gen` puts the generators behind one signature, independent of `stratum_sim`, and each
host registers them in `stratum_sim`'s registries, so neither links the other.

The `stratum_` prefix names the layout class they share: open ground extruded vertically between one floor and one
ceiling.

| Directory | Target |
| --- | --- |
| `assets/` | shared data, used by every host |
| `library/quetzal/stratum_sim/` | `quetzal_stratum_sim` |
| `library/quetzal/stratum_view/` | `quetzal_stratum_view` |
| `library/quetzal/stratum_core/` | `quetzal_stratum_core` --- the data types that the format, the simulation and the generators share (`## Core types`) |
| `library/quetzal/stratum_format/` | `quetzal_stratum_format` --- the zone file format and the game files |
| `library/quetzal/stratum_gen/` | `quetzal_stratum_gen` --- the generators by name, behind one signature |
| `library/quetzal/log/` | `quetzal_log` --- the process-wide log, which every library writes its warnings to (`log_plan.md`) |
| `library/quetzal/settings/` | `quetzal_settings` --- the process-wide store of settings by key (`settings_plan.md`) |
| `library/quetzal/testing/` | `quetzal_testing` --- the `main` of every test executable on doctest, and reference file checks, and named checks |
| `application/stratum_terminal/` | `stratum_terminal` |
| `application/stratum_godot/` | `stratum_godot` |
| `library/quetzal/godot/` | `quetzal_godot` |
| `tool/dungen/`, `tool/catagen/` | `dungen`, `catagen` --- the generator drivers |
| `test/stratum_core_test/`, `test/stratum_sim_test/`, `test/stratum_view_test/`, `test/stratum_format_test/`, `test/dungen_test/`, `test/catagen_test/`, `test/flecs_test/`, `test/settings_test/` | test executables |

## Simulation

A `World` contains zones. A zone is one independently loadable 3D coordinate space. Zones share no coordinate system,
so a placement is meaningful only within its own zone, and an entity's zone is the zone among its ancestors. Each zone
has a layout, which is its geometry and topology, and the entities in it.

The layout computes where a move ends, and a caller passes it the direction and distance.

### Types

`value_type` is `float` and `traits_type` is `math::CartesianTraits<value_type, 3>`, which defines `point_type`,
`vector_type`, `points_type` and `polygon_type`. `placement_type` is `geometry::DirectedPoint<traits_type>`, a position
and a facing direction.

`coord_type` is `std::int32_t`. `discrete_vector_type` is `math::DiscreteVector<coord_type, 2>`, a displacement on
the grid, and `discrete_point_type` is `geometry::DiscretePoint<coord_type, 2>`, a cell position. A cell position is
moved by adding a `discrete_vector_type`, the difference of two cell positions is the displacement between them, and
adding two does not compile. `cardinal_directions` are the four unit vectors `East`, `North`, `West` and `South`, in
`CardinalDirection` order, and `cardinal_direction(d)` returns one of them.

`point_projection_type` and `vector_projection_type` are `geometry::MinProjection` along the z axis onto the xy plane.
`grid_step(vector)` returns the cardinal direction nearest a direction.

`value_type`, `traits_type`, `point_type`, `coord_type`, `discrete_vector_type`, `discrete_point_type`, `seed_type`,
`zone_position_type` and `PersistentId` are `stratum_core`'s (`## Core types`), and `types.hpp` declares them in
`stratum_sim` with using-declarations. `World.hpp` does the same for
`Region`, `RegionTree` and `Worldspace`, `RegionSpec.hpp` for `RegionSpec`, and `ZoneSpec.hpp` for `ZoneSpec`.

### LayoutBase

`LayoutBase` is an interface to one zone's geometry and topology. Its queries --- `move`, `turn`, `face`,
`same_location`, `neighbors`, `visible`, `ahead` and `step_distance` --- are all pure virtual, and none of them
mentions a cell or a wall. `Rotation` (`CCW`, `CW`, `Half`), `MoveResult`, `crossable_function_type` and
`blocker_function_type` are declared with it.

A second, non-virtual `move` takes neither predicate, so no wall is crossable and no entity blocks. A derived layout
brings it back into scope with `using LayoutBase::move`.

The destructor is public and virtual, since a zone owns its layout through a `std::unique_ptr<LayoutBase>`. The other
special members are protected and defaulted, so a concrete layout copies and moves as itself and cannot be sliced
through the base.

`layout_as<L>(layout)` returns the layout as its concrete class, through a cast that asserts the class. The loader, the
view reductions, `Session`'s initial placement and the host status readouts use
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
`LayoutBase`, and that it is abstract.

### DiscreteLayout

`DiscreteLayout` is the static topology of one zone --- its walls, flats, and rooms --- stored as a table of cells over
the grid. Entities refer into it by id.

| Type | |
| --- | --- |
| `Cell` | One square of the grid. `open` is true if an actor may stand there, `walls` is a `DirectionMask` of the sides that have a wall facing into the cell, and `room_id` is the room that contains the cell, or `nullid`. |
| `DirectionMask` | A set of the four cardinal directions, one bit each in `CardinalDirection` order, with `direction_mask(direction)`. The view uses it for the directions in which a wall line continues. |
| `Flat` | The floor or the ceiling, which differ only by `ceiling`. Its height is given for the whole flat, per room (`room_height`) and per cell (`cell_height`), each overriding the one before; the per-room and per-cell tables are sparse. |
| `GridMapping` | `origin`, the corner of cell `(0, 0)`; `scale`; and `extent`, the cell count along each axis as a `discrete_vector_type`. It has `contains(cell_position)`, `cell_position_at(point)` and `position(cell_position)`, which returns the cell center. |
| `MoveResult` | The placement reached, possibly short of the distance asked for, and the `blocking_wall_id` or `blocking_entity_id` that stopped it. Only one of them is set, and both are `nullid` if nothing stopped it. Declared in `LayoutBase.hpp`. |

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

`destination_placement(site, egress)` returns the placement of an actor arriving at an access point with that site and
egress. The actor is on the floor of the cell of the wall or the flat cell plus the egress, facing in the cardinal
direction nearest the egress, or if it is zero, away from the wall or in the site's orientation.

`set_flats(height_floor, height_ceiling)` creates the two flats, once; `floor_id` and `ceiling_id` identify them.
`height(flat_id, cell_position)` returns the cell's own height if it has one, otherwise its room's, and otherwise the
flat's. `position(cell_position)` returns the cell center with z at the floor height there, or on the grid's plane
before the flats are set.

`turn` is relative and `face` absolute. A view with a relative turn calls `turn` for the resulting direction and sends
it as a facing direction, so the layout does the quantizing in both cases. `face` snaps to a cardinal direction and
leaves the facing direction unchanged for a zero direction.

`move` snaps the direction with `grid_step`, rounds the distance to whole cells, and moves cell by cell until
`crossable` rejects a wall or the cell ahead is occupied. It then sets the position from the cell reached, so rounding
error never accumulates. `blocker_at` returns the entity occupying a placement; the layout calls it before entering
each cell and reports the result as `blocking_entity_id`. `World::move` supplies it.

`step_distance` returns the grid scale, the distance one action of movement travels.

`slot(cell_position)` returns a cell's position in the cell table, and asserts that the cell is on the grid.

`neighbors` returns the placements reachable in one move. `visible` walks the cells that the ray between two
placements passes through, one edge at a time, and fails at the first wall that cannot be crossed. `field_of_view` is
declared and not implemented. `ahead(placement)` returns the wall of the placement's cell in its facing direction,
snapped to a cardinal direction, or `nullid` if there is none.

### ProximityIndexBase

`ProximityIndexBase` is an interface to the entities placed in one zone, by location: `place` adds an entity at a
placement or moves it there, `remove` takes out an entity that is in the index, and `contains` and `entity_count` report
on it. `entities_at(placement)` returns the entities at the same location as a placement, and
`entities_within(placement, range)` those whose positions are within the range of its position in 3D, both in ascending
id order. It refers to entities by `id_type`, so `LayoutBase.hpp` includes no flecs.
`LayoutBase::create_proximity_index()` returns an empty index of the layout's own type, which must not outlive the layout.
Each zone owns one (`### World`).

`DiscreteProximityIndex` is `DiscreteLayout`'s. It maps each cell's slot to the ids in it, kept in ascending order, and
each entity to its placement. `entities_at` takes a placement or a cell position. `entities_within` visits the cells
that the range overlaps, clipped to the grid, or, if the zone has fewer entities than those cells, every entity, and
keeps those within the range.

### load_layout

`load_layout(zone, document)` fills a zone that has no layout from a `ZoneDocument` that `stratum_format::read_zone` returned.
An empty zone's document, which has no `cells`, sets only the zone's `Document`, its position and its layout
components, and leaves it with no layout. Otherwise it builds the layout through the same `stratum_format` functions that validated it. Each floor/door cell is open, with a wall on
each side for which `has_wall` is true. Each room's cells, from `room_cells`, get its id, and `room_count` is the
number of rooms. The flats get the file's heights, the floor defaulting to 0 and the ceiling to 1. A stair cell is
floor in the block notation, with its staircase on its floor. In the wall notation it is a block, closed like solid,
and the floor beside it is walled toward it.

A doorway block is a cell an actor may stand in, walled toward solid on its own side and toward floor on the floor's
side alone. Two areas therefore never share an edge in the block notation. In the wall notation, two rooms that share
a wall are walled from both sides, which the block notation cannot express, so a block layout is a subset of a wall
layout.

It sets the zone's `Layout` to the `DiscreteLayout`, since both notations are grids of cells, and its `Document` to a copy of the document without its
entities and with its `unpopulated` flag cleared, whose geometry a saved zone is written with. It then
builds every entity in the file in two passes, in the
file's order. The first pass creates each entity in the zone, `IsA` its type, so that each has its persistent id ---
the file's persistent id in a saved zone --- before anything refers to it. A type that is not loaded is made with no
components, so the entity keeps its type. The second sets each entity's parent; its site, with the address resolved to
layout ids and the position and orientation to a placement, and a site in a cell of the zone made a placement at that
cell with no address. A door is the file's `door` entity, and its door facings are the `door_facing` children sited on the
walls of its doorway. Each entity that `layout.components.access_points` refers to is made an access point, with its
`access_point`'s `order`, `egress`, and `destination`.

Last, it decodes each component object --- of the layout, of each room, wall, flat and flat override, and of each
entity --- into the component registered under its name (`register_file_component`), and sets it on the zone's
entity, the layout entity, or the entity. The decoder walks the object's parsed values and sets each field through
flecs' reflection cursor, so no JSON reaches flecs. A component that inherits is decoded into a new value, so the
entity's own value has only the fields its object sets, and the chain supplies the rest. Any other component is
decoded over the entity's current value, which is a copy of its type's value if it has a type. A `std::optional` field
is set through its `OptionalField`. A reference in a component object becomes an `EntityReference`. In a template it
refers to the entity of the same file, with that entity's persistent id; in a saved zone it has the persistent id
written, and the entity if that entity is in the zone. `load_layout` returns the entities that a template's references
refer to, which `World::load_zone` adds to the world index. The decoding is `decode_components`, in
`decode_components.hpp`, which the character's loading shares.

`check_buildable(world, document)` checks, before anything is built, that the world can build a zone from a document,
and returns the first reason it cannot, with the path to it. The reason is a component object that has no component registered under its name, has a field the component does not have, or has
a value its field cannot take, which `check_decodable` finds by decoding every component object into a value that it
then discards. Flecs' own error logging is silenced meanwhile, so `World::load_zone` checks a file completely before
making anything. `load_layout` asserts the conditions that `check_buildable` checks.

`load_character(character, document)` fills the character's entity from a `CharacterDocument`: its persistent id and
its component objects, and each entity of its inventory, with its persistent id, its type, its parent, its site on that
parent, and its component objects. Every reference keeps its persistent id and is resolved when it is first followed.
The `check_buildable` overload for a `CharacterDocument` checks that each component object of the character and of its
inventory's entities has a component registered under its name, and fits it.

`register_file_component(entities, name, component)`, or `register_file_component<T>(entities, name)`, registers a
component, with its reflection as a struct, an enumeration, or an opaque type, as the one that a component object of that name decodes
into. A tag is registered too, and a file writes it as an empty object. The name is an entity under `components`.
`file_component(entities, name)` returns that component, or 0 if none is registered; `file_components(entities)`
returns every registered one with its name, in name order; and `is_tag` returns true for a component with no value.
`register_vector<T>` reflects a `std::vector<T>` as a list, which a file writes as an array.

`World` registers `name` as `Name`, an entity's name, such as a zone's `layout.components.name` or the character's
name, and `display_name` as `DisplayName`, what an entity is called for a player to read. `register_names`, in
`Name.hpp`, reflects both as strings, so the file's `"name": "..."` and `"display_name": "..."` decode into them, and
registers an observer that, when an entity's `Name` is set and it has no `DisplayName`, sets a placeholder
`DisplayName`: the name in capitals. `World` also registers the state of an entity that a saved zone writes back.
`register_actor_components`, in `actors.hpp`, registers `speed` (`Speed`) and the tags `blocking` (`Blocking`) and `dead`
(`Dead`), and the `ControlledActor` tag. `register_access_point_components`, in `access_points.hpp`, registers
`directionality` (`stratum_core::Directionality`), `Up` or `Down`, the direction in which the access point on an
entity leads. `register_attachment_components`, in `attachments.hpp`, registers `access`
(`Access`) and `detection` (`Detection`), and `register_ai_components`, in `ai.hpp`, registers `pursue` (`Pursue`).

`make_document(zone)` returns a loaded zone as a saved zone's `ZoneDocument`, the reverse of `load_layout`. The geometry is
its `Document`'s, which does not change during play, and the rest is the zone as it is now: its persistent id and
level; the component objects of the layout and of each room, wall, flat and flat override, each of its own file
components, with a wall or override that gained components during play added; its access points; and every entity in
its subtree but the layout entities, in persistent id order. The character, which the game owns, is in no zone's
subtree. An entity's file components are those it owns, so one it reads from its type through inheritance is the type's.
Each value is encoded through flecs reflection: an unset optional field is left out, an `EntityReference` is written as a
reference to its persistent id, and a struct field by field. A site with no address on an entity in the zone is written
in a cell of the zone, at the cell its placement is in. An access point is written as its `access_point`, and a bound
destination as the persistent id of the access point it is bound to. `rebuild_document(zone)`, which would rebuild the geometry from the layout as well, for a zone that has
no `Document`, is a skeleton with an empty body.

`make_character_document(character)` returns the character as a character file's `CharacterDocument`: its persistent id,
its own file components, its zone's persistent id, the cell its placement is in and its orientation, and every entity in
its subtree, in persistent id order, as its inventory, each encoded as an `Entity`, with its parent.

### Generation

`Generation` has the registries that `World` creates worldspaces, regions and zones from (`world_generation_plan.md`):
`m_region_specs`, a map from a name, `m_world_definition`, the `WorldDefinition` a new game's world is created from, and
`m_zone_generators`, a zone generator for each notation. A game has one zone generator: when a game is created or opened, `Game` sets `m_zone_generator` to the
one for the game's notation, so nothing after that reads the notation.
`stratum_core::RegionSpec` (`stratum_core/RegionSpec.hpp`)
and `stratum_core::ZoneSpec` (`stratum_core/ZoneSpec.hpp`) are the types of a region spec and a zone spec, and the `store` of each, a `KeyValueStore`,
contain the spec's keys. A `RegionSpec` also has members with a type of their own, each optional: `name`,
`display_name`, `parent`, `region_spec`, `seed`, `extent`, and a top-level spec's `zone_size`. `stratum_sim` uses it as
it is read. The keys that `stratum_sim` reads are in `stratum_core/spec_keys.hpp`. A `ZoneSpec`'s `position.x`, `position.y` and `position.z`, `position_x_key`, `position_y_key` and
`position_z_key`, are the zone's position in its worldspace.

A top-level `RegionSpec`'s `extent` and `zone_size` define its worldspace's coordinate system. An extent is a `stratum_core::RegionExtent` (`stratum_core/RegionExtent.hpp`): its `arrangement`, `Boxes` or `Positions`, and its
`boxes` or its `positions`. A box has an inclusive `math::Interval` on each axis, and an absent axis spans the parent's
whole range. A worldspace's extent is `Boxes` with a single box with every axis, and a worldspace with no zone size is
not a grid.

`ZoneSpec::position()` returns a spec's position, a `zone_position_type`, each coordinate it does not have 0, and throws `std::runtime_error`
for one that is not an integer; `ZoneSpec::set_position` writes one. The zone file, the generators' parameters and
`ZoneRequest` state a position.

A random engine's `seed()` returns a seed drawn uniformly from every value of its `seed_type`. `derived_seed(seed,
values)`, in `seeds.hpp`, returns a 64-bit seed that `std::seed_seq` generates from the low and high 32-bit
words of the seed and then of each value, so a derived seed is the same on every platform.

`specs.hpp` has two functions over a spec. `find_unknown_keys(spec, known_keys)` returns the spec's keys that are not
among the known keys, in key order, so a consumer reports a key it does not read. `select_prefixed(spec, prefix)` returns
the entries whose keys begin with the prefix, with the prefix removed, so one part of a spec, such as its `zone.` keys,
becomes a spec of its own.

A zone generator, `zone_generator_type`, returns the `stratum_format::ZoneDocument` a zone is built from, given the zone's
resolved `ZoneSpec`, and reads nothing from the world. `Generation.hpp` declares `ZoneDocument` without including its
header, so nlohmann reaches no file that includes `World.hpp`.

### World

`World` contains the zones and everything in them. One flecs world contains the entities, and a zone is an entity
whose contents are its children, so migration and unloading are subtree operations, and migration keeps each entity's
id. A zone is an entity with the `Zone` tag, a `Layout` component that owns its layout as a
`std::unique_ptr<LayoutBase>`, and a `ProximityIndex`. Deleting the entity deletes the layout. There is one `World` in a
game, owned by `Game`, and it cannot be copied or moved.

A worldspace is the coordinate system of its zones: the surface, a dungeon, a cave or a tower (`zone_plan.md`).
`stratum_core::Worldspace` (`stratum_core/Worldspace.hpp`) is a value, not an entity: its `worldspace_id`, a `PersistentId`, its name, its `seed`, its `extent`,
an `extent_type`, a half-open `geometry::DiscreteBox<coord_type, 3>`, its `zone_size` if it is a grid, and `regions`, its `RegionTree`.
`contains(position)` returns true if the extent contains a zone position. `Worldspace::create(worldspace_id, name, seed,
region_spec, root_region)` returns a worldspace with the seed, the region spec's worldspace extent and zone size, each
inclusive range converted to the box's half-open range, the top-level region, and no zones. It throws
`std::runtime_error` for a spec with no worldspace extent, or one that is not `Boxes` with a single box with a range on
every axis. `World` has its worldspaces in a vector,
`worldspaces()`, in the order they were created, which is id order. `worldspace(worldspace_id)` and `region(region_id)`
return one, each a linear search, and a reference to either is valid until a worldspace is created or unloaded. A zone's
position is its `zone_position_type` component. `zone_ids(worldspace_id)` returns a `zone_ids_type`, the persistent id of
each zone of the worldspace that has a position, by its position, loaded or not. `World` builds it, and it is never
written. An observer of the position lists a zone at its position when the position is set, so no two zones of a
worldspace have one position; zones of different worldspaces may.

A region groups zones of one worldspace: a dungeon, a crypt, a tower or a house. `stratum_core::Region`
(`stratum_core/Region.hpp`) is a value too: its `region_id`, its `parent_id` if it is a subregion, its `subregion_ids`,
its name, its `display_name` if its spec has one, its `components`, a `KeyValueStore`, its resolved `RegionSpec`, and
`zones`, a `stratum_core::Zone` for each zone created in it, in the order they were created, loaded or not, so its size
is the region's zone count. A `Zone` has the zone's persistent id, its position when it was listed, if it has one, and its name. A zone is listed when it is generated, loaded from a template, or created
with a layout. A saved zone read back was listed when it was created, so it is not listed again, and it is how `World`
finds the region to read an unloaded zone back into.

`stratum_core::RegionTree` (`stratum_core/RegionTree.hpp`) contains a worldspace's regions, rooted in its top-level region, which it is constructed
with. `regions()` has the top-level region first and each region before its subregions; `root()`,
`region(region_id)` and `contains(region_id)` find one; `add_subregion(parent_id, subregion)` adds one, setting its
parent and listing it in the parent's `subregion_ids`; and `extended_name(region_id)` returns the name of each region
from the top-level region down to the region, each preceded by `/`, such as `/surface/moor`.

A zone has no parent. Its `WorldspaceId` and `RegionId` components have the ids of the worldspace and the region it
is in, which `worldspace_of(zone)` and `region_of(zone)` return.

A worldspace and its top-level region have the root region spec's name, used as it is, or `worldspace_<count>` if the spec
has none, from the world's worldspace count, which rises with each worldspace created (`zone_plan.md`, `## Implementation`).
`create_worldspace(tree, seed)` makes a worldspace from a region spec tree and a seed, in five passes, with a random
engine seeded from the seed:

1. The worldspace, with that name, the seed, and the
   coordinate system of the root spec composed with its template by `composed_region_spec`, and its top-level region
   with the same name.
2. An empty zone in the top-level region at each position of the worldspace's extent, in worldspace position order:
   `z`, then `y`, then `x`, each from lowest to highest. Its spec has only its position and a seed that the random
   engine draws, and its name is the placeholder `<region name>_<zone counter>`.
3. The tree resolved by `resolve_region_spec_tree` with the same random engine, the top-level region given the root's resolved spec and display
   name, and a subregion for each spec below the root, with its spec's name, display name and resolved spec. Each
   subregion sets the region of the zone at each position of its extent, an absent axis of a box spanning its parent's
   bounding box on that axis. A subregion is created after its parent, so each zone ends in the most specific region
   whose extent contains its position.
4. Each zone, in worldspace position order, with its spec and its name replaced by those resolved from its region's
   spec, and listed in its region.
5. The root region spec's `entry_zone`, if it has one, replaced by `entry_zone_id`, the persistent id of the zone with
   that name, by `set_entry_zone_id`.

It returns the worldspace's id. It throws `std::runtime_error`, after unloading what it created, for a subregion spec
with no extent, or a subregion whose extent's bounding box reaches outside the worldspace's extent, or that claims a zone
that is not in its parent region, which is outside its parent's extent or in a sibling's, or for an `entry_zone` that
no zone, or more than one zone, has as its name.

A zone's spec is the one that `resolved_zone_spec(region_id, empty_zone_spec, zone_counter)` returns: the region spec's
`zone.` keys, with the empty zone's position, and its seed unless the `zone.` keys have one, its patterns resolved. The
zone counter counts the zones of the worldspace from 0 in worldspace position order. The name is the region spec's
`zone.name`, or else `&region_name .. '_' .. zone_counter`, as in `dungeon_9`. `resolve_zone_spec(zone_spec, resolver)`,
in `stratum_sim`'s `ZoneSpec.hpp`, resolves the spec's store
with a `PatternResolver` seeded with the zone's seed and the symbols `x`, `y`, `z`, `zone_counter`, `region_name`, and `region.<key>` for each of the region spec's
other store entries. With the surface's `zone.name`, `&region_name .. '_' .. x .. '_' .. y`, the zone at (3, 4, 0) is named
`surface_3_4`. A zone keeps its name when its content is generated or loaded from a template. Any other zone that
`World` builds or creates with a layout has the file's name, or else, if it has a position, the region's name and its x
and y in a grid worldspace and all three coordinates otherwise, or else the region's name. Zones of a region can share
a name, since a zone is identified by its persistent id.

`PatternResolver`, in `PatternResolver.hpp`, is a function object that resolves a key's value or a store. It is
constructed with its symbols, a `KeyValueStore`, its random engine, the world seed, and the `ScriptLimits` that `World`
is passed from `Config::pattern_limits`, and evaluates its patterns in a `ScriptSandbox` of its own with the symbols as
the sandbox's globals. A value that starts with `&` is a pattern, the Lua code after the `&`. A value that starts with
`&&` is literal text with one `&` removed, and any other value is literal text. The pattern `&nil` has no value, and
removes its entry from a store.

`ScriptSandbox`, in `ScriptSandbox.hpp`, is a Lua 5.5 state that evaluates code with its globals, within the
`ScriptLimits` in `ScriptLimits.hpp`: the most memory its code allocates and the most Lua instructions one evaluation
runs. Its string hashes are seeded from the world seed and its collector is stopped, so its memory is freed when it is
destroyed. `evaluate(name, code)` compiles the code as `return <code>`, or as a sequence of statements if it is not an
expression, in load mode `"t"`, and runs it in an environment of the globals: a global whose name has `.` is an entry of
a table, as `region.difficulty` is of `region`, and a global's text is an integer, a float, or a string, whichever it
reads as. A name the environment does not have is looked up in the sandbox's library: the base library's safe
functions, read-only copies of `math` without `random` and `randomseed`, `string` without `dump`, `table`, and `utf8`,
and `random`, with the interface of `math.random`, drawing from the sandbox's random engine. A name the library does not
have either is an error, and a table's absent entry is nil.

The code's value is returned as text: a boolean as `true` or `false`, an integer as an integer, and a float as
`std::format` writes a `double`, or `std::nullopt` for nil. Code that does not compile, fails, exceeds a limit, or has a
value that is not a boolean, a number, a string, or nil throws `std::runtime_error`.
`resolve_region_spec(region_spec, resolver)`, in `RegionSpec.hpp`, resolves a region spec's `name`, `display_name`, and
store entries, leaving the `zone.` keys as they are.
`zone_of` returns a null entity for a zone itself.
`unload_worldspace(worldspace_id)` unloads every zone of the worldspace, each completing in the `PersistentId` observer,
then removes the worldspace and its regions.

Every zone is made in a region. `create_zone(region_id, zone_id)` makes a zone with no layout in the region, with the
`Zone` tag, its `WorldspaceId` and `RegionId`, and that `PersistentId`, for a generator or the loader to fill. An
empty zone is a zone whose content has not been generated or loaded: it has no layout, and its `Document` is an
empty zone's document, with the zone's position, its name and its resolved `ZoneSpec` as its `zone_spec`.
`empty_zone(zone)` returns true for one, and it is saved, unloaded and read back as any other zone is.
`create_zone(region_id, position, name)` creates an empty zone with that name and a new id, the name also its spec's
`name` for the zone generator, and throws
`std::runtime_error` if the position is outside the worldspace's extent or is another zone's position. `create_zone(region_id, layout)` makes one with a new id,
that layout and nothing else, and asserts that it is given a layout. Each zone is identified by its persistent id. `zone_count` and `layout` access the zones, with
`layout` returning the interface; `create_entity`, `move` and `relocate` operate on their contents; and `unload_zone`
discards a zone, its contents and its layout.

`World` takes the game's `Generation` by reference in its constructor (`### Generation`). `world_seed()` and
`set_world_seed(seed)` access the world seed, which is 0 until it is set. `World::create` seeds a random engine from it,
whose first draw is the starting worldspace's seed. A worldspace created for an access point's destination, by
`resolve_destination` or `create_destination_worldspaces`, has the seed that `destination_seed(access_point)` derives from the world
seed and the access point's persistent id, a placeholder until a zone draws a seed pool.

`load_zone_at(worldspace_id, position)` returns the zone at the position in the worldspace, reading it back with
`load_saved_zone` if it has unloaded. If the zone is empty, it fills it from the `ZoneDocument` that `generate_zone(zone_spec)`
returns for its `zone_spec`: the game's zone generator's, unpopulated. Filling a zone keeps its position and its name,
checks the document with `check_buildable`, has `load_layout` fill the zone, creates the destination worldspace of each
access point whose unbound destination has a region spec name (`create_destination_worldspaces`), and populates it if the document is unpopulated. A worldspace with no zone at the position throws `std::runtime_error`, and an exception from the zone
generator passes on, with nothing filled. `load_template(worldspace_id, position, filepath, zone_spec)` fills the empty zone at
the position from a zone template file in the same way, and throws `std::runtime_error` for a saved zone's file or a zone
at the position that is not empty.

A zone's level, a `level_type` (`std::int32_t`), is the z of its `zone_position_type`. Access point destinations in the zone
data state levels, and `resolve_destination` matches zones by them. The loader sets a zone's `zone_position_type` from the
file's `position`, and a zone whose file has none has no `zone_position_type`; `make_document` writes the position back. A
zone built from a document whose position is outside its worldspace's extent, or is another zone's position in its
worldspace, throws `std::runtime_error` before anything is built.

`resolve_destination(access_point)` returns the access point at which an actor arrives through an access point whose destination
is unbound. The access point's zone must have a level, *L*. For the zone extended name `../[`*k*`]`, the zone is the one
that `load_relative_zone(access_point)` returns: at `new_zone_position(worldspace_id, L + k)` in the access point's
worldspace, a placeholder that returns `(0, 0, level)` until the world's layout is designed. Any other zone extended
name throws `std::runtime_error`. For a region spec name, the zone is the one that `load_entry_zone(worldspace_id)`
returns for the access point's destination worldspace. `destination_worldspace(access_point)` returns that worldspace: the one
with the region spec's name, or the region spec name if the spec has none, created from a tree of the one registered
region spec of that name if the world has none, so the access points with one region spec name lead to one worldspace. A region spec that is not registered throws
`std::runtime_error`. The entry zone is the zone whose persistent id is the root region spec's `entry_zone_id`, and a
spec with none throws `std::runtime_error`.

The zone is read back if it has unloaded and has its content generated if it is empty, and `resolve_destination` returns the
zone's `entry_point` for the access point's type and the offset from that zone's level to *L*: the lowest-ordered
access point of the type that accepts the arrival (`accepts_arrival`), one with no destination, which is arrival only, or an
unbound one that leads back to level *L* (`targets_level`). A zone with no such access point throws
`std::runtime_error`. It binds nothing.

`relocate_through(actor, access_point)` moves the actor through the access point and returns the access point it
arrives at. A bound destination is resolved to the access point it refers to; if that access point's zone has unloaded, the
zone is read back first with `load_saved_zone`, the world index having the zone of every entity something refers to. A
world with no game directory cannot read it back and throws `std::runtime_error`. An unbound destination is resolved by
`resolve_destination`, and the access point gone through and the access point arrived at are then bound to each other, each
destination becoming the persistent id of the other, which the world index then has, so going back arrives at the access point gone through, and
`entry_point` no longer chooses either. An arrival-only access point arrived at keeps no destination, so it is never bound
and receives every actor that comes through the access points bound to it. The actor is relocated to the access point's zone, at its `destination_placement`. Whatever it throws, it
moves nothing.

`random_type` is `math::Random<math::portable_random_engine_type>`, xoshiro256** seeded through splitmix64, with
distributions and a shuffle implemented in `math::Random`, so one seed draws the same values with every standard library;
`gaussian` uses the platform's `std::log`. dungen and catagen draw through the same type. `state()` returns the random
engine's state as text, and `set_state(text)` restores it, throwing `std::invalid_argument` if the text is not a state
of the random engine. `math::random_seed()` draws a 64-bit seed from `std::random_device`.

The gameplay random engine is process-wide, and is passed to no function or constructor. `gameplay_random()`, in
`RandomStream.hpp`, returns it: a `RandomStream`, which has `math::Random`'s members for seeding, drawing and saving,
and locks its mutex for the whole of each one, so threads drawing at once never corrupt its state. Its mutex and its
`random_type` are private, and no member returns either.

`load_zone(region_id, filepath)` reads the file and builds the zone from its document, which it checks with `check_buildable` before making the zone. The zone has the file's persistent id if the file has
a `zone` section, and a new one otherwise. `load_zone` then has `load_layout` fill the zone and populates it if its
document is unpopulated. It throws `std::runtime_error`, having made nothing, if the reader
rejects the file, `check_buildable` finds a reason, or the file is a saved zone that is already loaded; a template makes a
new zone each time it is loaded. An empty zone's file, which has no `cells`, is read as an empty zone with the file's
id, position, name and `zone_spec`. `load_saved_zone(zone_id)` reads a zone of the game that has unloaded back from the
game directory's `zones/<persistent id>.zone`, into the region whose `zones` lists it, and throws
`std::runtime_error` if the world has no game directory or no region lists it. `World` handles no doors itself, and neither function unloads anything.

`populate(zone)` places the zone's denizens: one denizen at the last open cell in reverse row order, facing west,
`Blocking`, with a `Speed` and no behavior, so it does not move.

`Speed` is the rate at which an actor gains energy in the turn order.

`zones()` returns the world's zones in id order, queried on each call, and `zone_count` returns how many there are. A
zone that has begun unloading is excluded, even if its deletion is queued on a stage.

`trim_zones(zone_entered)` has the world's `ZoneEntryIndex` mark `zone_entered` as the most recently entered zone and
choose the zones to unload among the zones that are not empty, then saves each, if the world has a game directory, and
unloads it. `zone_count_max`, a constructor argument, is the maximum number of zones that are not empty that the world
trims to, or 0 for no trimming, and `set_zone_count_max` changes it. `Game`
passes its config's `zone_count_max`.

A world has the game directory, the game's directory in the config's working directory, from
`create(region_spec_name, game_directory)` or `open(game_directory)` until `clear`. It has none while no game is in progress,
and a world that is part of no game has none at all, so it saves nothing. `save_zone(zone)` writes the
zone's `make_document` to `zones/<persistent id>.zone` in the game directory, creating `zones` if it is absent; it
requires a game directory and a zone built from a document. Leaving a
zone does not save it, and neither does `unload_zone` nor `unload_worldspace`, which discard what they unload, as clearing a
game does (`game_file_spec.md`, `## A saved game's files`). `save()` saves every loaded zone and then writes
`world.json` into the game directory: the worldspaces as they are, in persistent id order,
and the world index. `open(game_directory)`, on a cleared world, reads `world.json` back: the worldspaces, from
which it builds each worldspace's `zone_ids` and sets the worldspace count, and the world index. It loads no zone; each loads when it is entered.

`unload_zone(zone)` begins an unload: it adds the zone's id to the zones unloading and deletes the entity. Flecs deletes
a zone's subtree before the zone itself, and if the deletion is queued on a stage, as it is in readonly mode, it runs
the whole deletion when the stage is merged. The unload is complete in the `PersistentId` observer on the zone's own removal, which takes the zone
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
cell, since those addresses belong to the zone it left. `set_parent(e, parent)` sets only the parent, for an entity that has no site. `world_handle(e)` returns a
handle whose operations run on the world: flecs passes an observer handles that use its stage, so the handles that
`World`'s entity index, a zone's layout entity index, and `AttachedEntities` keep use the world. A zone contains
its entities through `ChildOf`, and an entity contains its children through flecs' non-fragmenting `Parent`.
`parent()`, `children()` and a `ChildOf` query term work with both, and `has(ChildOf, ...)` is true for a `Parent`
child as well.

Occupancy is `entities_at` and `entities_within`, over the entities in the zone whose sites have no address, in
ascending id order, which the zone's proximity index returns (`### ProximityIndexBase`). `entities_at` takes either a
placement or a cell position, through `DiscreteProximityIndex`. `proximity_index(zone)` returns the index.

A zone's `ProximityIndex` owns its index. `set_layout(zone, layout)` sets the zone's `Layout` and the empty
index the layout creates, and `create_zone(region_id, layout)` and `load_layout` set a zone's layout through it. Observers on
`Site` keep the index current. An entity that is a child of a zone, or is in it through `InZone`, is placed in the
zone's index when its site is set with no address, and removed when its site gains an address or is removed. An entity
in a zone through another entity, such as a coin in a chest, is not indexed. Every change of zone, through `reparent` or
`relocate`, removes the site first and sets it again after, so the entity leaves one index and joins the other. A zone
being unloaded removes nothing from its index, which is deleted with it.

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
types until they load from data: `door`, with a closed `Access`, and `staircase`, `portal`, and `waypoint`. The tests
call it too.

A layout entity is the entity of a room, a wall, a flat, a flat's override for a room, or a flat cell. Its key is
its component, and it is found by that key: a `RoomId`, `WallId`, `FlatId` or `FlatCellPosition`, the four types that
`for_each_layout_entity` visits. It has no persistent id. A flat's override for a room or a cell is a child of the
flat's entity through `Parent`, and every other layout entity is a child of its zone. A flat's override for a room has
the room's `RoomId`, and is told from the room by its parent.
The loader gives every room and each flat an entity. A wall, a flat's override for a room, and a flat cell have one
only while they have components of their own or something sited on them.

`layout_entities.hpp` has templates over the key type that take the zone. `ensure_layout_entity(zone, key)`
makes a layout entity once, and `layout_entity(zone, key)` finds it, returning a null entity if there is none.
`room_entity`, `wall_entity`, `flat_entity`, `flat_room_entity` and `flat_cell_entity`, and an `ensure_` form of each,
do the same by id. A layout entity has the `LayoutEntity` tag, which `ensure_layout_entity` adds; the observer that gives
entities in a zone their persistent id skips an entity with it. For each key type the zone has
an `EntityIndex<T>` component, a map from key to entity, and a flat's entity has an `EntityIndex<RoomId>` of its
overrides for rooms. `erase_layout_entity` removes a deleted entity from the index of its zone or of its parent.
`for_each_layout_entity` calls a function with each key type, through a fold over the four types, and `World` uses it
to register the key and index components and its observers for all four.

A wall's or flat cell's entity lists the entities sited on it in `AttachedEntities`, kept current by observers on
`Site`. Setting a site with an address creates the layout entity if there is none and lists the sited entity once;
removing the site takes the entity off the list. The wall's or cell's entity is deleted when its list is empty and it
has no components of its own. Deleting a layout entity removes the sites of the entities on it. `reparent` removes
the site before changing the parent, so an entity leaves what it was sited on while it is still in that zone. Deleting
a zone or the world unlinks nothing.

`property<T>(e)` resolves a component along a chain: the entity, its type and its type's type through `IsA`, the
room that contains it, and last the layout, on the zone entity. The first level that has the
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
was never entered, which has no entry, and two trims whose deletions are queued together choose no more than the excess. `clear`
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

`Site` is where an entity is: a `Relation`, `In` or `On`; an optional address, which is a `WallId` of one wall id
or a `FlatCellPosition` of a flat id and a cell position; and a `placement_type` relative to the address, or to the
parent if there is no address. An actor is `In` its zone with a placement and no address, and so is an entity resting
on a floor. `wall_id_of(site)` returns the site's wall id, if it has one. `placement_of(e)` and
`set_placement(e, placement)` read and write the placement and leave the rest of the site unchanged; `set_placement`
on an entity with no site gives it one.

A door is an entity with `Access`, so both of its sides are open or shut together. It has a door facing child on each
of its walls, sited `On` a `WallId`; no component marks a door facing as one. A door's walls are its sides: in
the wall notation, the two sides of one cell edge, and in the block notation, the walls the floor has toward the
doorway block. A door with one wall is a doorway into a dead end. The loader builds a door from the file's entities,
with the `door` type giving it a closed `Access`, and a generator builds one the same way.

`Access` is the state of a way through --- `Open`, `Closed` or `Locked`. A door off its hinges is
open, and one smashed into rubble that blocks the doorway is closed. `access_entity(e)` returns the entity whose
`Access` applies to `e`: `e` itself, or its nearest ancestor that has one. `access(e)` returns that state, so a door
facing has its door's state. `Detection` is how actors perceive an entity: a general `DetectionState` (`Visible`,
`Concealed` or `Revealed`) and a list of `ActorDetection`, each an actor's persistent id and its own state, for each
actor whose own state differs.
`detection(e, actor)` reads it from `e` or its nearest ancestor that has one, and returns the actor's own state if it
has one and the general state otherwise. A null actor gets the general state, and an entity with no `Detection` in its
chain is visible. One side of a secret door is concealed by a `Detection` on that door facing.

An access point is an entity at which a zone is entered or left, sited `On` a surface --- a wall, or a floor or ceiling
cell --- with a `stratum_core::AccessPoint`, the component that the zone file's `access_point` decodes to. The
`AccessPoint` has its `order` among the zone's access points of its type, counting from 0, `automatic_entry`, its
`egress`, and an optional `destination`, to which going through it moves an actor: an `AccessPoint::destination_type`,
unbound, or the persistent id of the access point it is bound to. An access point with no destination is arrival only. Its type is its entity's type, the prefab it `IsA`, so the access
points of one type are found by a query on `(IsA, <type>)`.
`destination_placement(access_point)` returns the placement given to an arriving actor, computed by the zone's
`DiscreteLayout` from the access point's site and egress. A staircase in the wall notation is sited on the floor of its
stair block, with the egress toward the floor it opens onto.

An actor arrives through an access point at an access point of the same type. The type is what the access point is,
and its destination determines whether it goes up, down, or across.

`set_access_point(e, access_point)` makes an entity of a type, sited on a surface, an access point, and
`create_access_point(zone, type, site, access_point)` creates one of the type.
`targets_level(e, offset)` returns true if the entity's access point destination is unbound and is the level
`offset` levels from the entity's zone: a relative destination with that offset, or an absolute destination at the zone's level
plus the offset, which only a zone with a level matches. `accepts_arrival(e, offset)` returns true if an actor coming
from the level `offset` levels away can arrive at the entity's access point: it has no destination, or `targets_level(e,
offset)`. `entry_point(zone, type, offset)` returns the zone's lowest-ordered access point of the type that accepts an
arrival from the level `offset` levels away, or a null entity if the zone has none. A staircase leading up is `entry_point(zone, staircase, 1)`, and an actor arriving from another level
passes the offset that leads back. A door facing may be an access point.

`find_access_attachment(zone, wall_id)` returns an entity sited on the wall that has access, found through the wall's entity and
its attached entities, or a null entity if there is none. `crossable(zone, wall_id, actor)` returns true if the actor
can get through the wall: an entity sited on it has open access, and the actor does not find it concealed. A wall with
no such entity is solid. `World::move`, `neighbors` and `visible` bind it to the entity moving or looking, so each
actor crosses a wall as it perceives the entities on it.

`Interact` opens the access of the way through on the wall ahead if it is closed, or closes it if it is open, and
reports `Interaction::Opened`, `Closed` or `Nothing`. A locked way through stays locked. If the controlled actor
`can_enter` the access point that `find_entered_access_point` returns for the wall ahead, `Interact` goes through it
instead, and an access point beyond the wall that has access is the way through that it opens or closes. `Interacted` has the entity acted
on, the way through, and `entity_type_name(e)` returns the name of an entity's type, such as
`staircase`, for the hosts' messages.

`find_access_point(zone, address)` returns an access point sited on a wall or a flat cell, found through the layout
entity's attached entities, or a null entity if there is none. `find_entered_access_point(zone, wall)` returns the
access point that an actor facing the wall enters: one sited on the wall, or one on the floor of the cell beyond it
whose egress leads back through the wall. Moving into a wall and the hosts' messages about a blocked move use
it too. `can_enter(access_point, actor)` returns true if the
actor can go through the access point: it has a destination, its access, if it has any, is open, and the actor does not find
it concealed. A staircase has no `Access`, so it can always be entered, and a door facing that is an access point only
while the door is open.

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

Granting four units to actors of speed 100 and 25 leaves the fast one with four actions' worth of energy by the time the
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
to the directions it supports. `Intent` registers no reflection, so it is not serialized.

### ai

`think` sets an `Intent` for every actor that has a behavior, except the controlled one. It gathers the entities first,
since setting a component during iteration can move an entity to another table.

`Pursue` is the one behavior: the actor moves toward a target while the target is visible and in the pursuer's zone,
then toward where it was last seen, and stops there if the target is not in sight. The position last seen is stored on the
entity, so it unloads, migrates and saves with it. Its `target` is an `EntityReference`, resolved each time, and a
`Pursue` with no `target` pursues the controlled actor that `think` is given.

The move taken is to the reachable neighbor nearest the goal, if one is nearer than the current cell. `neighbors`
returns placements in a fixed order, so ties break the same way every run. Only topology is consulted, so a pursuer is
stopped by an occupied cell in `World::move`.

### The settings store

`quetzal::settings::store()` returns the process-wide `Settings`, constructed on first use, which keeps each setting
under its key. A key is compared only as a whole string. `insert(key, setting)` adds a `Setting` that has no value,
and throws `std::runtime_error` if the key is empty or is a setting's already, the type name is not registered, the
default is not a value of the type or is out of range, or a minimum or maximum is not a value of the type or is given
for a type that is not an integer or a real. A `Setting` has its type name, its default, its scope, a name the client
chooses, whose values the client saves and loads apart from the other scopes' and which every function over a scope
takes as a `std::string_view`, whether a restart is required, its display name, description and category, the menu page that shows it, an
optional minimum and maximum, and its value as text, which is empty while the setting has its default.

`register_type<T>(type_name, values)` registers a type for any `T` that `std::format` writes and `from_chars` reads,
with its values if it is an enumeration, each of which must parse as `T`. The store registers `boolean`, `integer` as
`std::int64_t`, `real` as `double`, and `text` as `std::string`. `register_enumeration(type_name, values)` registers an
enumeration defined in data, a `std::string` whose text must be one of its values, and throws `std::runtime_error` if
the type name is registered already or there are no values. `type(type_name)` returns the `SettingType`: the C++ type;
the base type, `Boolean`, `Integer`, `Real` or `Text`, which is `Text` for every enumeration; an enumeration's values;
and a function that returns true if a text is a value of the type.

`value<T>(key)` and `set_value<T>(key, value)` read and write a setting as `T`, and assert that its type's C++ type is
`T`, which is never deduced from the value. `text` and `set_text` read and write it as text, and throw if the key is
not a setting's or the text is not a value of its type. `values(scope)` returns the settings of a scope that are set,
by key, as text; `set_values` sets each of them, and sets none if any is invalid; `reset(scope)` returns the scope's
settings to their defaults. `set_value`, `set_text` and `set_values` return false, setting nothing, if a value is out
of its setting's range, and `bounded_inclusive(key, text)` checks a value before it is set.

A change listener, added with `add_change_listener`, is called once for each `set_value`, `set_text`, `set_values` or
`reset` that changes the value of any setting, a member's or a group's included, after all of them have changed, with
their keys. Each add returns a `Settings::listener_id_type`, which `remove_listener` takes, and `clear` keeps every
listener.

A setting group is itself a setting, whose value is the name of its selected preset. `insert_group(key, scope,
modifiable, default_preset_name, member_keys, presets, display_name, category)` registers a type under the group's key, an
enumeration of the presets' names in order whose `SettingType::group` is true, and inserts the group's setting of that
type, whose default is the default preset. It requires the members to be inserted already, and throws
`std::runtime_error` for a key that is empty or a setting's or a type's already; no members or a member given twice;
a member that is not a setting, is of another scope, is another group's member, or is a group; a default preset that
is missing or modifiable; a preset name that is empty or used; a second modifiable preset or one with values; and a
preset that is not modifiable and has no values, does not set each of the members, sets a key that is not one, or sets
a value that is not one of its member's type or is out of its range. It inserts nothing if it throws.
`contains_group(key)`, `group(key)`, a `SettingGroup` of the group's key and member keys, `member_group_key(key)`, and
`preset(group_key, preset_name)` read them. A `Preset`, in `SettingGroup.hpp`, has its display name, whether it is
modifiable, and its values by member key, which are empty for a modifiable preset until they are defined.

A member's value is the selected preset's value for it, and a member never has a value of its own. Setting a group's
value selects a preset; a modifiable preset whose values are undefined first takes the values of the preset selected
before it. Setting a member writes it into the group's modifiable preset; if another preset is selected, the modifiable
preset first takes that preset's values and is selected. A group with no modifiable preset makes setting a member an
error. `set_values` selects groups' presets before it sets members, and its overload with a
`Settings::preset_values_type`, a map from a group key and a modifiable preset's name to the preset's values, sets
those values first, each set required to set each of the group's members. Either changes nothing if any value is
invalid, and notifies once, with the group's key and each member whose value changed. `reset(scope)` also leaves the
values of the scope's modifiable presets undefined, and `preset_values(scope)` returns those that are defined.

A setting can be changed by `set_value`, `set_text` and `set_values` only if its `Setting::modifiable` is true, and a
group's selection only if the `modifiable` that `insert_group` takes is. They throw `std::runtime_error`, changing
nothing, for a setting that is not modifiable; for a group member unless the member, the group, and its modifiable
preset all are; and for a group's preset values unless the group and each member are. `initialize_values(values,
preset_values)` sets values in one operation whether or not they are modifiable, and `SettingsSnapshot::restore` uses
it, so opening a game or loading a settings file sets settings that are not modifiable. Modifiability works the same in
every scope; the settings library has no rule of a client's.

`insert_definitions` applies stratum's rules: every user setting and group is modifiable, and a game setting or group
only if its definition has `modifiable` true. It throws `std::runtime_error` for a user setting or group whose
definition has `modifiable`. `Game::create` sets the game settings in the manifest's `settings` with
`initialize_values`, and throws `std::runtime_error` for a key that is not a game setting or a value outside its
setting's range. `setting_definitions.hpp` has stratum's two scope names, `user_setting_scope`,
`"user"`, and `game_setting_scope`, `"game"`, to which `insert_definitions` maps a definition file's `user` and `game`.

### Game and Character

`Game` is the game in its entirety, the one object a host owns, for as long as the host runs. It is constructed with a
`Config`, the program's configuration, which is the same for every game: `saves_directory`, the directory in which
each save is a directory; `working_directory`, which has the game directory of the game in progress and nothing else;
`zone_count_max`, the most zones the world keeps loaded at once, or 0 for no limit; `assets_directory`, the program's own data, which it
only reads; `user_directory`, in which it writes everything it creates; `logs_directory`; `log_file_count_max`, the
most log files kept in the logs directory, the current run's included, or 0 for no limit; `cache_directory`, for data
the program can make again; `mods_directory`; and `user_settings_filepath`, the user settings file. `config()` returns
it. The terminal passes `load_config(executable_directory())`, `executable_directory()` being `quetzal/common`'s
`file_util`, which reads the executable's path with `GetModuleFileNameW` and throws `std::runtime_error` on other
platforms. The Godot host's executable is Godot's, so it passes its project's directory, from `ProjectSettings`'s
`globalize_path("res://")`. `load_config(program_directory)` builds the defaults: `assets` in the program directory,
`default_user_directory()`, which is `KML/Stratum` in `%LOCALAPPDATA%` on Windows, `~/Library/Application Support` on
macOS, and `$XDG_DATA_HOME` or `~/.local/share` elsewhere, read from the environment, and `saves`, `working`, `logs`,
`cache`, `mods` and `settings.json` in the user directory, no zone limit, and 10 log files. If the program directory has a `config.json`, each
value it has overrides its default, the assets and user directories relative to the program directory and the rest
relative to the user directory, an absolute location replacing either (`game_file_spec.md`, `## The config file`). It
writes nothing. The terminal's build copies its development `config.json`, which sets `assets_directory` to
`../../assets`, beside the executable in `bin/<Config>/`, and the Godot project has its own.

`Game` is also constructed with a `stratum_format::GameManifestDocument`, in `GameManifestDocument.hpp`, which defines
the game the program plays and lasts as long as the host: its `notation`, the default notation of a new game; its
`views`, names that only the host interprets; `view_menu`, true if the host's menu offers the view and the notation;
`default_view`, one of the views; and `settings`, the values a new game's game settings take in place of their
defaults. `manifest()` returns it. A game has one notation, the notation of every zone, which
`create` is given and `open` reads from `game.json`; `notation()` returns it, or the manifest's before any game. `read_game_manifest(filepath)` reads a game manifest file,
and throws `std::runtime_error` for a key the format does not have, a notation that is not one, no views, a view
listed twice, or a default view that is not one of the views (`game_file_spec.md`, `## The game manifest file`). Each host loads the file that its `manifest=` argument refers to, relative to the assets
directory.

`Game` owns the structures that survive outside play: the config, the manifest, a
`CountingIdAllocator` counting from zero, the `Generation`, the `World` and the `Character`, and the `Session` that plays them, constructed in
that order, each referring to those before it. `generation()` returns the `Generation`, with which a host registers
its zone generators and region specs. Only the `Generation` can be copied or moved. The constructor makes no
character, region or zone; until a game is created, there is none.

The game's settings are in the process-wide `settings::store()`.

In the settings library, `SettingsSnapshot`, in `SettingsSnapshot.hpp`, is one scope's values as they are saved and
restored: each setting that is set, groups' selections among them, and the defined values of each modifiable preset, as
text. `SettingsSnapshot::capture(scope, store)` takes one, and `restore(scope, store)` checks every key, preset and
value, then sets the presets' values and then the settings, with one `set_values`, so a member's value among the
settings is set as `set_values` sets one. It throws `std::runtime_error`, changing nothing, for a key that is not a
setting's or a group's of the scope, a preset or value that is invalid, or a value out of range, naming the key.

In `stratum_sim`, `setting_definitions.hpp` has `insert_definitions(document, store)`, which registers a definition
document's enumerations, then inserts its settings, then its groups, a modifiable preset with no values in the file
being undefined, and throws as they do for a definition that is invalid, and `load_setting_definitions(assets_directory,
store)`, which reads the game's definition file and inserts it.

`PersistentSettings(filepath, scope, store)`, in `PersistentSettings.hpp`, is one scope's settings, kept in a settings
file. It adds a change listener, removed when it is destroyed, that notes whether a setting of its scope changed. `load` restores the file's snapshot if there is a file, counting
none of it as a change, and throws, naming the file, if it cannot be read or restored; `changed` returns true if any
changed since the settings were loaded or saved; and `save` and `save_if_changed` write the scope's snapshot, creating
the file's directory if needed.

Each host calls `load_setting_definitions` once when it starts, then loads a `PersistentSettings` of the user settings
file and `user_setting_scope` (`game_file_spec.md`, `## The user settings file`), and calls its `save_if_changed` when
it closes: the terminal in `Application`'s destructor, and Godot in `uninitialize_stratum_module`. In
`stratum_sim_test`, `WorldFixture` calls `load_setting_definitions` once in the process, and returns every setting to its
default before each test case that starts from it.

`Game` has the menu and lifecycle operations a host calls: `menu_open`, `menu_close`, `clear`, `create`, `open`,
`save`, `quit` and `saved_games`. They call `clear`, `create`, `open` and `save` on `World` and `Character`, and
`clear`, `start`, `pause` and `resume` on `Session`.

`Game::clear` returns the game scope's settings to their defaults, which selects its groups' default presets and leaves
their modifiable presets undefined, so a game created or opened starts from them.

The working directory has only the game in progress. `Game::create(character_spec, notation, seed)` clears, applies
the manifest's game settings, sets the game's notation, resets the id allocator to count from zero, and sets the world seed to the seed, or to one from `math::random_seed()`
if none is passed, with `World::set_world_seed`. It seeds the gameplay random engine with the world seed, keeps it, and
writes it to the log, so nothing passes from one game to the next. It empties the working directory and creates the game directory there, named by the
character, whose name the spec must have, with each character a directory name cannot have replaced by `_`. It has the
character created from the `CharacterSpec`, and the world's starting worldspace and its first zone from the root spec of
the world definition's starting region, with `World::create` and the game setting
`Game::start_zone_key`, `world.start_zone`, and starts the session in that zone at the `start_placement` for the game
setting `Game::start_cell_key`, `world.start_cell`.
If either throws, it clears again and removes the game directory before passing the exception on, so a failure leaves
no game in progress.

`Game::open(save_name)` clears, then empties the working directory and copies the save into it as the game directory,
named by the character as `create` names it, read from the top of the save's `character.json`. It reads `game.json`: the
game's notation is set from `notation`, the id allocator counts on from `next_id`, the game's settings snapshot is restored, the world seed is set from `seed`, and the gameplay random engine's state is restored with `set_state`, or the
random engine is reseeded from `math::random_seed()`, the seed written to the log, if the boolean setting
`Game::reseed_on_open_key`, `gameplay.reseed_on_open`, is true; the random engine and the world seed are the same either way. Restoring repeats the outcomes the save would have had; reseeding gives
each opening different outcomes. `World::open`
then reads `world.json`, `Character::open` reads `character.json`, and the session starts in the character's zone,
which `load_saved_zone` reads, at its cell and orientation. It throws `std::runtime_error` if there is no such save or
a file in it cannot be read, and clears and removes the game directory first, so a failure leaves no game in progress.

`saved_games()` returns a `SavedGameDetails` for each save in the saves directory, newest first: the save's name,
which `open` takes; the character's name as the player entered it, read from the top of the save's `character.json`
without parsing the character; an empty
description; the time `game.json` was written; and its `SaveType`, from the suffix that ends the save's name. It throws `std::runtime_error` if a save's character file cannot be
read or has no name.

`Game::save(save_type)` writes `game.json` into the game directory, with the game's notation, the id allocator's next id, the world seed, and the gameplay
random engine's state as `state()` returns it, under `random_engines`, and the game scope's settings snapshot
under `settings` and `presets`; has the character write `character.json` there, through
`make_character_document`, and the world save; and then copies the game directory to a new directory in the saves
directory, `<character name>_<save time>`, the name the character's `Name` and the time local and in ISO 8601
basic format, which it returns. A `SaveType` of `Auto` or `Quick` appends `_autosave` or `_quicksave`, and `Explicit`
nothing. A character of the name that Windows does not allow in a file name, or a control character, is replaced by
`_`. If a save of that name exists, it waits for the next second. An earlier save is never
replaced. It requires a game in progress whose character is alive and named.

`World::create(start_zone, game_directory)` creates the starting worldspace with `create_worldspace` from the tree of the
world definition's `starting_region`, named for that region, and returns the zone at `start_zone` from `load_zone_at`,
generated: `center`
(`center_start`), the zone at the centroid of the worldspace's extent with each coordinate rounded toward the lower bound,
or a zone position written `x,y,z`. `start_placement(zone, start_cell)` returns the placement of an actor starting in the
zone, facing -y: `center`, the cell nearest the center of the zone's layout that an actor can occupy, the first in row
order of those as near, or a cell position written `i,j`. Each throws `std::runtime_error` for a value that is neither, a
zone position with no zone in the worldspace, or a cell position an actor cannot occupy, and `World::create` for a start
tree that is not one of the trees, or a `region_spec` that is not registered.

`Game::clear` clears the session, the character and the world, in that order, in place. `Session::clear` ends play:
no zone is played, and the turn order and the clock's fraction of a unit are emptied, while the driver remains.
`Character::clear` deletes the character's entity, with what it carries and its pending command. `World::clear` unloads
every worldspace, empties the world index and the `ZoneEntryIndex`, and resets the worldspace count, and keeps the registrations: the entity types, the file components, and the `Generation` it was given. `menu_open` pauses the session and
`menu_close` resumes it, and `quit` clears the game. `remove(save_name)` deletes a save, named as `saved_games`
names it, after checking that the name is of a directory directly in the saves directory, with no separator and not
`.` or `..`, that has a character file; it throws `std::runtime_error`, deleting nothing, otherwise. `CharacterSpec`, which `create` takes, is the choices a new game makes: a `name`; `stats`, a map
from a name to a value; `skills`, which include feats and abilities, and `aspects`, such as the spells of a
spellcasting skill, each a map from a name to its properties; an `inventory` of `InventoryItem`, each an item's `type`, its
`InventoryLocation` (a `relation` and a `place`), and its `properties`; and the game's other choices, such as race and
class, as `properties`.
`session_plan.md` (`## Games`) has their design.

`Character` is the entity a host controls. `create(character_spec)` makes it, with `Blocking`, the `ControlledActor`
tag, a `Speed`, a new `PersistentId`, and the spec's `name` as its `Name`, and it is in no zone until one is
entered. `save(game_directory)` writes its `make_character_document` to `character.json` there, and
`open(game_directory)` makes the entity from `character.json` there, with the `ControlledActor` tag and what
`load_character` fills, in no zone, and returns its `CharacterPlacement`: its zone's persistent id, and its cell and
orientation. `entity()` is null before `create` and after `clear`. It is owned by the game and has no parent, so it outlives every zone it moves through. A host
acts through it: `set_intent(intent)` sets the character's `Intent`, and `submit(action)` sets a `PendingCommand` of the
action for the character's entity, which the clock applies the next time the character is ready. `entity()` returns the
entity, for queries and drawing.

### Session

`Session` has the zone being played: references to the game's `World` and `Character`, a `Scheduler`, a `Clock`, and the
zone. A host supplies input mapping and drawing.

`Session` makes no zone. Each host creates a game with `Game::create`, which starts the session in the generated first
zone. The library has no default entry type or offset.

Each host installs its sinks on the process-wide log (`log_plan.md`). Each writes a log file for each run, which may
span several games, in the config's `logs_directory`. Its base path is `stratum_terminal.log` or `stratum_godot.log`
there, and when it starts it installs a `FileLogSink` writing to the path `log::timestamped_filepath` returns for the
base path and the time, such as `stratum_terminal_20261022T223005.123Z.log`. Then
`log::remove_oldest_timestamped_files` removes its oldest files of that form until at most the config's
`log_file_count_max` remain, the new one included, and the host logs a warning for each file it returns as not removed.

In `quetzal/log/timestamped_files.hpp`, `timestamped_filepath(base_filepath, time)` inserts the time before the base
path's extension, after `_`, in ISO 8601 basic format in UTC to the millisecond, so the files of one base path sort by
time. `remove_oldest_timestamped_files(base_filepath, file_count_max)` keeps the newest `file_count_max` files of that
form for the base path, removes each older one, and returns each it could not remove as an `UnremovedFile` with its
`std::error_code`. A file it could not remove takes no newer file's place, and a `file_count_max` of 0 removes nothing.

The terminal's file is its only sink, since anything written to the console would
write over its frame. Godot installs its sinks when its extension initializes at the scene level, reading the config
for the purpose, and clears them when it uninitializes: a sink writing errors and critical entries with `push_error`,
warnings with `push_warning`, and the rest with `print`, and its run log file. The log is at `info`.

Each host registers, for each notation, the generator of `stratum_gen::generators()` that `stratum_gen::generator_name`
returns, `dungen` for the block notation and `catagen` for the wall notation, in `m_zone_generators`, as a
`Generation::zone_generator_type` that makes a `ZoneRequest` from the `ZoneSpec` and passes it on: the spec's `name`,
the spec's position, the `seed`, which `resolved_zone_spec` adds to every `ZoneSpec`, and
the keys after `parameters.`, with a temporary `extent` from the game settings `world.zone_size.x` and `.y` if the spec
has none. It registers the region specs that `load_region_specs` reads from the assets directory and the region spec
world definition that `load_world_definition` reads, and adds to the starting region's spec the default zone size of the game
settings as its temporary `zone_size`. It creates the game with a temporary `CharacterSpec` with the name
"Adventurer". So the character begins at the center of the surface zone at (4, 4, 0), which is `surface1`'s open
ground, and each of its staircases leads down to the first level of a dungeon worldspace of its own, `dungeon_1` and
on, created from the `dungeon` spec when the surface zone is generated.

`stratum_gen` links no part of `stratum_sim`. A `ZoneRequest` is what one zone is generated from: its `name`, empty for
none, an optional `position`, a `seed`, and `parameters`, which the generator reads, as the tokens its zone file's `generator`
section has. `generator_function_type` takes a `ZoneRequest` and returns a `stratum_format::ZoneDocument`, and
`stratum_gen::generators()` returns a map from each generator's name to its function. `generate_dungen`, under
`dungen`, and `generate_catagen`, under `catagen`, each set the parameters the request's `parameters` refer to with
their library's `set_parameter` and skip the keys that are not its parameters, since every generator is given the same
tokens. The request's position and seed replace any the tokens set, and each returns the document its library's
`make_document` builds in memory. dungen writes its warnings to the
process-wide log. The `ZoneSpec` keys are `stratum_sim::name_key`, the position keys, `seed_key` and
`parameters_prefix`.

`start(zone, placement)` relocates the character to the placement in the zone, and `start(access_point)` to the access
point's arrival placement in its zone; each then has the world `trim_zones` with the zone entered, so no zone the character is in can unload. It makes the zone the one being played,
removes the character's pending command, and rebuilds the turn order from each entity in the zone that has a `Speed`,
the character included. `enter(zone, type, offset)` starts at the zone's `entry_point(zone, type, offset)`; if the zone
has no such entry point, it throws `std::runtime_error`, and the character stays where it was.

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
omits it. If the character went through an access point into another zone, it then makes that zone the one being played, as
`enter` does once it has placed the character: the world trims its zones, the character's pending command is dropped, and
the turn order is rebuilt from the new zone. The actors of the zone left finish the advance in which the character left.

`Dead` is the tag for an actor that has died. `over()` returns true once the character of the game being played has it.
The clock then runs no actor, stopping within an advance if the character dies during it, and turn-based skipping ahead
stops too. Every `advance` once the game is over runs no actor, reports `Died` with the character in its events, and
returns `nullid`. Each host shows the death with the game's last state until a key is pressed, then calls
`Game::quit` and returns to its menu.

Each host maps `<` and `>` to `Enter`. The hosts report a `Relocated` event with the level arrived on, shown as its
distance from the surface, and a step `Blocked` by a wall with an access point on it as "There is a staircase before
you", with the access point's type.

### Commands and events

`Command` and `Event` are type erased over `std::any`, so a module can add either. A command has the id of the entity
it acts for. `CommandQueue` and `EventQueue` are deques in submission and occurrence order.

The commands are `Step`, `TurnTo`, `Interact`, and `Enter`. `Step` has a direction, zero for the actor's facing
direction; a distance; and `face`, which turns toward the direction within the same action. `TurnTo` has an absolute
facing direction, and `Interact` acts on the wall `ahead`. `Enter` enters the access point on the floor of the actor's
cell, in a discrete layout. They produce `Moved`, `Blocked`, `Turned`, `Interacted`, and `Relocated` events, and `Session::advance` produces
`Died`.

The controlled actor enters an access point on the wall ahead with `Interact`, and one on the floor of its cell with
`Enter`, through `World::relocate_through`; any other actor does not enter an access point. A `Step` into a wall with
an access point on it is `Blocked`, so a held movement key never takes the stairs. Entering by
moving into the wall is kept in `commands.cpp` behind `enter_access_point_by_moving`, which is off. An access point on
the floor does not impede movement. `Relocated` has the actor, the departure access point, and the destination access
point.

The clock builds `Step` and `TurnTo` from an actor's `Intent`. A one-shot command, such as `Interact`, waits on the
actor as a `PendingCommand` component, which the clock applies before the actor's `Intent` the next time the actor is
ready; a host submits one through `Character::submit`.

`apply(world, command, events)` returns the command's cost, or zero if it did nothing; the `CommandQueue` overload
drains the queue. A move costs in proportion to the distance traveled against `step_distance`, so a move stopped short
is charged for the distance reached and a fraction of a cell costs a fraction of an action. A turn and an interaction
cost one action; a `TurnTo` to the current facing direction and an `Interact` with no wall ahead cost nothing. Going
through an access point costs one action, and an `Enter` with nothing to enter costs nothing.

## Core types

`quetzal::stratum_core` contains the data types that `stratum_format`, `stratum_sim` and the generators share, with
their operations, and has no file format and no ECS. It links `quetzal_common` and `quetzal_math`, and `quetzal_log`
privately, and includes the `quetzal/geometry` headers.

- `types.hpp`: `log_category`, `value_type`, `traits_type`, `point_type`, `coord_type`, `discrete_vector_type`,
  `discrete_point_type`, `zone_position_type`, a `geometry::DiscretePoint<coord_type, 3>` that is a position in its
  worldspace, and `seed_type`.
- `PersistentId`, an entity's identity in a world, with its `std::hash`.
- `AccessPoint` and `RegionSpecName`, `ZoneExtendedName`, `TransitEndpoint`, `Directionality` and `Notation`, which
  `## Zone file format` describes.
- `RegionExtent`, `RegionSpec` and `ZoneSpec`, which `### Generation` describes, and the spec keys in `spec_keys.hpp`.
- `Region`, `RegionTree`, `Worldspace` and `Zone`, which `### World` describes.

## Zone file format

`quetzal::stratum_format` reads and writes the JSON zone file that `discrete_layout.md` specifies, version `0.6`. It
depends on `quetzal_common`, `quetzal_stratum_core` and nlohmann/json. `load_layout` fills a zone from what `read_zone` returns, and both generators
write through it.

`grid.hpp` has the geometry the rows define --- `on_grid`, `floor_cell`, `floor_door_cell`, `stair_cell`, `has_wall`,
`room_cells` and `door_walls` --- which `read_zone` uses to validate a file and `load_layout` uses to build a layout.

`references.hpp` finds and translates the references a file writes as `{ "entity": id }`. `make_reference` writes one,
`reference_id` returns the id if a value is a reference, `references` returns every reference within an object of file
components in the order written, and `translate_references` replaces each with the id a function returns for it.

`ZoneDocument` is the file as data structures. The geometry is decoded into types: the grid and its rows, expanded from
runs; the barriers; each room's perimeter or `RoomLocator`; wall placements; and each flat's heights. So are each
entity's `site`, `parent` and `access_point` file components, which the reader validates. A `Site`'s address is a
`WallPlacement`, a `FlatCell`, or a `ZoneCell`, a cell of the zone that an entity such as an actor is in, with the
relation `in` and an orientation required. An entity's `access_point` is a `stratum_core::AccessPoint`
(`stratum_core/AccessPoint.hpp`), which the world sets on the entity as it is. The layout's `access_points` are `ZoneDocument::access_points`, a list of entity
ids, its optional `position` is `ZoneDocument::position`, a `zone_position_type` (`geometry::DiscretePoint` of three
coordinates), and its `unpopulated` flag is `ZoneDocument::is_unpopulated`, false if
absent. The `generator` section is `ZoneDocument::generator`, an optional `Generator` with its name and its parameters'
tokens, a `KeyValueStore`. Every other file component is `raw_values_type`, nlohmann's `ordered_json`, as
the file wrote it. A room is kept as the file identified it.

An `AccessPoint::destination_type` is where going through an access point leads: a `std::variant` of the `PersistentId`
of the access point it is bound to, a `RegionSpecName`, and a `ZoneExtendedName` (`stratum_core/ZoneExtendedName.hpp`), each of
the last two having its text as its `value`. `parse_zone_extended_name(text)` returns a zone extended name for text
that matches the whole extended name grammar (`discrete_layout.md`, `### An access point's destination`), or nothing.
`relative_zone_extended_name(k)` returns `../[`*k*`]`, *k* zone levels from the access point's zone, with *k* signed,
and `relative_zone_offset(zone_extended_name)` returns *k* for a name of that form. `valid_segment_name(text)` returns true for a
letter or underscore followed by letters, digits, and underscores. `parse_access_point_destination(text)` returns a region spec name
for a name and a zone extended name for any other text that `parse_zone_extended_name` accepts, or nothing, and
`format_access_point_destination(destination)` writes either back as the same text. A `TransitEndpoint` (`stratum_core/TransitEndpoint.hpp`) is one end of a transit between zones: an id,
an optional name, its entity type, its cell position, its orientation, its egress direction, and an optional `Directionality`
(`stratum_core/Directionality.hpp`), up or down, with `name` and `to_directionality`. A saved zone's `zone.id` is `ZoneDocument::zone_id`, which makes every id in the file persistent; it is
empty in a zone template.

`read_zone` validates the file, throwing `std::runtime_error` with the path within the document and what was wrong there.
Beyond each section's structure, it checks what the rows determine: in the wall notation, a wall between every floor
cell and a blank cell, a stair cell or the edge of the grid; a wall in the grid at every wall placement, whose cell is a
floor/door cell; a room's `RoomLocator` cell on a floor cell; a perimeter within the grid and along cell edges that
never meets itself, each of whose cell edges is a wall toward a floor/door cell or a virtual wall between two; and no
cell in two rooms, compared only for rooms whose bounding boxes intersect.

It then checks the entities. Ids must be whole, non-negative and unique. Every entity must be sited on the geometry, be
a child, or be a parent, with an entity sited on the geometry in every hierarchy and a parent for every entity sited on
one, and no `parent` may lead back to its own entity. In a template, every reference must be an entity's id; a saved
zone's references may refer to another zone and are not checked. Each door glyph requires one entity with a child
sited on each wall of its doorway and on no other wall. `layout.components.access_points` must refer to at least one
entity with an `access_point` sited on a surface, one on the floor or the ceiling with its site's `orientation`, and
each access point must have a `type` and an `order` counting from 0 among the access points of its type, and place an
arriving actor in a cell an actor can stand in: the cell of the wall or the flat cell, plus its egress. Every entity with an `access_point`
must be listed in `access_points`. An access point's destination is optional, and must be a reference, a region spec name, or
a zone extended name. The stair glyphs are visual aids, so a stair glyph with no access point on its
floor, or with a second one, and an access point on a wall that is not a door's wall, are warnings. So is `unpopulated`
in a file with a `zone` section.
`read_zone` writes them to the process-wide log as warnings in the category `stratum_format`
(`log_plan.md`). Keys the format does not define are not tracked.

`write` emits the written form: two-space indentation, a grid row per line, an entry per line with each of a flat's
per-room and per-cell entries on a line of its own. Top-level sections follow the format's key table. Within an entry, its address or
identification comes first, then the rest in its section's table order, and file components stay in the order read.
An entity's `parent`, `site` and `access_point` are written into its `components` ahead of the rest. In runs, a count is written for a
run of three glyphs or more. A document that `read_zone` produces is written back equivalent to it.

The game, character and world files that `game_file_spec.md` specifies, format `quetzal.stratum_game`, version `0.4`
(`saved_game_format.hpp`), are each a data structure in a header of its own: `GameDocument`, with the game's next id, world seed, each random engine's state by stream in `random_engines`, and its
settings; `CharacterDocument`, with the character's id, components, zone, placement, and inventory, whose entities are
`Entity`s; and `WorldDocument`, with the worldspaces, each a `stratum_core::Worldspace`, and
`entity_zones`, the world index. A `write` overload for each, in `write_game.hpp`, `write_character.hpp` and `write_world.hpp`, emits its
written form, with an absent object written as an empty one. These writers and the zone's, `write_zone`, share the text
of the written form through `json_util.hpp`, internal to the library in `stratum_format::detail`.

`read_game`, `read_character` and `read_world`, in `read_game.hpp`, `read_character.hpp` and `read_world.hpp`, read and
validate the game files, from a file or from a stream with a name for messages, and throw `std::runtime_error` with the
path within the document and what was wrong there. Each requires the format and version,
and the keys and value types that `game_file_spec.md` specifies. `read_character` also requires that the file's `name`,
a copy of the character's name, equals the `name` component, and that every entity of the inventory has a unique id
and a parent, the character or another entity of the inventory, with no cycle, and that it
is sited on its parent if sited at all, with no address and no access point. `read_world` requires worldspace, region and
zone ids unique across the world, zone positions unique in each worldspace, a top-level region spec that
`Worldspace::create` accepts, a string for each region component, and every entry of
the world index in a zone that a region lists, once. The readers share
the checked reading of values through `checked_values.hpp`, in `stratum_format::detail`. Each file's header is its
leading keys, `format` and `version`, and in the character file `name` after them; `check_file_header` checks it, and a
writer emits it with `file_header_sections`.

The config file is in the same library with a format of its own, `quetzal.stratum_config`, in `config_format.hpp` with
its filename, `config.json`. `ConfigDocument` has each location as written, `zone_count_max`, and `log_file_count_max`; `write`, in
`write_config.hpp`, emits each key on a line of its own, and `read_config`, in `read_config.hpp`, requires the format,
the version, and every key (`game_file_spec.md`, `## The config file`).

A setting definition file has the format `quetzal.stratum_setting_definitions` (`settings_plan.md`, `### The definition
file`). `SettingDefinitionsDocument`, in `SettingDefinitionsDocument.hpp`, has the format's name and version and the file's path, `data/setting_definitions.json` relative to the assets directory or
a mod's directory, and the file's `enumerations`, `settings` and `groups`, each in the file's order, with every value as
text and a scope as `Scope::User` or `Scope::Game`. A setting's `default` is required, and so are a group's `members` and
`presets`. A setting and a group have `modifiable`, false if absent. `read_setting_definitions` requires the format
and the version, allows each section to be absent, and throws `std::runtime_error` for a key the format does not have, a
required key that is absent, a value that is not text, or a scope that is not `user` or `game`. The game's file is
`assets/data/setting_definitions.json`, with the difficulty multipliers and the `gameplay.difficulty` group, all
modifiable, and the world's dimensions: the game settings `world.world_size.x` and `.y`, 1000000 by default, and
`world.region_size.x`, `.y` and `.z`, 1000 by default, and `world.zone_size.x` and `.y`, the size of a zone in cells, 40
and 20 by default, each fixed for the game. The terminal's manifests set the zone size: 40 by 20 in the block notation
and 32 by 12 in the wall notation, whose lattice view is about twice as large. The text game settings `world.start_zone`
and `world.start_cell`, `center` by default, are where a new game starts.

A region spec file has the format `quetzal.stratum_region_specs`. `RegionSpecsDocument`, in `RegionSpecsDocument.hpp`,
has the format's name and version, the file's path, `data/region_specs.json` relative to the assets directory or a
mod's directory, and the file's `region_specs`, each a `stratum_core::RegionSpec` by its name.
`stratum_core::RegionSpec` is a region spec's file form, read and written by `stratum_format`'s `RegionSpec.hpp`: `name`, `display_name`, `parent` and `region_spec` as text, `seed`
written as text and read as a seed, `extent`, `zone_size` as `[x, y]`, and its other keys, each a dotted path such as
`zone.parameters.algorithm` with its value as text. An extent is an object with exactly one of `boxes`, its
boxes, and `positions`, its zone positions, which sets its `arrangement`; a box's axis is an integer or `[lower, upper]`, and a position is
`[x, y, z]`. `read_region_specs` requires the format, the version, and `region_specs`, and throws
`std::runtime_error` for a key the format does not have, a region spec that is not an object, a value that is not text,
a seed that is not a seed, a key under `extent.` or `zone_size.`, which belongs in the member's value, an extent
without exactly one of `boxes` and `positions`, or an interval whose lower bound is
above its upper bound. `stratum_sim::load_region_specs(assets_directory)`, in `RegionSpec.hpp`, returns the
`RegionSpec`s by name. The game's file is `assets/data/region_specs.json`, with two top-level region specs: `surface`,
whose extent is x and y 0 to 9 and z 0, whose `zone.name` is `&region_name .. '_' .. x .. '_' .. y`, and whose zones are
generated by `surface1` with 2 to 4 staircases leading down, none leading up, and the `stairs_down_region_spec`
`dungeon`; and `dungeon`, whose extent is z -100 to -1 and whose `zone.name` is `&region_name .. '_' .. math.abs(z)`.

A world definition file has the format `quetzal.stratum_world_definition`. `WorldDefinitionDocument`, in
`WorldDefinitionDocument.hpp`, has the format's name and version, the file's path, `data/world_definition.json` relative
to the assets directory or a mod's directory, the file's `starting_region`, the root region name of the tree a
new game starts in, and its `region_specs`, a list of trees, each a list of region specs, as in a region spec file.
`read_world_definition` requires the format, the version, `starting_region`, and `region_specs`, and throws
`std::runtime_error` for a key the format does not have, a tree that is not an array, a region spec that is not an
object, or a region spec the region spec reader rejects. `write` writes each tree's region specs a line each, the members
with a type of their own before the keys. In a tree, a spec's `name` is its region's, its `parent` is the name of its
parent region, which the root spec does not have, and its `region_spec` refers to the registered spec it is based on.
`stratum_sim::load_world_definition(assets_directory)`, in `WorldDefinition.hpp`, returns a `WorldDefinition`: its
`region_spec_trees`, each tree by its root region's name, and its `starting_region`. A tree is a vector of
`RegionSpecNode`s (`RegionSpecNode.hpp`), the root spec first and each spec before its subregions. A spec's id is its
slot, and each `RegionSpecNode` has its `RegionSpec`, its parent's id, `nullid` for the root, and its subregions' ids.
It throws `std::runtime_error` for a tree without exactly one root spec, a spec with no `name`, a `parent` that is not
the name of exactly one spec of the tree, two specs with the same parent and name, a spec that is not below the root, a
root spec with the name of another tree's root, or a starting region that is not a tree's root.
`resolve_region_spec_tree(tree, root_region_name, region_specs, random, world_seed, script_limits)` returns the tree with each spec resolved and
composed, parents first. `composed_region_spec(region_spec, region_specs)`, in `RegionSpec.hpp`, layers a spec's members
and keys over those of the registered spec its `region_spec` refers to, without `region_spec` and `parent`, and
`resolve_region_spec` resolves the result with a resolver seeded by the spec's `seed`, or else by the seed that the
random engine draws for it, one for each spec in tree order. The resolver's symbols are `region_name`, the root
region's name for the root spec and the spec's `name` for a subregion spec, and for a subregion the parent's `name`,
`display_name`, and store entries other than the `zone.` keys, each under `parent.`. A spec's `entry_zone`, such as the
dungeon's `&region_name .. '_1'`, is the name of the zone at which a worldspace created from it is entered. A subregion's resolved store is then layered over its parent's resolved store, less
the keys whose patterns resolved to nil, since a subregion does not inherit the typed members. A parent's values are
resolved once, so an inherited literal that starts with `&` stays literal. The game's file is
`assets/data/world_definition.json`, starting in `surface`, with two trees of one spec each, `surface` and `dungeon`,
each based on the registered spec of its name.

A settings file, such as the user settings file, has the format `quetzal.stratum_settings` (`game_file_spec.md`, `## The
user settings file`). `SettingsDocument`, in `SettingsDocument.hpp`, has one scope's `settings::SettingsSnapshot`, as
`GameDocument` has the game scope's. `read_settings` and `write` read and write a settings file, and
`detail::read_snapshot` and `detail::add_snapshot_sections` read and write a snapshot's two sections for both files.
`quetzal_stratum_format` links `quetzal_settings` for the snapshot.

`read_character_name` reads only the top of a character file, through nlohmann's SAX parser, and stops once it has the
format, the version and the name, so nothing after the name is parsed, however much the character carries. It throws
`std::runtime_error` if any of the three is absent or wrong, or the file is not JSON up to them. `Game::saved_games` and
`Game::open` read a save's character name with it.

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
cell. A wall or door on the edge between two cells has no position there, so
it reduces to `Floor` on both sides.

In both reductions, stairs are drawn on a floor cell with an access point on it, and on the closed cell beyond a wall
with an access point on it, which is a stair block. They are `StairsDown` if the entity's `Directionality` is `Down`, and
`StairsUp` otherwise. The site's orientation is not drawn. The glyph
sets draw `StairsUp` as `<`, the entrance glyph, and `StairsDown` as `>`, and `read_glyphs` names them `stairs_up`
and `stairs_down`.

`lattice(world, zone, viewer)` reduces the zone onto the wall notation's lattice: `(2W+1)` by `(2H+1)`, with cell
`(i, j)` at `(2i+1, 2j+1)`, the wall west of it at `(2i, 2j+1)`, the wall south of it at `(2i+1, 2j)`, and corners at
the positions that are even in both. It emits cells, `Wall` or `Door` on edges, and `Wall` at a corner if any of its
four edges has a wall or a door.

A door occupies the cell beyond it if that cell is open and has no wall back toward it, and is
then placed once in that cell; otherwise it is on the edge. Each wall is visited from the open cell it faces, which
reaches every wall.

`mask` is the set of directions in which the wall line continues from a position: a direction is set if a wall or a
door is on the edge or corner that way. A cell is never part of a wall line, so a door that occupies one is a gap in
it. There are sixteen masks, and a glyph set with fewer glyphs maps several of them to one.

A position is `Solid` if every cell it touches is closed. A cell touches itself, an edge touches the two cells it lies
between, and a corner touches the four that meet at it; positions beyond the grid count as closed. A wall faces open
floor, so no wall is at a solid position, which the reduction asserts.

Both reductions set the actor flags last, from `each_actor_cell`.

`Buffer<T>` is the grid, with `extent`, `contains`, `at`, `set` and `fill`, and row 0 lowest. Its
position and extent are `stratum_view`'s discrete point and vector types. A private
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

`cell_glyphs()` has lines for the three types a cell can be, a door shut and open, and an actor on each type it can be on.
Its actor lines come first, so an actor in an open doorway is drawn. `lattice_glyphs()` draws in the wall notation's
glyphs, with floor as a space and a line for each of the sixteen wall masks.

`read_glyphs` reads a glyph set from a file; `assets/glyphs/` has the two built-in sets. The first line states the
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

The host starts at the main page of the menu, drawn in place of the map, with `n` for a new game, `c` to continue the
game in progress and `s` to save it, each offered only while there is one, `o` to open a save, and `q` to quit, which
clears the game and ends the run. With the manifest's `view_menu` true, the page also shows the view, starting on the
default view, with `v` to change it to the next of the manifest's views, and the notation, with `t` to change it to
the other one; with `view_menu` false, the host uses the default view and the manifest's notation. The view applies at once; the notation is the one a new game is
created in, the manifest's at first and the opened game's after a save is opened. Escape on the main page also quits, so Escape twice during play ends the run. A new game is created from the host's temporary specs, its
character named "Adventurer". A failure to create one is shown on the menu's status line, and so is "Game saved.",
after `Game::save` with `SaveType::Explicit`, or the reason it failed. `o` shows the saves page: the nine newest saves
from `Game::saved_games`, numbered, each as its character's name, the local time of the save to the minute, and its
save type, "Autosave", "Quicksave" or nothing for an explicit save, and never by its directory's name, with the selected save marked `>`. Up and Down, or `k` and `j`, move the selection,
Enter opens the selected save and a digit the save of that number, with `Game::open`, and start play. Delete asks on
the status line whether to delete the selected save; `y` deletes it with `Game::remove` and lists the saves again,
keeping the selection on its row or the new last one, and any other key keeps it. Escape returns to the main page. A
failure to list or to open is shown on the main page's status line, and a failure to delete on the saves page's.
During play, Escape opens the menu, through `Game::menu_open`, and no other key quits. When the
character dies, the map stays with "You have died." on the status line until a key is pressed, and the host then quits
the game and opens the menu. A game whose `advance` throws cannot continue, so the host quits it and opens the menu,
with the reason on its status line.

The loop advances the session to the actor it is waiting on, draws, reads a key, and submits it: movement and facing
direction as the actor's intent, and a one-shot action as a command. Arrows or `hjkl` move, space interacts, `.`
waits, and `^L` redraws. The view does not show the facing direction, so a move also turns toward the way it goes. The
host charges a wait. `read_key` returns `Key::Enter` and `Key::Delete` as it returns the arrows.

The host's arguments are `key=value` tokens: `manifest=file`, which is required, `script=keys`, and
`incremental=true|false`. `main` reads the config and the manifest and constructs `Application` with them; a missing
manifest, an argument that is not a token, or a token the host does not take prints the usage line and exits with 1.
`script=` plays a key sequence instead of the keyboard, beginning at the menu, so a script that plays starts with `n`;
an ESC character in it is the Escape key, a newline the Enter
key, and a DEL character the Delete key, and the run quits once the script is exhausted. `incremental=true` uses the
cursor-addressed output, and `false` writes every frame in full, one after another; without the token, a run is
incremental unless it has a script. Visual Studio runs the host with the block notation manifest.

The terminal's views are `cell` and `lattice`, the two reductions drawn: a glyph per cell, or the lattice, which also
draws walls and doors on edges. Each uses its own glyph set. A manifest that lists any other view is an error, which
`Application`'s constructor throws. The host takes no zone file.

## Godot host

Godot provides `main()` and the frame loop, and loads the simulation as a shared library. `stratum_godot` builds that
library, a GDExtension, and `application/stratum_godot/project/` is the Godot project that loads it, through
`stratum.gdextension`, which refers to `res://bin/`, where the build writes it.

Outside the editor, `initialize_stratum_module` loads the game manifest that a `manifest=file` argument after `--` on
Godot's command line refers to, relative to the assets directory, and `game_manifest()` returns it to each
`StratumWorld`'s `Game`. The host's one view is `tile`, and a manifest that lists another is an error; with one view it
shows no view menu. A missing argument or a manifest that cannot be loaded is written to the log and ends Godot with
1, through `quick_exit`, since a module's initialization cannot fail and an exception that escapes into Godot ends the
process without its message. The editor loads no manifest. The project's `editor/run/main_run_args` and Visual
Studio's debugger arguments, which `quetzal_configure_gdextension` takes after the project directory, pass the block
notation manifest.

`StratumWorld` is the one registered class, a `RefCounted` that owns a `Game`, with only Godot types in its
interface: `create_game`, `has_game`, `game_over`, `menu_open`, `menu_close`, `quit_game`, `save_game`, `saved_games`, `open_game`, `remove_game`,
`view`, `cycle_view`, `notation`, `cycle_notation`, `extent`, `cell`, `render`, `cells`,
`actors`, `step`, `interact`,
`enter`, `turn`, and `message`. `step` sets the character's intent, turning toward the direction moved as the view does not show the
facing direction, and advances through the turn-based driver; `interact` submits `Interact` and `enter` submits
`Enter`, and each advances. `message` returns what the last of them did that the map does not show, such as a door
opening, a staircase before the character, or the level arrived on, and the tile view shows it in its status line. A step ends with the next turn already granted,
so `turn` is the turn awaiting input. `create_game` creates a new game, `has_game` returns true
while one is in progress, `game_over` returns true once the character is dead, and `message` then reports the death,
`menu_open` and `menu_close` pause and resume play, `quit_game` ends the game in progress, `save_game` saves it
explicitly and returns the save's name, `saved_games` returns the saves, newest first, each a `Dictionary` of its
`save_name`, its `character_name`, its local `time` to the minute, and its `save_type`, shown in the tile view's saves
page as the terminal host shows it, `open_game(save_name)` opens one and starts
play, and `remove_game(save_name)` deletes one. `view` returns the view, the manifest's default view at first, and `cycle_view`
changes it to the next of the manifest's views. `notation` returns the
notation a new game is created in, the manifest's at first and the opened game's after `open_game`, and
`cycle_notation` changes it to the other one. Exceptions do not cross into Godot: a failed creation, opening or
deletion pushes the reason to Godot's error list and returns false, and a failed creation or opening leaves no game in
progress; a failed save pushes the reason and returns an empty name, and a failed
listing an empty array; and a game whose `advance` throws during `step`, `interact` or `enter` pushes the reason and
is quit, as it cannot continue.

`cells` is one code per cell from `stratum_view::cells`, row major from row zero; its `PlaceType` constants repeat
`stratum_view::PlaceType` for Godot, followed by `PlaceTypeDoorOpen` for a door whose access is open. `actors` is one
`Occupant` per cell in the same order: `OccupantNone`, `OccupantCharacter`, or `OccupantOther` for any other actor.

`game.tscn` is the tile view: a `TileMapLayer` fed from `cells`, a second layer for actors, a camera, and arrow or
`hjkl` keys, with space calling `interact` and `<` or `>` calling `enter`. A held movement key repeats, and any other
key acts once per press. Its tileset is built at run time from flat colors: stairs in two blues, an open door lighter than a closed one, the
character in yellow, and any other actor in red. It repaints from the new extent when the character enters a zone of another
size, and sizes the window to the zone below a 32 pixel status line, centered on the screen, with each tile drawn at
twice its 16 pixels, or smaller for a zone too large for the screen. Row zero is the lowest in the simulation and y grows
downward in Godot, so the view inverts j when it paints.

The tile view starts at its menu, a dark screen over the view with the same choices and keys as the terminal host's:
`N` for a new game, `C` to continue the game in progress, `S` to save it, which shows "Game saved" on the
menu, `O` for the saves page, which lists, selects, opens and deletes saves, with Y to
confirm a deletion, `T` to change the notation, `V` to change the view, and `Q` or Escape to quit the game and
Godot. The menu shows the view and the notation. During play,
Escape opens the menu, and no other key quits. When `game_over` returns true, the status line asks for a key, which
quits the game and shows the menu. A game quit because it failed shows the menu with a message referring to Godot's
error list. `--new-game` after `--` on Godot's command line starts a new game without showing the menu.

`quetzal_godot` provides what any Godot host needs: `gdextension.cmake` and its `quetzal_configure_gdextension`, which
names the library as Godot expects and writes it into the project rather than a per-configuration subdirectory.

The root forces `CMAKE_MSVC_RUNTIME_LIBRARY` into the cache, because godot-cpp caches the static release CRT even for
a debug build, and quetzal's static libraries then do not link. `library3` turns `GODOTCPP_DISABLE_EXCEPTIONS` off,
since the extension catches the exceptions of the quetzal libraries, and MSVC's `std::exception` has another layout
with exceptions disabled. godot-cpp's headers are SYSTEM includes,
so they do not report `/W4` warnings in the extension's sources. `KML_DEV_GODOT` governs all of it; when it is
configured off, godot-cpp is not processed and neither the library nor the host exists.

Two headless checks need no display: `--path project --script main.gd -- manifest=data/manifests/godot_block.json`
exercises the extension directly, through a save, the opening of that save, which restores the notation changed
before it, and its deletion, and
`--path project --quit-after 5 -- manifest=data/manifests/godot_block.json --new-game` runs the tile scene in a new
game, which reports the cells it painted and the window's size; without `--new-game` it reports that the menu is shown. A project has to be imported
once (`--headless --path project --import`) before Godot sees the extension; otherwise the class is undeclared, with
no indication why.

Visual Studio launches Godot on the project when the extension is run, so breakpoints in the extension are live. Godot
must not already have the project open, or the library is locked and the build fails.

## Tests

`stratum_core_test` covers access point destinations, relative zone extended names, the names of `Directionality` and
`Notation`, a zone spec's position, `RegionTree`, and `Worldspace::create` and `contains`.

`stratum_sim_test` covers the layout hierarchy, layout structure, rooms, the grid, the proximity index and each zone's,
flats, blocking, the scheduler, the
clock, movement, adjacency and sight, loading, doors, the world, persistent ids, access points, residency, commands,
occupancy, zone names and their patterns, the game's region specs, region spec trees and their composition, and the game's setting definitions inserted into a store. It and `stratum_view_test` each check that every zone file in their `data/` has one `door` entity per door
glyph, with a `door_facing` child on each wall that `stratum_format::door_walls` finds for that glyph and no door
facing anywhere else.

`stratum_view_test` covers the console buffer, frame differencing, glyph sets and their files, the cell and lattice
reductions, both renderings and both round trips, and the reference frames. The reference frames are fixed command
sequences rendered to text and compared byte for byte by `check_reference` against `test/stratum_view_test/reference/`.
The sequences are played against the two libraries, not through a host. Three scenarios are block notation zones drawn
through the cell view. Two use the lattice view: `walls_door`, a wall notation zone, moves through a door, back, and into
the wall two rooms share, which blocks, then interacts with it; `door_cell`, a block notation zone, moves into a
doorway, where the door is drawn in its cell, and out the far side.

`dungen_test` and `catagen_test` compare reference zone files written now against those checked in, which must be
equivalent as parsed JSON, and check determinism, parameter validation, grid dimensions, and that the reader accepts every
generated zone. For their scenarios and over a sweep of seeds, both check that each door glyph has one `door` entity
whose door facings are on that doorway's walls in the order `door_walls` returns them, and that no door facing is
anywhere else. `dungen_test` also reads back the rooms of zones with every door, half of them, and none, over a sweep
of seeds, and checks that each room contains exactly the cells generated as it, which fails if a barrier is missing at
an open doorway.

Over another sweep with ranges of staircases each way, both check the written zone. Neither count may exceed its
maximum, and a count leading up may fall short of its minimum only if no staircase leads down. Every floor cell must be
reachable from every other, except in maze1, which can leave corridor that no door reaches, and every staircase must
have the destination `../[-1]` or `../[+1]` as its glyph indicates, or the region spec's name for a staircase down with a
region spec. `access_points` must refer to every staircase. In `dungen_test`, every staircase must be on the floor of its cell,
with an orientation in a cardinal direction, and neither on a doorway nor beside one. In `catagen_test`, with the stair blocks in place, no
staircase may block a doorway if a cell beside it would not, and every staircase must be sited on exactly one of the
walls around its block and open onto a side with floor beyond, or a door if there is none. Both take what the written
zone does not record about a stair cell --- its room in dungen, and which of its sides were open in catagen --- from
the zone generated again. `catagen_test` also checks that every edge between a stair cell and floor is written as
a wall. Validation rejects a staircase up count whose minimum is zero, including when only staircases down are asked
for, for every algorithm but surface1. Each also reads a reference zone's generator parameters back and regenerates from them, since a zone file
has every parameter of its run. A parameter that the writer emits and the test does not read fails, as does one that
the test reads and the file lacks.

`tool/dungen` and `tool/catagen` build the drivers `dungen` and `catagen`; the library targets have the `quetzal_`
prefix and the test executables the `_test` suffix.

`stratum_format_test` reads zone files and checks what they decode to, and rejects every error the reader raises from a
valid file with one change made to it. It checks that references are found and translated wherever they are written,
and writes each file through both `write` overloads and reads it back, comparing the two documents as maps. It writes
each game file document and compares it with its written form, text for text, reads each back and writes it again
identically, and rejects each of a set of game files with one change made to it, with the error the change raises. It
reads the setting definition and region spec files, the game's included, and reads the world definition file, the
game's included, writes it identically, and rejects it with one change made to it.

`flecs_test` checks that the ECS supports zone partitioning, relationships, migration, unload, serialization and
deterministic iteration.

`settings_test` covers the settings store: types, values, scopes, change listeners, and setting groups.

Each test runs from its own directory, since its data and reference paths are relative.

Every test executable is written in doctest and links `quetzal_testing`, which compiles doctest's implementation and
defines `main`. `main` reads `--update-references` and passes every other argument to doctest. A test case has a named
check for each condition: `CHECK_CASE` reports its name, and the detail it is given, when it fails, and
`CHECK_CASE_EQUAL` its name and both values. A case repeated for each scenario is a subcase of one test case.
`check_reference` compares a file a test case wrote to `out/` with the reference file at the same path in `reference/`,
through a comparison the caller passes, byte for byte by default; dungen_test's and catagen_test's compare zone files as
parsed JSON. With `--update-references`, it copies the file over the reference instead.

`stratum_sim_test`'s test cases are in a source file for each area, each a test suite named for it: `scheduling.cpp`,
`layout.cpp`, `occupancy.cpp`, `loading.cpp`, `world.cpp`, `relocation.cpp` and `game.cpp`. `test_support.hpp` declares
what they share: `TestWorld`, a world with the game's entity types and a region made from the test registries of
`test_generation`, and the helpers that load and enter zone files. A test case that builds a world or a game starts from
`WorldFixture`, which loads the setting definitions once in the process, returns every setting to its default, and
seeds the gameplay random engine with `test_random_seed`, so it runs the same alone or in the full run.

`doctest_discover_tests` registers each test case with CTest, named for the test executable, a colon and the test case,
and run from the test executable's directory. Stratum's test executables have the label `stratum`, the geometry and mesh
file tests `mesh_operations`, and the other libraries' tests `quetzal`. `stratum_sim_test`'s test cases share
`out/saves` and `out/working`, so a CTest resource lock runs them one at a time.

No test executable covers a host. The terminal host is checked by running it with `--script` and reading the frames
back, and the Godot host by its two headless invocations.

## Build

The build is CMake, with the solution at `build/kml_dev.slnx`. `/std:c++latest /bigobj /W4` is used throughout, with
third-party sources at `/W1` and flecs and godot-cpp included as SYSTEM. `quetzal_stratum_sim` links `quetzal_common`,
`quetzal_settings`, `quetzal_stratum_core`, `quetzal_stratum_format` and `flecs_static`, and privately `lua_static` and `quetzal_log`; `quetzal_stratum_view` links `quetzal_stratum_sim`; `quetzal_settings`
links `quetzal_common`;
`quetzal_stratum_gen` links `quetzal_catagen`, `quetzal_common`, `quetzal_dungen` and `quetzal_stratum_format`; both hosts link `quetzal_log`,
`quetzal_stratum_gen` and `quetzal_stratum_view`. `quetzal_dungen` links `quetzal_log`, and `quetzal_dungen` and
`quetzal_catagen` link `quetzal_stratum_core`. `quetzal_stratum_core` links `quetzal_common` and `quetzal_math`, and
privately `quetzal_log`. `quetzal_stratum_format`
links `quetzal_common`, `quetzal_log`, `quetzal_settings`, `quetzal_stratum_core` and `nlohmann_json`, vendored at
`library3/json` with its headers SYSTEM.
`lua_static` is Lua 5.5.1, from `library3/lua` without its standalone interpreter, `onelua.c`, and `ltests.c`, compiled as C++ so that a
Lua error unwinds C++ frames. Its clients include `lua.h`, `lualib.h`, and `lauxlib.h`.

`KML_DEV_GODOT` governs the Godot targets and defaults on. When it is configured off, godot-cpp is not processed and
neither `quetzal_godot` nor `stratum_godot` exists. `KML_DEV_VULKAN` governs the Vulkan applications and their
libraries, and defaults on only where the Vulkan SDK is found, so the stratum targets configure without it.

`KML_DEV_ASSETS_ROOT` is a per-target compile definition that defines the shared data root, so no source file has an
absolute path and no program depends on where it was started from. `KML_DEV_GODOT_VERSION` is the Godot release the
host targets, and `KML_DEV_GODOT_EXECUTABLE`, found from it, is what Visual Studio launches.

`application/stratum_godot` is added after `library3`, because configuring a GDExtension reads a property of the
godot-cpp target.

---

*Current stage: Phase 3, all but presentation tables. `Glyphs` is a table that a file supplies; the Godot view's color
array is still hardcoded, and making it data requires asset ids, which is Phase 4's work.*
