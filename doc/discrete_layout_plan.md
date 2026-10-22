# Discrete Layout --- Plan

Date: 2026-09-11, revised 2026-09-26

This plan describes the structures a zone file loads into, and the reader and writer on either side of them.
`discrete_layout.md` is the format and is the authority on the file itself.

## Format contents

The file states five things, and each goes somewhere different:

| The file states | Where it goes |
| --- | --- |
| the grid --- extent, origin, scale, and a glyph per position | the layout's cells and walls |
| barriers | nowhere: they bound the room pass and are discarded |
| rooms --- a perimeter or a fill | `room_id` on each cell in the room |
| flats --- a height at three levels | the layout's two flats |
| components and entities | the caller, as entities |

The layout contains static topology addressed by position. Anything with an identity or mutable state is an entity,
built by the caller.

## The structures

### The grid

The grid is the origin, scale and extent, and the conversions between a position in the zone and a cell of the grid,
stored once.

### Cells

There is one record per cell, stored densely, since movement queries cells constantly.

```cpp
    struct Cell
    {
        bool open = false; // True if an actor may stand here
        DirectionMask walls = DirectionMask::None; // The sides with a wall facing into the cell
        id_type room_id = nullid; // The room that contains the cell, or nullid
    };
```

There is one bit per side of a cell: on a square grid, the four-bit `DirectionMask` that the view uses as well.

### Walls

A wall is on one side of a cell, addressed by that cell and the wall's normal, which points into the cell. This is the
format's wall address unchanged. The normal is a vector rather than one of four directions, so the grid determines
which sides a cell has, and nothing here is fixed to a square grid.

Everything in a wall record is derived from the address:

| | |
| --- | --- |
| whether a wall is there | a bit in the cell's mask |
| the wall on the other side of its edge | the neighbor at `-normal`, and `-normal` |
| its two ends | the corners of that cell edge, from the grid |
| its id | the cell, and the normal's position among the grid's own directions |

The two sides of a cell edge are independent: a wall on one side does not imply one on the other, and an actor
interacts only with the wall on its own side. A doorway cell has walls facing outward only, so an actor that has
opened a door and entered the cell is not stopped leaving it.

Wall data is sparse and keyed by address: a wall with components or with something on it has an entity that its zone
finds by wall id.

### Rooms

A room is `room_id` on each cell in it, the id being its position in the file, together with a count of rooms. A
room's properties --- name, style and the rest --- are components on its entity. A height resolves from flat to room to
cell, each overriding the one before; a cell in no room takes the flat's height.

A room has no perimeter and no cell list. Containment by cell is `room_id`, and containment by position is the cell's.
A room's cells are one sweep, and a perimeter is traced from them for a caller that needs a polygon. The count lets a
flat that refers to a room that does not exist be rejected.

### Flats

There are two per zone, the floor and the ceiling, each storing the three levels the format writes.

```cpp
    class Flat
    {
        bool m_ceiling = false;
        value_type m_height = value_type(0);
        std::map<id_type, value_type> m_heights_room;
        std::map<discrete_point_type, value_type> m_heights_cell;
    };
```

A level states only what it overrides, so the maps are sparse: a zone with three sunken rooms has three entries, not
one per cell. The layout resolves a height, not the flat, since the room that contains a cell is in the layout's cells.

### Data outside the layout

Components and entities are not. The layout has no world to build them in, so they pass through to a caller that
does.

## The queries

| | |
| --- | --- |
| whether a cell is open | one lookup |
| the wall on a side | one bit test |
| the wall on the other side of an edge | one bit test on the neighbor |
| the room that contains a cell | one lookup |
| the room that contains a position | through the cell |
| a flat's height at a cell | cell map, room map, then the constant |
| a position from a cell and back | grid arithmetic |

None of them scans, and none needs an index beside the layout.

## Reading a file

Reading has five passes. Nothing is built until every check has passed, so a rejected file leaves nothing behind.

1. Parse the file into a `Document`, validating its geometry.
2. Turn the rows into cells and walls; this is the one notation-specific pass.
3. Turn the rooms into `room_id`, each room filled or taken from its perimeter, with no fill crossing a barrier.
4. Build the flats, each at the three levels it was written at.
5. Pass the components and entities through as written.

### The document

The document reader loads and validates the geometry and passes all content on as written. A document written back is
equivalent to the file it was read from, if not identical to it.

The grid rows are expanded from runs, and every coordinate and address is decoded. Components, entities and the
generator are kept as the JSON the file wrote, and their meaning is the caller's to determine. A document stores each
room as the file wrote it: cells, walls, room ids and heights are built by passes 2 to 4, and the consumer resolves
inherited components.

The check that no two rooms share a cell finds every room's cells and bounding box, compares cells only for rooms
whose boxes intersect, and keeps none of it.

Keys the format does not define are not tracked. A change to the structure of geometry data is a new version, which
the reader rejects.

### Rejecting a file

A file breaking any rule in the format's `Errors` is rejected with a `std::runtime_error` stating what was wrong and
where.

## Writing a file

`stratum_format` writes the form that the format specifies; the generators and the tests write through it. Run
compression is the writer's choice per file, and a compressed file and a character one read back the same.

The reader uses nlohmann/json, vendored at `library3/json`.

## Format changes

- **Both generators write it**, with their parameter headers becoming `generator.parameters` with the same tokens.
- **`load_layout` reads the notation from `cells.notation`**, without inspecting a header.
- **No file of an earlier version is read.** Every checked-in zone file is in the current version, and so are the
  reference frames in `stratum_view_test`, `dungen_test` and `catagen_test`.
- **The byte-exact round trip is of the rows.** `stratum_view_test` compares its lattice against `cells.rows` rather
  than against the whole file.
- **Zone files are checked for equivalence.** A test parses a file and its reference and compares them, objects by
  key, instead of requiring the two to be identical.
- **dungen's room and corridor glyphs are floor**, since rooms have a section of their own. There is no `0`, which
  leaves every digit free to be a run count. catagen states rooms.
- **The wall notation has no `#` and no corner glyphs.** A solid cell and a corner are both blank, and `extent` is
  required, since a row may be blank to its end.
- **A flat is sampled at cell scale.** `sim_zone_model.md` keeps the surface sampling grid separate from the movement
  grid and allows it to be finer; this format writes heights on the cell grid.
- **A zone is identified by its file.**
- **A staircase has a destination**, its relocator's target.
- **A flat has a file representation**, replacing `load_layout`'s `height_ceiling` parameter.
- **A room has components**, stored on its entity.
- **`door_type` goes when a zone is loaded from its entities** (`game_system_reference.md`, `### load_layout`), since a door is one
  entity among many.

## Open

The open items are tracked in `work_plan.md`, under Discrete layout format.
