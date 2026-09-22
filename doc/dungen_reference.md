# dungen --- Reference

What exists, as built. `dungen_plan.md` is the intent.

`dungen` is a block oriented zone generator: a wall takes a whole cell, so solid cells lie between rooms, and a door is
one of those cells an actor may pass through. `catagen` is the wall based generator beside it, where a wall is a
property of the edge between two cells. Neither shares code with the other.

- `library/quetzal/dungen` --- the structures and the generators
- `tool/dungen` --- the driver
- `library/quetzal/stratum_sim/load_layout.cpp` --- reads a zone file into a `Layout`

## The zone file

A discrete layout file in the block notation, written through `stratum_format`; `discrete_layout.md` is the format.
`generator.parameters` has the parameters of the run as the tokens its options take: the ones every algorithm shares,
then the chosen algorithm's group, then the seed, so the file reproduces the zone exactly.

One glyph per cell:

| Glyph | Cell type | |
| --- | --- | --- |
| `#` | `Solid`, `Null` | solid; `Null` is a cell no generator assigned, and none of the three leave any |
| `.` | `Room`, `Corridor` | floor |
| `+` | `Door` | a solid cell an actor may pass through |
| `>` | `Stair` | a staircase leading down |
| `<` | `Stair` | a staircase leading up; the cell type is the same, and the `Staircase` record has the direction |

The origin is the lower left cell, and the highest row is written first, so the file reads as a map oriented as the
display is.

Each dungen room is a room of the file, identified by the first of its room cells. Room floor lies against corridor
floor with nothing between, so every edge between two floor cells in different rooms, or in a room and none, is a
barrier. A stair cell is a block, which the floor beside it is walled against in the block notation without anything
written, and it is part of no room.

Each door is a `door` entity with a `door_facing` child on each wall `stratum_format::door_walls` finds for its glyph.
Each staircase is a `staircase` entity sited on the wall that the cell it opens onto has toward its block, with a
`relocator` whose `target` is `down` or `up`. `layout.components.access_points` refers to every staircase. Local ids
are in the order the entities are written --- each door, then its door facings, and the staircases last --- so the
same parameters give the same ids.

## The library

Structures, the same whichever algorithm fills them:

- `Cell` --- a type, the id of the room it belongs to, and four `Wall`s.
- `Cells` --- a rectangle of them, addressed by `position_type`.
- `Level` --- the cells and the `Room`, `Door` and `Staircase` records over them, and a renderer for printing.
  `create_room`, `create_door` and `create_staircase` each set the cell type and add the record. A `Staircase` has its
  position, its `StaircaseDirection`, down or up, and its facing direction.
- `Pattern` --- a `Cells` with an origin and a shift, matched and replaced over a `Level` by `match` and
  `match_replace`. `maze1` prunes with these.
- `types.hpp` --- `coord_type`, `vector_type` in its `position_type` and `dimensions_type` roles, and `random_type`,
  the random number source of a run.

Generation:

- `Algorithm` --- `Maze1`, `Grid1`, `Cave1`, with `to_string` and `to_algorithm` for the command line and the zone
  file's parameters.
- `Parameters` --- `algorithm`, `size`, `staircase_down_count`, `staircase_up_count` and `seed` in common, then one
  group per algorithm: `Maze1Parameters`, `Grid1Parameters`, `Cave1Parameters`. `validate` checks the shared fields
  and the chosen algorithm's group. `random_seed` draws a seed from the platform. The staircase counts are closed
  ranges of `size_t`, 1-1 by default, and `validate` rejects one whose maximum is below its minimum, and counts whose
  minimums are both zero, every staircase being an access point and a zone requiring one.
- `generate` --- dispatches on `parameters.algorithm` to `generate_maze1`, `generate_grid1` or `generate_cave1`, one
  file each. Each asserts the algorithm it was given, validates, seeds its own `random_type` from `parameters.seed`,
  and is deterministic in its parameters. It throws `std::runtime_error` where no staircase could be placed to be an
  access point, which takes a zone with no candidate cell at all.
- `place_staircases` --- shared, given the candidate cells by each algorithm. It draws a count leading down from
  `staircase_down_count` and a count leading up from `staircase_up_count`, a range of one drawing nothing, shuffles
  the candidates, and places the staircases leading up first. A candidate is passed over where its block would cut a
  passage, its passable neighbors no longer reaching one another around it, or leave it or a staircase beside it no
  passable side to face, a door counting as passable. Running out of candidates places fewer, those leading down going
  short first, and placing none throws.
- `remove_doors` --- shared, and run by `maze1` and `grid1` after every pass but the staircases'. Each algorithm places
  every doorway as a door and reads it as one; this keeps each door with probability `door_probability`, in the
  algorithm's group, and leaves the rest of the doorways open as corridor.
- `move_staircases_off_doorways` and `face_staircases` --- shared, and the last two passes of every algorithm. A
  doorway is a `Door` cell, or a corridor cell beside room floor, as a removed door leaves. A staircase beside one
  moves a cell, in a random direction onto `Room` or `Corridor` floor of its own space that is not a doorway, is beside
  none, and allows a block as `place_staircases` does, and stays where there is no such direction. Its facing direction
  is then drawn from the sides of its cell onto `Room` or `Corridor` floor, or where there are none, from the sides
  onto a door, and stored in `Staircase::facing`. Both draw last, so nothing before them changes.
- `write_level` --- the zone file.

## maze1

Rooms dropped at random, a maze flooded through every cell they left, then pruned back. Both dimensions are rounded down
to odd, the rooms and the maze running along the odd cells.

1. **Rooms.** A count drawn from the grid area and `room_density` sets how many placements are attempted. Each draws a
   size from `room_dimension_base` plus a value below `room_dimension_variation`, forced odd, and a position on the odd
   lattice. A placement overlapping two or more rooms is dropped; one overlapping a single room is dropped as well,
   unless `room_merge` grows that room to include it.
2. **Doors.** Every other cell along a room's walls is a candidate. The count comes from the room's size, and a door
   between two rooms is placed only once.
3. **Corridors.** A recursive backtracker floods every solid odd cell. `corridor_straightness` is the chance it keeps
   its heading where it can.
4. **Staircases,** at the corridor dead ends, before they are pruned, or at the center of each room where rooms and
   loops have left no dead end.
5. **Pruning.** `probability_dead_end_removal` walks each dead end back a cell at a time; `probability_loop_removal`
   closes the loops the flood left.
6. **Open doorways.** `remove_doors` at `door_probability`.
7. **Staircases off doorways, and their facing directions.** `move_staircases_off_doorways`, then `face_staircases`.

## grid1

The zone divided into `divisions` sectors, a room inside most of them, adjacent sectors joined.

1. **Sectors.** Sector bounds divide the zone, spreading the remainder of an uneven division across the sectors.
2. **Rooms.** `room_density` is the fraction of sectors with a room, not a per-sector probability, so a zone is never
   left empty. A room has a random size from `room_dimension_min` up to what its sector allows, and a random position
   inside it, with `room_margin` cells clear on every side. A sector without a room has a single cell instead, through
   which its neighbors join.
3. **Connections.** A randomized Kruskal over the orthogonal adjacencies of the sector grid: an adjacency joining two
   components is always kept, and one closing a loop is kept with probability `loop`. The result spans every sector, so
   every room is reachable.
4. **Corridors.** Each connection leaves one room through the wall toward the other, turns once, and enters the other,
   with a `Door` at each end on a room. The turn is on a line through the solid cells strictly between the two rooms'
   facing edges, and both legs stay within the two sectors' shared band, so a corridor never crosses a room. A
   `room_margin` of at least one cell leaves those cells, and `validate` requires it.
5. **Staircases,** in the rooms, or in the corridors where the density left no rooms.
6. **Open doorways.** `remove_doors` at `door_probability`.
7. **Staircases off doorways, and their facing directions.** `move_staircases_off_doorways`, then `face_staircases`.

An adjacency stores its orientation, which the difference between the two sector slots leaves ambiguous for a single
column of sectors.

## cave1

Noise smoothed into caverns. No rooms and no doors: open floor is corridor, and the border is left solid.

1. **Noise.** Every cell off the border drawn solid with probability `fill`.
2. **Smoothing.** `smoothing_passes` passes of a cellular automaton over the eight neighbors of a cell: a solid cell
   stays solid with at least `survival_threshold` solid neighbors, and an open cell turns solid with at least
   `birth_threshold`. Every cell of a pass is computed from the same input grid.
3. **One region.** The largest connected region of open cells is kept and the rest left solid, so every open cell is
   reachable from every other.
4. **Staircases,** anywhere on that region.
5. **Staircases off doorways, and their facing directions.** `move_staircases_off_doorways`, which finds no doorway
   here, then `face_staircases`.

Parameters at their extremes --- `fill` at 1.0, `birth_threshold` at 0, or a zone small enough that every interior cell
has eight solid neighbors --- generate a zone solid throughout.

## The tool

`dungen` parses the command line, generates, and writes the zone to a file and to standard output.

```
dungen [zone] [--algorithm maze1|grid1|cave1] [--size nx,ny]
            [--stairs-down min-max] [--stairs-up min-max] [--room-density d] [--door-probability p] [--seed n]
  maze1: [--room-dimension-base n] [--room-dimension-variation n] [--room-merge]
         [--corridor-straightness s] [--probability-dead-end-removal p]
         [--probability-loop-removal p]
  grid1: [--divisions nx,ny] [--room-dimension-min n] [--room-margin n] [--loop l]
  cave1: [--fill f] [--smoothing-passes n] [--birth-threshold n] [--survival-threshold n]
```

The zone path defaults to `Level_0.level`. A run given no `--seed` draws one, so the file has a seed either way and any
zone can be reproduced. `--seed` is read by `math::to_seed`, as the seed's own unsigned type, wider than a `long` on
Windows; `catagen` shares the helper.

Each option sets the group it belongs to, whichever algorithm was chosen, so an option may be given before the
`--algorithm` it applies to. `--room-density` and `--door-probability` are shared by two groups and set both.
`--stairs-down` and `--stairs-up` take a closed range of counts, written in the zone file's parameters as
`stairs_down` and `stairs_up`; an option keeps the shorter "stairs" to type, where names in code and prose say
"staircase".

## Reading it back

`stratum_sim::load_layout` reads the file through `stratum_format`, which `game_system_reference.md` describes. The
loader does not read the parameters, so the three algorithms share one file form.

A doorway is a cell an actor may stand in, walled toward solid on its own side and toward floor on the floor's side
alone. A stair cell is a block to the loader, closed like solid, and the floor and doorways beside it are walled toward
it.

## Known limits

- `pack_rooms`, which `maze1` calls where `room_density` reaches 1.0, is empty, so that density leaves a zone with no
  rooms, only the maze.
- `Level::expand_room` repaints cells into a room but leaves the `Room`'s position and size as created, so the record
  understates a merged room, and `maze1`'s `door_candidates`, walking the record's four sides, offers a merged room
  doors only along its original rectangle. `--room-merge` is off by default.
