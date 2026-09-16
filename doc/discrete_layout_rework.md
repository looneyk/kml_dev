
# Discrete Layout Rework

Status: the structures and the loader are built; loading a zone is outlined
Date: 2026-09-12

Working notes for the rework. `discrete_layout.md` is the format, `discrete_layout_plan.md` the
structures it asks for; this is the reading of the current code against them and the detail of the
work.

## The current code

### Untouched

Nothing here mentions a wall record or a room, so the structures can change under all of it.

| | |
| --- | --- |
| `Layout` | The interface mentions no cell and no wall. The new structures satisfy it as the old ones do |
| `World`, `Session`, `Clock`, `Scheduler`, `commands`, `ai` | Never see a wall or a room |
| `attachments.cpp` | A door is entities keyed by a wall id, and the id is a token it never looks inside |
| `stratum_view` | `cells` and `lattice` are written against `open`, `wall_toward`, `grid` and `grid_position_at`, which are the queries the plan keeps |
| `GridMapping` | Origin, scale, extent and the two conversions. The plan's grid, already built |

**A wall id is already opaque above the layout.** `OnWall`, `facing_on`, `crossable`, `ahead`,
`Blocked::obstruction_wall_id` and the view's `wall_toward` then `facing_on` all pass one along
without indexing anything with it. Making the id an encoded address changes none of them.

### Reshaped

| | |
| --- | --- |
| `Cell` | Keeps `open` and `room_id`, gains the wall mask |
| `DirectionMask` | Already the four-bit mask, in `stratum_view/cells.hpp`. Moves down to the sim |
| `Flat` | Keeps its role and its constant. Its own `GridMapping` and the dense `std::vector<std::optional<value_type>>` become the two sparse maps, and resolving moves to the layout |
| `wall_toward` | The query stays and the body becomes a bit test |
| `read_block`, `read_walls` | The glyph reading is what pass 2 needs. What they fill changes from `detail::Map` to cells directly |

### Goes

- `Wall` entirely: `partner_id`, `next_id`, `prev_id`, `position`, `deleted`.
- `DiscreteLayout`: `wall_store` and its accessors, `create_walls`, `opposed`, `connect`,
  `disconnect`, `connect_walls`, `connect_folds`. About a third of the file, `wall_toward`'s body
  included.
- `Room` entirely, and with it `room_store`, `create_room` and `room_at(point)`.
- `load_layout`: `trace_outlines`, `to_polygon`, the `WallIndex`, the ring chaining in `build`, the
  block ring construction and `create_rooms`' outline tracing. About half the file.
- `find_components`, and the `blocks` and `passages` split in `detail::Map` with it.

**Components existed to give each area a boundary to trace.** With a wall stored as a bit on the
side of the cell that faces it, every wall comes from one glyph and its two neighbors, so nothing has to be
grouped first. The block notation's doorway is the case that looks like it needs more and does not:
the edge between a block and solid sets the bit on the block's side, the edge between a block and
floor sets it on the floor cell's side alone, and floor to solid sets it on the floor's. All three
are local rules on one edge.

### Two findings from the survey

**Nothing reads the wall records.** `wall()`, `wall_store()`, `create_walls`, `connect` and
`opposed` are called by the loader that fills them and by nothing else. The partner links and the
`next`/`prev` rings are built at load and never asked a question.

**The cell reduction is O(cells x walls).** `wall_toward` reconstructs four float points from the
grid and scans every wall comparing with `coincident`; `cells.cpp` calls it once per direction per
position. That is the view's inner loop, and what it computes there four scans at a time is the
mask the cell would store.

### Whether this is a rewrite

It is not one. The interface, the view, the entity side and the session are untouched, and what
changes is concentrated in `DiscreteLayout` and `load_layout` --- most of it deletion. The format
reading in the two per-format files survives the change of what it fills.

## Rooms

```cpp
load the raw layout grid data

// Rooms
for each room
    if both perimeter and location or neither
        error
    create a room entity
    room_id = create a new room
    // set the rooms cells to reflect room membership, flood fill to determine the cell set
    // probably separate versions of this for block and wall formats
    if perimeter
        inclusion_condition = perimeter
        error_condition = a block/wall is encountered within the perimeter, unless it is unconnected interior and completely enclosed
   else
        inclusion_condition = {location, barriers}
        error_condition = ? // interior ignored, it simply constrains the flood fill
    flood_fill(cells, room_id, inclusion_condition, error_condition)
    either link the room entity into the layout on success, or delete it on failure

separate function to build a perimeter given a location after room_ids have been assigned
```

## The structures and the loader

### One build path, one set of glyph rules

    file --stratum_format::read--> Document --load_layout--> DiscreteLayout

`stratum_format` already decides, per cell, whether it is floor/stair/door, whether it has a wall on
each side, and which room covers it, because validating a file needs all three. Passes 2 and 3 need
exactly those answers, so they become public functions over a `Document` and the reader's checks and
the build both use them. The glyph reading then exists once: `load_layout_dungen`,
`load_layout_catagen`, their `read_block` and `read_walls`, and `detail::Map` go rather than being
reshaped.

### stratum_format

```cpp
bool floor_stair_cell(const Cells& cells, const discrete_point_type& grid_position);
bool floor_stair_door_cell(const Cells& cells, const discrete_point_type& grid_position);
bool has_wall(const Cells& cells, const discrete_point_type& grid_position, const discrete_vector_type& direction);

// The cells each room covers, by room id, from a document read has validated
std::vector<std::set<discrete_point_type>> room_cells(const Document& document);
```

The reader's overlap check becomes a check over `room_cells`, its bounding boxes staying internal.

### stratum_sim structures

- `DirectionMask` moves from `stratum_view/cells.hpp` into the sim.
- `Cell` is `open`, `walls` and `room_id`.
- `Flat` is its role, a constant height and the room and cell height maps; the sampling grid goes.
- `DiscreteLayout` is the grid, the cells, two flats and a room count. A wall id encodes the cell and
  the direction's place among the four, so `wall_toward`, `blocked` and `ahead` are bit tests and
  the id stays opaque above the layout. A flat's height at a cell resolves cell, then room, then the
  constant.
- Deleted: `Wall`, `Room`, the wall and room stores, `create_walls`, `connect`, `opposed`,
  `connect_walls`, `connect_folds`, and `room_at(point)`.
- `Format` becomes `stratum_format::Notation`.

### load_layout

```cpp
load_layout(filepath, door) -> unique_ptr<DiscreteLayout>
    document = stratum_format::read(filepath)     // throws, and nothing is built before it returns
    layout.set_grid(origin, scale, extent)

    for each cell p
        cell.open = floor_stair_door_cell(cells, p)
        for each direction d
            if cell.open and has_wall(cells, p, d)
                cell.walls |= d

    for each room_id, grid_positions in room_cells(document)
        for each p in grid_positions
            cell(p).room_id = room_id
    layout.set_room_count(document.rooms.size())

    flats from document.flats, the floor at 0 and the ceiling at 1 where absent

    for each door glyph                           // a '+' edge, or a '+' block's walls toward floor
        door(wall ids)
```

The `height_ceiling` parameter goes. Doors are still reported from the glyphs; attachments,
room entities and stair targets are step 5's content report.

### Levels

The loader reads 0.3 alone, so every level a suite or a host loads is 0.3 when this lands: nine in
`stratum_sim_test/data`, five in `stratum_view_test/data`, two in `assets/level`. The generators write
0.3 before this step, so the six generated copies are rerun from their own parameters and checked by
the reader as they are written; the ten hand-written ones are a few lines each and convert by hand,
checked by the reader and by the suites that load them. `walls_no_extent.level` goes, deriving an
extent being a 0.2 feature.

### What checks it

`stratum_view_test`'s reference frames come out unchanged: the same geometry through the new
structures renders the same. `stratum_sim_test` loses the suites for what is deleted --- partner
linking, wall folds, outline containment --- and its loading, movement, door and sight suites run
against the 0.3 levels.

### Order

Each ends with every suite passing.

1. The grid functions public in `stratum_format`, the reader rewritten over them.
2. The structures, `load_layout` and the sixteen levels together: the old loader builds walls the new
   structures do not have, and the levels cannot change format before the loader does. The suites
   follow, and the frames confirm it.

## Loading a zone

Step 5. A `Document` stays the file with its geometry validated. Loading turns it into what a zone
is built from: the layout, and `Content`, everything else placed by the layout's ids. A generator
produces the same two with no file. Building makes the zone in the world, its layout and an entity for
everything in `Content`. Loading has no world; building has no document.

    file --stratum_format::read--> Document --load_layout, load_content--> DiscreteLayout, Content
                                  Generator --generate----------------------> DiscreteLayout, Content
    World, with its types --create_zone(name, DiscreteLayout, Content)------> Zone

### What happens now, and where

- **The document keeps all content, and nothing reads it.** `stratum_format::read`
  (`stratum_format/read.cpp`) stores `Document::components`, `Room::components`, `Wall::components`,
  `FlatProperties::components` for a flat, each room and each cell, and every `attachments` entry as a
  `stratum_format::Attachment` (`stratum_format/Document.hpp`): `attached_to`, `position` and
  `entities`. No code in `stratum_sim` refers to any of them.
- **Doors are reported from the rows alone.** `stratum_sim::load_layout`
  (`stratum_sim/load_layout.cpp`) takes each address list `stratum_format::door_walls` returns,
  converts each address with `DiscreteLayout::wall_toward(address.grid_position, -address.normal)`,
  and passes the wall ids to its `door_type` callback (`stratum_sim/load_layout.hpp`). The `door`
  entity every level has in `attachments` at those walls is ignored.
- **Stairs are reported not at all.** A stair glyph loads as open floor.
- **Doors are built by the callers.** Each collects the wall ids into a `stratum_sim::door_walls_type`
  and, after `World::create_zone`, calls `stratum_sim::create_doors` (`stratum_sim/attachments.cpp`).
  That calls `create_door` per list: a `Door` entity with `Openable {false}` as a child of the zone,
  and a `DoorFacing` child with `OnWall` for each wall id. The callers are `Session::load` and
  `Session::populate` (`stratum_sim/Session.cpp`), `test_load_walls` and `test_doors` in
  `stratum_sim_test/main.cpp`, and `load_zone` in `stratum_view_test/main.cpp`.
- **A generator reports doors the same way.** `Session::create` calls
  `stratum_sim::Generator::generate(door)` (`stratum_sim/Generator.hpp`), which returns a layout and
  reports doors through the same callback. Its one implementation is `TestGenerator` in
  `stratum_sim_test/main.cpp`, which builds a `DiscreteLayout` directly.
- **`World` contains layouts beside zones.** `World::m_layouts` (`stratum_sim/World.hpp`) maps a zone
  entity's id to its layout, and `stratum_sim::Zone` is an empty tag on that entity, whose children
  are the zone's contents.

### Edges, walls and ids

Edges, walls, wall addresses, wall ids, flat cells and entity ids are specified in `addressing.md`. An
address becomes a wall id through `DiscreteLayout::wall_toward(address.grid_position, -address.normal)`.

### The document

Unchanged. `stratum_format::read` validates the geometry and keeps all content as the file wrote it,
and a document written back is equivalent to the file. Nothing loading adds is stored in it.

`stratum_format/grid.hpp` gains the stair glyphs beside `door_walls`:

```cpp
enum class StairDirection
{
    Down, // >
    Up    // <
};

std::vector<std::pair<discrete_point_type, StairDirection>> stair_cells(const Cells& cells);
```

### Loading

```cpp
namespace quetzal::stratum_sim
{
    // A cell of one flat
    struct FlatCell
    {
        id_type flat_id = nullid;
        discrete_point_type grid_position;
    };

    // Everything in a zone that is not geometry, placed by the layout's ids
    struct Content
    {
        struct FlatContent
        {
            values_type components;
            std::map<id_type, values_type> rooms; // By room id
            std::map<discrete_point_type, values_type> cells;
        };

        struct Attachment
        {
            std::variant<std::vector<id_type>, FlatCell> placement; // Wall ids, one per side presented, or a flat cell
            std::optional<surface_point_type> position;
            values_type entities;
        };

        values_type layout;
        std::vector<values_type> rooms; // By room id
        std::map<id_type, values_type> walls; // By wall id
        std::map<id_type, FlatContent> flats; // By flat id
        std::vector<Attachment> attachments;
    };

    std::unique_ptr<DiscreteLayout> load_layout(const stratum_format::Document& document);
    Content load_content(const stratum_format::Document& document, const DiscreteLayout& layout);
}
```

- **`load_layout`** is today's without the file read, the `door` callback or the `notation`
  out-parameter. The caller reads the document, whose `cells.notation` is the notation.
- **`load_content`** is new, in `stratum_sim/load_content.cpp`. It copies every component object out
  of the document by id: `Document::components`, `Room::components` by room id, `Wall::components` by
  wall id, and each flat's components for the flat, each room and each cell, by flat id. Every
  `attachments` entry is
  copied with its addresses turned into wall ids, or its `stratum_format::FlatCell` into
  `DiscreteLayout::floor_id()` or `ceiling_id()` and the cell.
- **Glyphs become attachments.** Each door glyph becomes a door attachment, a `door` entity on the
  wall ids of the addresses `door_walls` returns for it. Each stair glyph becomes a staircase
  attachment, a `staircase` entity on the floor at its cell whose `relocator` has a `target` of
  `level_down` for `>` and `level_up` for `<`, as `discrete_layout.md` (`### Staircases and entrances`)
  requires. An `attachments` entry at the same placement is the same attachment: its entities are
  added, an entity of the glyph's type being the glyph's entity with the components the entry writes,
  and a relocator target it gives replacing the default. A staircase's `facing` comes from its entry.
  `<` is the entrance glyph for any non-local transporter, so an entry there of another type --- a
  waypoint, a portal --- is the entity at that cell, and no staircase is added beside it.
- **The entry.** `load_content` reads the layout's `entry` cells into `Content`. The reader has already
  rejected a file without them, or listing a cell that is not written `<` or `>` with an entity attached
  there that has a relocator.
- **Nothing records a glyph.** Glyphs and attachments are related only while loading and saving. A
  glyph set, a custom one included, is mapped to attachments on load, and saving renders the
  attachments as the format's glyphs.
- **`Generator::generate`** returns the layout and fills a `Content` rather than reporting doors
  through a callback. `TestGenerator` fills one door attachment on the two walls it reports now.

### Types

A type is an entity. The set of types is an asset, authored by artists and designers, and `World`
loads it independently of any zone.

```cpp
class World
{
    flecs::entity create_type(const std::string& type, const values_type& components);
    flecs::entity type(const std::string& type) const; // Null where none is loaded
};
```

**Inheritance resolves when queried, and nothing is merged.** An entity built with a `type` refers to
that type and has only the components written on it. When an object in a zone is queried for a
property:

1. its own value, where it has one;
2. failing that, the room covering it, where its type looks to its parent;
3. failing that, the layout;
4. failing that, the default its type gives, or nothing where that is appropriate.

Each step is a lookup rather than a search: a component on the entity, the room's entity by the cell's
`room_id`, a component on the zone's entity, a component on the type's. This is the chain
`discrete_layout.md` (`### Inheritance`) specifies, property by property, so a component whose value is
an object answers for the properties it states and passes the rest along. Nothing is copied down the
chain when the zone is built. A flat's height already resolves this way, `DiscreteLayout` descending
cell, room and flat on each query. Where no type is loaded for an entity, the chain ends at the layout.

`type` is the key each element of an `entities` list has, beside `components` and `contains`
(`discrete_layout.md`, `## attachments`).

### Zones

`World` contains zones rather than layouts. `World::m_layouts` goes, and a `Zone` is the zone itself:
its layout, its entity, whose children are everything in it, and the links below. `World::layout(zone)`
returns the zone's layout.

```cpp
flecs::entity World::create_zone(const std::string& name, std::unique_ptr<Layout> layout, const Content& content);
```

`create_zone` builds everything in `Content`, every entity nested under the zone's:

- **The layout's components** on the zone entity, and the zone's entry cells, among which an arriving
  mover is placed by what it arrives through.
- **Room and flat entities.** One per room id and one per flat, which the zone finds by id, with their
  components. A flat's overrides for a room or a cell are entities nested under the flat's, which it
  finds by room id and by cell.
- **Wall entities, sparse.** A wall itself is a bit in the layout. A wall with
  components or with something attached to it has one, which the zone finds by wall id. It has the
  wall's components and links to what is attached to it. The count stays that of what is written
  on walls rather than of walls, which is what `decision_rationale.md` ("A wall is a record, not an
  entity") guards.
- **Flat cell entities, sparse,** the same way: one for each flat cell with something attached, which
  the zone finds by flat id and cell, linking to what is attached.
- **Attachments.** An entity per element of an attachment's `entities`, referring to its type, with
  its own components. On walls it has a facing on each wall, one or more, as a door does: each facing
  is a child of the entity, the entity links to its facings, each facing links to its wall, whose id
  provides the normal and so the facing's absolute direction, and the wall's entity links back to the
  facing. On a flat cell the entity links to that cell's entity and the cell's entity to it.
  `stratum_sim::Attachment::position` is the attachment's `position`, centered where absent.
- **The zone lists its doors,** and every other attachment's entity, apart from its walls. A door
  has what its sides share --- `Openable`, `Lockable` --- and each facing has what is reached from its
  side, nested under it: `Concealed`, a keyhole, a knocker.
- **A contents entity** nested under each attachment's entity, empty where nothing is written in
  `contains`, and each contained entity built nested under it in turn.
- **Every component kept.** The component object written for an entity is kept on it as written, and
  a component the sim has a struct for also sets that struct: `open` sets `Openable::open`, a
  `relocator` sets a `Relocator` component with its target, on whatever entity has it, and a staircase's
  `facing` sets its facing. Traversing a relocator is stage 6's "Relocating through a relocator"
  (`work_plan.md`).

**What the links replace.** `crossable(zone, wall_id)` finds the wall's entity by id and follows its
link to the facing and from there to the door, where `facing_on` searched every `OnWall` in the world.
`other_facing` and its sibling search go, the door linking to both facings. `OnWall` stays as a
facing's link to its wall and is no longer searched.

`create_doors`, `door_walls_type` and `door_type` go. `Session::load` reads the document, calls
`load_layout` and `load_content`, and passes both to `populate`, which passes them to `create_zone`.
`Session::create` does the same with what `Generator::generate` returns. The two suites load the
same way.

### Entities, attachments and ids

Loading a zone as outlined above reads the 0.3 `attachments`, where entities are listed under the place
they are attached. The format's next version replaces that with one list of entities, where what an
entity is attached to and what it is part of are its `attached` and `parent` components, and a zone's
entities get persistent ids when it is made. `addressing.md`
specifies both and `addressing_plan.md` is the design, including the order in which it and loading a
zone are built.

### Staircase placement and facing

The generators write every staircase as a `staircase` entity with a facing and a relocator
(`discrete_layout.md`, `### A staircase's facing` and `### A relocator's target`), as an
`attachments` entry at the stair cell:

```json
{ "flat": "floor", "cell": [3, 2], "entities": [ { "type": "staircase", "components": { "facing": [0, -1], "relocator": { "target": "level_down" } } } ] }
```

- **Off a doorway.** A staircase placed where it blocks a doorway is moved one cell, in a random
  direction among those available. It blocks a doorway where a mover coming through the doorway
  steps straight onto it. An available direction is a side with no wall and no doorway, onto a floor
  cell of the same space that blocks no doorway itself and holds no other staircase. Where no direction
  is available the staircase stays.
- **The facing chosen.** A random choice among the stair cell's sides with no wall on them. A side
  with a door is avoided, and chosen from only where no side is open.
- **The generator's last pass.** Each generator moves staircases off doorways and then draws the
  facings, from its own random source, after every other pass, so no earlier draw moves and a
  reference level's rows change only where a staircase moved.
  - dungen, built: `Staircase` stores its facing, and `move_staircases_off_doorways` and
    `face_staircases` follow `remove_doors`, which decides which doorways keep a door. A doorway is a
    `Door` cell, or the cell a door was removed from, which is floor of a different space from the room
    it opens into. A side is open where the neighbor is `Room`, `Corridor` or `Stair`. Moving a
    staircase restores the cell it leaves to what it was and updates the `Staircase`'s position.
  - catagen, built: `Level` stores a facing for each stair cell, beside its cells and walls. A
    doorway is a wall of `WallType::Door`, and a side is open where the wall on it is `WallType::Open`.
- **Written by `write_level`.** Each generator adds an entry for each staircase to the document: a
  `staircase` entity with its facing and a relocator whose target is `level_down` for `>` and
  `level_up` for `<`. Both are built.
- **Written, not supplied.** A generated level states every staircase's relocator. `load_content`
  supplies the default target only for a stair glyph with no entry giving one, and supplies no facing:
  a staircase has the facing its entry writes. The `Document` stays as
  the file wrote it either way.
- **The entry cells, built.** Every staircase is an entry, and `write_level` writes the cells of all of
  them as `layout.components.entry` (`discrete_layout.md`, `## layout`); the caller chooses among them
  by what it arrives through. Staircases leading up are placed before those leading down. A level with
  no staircase is an error: `validate` rejects staircase counts whose minimums are both zero, and
  `generate` throws where no staircase could be placed, which takes a level with no candidate cell at
  all: maze1 falls back to its rooms where a maze has no dead end.
- **The reader checks it.** `stratum_format::read` rejects a file whose `layout.components.entry` is
  absent, not a list, or lists no cell, or lists a cell that is not a cell, is off the grid, is not
  written `<` or `>`, or has no entity attached to the floor there that has a `relocator` component.
  The entity's `type` is not checked, types being prefabs, and a relocator written on the floor cell
  rather than on an attached entity does not count.

### What checks it

- `stratum_format_test`: `stair_cells` in both notations; built, a file with no entry, an entry that is
  not a list or lists no cell, and an entry cell that is not a cell, off the grid, on a blank cell, on
  floor, with nothing attached, with its staircase on the ceiling, with a staircase that has no
  relocator, or with the relocator on the floor cell rather than on an entity rejected, and an entry
  cell on a waypoint, on a type no one has defined, or on a staircase leading down alone as a tower's
  entry accepted.
- `dungen_test` and `catagen_test`: over the seed sweeps, no staircase blocks a doorway it had an
  available direction off, and every stair glyph has an entry whose `facing` is a side with no wall,
  or a side with a door where the stair cell has no side without a wall. Built, every level's `entry`
  lists exactly its staircase cells, staircase counts whose minimums are both zero are rejected, and a
  staircase down alone is accepted.
- `stratum_sim_test`: `load_content` over data files with component objects for the layout, rooms,
  walls, flats, flat rooms and flat cells, and `attachments` entries at door walls, at stair cells
  with and without a `target`, and elsewhere, the ids checked against the layout. `create_zone`
  builds room, flat, wall and flat cell entities found by their ids, a door with its facings linked
  to their walls and back, `open` setting `Openable`, a relocator with its target and its facing, and a
  contents entity with and without contents. `crossable` answers through the links. A property query
  answers from the object, then the room covering it, then the layout, then its type's default; an
  object stating part of an object-valued component answers the rest from further along; a type that
  does not look to its parent skips the room and the layout; an object whose type is not loaded ends
  at the layout.
  The loading, door and world suites run through the new path.
- `stratum_view_test`: its frames unchanged.

### Order

1. The two generators moving staircases off doorways and choosing and writing a facing for every
   staircase, both done; the generators writing the entry cells; and `stair_cells` in
   `stratum_format`.
2. `Content` and `load_content`, and `load_layout` taking a document.
3. Types in `World`, and the property query.
4. `Zone` containing its layout and its entities by id, and `create_zone` building everything in
   `Content` with its links.
5. `crossable`, `door_of` and the view on the links, `facing_on` and `other_facing` deleted.
6. `Session`, `Generator` and the suites on the new path, and `create_doors`, `door_walls_type` and
   `door_type` deleted.
