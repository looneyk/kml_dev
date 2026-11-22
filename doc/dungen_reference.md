# dungen --- Reference

This document describes what exists, as built.

`dungen` is a block-oriented zone generator: a wall takes a whole cell, so solid cells lie between rooms, and a door is
one of those cells that an actor may pass through. `catagen` is the wall-based generator beside it, where a wall is a
property of the edge between two cells. The two share no code.

- `library/quetzal/dungen` --- the structures and the generators
- `tool/dungen` --- the driver
- `library/quetzal/stratum_sim/load_layout.cpp` --- reads a zone file into a zone

## The zone file

The generator builds a discrete layout document in the block notation, which `stratum_format` writes as a file;
`discrete_layout.md` is the format. `generator.parameters` has the parameters of the run as the tokens their options take, in key order: the
ones every algorithm shares, the chosen algorithm's group, the position if one was given, and the seed, so the file
reproduces the zone exactly. A position given is also written as the layout's `position`, and the zone's name, if it has one,
as the layout's `name` and `display_name` components. The name is not a parameter: it does not affect the grid.

There is one glyph per cell:

| Glyph | Cell type | |
| --- | --- | --- |
| `#` | `Solid`, `Null` | solid; `Null` is a cell that no generator assigned, and no algorithm leaves any |
| `.` | `Room`, `Corridor` | floor |
| `+` | `Door` | a solid cell an actor may pass through |
| `>` | `Staircase` | a staircase leading down |
| `<` | `Staircase` | a staircase leading up; the cell type is the same, and the zone's `TransitEndpoint` has the directionality |

The origin is the lower left cell, and the highest row is written first, so the file reads as a map oriented as the
display is.

Each dungen room is a room of the file, identified by the first of its room cells. Room floor lies against corridor
floor with nothing between, so every edge between two floor cells in different rooms, or between a room cell and a
cell in no room, is a barrier. A staircase cell is floor, part of the room it was placed in, if any.

Each door is a `door` entity with a `door_facing` child on each wall that `stratum_format::door_walls` finds for its
glyph. Each staircase is a `staircase` entity sited on the floor of its cell, with the orientation it was placed with,
so an actor enters it from any side. Its `access_point` has a `destination` of `../[-1]` or `../[+1]`; and an `order` that counts from 0 among all the
staircases, in the order they are written. Its `directionality` is `Down` or `Up`. With a `staircase_down_region_spec`, each staircase leading down has the
region spec's name as its destination, so it leads to the entry zone of a worldspace of its own, which the world creates from
that region spec.
`layout.components.access_points` refers to every staircase. Local ids are assigned in the order the entities are
written --- each door, then its door facings, and the staircases last --- so the same parameters give the same ids.

## The library

These structures are the same whichever algorithm fills them:

- `Cell` --- a type, the id of the room it belongs to, and four `Wall`s.
- `Cells` --- a rectangle of cells, addressed by `point_type`.
- `Zone` --- an id, a name, the cells and their `extent`, the `Room` and `Door` records and the
  `stratum_format::TransitEndpoint`s over them, and a renderer for printing.
  `create_room`, `create_door` and `create_staircase` each set the cell type and add the record. `create_room` takes a
  rectangle or a list of cells, whose bounding box is the room's rectangle. `create_staircase(placement, directionality)`
  adds a `TransitEndpoint` of the type `staircase`, at the placement's cell position, with the egress direction the
  direction it opens onto and the orientation its opposite, and the `Directionality`, up or down.
- `Pattern` --- a `Cells` with an origin and a shift, matched and replaced over a `Zone` by `match` and
  `match_replace`. `maze1` prunes with these.
- `types.hpp` --- `coord_type`, `point_type`, `vector_type`, `directed_point_type`, `random_type` (the random number
  source of a run), and `log_category`, `dungen`, the category of dungen's entries in the process-wide log
  (`log_plan.md`).

Generation:

- `Algorithm` --- `Maze1`, `Grid1`, `Cave1` and `Surface1`, with `to_string` and `to_algorithm` for the command line and
  the zone file's parameters.
- `Parameters` --- `algorithm`, `extent`, `staircase_down_count`, `staircase_up_count`, an optional
  `staircase_down_region_spec`, an optional `position`, and `seed` in common, then one
  group per algorithm: `Maze1Parameters`, `Grid1Parameters` and `Cave1Parameters`. `validate` checks the shared fields
  and the chosen algorithm's group. The staircase counts are closed
  ranges of `size_t`, 1-1 by default. `validate` rejects one whose maximum is below its minimum, an empty
  `staircase_down_region_spec`, and, for every algorithm but `surface1`, a staircase up count whose minimum is zero,
  since a dungeon zone is entered and left by a staircase leading up.
- `generate` --- dispatches on `parameters.algorithm` to `generate_maze1`, `generate_grid1`, `generate_cave1` or
  `generate_surface1`, one file each. Each asserts the algorithm it was given, validates, seeds its own `random_type` from `parameters.seed`, and
  is deterministic in its parameters.
- `place_staircases` --- shared, and given the candidate cells by each algorithm. It draws a count leading down from
  `staircase_down_count` and a count leading up from `staircase_up_count`, with a range of one value drawing nothing,
  and shuffles the candidates. It then places staircases alternately leading up and leading down, while both counts
  have some left. A candidate is passed over if it is not floor, or is a doorway or beside one in any of the four
  directions: a doorway is a `Door` cell, or a corridor cell beside room floor, as a removed door leaves. A placed staircase is on the floor of its cell, which keeps its room,
  oriented toward a random passable side, a door or another staircase counting as passable. If a count above zero has none placed,
  `place_staircases` throws `std::runtime_error`; if the candidates run out with fewer placed than drawn, it writes a
  warning to the process-wide log.
- `remove_doors` --- shared, and run by `maze1` and `grid1` after every other pass. Each algorithm places every doorway
  as a door, and this keeps each door with probability `door_probability`, in the algorithm's group, and leaves the rest
  of the doorways open as corridor.
- `make_document` --- builds the zone's `stratum_format::ZoneDocument` in memory, which `load_layout` reads directly.
- `write_zone` --- writes that document as the zone file.

## maze1

Rooms are dropped at random, a maze is flooded through every cell they left, and it is then pruned back. Both
dimensions are rounded down to odd, and the rooms and the maze run along the odd cells.

1. **Rooms.** A count drawn from the grid area and `room_density` sets how many placements are attempted. Each draws a
   size from `room_dimension_base` plus a value below `room_dimension_variation`, forced odd, and a position on the odd
   lattice. A placement overlapping two or more rooms is dropped; one overlapping a single room is dropped as well,
   unless `room_merge` grows that room to include it.
2. **Doors.** Every other cell along a room's walls is a candidate. The count comes from the room's size, and a door
   between two rooms is placed only once.
3. **Corridors.** A recursive backtracker floods every solid odd cell. `corridor_straightness` is the chance that it
   keeps its heading where it can.
4. **Staircases,** at the corridor dead ends, before they are pruned, or at the center of each room if rooms and loops
   have left no dead end.
5. **Pruning.** `probability_dead_end_removal` walks each dead end back a cell at a time; `probability_loop_removal`
   closes the loops that the flood left.
6. **Open doorways.** `remove_doors` at `door_probability`.

## grid1

The zone is divided into `divisions` sectors, with a room inside most of them, and adjacent sectors are joined.

1. **Sectors.** Sector bounds divide the zone, spreading the remainder of an uneven division across the sectors.
2. **Rooms.** `room_density` is the fraction of sectors with a room, not a per-sector probability, so a zone is never
   left empty. A room has a random size from `room_dimension_min` up to what its sector allows, and a random position
   inside it, with `room_margin` cells clear on every side. A sector without a room has a single cell instead, through
   which its neighbors join.
3. **Connections.** A randomized Kruskal over the orthogonal adjacencies of the sector grid: an adjacency joining two
   components is always kept, and one closing a loop is kept with probability `loop`. The result spans every sector, so
   every room is reachable.
4. **Corridors.** Each connection leaves one room through the wall toward the other, turns once, and enters the other,
   with a `Door` at each end that is on a room. The turn is on a line through the solid cells strictly between the two
   rooms' facing edges, and both legs stay within the two sectors' shared band, so a corridor never crosses a room. A
   `room_margin` of at least one cell leaves those cells, and `validate` requires it.
5. **Staircases,** in the rooms, or in the corridors if the density left no rooms.
6. **Open doorways.** `remove_doors` at `door_probability`.

An adjacency stores its orientation, which the difference between the two sector slots leaves ambiguous for a single
column of sectors.

## cave1

Noise is smoothed into caverns. The open floor is one room, there are no doors, and the border is left solid.

1. **Noise.** Every cell off the border is drawn solid with probability `fill`.
2. **Smoothing.** `smoothing_passes` passes of a cellular automaton over the eight neighbors of a cell: a solid cell
   stays solid with at least `survival_threshold` solid neighbors, and an open cell turns solid with at least
   `birth_threshold`. Every cell of a pass is computed from the same input grid.
3. **One area.** The largest connected area of open cells is kept as the room, and the rest is left solid, so every
   open cell is reachable from every other.
4. **Staircases,** anywhere in that area.

Parameters at their extremes --- `fill` at 1.0, `birth_threshold` at 0, or a zone small enough that every interior cell
has eight solid neighbors --- generate a zone that is solid throughout.

## surface1

Every cell off the border is the floor of one room, with no doors, and the staircases are placed anywhere on it. It is the surface zone a game starts in, so a staircase up count whose minimum is zero is allowed, and its
staircases leading down are given a `staircase_down_region_spec` to lead each to a dungeon worldspace of its own.

## The tool

`dungen` parses the command line, generates, and writes the zone to a file and to standard output, with its layout's
`unpopulated` set. It installs a `StreamLogSink` writing the process-wide log to `std::clog`, at the level
`log_level` sets, `info` by default.

```
dungen [zone] [algorithm=maze1|grid1|cave1|surface1] [extent=nx,ny] [name=text]
            [stairs_down=min-max] [stairs_up=min-max] [stairs_down_region_spec=name] [room_density=d] [door_probability=p]
            [position=x,y,z] [seed=n]
            [log_level=trace|debug|info|warning|error|critical|off]
  maze1: [room_dimension_base=n] [room_dimension_variation=n] [room_merge=true|false]
         [corridor_straightness=s] [probability_dead_end_removal=p]
         [probability_loop_removal=p]
  grid1: [divisions=nx,ny] [room_dimension_min=n] [room_margin=n] [loop=l]
  cave1: [fill=f] [smoothing_passes=n] [birth_threshold=n] [survival_threshold=n]
```

An argument with an `=` is a token, split at the first `=` into a key and a value; any other argument is the zone path,
`dungen.zone` by default. The tool takes `name` and `log_level` for itself and passes the other tokens to
`set_parameters`. Its keys are those of the zone file's generator parameters, so a file's parameters, given as tokens,
reproduce its zone. A key that is not a parameter, or a value that is not of the parameter's type, throws
`std::invalid_argument`. `set_parameters` calls `set_parameter` for each token, which returns false for a key that is
not a parameter, and `stratum_gen` calls `set_parameter` directly so that it skips such keys. A run given no `seed` draws one, so the file has a seed either way.

Each token sets the group it belongs to, whichever algorithm was chosen, so a token may be given before the `algorithm`
it applies to. `room_density` and `door_probability` are shared by two groups and set both. `stairs_down` and
`stairs_up` take a closed range of counts, with `-` between the bounds; the keys keep the shorter "stairs", where names
in code and prose say "staircase".

## Reading it back

`stratum_sim::load_layout` reads the file through `stratum_format`, which `game_system_reference.md` describes. The
loader does not read the parameters, so the algorithms share one file form.

A doorway is a cell an actor may stand in, walled toward solid on its own side and toward floor on the floor's side
alone. A staircase cell is floor to the loader, with the staircase's access point on its floor.

## Known limits

- `pack_rooms`, which `maze1` calls if `room_density` reaches 1.0, is empty, so that density leaves a zone with no
  rooms, only the maze.
- `Zone::expand_room` repaints cells into a room but leaves the `Room`'s position and size as created, so the record
  understates a merged room, and `maze1`'s `door_candidates`, walking the record's four sides, offers a merged room
  doors only along its original rectangle. `room_merge` is off by default.
