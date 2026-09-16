# Discrete Layout --- Plan

Status: in progress
Date: 2026-09-11

The structures a layout file loads into, and the reader and writer either side of it.
`discrete_layout.md` is the format and owns the file itself.

These are derived from the format rather than from the current code. What of that code carries over
is decided afterward, against this.

## What the format asks for

The file states five things, and each goes somewhere different:

| The file states | Where it goes |
| --- | --- |
| the grid --- extent, origin, scale, and a glyph per position | the layout's cells and walls |
| barriers | nowhere: they bound the room pass and are gone |
| rooms --- a perimeter or a fill | `room_id` on each cell covered |
| flats --- a height at three levels | the layout's two flats |
| components and attachments | the caller, as entities |

The layout contains static topology addressed by position. Anything with an identity or mutable state
is an entity, built by the caller from what the reader hands back.

## The structures

### The grid

Origin, scale and extent, and the conversions between a position in the zone and a cell of the
grid. One value, stored once.

### Cells

One record per cell, dense. This is what movement asks, and it asks constantly.

```cpp
    // What the layout stores for one square of the grid
    struct Cell
    {
        bool open = false; // Whether a mover may stand here
        DirectionMask walls; // Which of its sides have a wall
        id_type room_id = nullid; // The room covering it, or nullid where none does
    };
```

One bit per step the grid has. On a square grid that is the four-bit `DirectionMask`
`stratum_view/cells.hpp` already defines, and the layout needs the same mask, so it belongs below
the view.

### Walls

A wall is on one side of a cell, addressed by that cell and the wall's normal, which points into the
cell as a face normal points out from a textured surface. It is the format's wall address unchanged.

The normal is a vector rather than one of four names, so the grid determines which faces a cell has
and nothing here is fixed to a square one.

Everything in a wall record can be derived from the address:

| | |
| --- | --- |
| whether a wall is there | a bit in the cell's mask |
| the wall on the other side of its edge | the neighbor at `-normal`, and `-normal` |
| its two ends | the corners of that cell edge, from the grid |
| its id | the cell, and the normal's place among the grid's own |

The two sides of a cell edge are independent, and a wall on one side does not imply one on the other.
A mover interacts with the wall on its own side alone.

Doorway blocks have only outward facing walls. So once a mover opens a door and enters the cell,
there is no wall facing to stop them from leaving.

Wall data is sparse, keyed by address: a wall with components or with something attached has an
entity its zone finds by wall id, and anything attached to a wall is an entity linked to it.

### Rooms

A room is `room_id` on each cell it covers, the id being its position as provided by the format,
and a count of how many there are. All room properties, name, style, etc. are components on an
entity. Rooms are primarily metadata with the single exception of height at the moment which is
evaluated from flat to room to cell, each potentially overriding the previous one. A given cell may
not be part of a room in which case that level of the hierarchy is skipped and simply defaults to
the flat.

No perimeter and no cell list. Containment by cell is `room_id` and containment by position is the
cell's; a room's cells are one sweep; a perimeter is traced from them for a caller that needs a
polygon. The count is there so a flat referring to a room that does not exist can be rejected.

### Flats

Two per zone, floor and ceiling, each storing the three levels the format writes.

```cpp
    // A bounding surface, floor or ceiling, as the three levels a file states it at. A level
    // states only what it overrides, so the ordinary flat is one value and two empty maps
    class Flat
    {
        bool m_ceiling = false;
        value_type m_height = value_type(0);
        std::map<id_type, value_type> m_heights_room;
        std::map<discrete_point_type, value_type> m_heights_cell;
    };
```

The maps are sparse. A level with three sunken rooms has three entries, not one per cell.

The layout resolves a height, not the flat: the room covering a cell, or the absence of one, is in
the layout's cells. The flat is the three tables it descends. If an indexed lookup ever measures
better, a resolved table is a cache built from these.

### What is not in the layout

Attachments and component objects. They are entities or data for entities, and the layout
has no world to build them in. The reader hands them back for a caller that does.

## The queries

What each costs:

| | |
| --- | --- |
| whether a cell is open | one lookup |
| the wall on a side | one bit test |
| the wall on the other side of an edge | one bit test on the neighbor |
| the room covering a cell | one lookup |
| the room containing a position | through the cell |
| a flat's height at a cell | cell map, room map, constant |
| a position from a cell and back | grid arithmetic |

Nothing here scans, and nothing needs an index beside it.

## Reading a file

Five passes. Nothing is built until every check has passed, so a rejected file leaves nothing
behind.

1. Parse the file into a `Document`, validating its geometry.
2. The rows into cells and walls. The one notation-specific pass.
3. The rooms into `room_id`, each room filled or taken from its perimeter, no fill crossing a
   barrier. One approach writes the barriers over the grid for the fill and puts them back after.
4. The flats, each as the three levels it was written at.
5. The content, passed through as written.

### The document

The document reader loads and validates the geometry and passes all content on in its original form.
A document written back is equivalent to the file it was read from and complete, if not identical to
it.

The grid rows are expanded from runs and every coordinate and address is decoded. Component
objects, entities and the generator are kept as the JSON the file wrote; what content means is the
caller's. A document stores each room as the file identified it: cells, walls, room ids and heights
are built by passes 2 to 4, and inherited components are resolved by the consumer. The writer emits one, and
passes 2 to 5 build from one.

Validating two rooms covering one cell finds every room's cells and a box bounding them, compares
cells only for rooms whose boxes intersect, and keeps none of it.

Keys the format does not define are not tracked. A change to the structure of geometry data is a new
version, which the reader rejects.

### What comes back

Two values: the layout, and everything the layout has no place for.

```cpp
    // Everything in the file that is not geometry, as the file gave it. The reader interprets
    // and resolves none of it
    struct Content
    {
        // On a wall, or on one cell of a flat, never on both and never on a flat as a whole
        struct Attachment
        {
            std::vector<id_type> wall_ids; // Empty where it is on a flat cell
            id_type flat_id = nullid; // With the cell below, the flat cell it is on
            discrete_point_type grid_position;
            std::optional<surface_point_type> position; // Along and up, or xy
            values_type entities; // Each with whatever it contains, nested
        };

        values_type layout;
        std::vector<values_type> rooms; // By room id
        ...
        std::vector<Attachment> attachments;
    };
```

One value out rather than a callback per kind. `values_type` is the JSON reader's value type; the
format defines no content and neither does this.

| Reported | From |
| --- | --- |
| The layout's components | `layout` |
| Each room's components, by room id | `rooms` |
| Each wall's components, by wall address | `walls` |
| Each flat's components, at each of the three levels written | `flats` |
| Every attachment, its walls or its flat cell, and its entities | `attachments` |
| A stairway at every stair glyph, its target defaulted where the file has none | `cells.rows` |

### Rejecting one

A file breaking any rule in the format's `Errors` is rejected by `std::runtime_error`, stating what
was wrong and where.

## Writing one

The writer is ours. No library's pretty-printer produces the written form the format specifies, and
that form is what makes a level file diff. The generators and the test suites write through it.

Run compression is the writer's choice per file. The check on it is that a compressed file and a
character one read back the same.

## Reading and writing JSON

Read with nlohmann/json. It parses to a document the reader can validate and report from,
`ordered_json` preserves key order, and it is what most open source C++ projects use. It is vendored
at `library3/json`.

simdjson is already in the build and is a parser alone, on-demand and read-only. That suits a hot
path, not a reader that must report what was wrong and where.

## What this affects

- **`catagen_reference.md` and `dungen_reference.md`.** The level file section of each is
  superseded. Both generators write this format, their parameter headers becoming
  `generator.parameters` with the same tokens.
- **`load_layout`.** The header sniff goes, a file declaring its format in `format`. `Format` becomes
  `cells.notation`.
- **Version 0.3.** No 0.2 file is read. Every checked-in level regenerates, and with them the
  reference frames in `stratum_view_test`, `dungen_test` and `catagen_test`.
- **The byte-exact round trip narrows to the rows.** `stratum_view_test` compares its lattice
  against `cells.rows` rather than against the whole file.
- **Level files are checked for equivalence.** A suite checking one parses it and its reference and
  compares those, objects by key, rather than requiring the two to be identical.
- **dungen's room and corridor glyphs collapse to floor**, rooms being a section of their own.
  `0` goes, being solid twice over, which leaves every digit free to be a run count. catagen gains
  rooms it had no way to express.
- **The wall notation loses `#` and its corner glyphs.** A solid cell and a corner are both blank.
  `extent` becomes required, a row now being free to run blank to its end.
- **A flat is sampled at cell scale.** `sim_zone_model.md` keeps the surface sampling grid apart
  from the movement grid and allows it to be finer; this format writes heights on the cell grid.
- **A zone is named by its file.** `World` has no name for a zone and relocators are unbuilt; this
  settles the first question either will ask.
- **Stairs get a destination**, which is the open stage 6 item in `work_plan.md`.
- **`door_type` is subsumed** by the content report, a door being one attachment among many.
- **A flat gets a file representation**, and `load_layout`'s `height_ceiling` parameter goes with
  it.
- **A room gains components**, which are stored on its entity.
- **`loader_plan.md` is partly overtaken.** Its `NewCell` has a `room` read from a glyph, and the
  format states rooms in a section of their own. What survives is separating the grid's addressing
  from a reader's output, which the passes above need as much as it did.

## Order of work

1. Settle the structures, which is this document, then read the current code against it.
2. The format, with nothing behind it.

   a. JSON in the build, and the reader and writer.

   b. The round trip over them: a file read, written and read again gives a document equivalent to
   the first.
3. The generators write 0.3, and their reference levels regenerate. Nothing loads them yet, so this
   waits on nothing.
4. The structures, then the reader filling them, the levels the suites and hosts load moving to 0.3
   with it.
5. Loading a zone: the content report, and a world building a zone from the layout and it, every
   component object and attachment an entity.

## Open

The open items are in `work_plan.md`, under Discrete layout format.
