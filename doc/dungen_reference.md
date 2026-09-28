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
`discrete_layout.md` is the format. `generator.parameters` has the parameters of the run as the tokens their options take: the ones every
algorithm shares, then the chosen algorithm's group, then the level if one was given, then the seed, so the file
reproduces the zone exactly. A level given is also written as the layout's `level`, and the zone's name, if it has one,
as the layout's `name` component. The name is not a parameter: it does not affect the grid.

There is one glyph per cell:

| Glyph | Cell type | |
| --- | --- | --- |
| `#` | `Solid`, `Null` | solid; `Null` is a cell that no generator assigned, and none of the three leaves any |
| `.` | `Room`, `Corridor` | floor |
| `+` | `Door` | a solid cell an actor may pass through |
| `>` | `Staircase` | a staircase leading down |
| `<` | `Staircase` | a staircase leading up; the cell type is the same, and the zone's `Staircase` record has the rise |

The origin is the lower left cell, and the highest row is written first, so the file reads as a map oriented as the
display is.

Each dungen room is a room of the file, identified by the first of its room cells. Room floor lies against corridor
floor with nothing between, so every edge between two floor cells in different rooms, or between a room cell and a
cell in no room, is a barrier. A staircase cell is a block, which the floor beside it is walled against in the block
notation without anything written, and it is part of no room.

Each door is a `door` entity with a `door_facing` child on each wall that `stratum_format::door_walls` finds for its
glyph. Each staircase is a `staircase` entity sited on the wall that the cell it opens onto has toward its block. Its
`relocator` has the `type` `staircase`; a `target` of `-1` or `+1`; an `order` that counts from 0 among all the
staircases, in the order they are written; and a `placement`, which is the floor its open side faces, facing away from
it.
`layout.components.access_points` refers to every staircase. Local ids are assigned in the order the entities are
written --- each door, then its door facings, and the staircases last --- so the same parameters give the same ids.

## The library

These structures are the same whichever algorithm fills them:

- `Cell` --- a type, the id of the room it belongs to, and four `Wall`s.
- `Cells` --- a rectangle of cells, addressed by `point_type`.
- `Zone` --- an id, a name, the cells and their `extent`, the `Room`, `Door` and `Staircase` records over them, and a
  renderer for printing.
  `create_room`, `create_door` and `create_staircase` each set the cell type and add the record. A `Staircase` has its
  `directed_point_type`, the cell position and the direction it opens onto, and its `Rise`, down or up.
- `Pattern` --- a `Cells` with an origin and a shift, matched and replaced over a `Zone` by `match` and
  `match_replace`. `maze1` prunes with these.
- `types.hpp` --- `coord_type`, `point_type`, `vector_type`, `directed_point_type`, `random_type` (the random number
  source of a run), and `warning_function_type`.

Generation:

- `Algorithm` --- `Maze1`, `Grid1` and `Cave1`, with `to_string` and `to_algorithm` for the command line and the zone
  file's parameters.
- `Parameters` --- `algorithm`, `extent`, `staircase_down_count`, `staircase_up_count`, an optional `level`, and `seed`
  in common, then one
  group per algorithm: `Maze1Parameters`, `Grid1Parameters` and `Cave1Parameters`. `validate` checks the shared fields
  and the chosen algorithm's group, and `random_seed` draws a seed from the platform. The staircase counts are closed
  ranges of `size_t`, 1-1 by default. `validate` rejects one whose maximum is below its minimum, and a staircase up
  count whose minimum is zero, since a dungeon zone is entered and left by a staircase leading up.
- `generate` --- dispatches on `parameters.algorithm` to `generate_maze1`, `generate_grid1` or `generate_cave1`, one
  file each. Each asserts the algorithm it was given, validates, seeds its own `random_type` from `parameters.seed`, and
  is deterministic in its parameters. `generate` takes a warning function, `log_warning` by default, which writes to
  `std::clog`.
- `place_staircases` --- shared, and given the candidate cells by each algorithm. It draws a count leading down from
  `staircase_down_count` and a count leading up from `staircase_up_count`, with a range of one value drawing nothing,
  and shuffles the candidates. It then places staircases alternately leading up and leading down, while both counts
  have some left. A candidate is passed over if it is not floor, is a doorway or beside one, is the floor another
  staircase faces, or would leave its block cutting a passage or leave it or a staircase beside it no passable side to
  face; a door counts as passable. A doorway is a `Door` cell, or a corridor cell beside room floor, as a removed door
  leaves. A placed staircase faces a random side with floor beyond. If a count above zero has none placed,
  `place_staircases` throws `std::runtime_error`; if the candidates run out with fewer placed than drawn, it reports a
  warning.
- `remove_doors` --- shared, and run by `maze1` and `grid1` after every other pass. Each algorithm places every doorway
  as a door, and this keeps each door with probability `door_probability`, in the algorithm's group, and leaves the rest
  of the doorways open as corridor.
- `make_document` --- builds the zone's `stratum_format::Document` in memory, which `load_layout` reads directly.
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

Noise is smoothed into caverns. There are no rooms and no doors: open floor is corridor, and the border is left solid.

1. **Noise.** Every cell off the border is drawn solid with probability `fill`.
2. **Smoothing.** `smoothing_passes` passes of a cellular automaton over the eight neighbors of a cell: a solid cell
   stays solid with at least `survival_threshold` solid neighbors, and an open cell turns solid with at least
   `birth_threshold`. Every cell of a pass is computed from the same input grid.
3. **One area.** The largest connected area of open cells is kept and the rest is left solid, so every open cell is
   reachable from every other.
4. **Staircases,** anywhere in that area.

Parameters at their extremes --- `fill` at 1.0, `birth_threshold` at 0, or a zone small enough that every interior cell
has eight solid neighbors --- generate a zone that is solid throughout.

## The tool

`dungen` parses the command line, generates, and writes the zone to a file and to standard output. It writes warnings
to `std::clog`.

```
dungen [zone] [--algorithm maze1|grid1|cave1] [--extent nx,ny] [--name text]
            [--stairs-down min-max] [--stairs-up min-max] [--room-density d] [--door-probability p] [--level n] [--seed n]
  maze1: [--room-dimension-base n] [--room-dimension-variation n] [--room-merge]
         [--corridor-straightness s] [--probability-dead-end-removal p]
         [--probability-loop-removal p]
  grid1: [--divisions nx,ny] [--room-dimension-min n] [--room-margin n] [--loop l]
  cave1: [--fill f] [--smoothing-passes n] [--birth-threshold n] [--survival-threshold n]
```

The zone path defaults to `dungen.zone`, and the name to none. A run given no `--seed` draws one, so the file has a seed either way and any
zone can be reproduced. `--seed` is read by `math::to_seed`, as the seed's own unsigned type, which is wider than a
`long` on Windows; `catagen` shares the helper.

Each option sets the group it belongs to, whichever algorithm was chosen, so an option may be given before the
`--algorithm` it applies to. `--room-density` and `--door-probability` are shared by two groups and set both.
`--stairs-down` and `--stairs-up` take a closed range of counts, written in the zone file's parameters as
`stairs_down` and `stairs_up`; the options keep the shorter "stairs" for typing, where names in code and prose say
"staircase".

## Reading it back

`stratum_sim::load_layout` reads the file through `stratum_format`, which `game_system_reference.md` describes. The
loader does not read the parameters, so the three algorithms share one file form.

A doorway is a cell an actor may stand in, walled toward solid on its own side and toward floor on the floor's side
alone. A staircase cell is a block to the loader, closed like solid, and the floor and doorways beside it are walled toward
it.

## Known limits

- `pack_rooms`, which `maze1` calls if `room_density` reaches 1.0, is empty, so that density leaves a zone with no
  rooms, only the maze.
- `Zone::expand_room` repaints cells into a room but leaves the `Room`'s position and size as created, so the record
  understates a merged room, and `maze1`'s `door_candidates`, walking the record's four sides, offers a merged room
  doors only along its original rectangle. `--room-merge` is off by default.
