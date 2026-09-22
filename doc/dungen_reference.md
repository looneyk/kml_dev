# dungen --- Reference

What exists, as built. `dungen_plan.md` holds the intent.

`dungen` is a block oriented dungeon generator: a wall costs a whole cell, so solid cells lie between
rooms and a door is one of those cells a mover may pass through. `catagen` is the wall based
generator alongside it, where a wall is a property of the edge between two cells. Neither shares
code with the other.

- `library/quetzal/dungen` --- the structures and the generators
- `tool/dungen` --- the driver
- `library/quetzal/stratum_sim/load_layout.cpp` --- reads a zone file into a `Layout`

## The zone file

A discrete layout file in the block notation, written through `stratum_format`; `discrete_layout.md`
is the format. `generator.parameters` has the parameters of the run as the tokens its options take:
the ones every algorithm shares, then the group belonging to the algorithm named, then the seed, which
is what reproduces the zone exactly.

One character per cell:

| Glyph | Cell type | |
| ----- | --------- | - |
| `#` | `Solid`, `Null` | filled, with no way into it; `Null` is a cell no generator assigned, and none of the three leave any |
| `.` | `Room`, `Corridor` | floor |
| `+` | `Door` | a solid cell a mover may pass through |
| `>` | `Stair` | a staircase leading down |
| `<` | `Stair` | a staircase leading up; the cell type is the same, and the `Staircase` record gives the direction |

The origin is the lower left cell, and the highest row is written first, so the file reads as a map
and is oriented the same way up as the display.

Each dungen room is a room of the file, identified by the first of its room cells. Room floor lies
against corridor floor with nothing between, so every edge between two floor cells in different rooms,
or in a room and none, is a barrier. A stair cell is a block, which the floor beside it is walled
against in the block notation without anything written, and it is part of no room.

Each door is a `door` entity with a `door_facing` child on each wall `stratum_format::door_walls` finds
for its glyph. Each staircase is a `staircase` entity attached to the wall the cell it opens onto
presents to its block, with a `relocator` whose `target` is `down` or `up`.
`layout.components.access_points` refers to every staircase. The local ids run in the order the entities
are written --- each door, then its door facings, and the staircases last --- so the same parameters give
the same ids.

## The library

Structures, unchanged by which algorithm fills them:

- `Cell` --- a type, the id of the room it belongs to, and four `Wall`s.
- `Cells` --- a rectangle of them, addressed by `position_type`.
- `Level` --- the cells plus the `Room`, `Door` and `Staircase` records over them, and a renderer for
  printing. `create_room`, `create_door` and `create_staircase` each set the cell type and add the
  record. A `Staircase` has its position, its `StaircaseDirection`, down or up, and its facing.
- `Pattern` --- a `Cells` with an origin and a shift, matched and replaced over a `Level` by `match`
  and `match_replace`. `maze1` prunes with these; nothing else uses them.
- `types.hpp` --- `coord_type`, `vector_type` and its `position_type` and `dimensions_type` roles,
  and `random_type`, which is what a run draws from.

Generation:

- `Algorithm` --- `Maze1`, `Grid1`, `Cave1`, with `to_string` and `to_algorithm` for the command line
  and the zone file's parameters.
- `Parameters` --- `algorithm`, `size`, `staircase_down_count`, `staircase_up_count` and `seed` in
  common, then one group per
  algorithm: `Maze1Parameters`, `Grid1Parameters`, `Cave1Parameters`. `validate` checks the shared
  fields and the chosen algorithm's group; the other groups go unread, so they may hold anything a
  command line or a zone file put there. `random_seed` draws one from the platform. The two
  staircase counts are closed ranges of `size_t`, 1-1 by default, and `validate` rejects one whose
  maximum is below its minimum, and counts whose minimums are both zero, every staircase being an
  access point and a zone needing one.
- `generate` --- dispatches on `parameters.algorithm` to `generate_maze1`, `generate_grid1` or
  `generate_cave1`, one file each. Each asserts the algorithm it was given, validates, seeds its own
  `random_type` from `parameters.seed`, and is deterministic in what it was handed. It throws
  `std::runtime_error` where no staircase could be placed to be an access point, which takes a zone
  with no candidate cell at all.
- `place_staircases` --- shared. Each algorithm hands it the cells its own layout allows a staircase to
  occupy. It draws a count leading down from `staircase_down_count` and a count leading up from
  `staircase_up_count`, a range of one drawing nothing, then shuffles the candidates and places the
  staircases leading up first. A candidate is passed over where its block would cut a passage, its
  passable neighbors no longer reaching one another around it, or would leave it or a staircase beside it
  no passable side to face, a door counting as passable. Running out of candidates places fewer, those
  leading down going short first, and having none to place throws.
- `remove_doors` --- shared, and run by `maze1` and `grid1` after every pass but the staircases'. Each
  algorithm places every doorway as a door and reads it as one; this keeps each door with probability
  `door_probability`, in the algorithm's group, and leaves the rest of the doorways open as corridor.
- `move_staircases_off_doorways` and `face_staircases` --- shared, and the last two passes of every
  algorithm. A doorway is a `Door` cell, or a corridor cell beside room floor, which is what a removed
  door leaves. A staircase beside one moves a cell, in a random direction onto `Room` or `Corridor`
  floor of its own space that is not a doorway, is beside none, and takes a block as `place_staircases`
  allows one, and stays where there is no such direction. Its facing is then drawn from the sides of its
  cell onto `Room` or `Corridor` floor, or where there are none, from the sides onto a door, and
  `Staircase::facing` stores it. Both draw last, so nothing
  before them changes.
- `write_level` --- the zone file.

## maze1

Rooms dropped at random, a maze flooded through everything they left, then pruned back. Both
dimensions are rounded down to odd, the rooms and the maze running along the odd cells.

1. **Rooms.** A count drawn from the grid area and `room_density` decides how many placements are
   attempted. Each draws a size from `room_dimension_base` plus a value below
   `room_dimension_variation`, forced odd, and a position on the odd lattice. A placement
   overlapping two or more rooms is dropped; one overlapping a single room is dropped as well,
   unless `room_merge` grows that room to cover it.
2. **Doors.** Every other cell along a room's walls is a candidate. The count comes from the room's
   size, and a door between two rooms is only ever placed once, so no two rooms are joined twice.
3. **Corridors.** A recursive backtracker floods every solid odd cell.
   `corridor_straightness` is the chance it keeps its heading where it can.
4. **Staircases,** at the corridor dead ends, before they are pruned. Rooms and loops can leave a maze
   with no dead end, and then the candidates are the rooms, one cell at the center of each.
5. **Pruning.** `probability_dead_end_removal` walks each dead end back a cell at a time;
   `probability_loop_removal` closes the loops the flood left.
6. **Open doorways.** `remove_doors` at `door_probability`.
7. **Staircases off doorways, and facing them.** `move_staircases_off_doorways`, then
   `face_staircases`.

## grid1

The zone cut into `divisions` sectors, a room inside most of them, adjacent sectors joined.

1. **Sectors.** Sector bounds divide the zone, an uneven division spreading its remainder across
   the sectors rather than leaving it all at one end.
2. **Rooms.** `room_density` fixes how many of the sectors hold a room rather than the chance of any
   one of them doing so, so a zone does not come out empty. A room takes a random size from
   `room_dimension_min` up to what its sector allows, and a random position inside it, with
   `room_margin` cells kept clear on every side. A sector holding no room gets a single cell instead,
   which its neighbours still join through.
3. **Connections.** A randomized Kruskal over the orthogonal adjacencies of the sector grid: an
   adjacency joining two components is always kept, one that would close a loop is kept with
   probability `loop`. The result spans every sector, so every room is reachable.
4. **Corridors.** Each connection leaves one room through the wall it faces the other across, turns
   once, and enters the other, with a `Door` at each end that falls on a room. The turn is taken on
   a line drawn from the solid cells strictly between the two rooms' facing edges, and both legs
   stay within the two sectors' shared band, so a corridor never crosses a room. `room_margin` of at
   least one cell is what leaves those cells, and `validate` requires it.
5. **Staircases,** in the rooms, or in the corridors where the density left no rooms at all.
6. **Open doorways.** `remove_doors` at `door_probability`.
7. **Staircases off doorways, and facing them.** `move_staircases_off_doorways`, then
   `face_staircases`.

The orientation of an adjacency is stored on it rather than read back out of the difference between
the two sector slots: a single column of sectors puts its neighbours one slot apart just as a row
does.

## cave1

Noise smoothed into caverns. No rooms and no doors --- open floor is corridor, and the border is left
solid so that no cavern opens onto the border of the zone.

1. **Noise.** Every cell off the border drawn solid with probability `fill`.
2. **Smoothing.** `smoothing_passes` passes of a cellular automaton over the eight neighbours of a
   cell: a solid cell stays solid while at least `survival_threshold` of them are solid, and open
   ground fills in where at least `birth_threshold` are. A pass reads the grid it was given
   throughout, so every cell turns on the same state.
3. **One region.** The largest region of open ground is kept and the rest left solid, so everything
   the zone holds is reachable from everywhere else it holds.
4. **Staircases,** anywhere on that region.
5. **Staircases off doorways, and facing them.** `move_staircases_off_doorways`, which finds no
   doorway here, then `face_staircases`.

Parameters at their extremes leave a zone solid throughout --- `fill` at 1.0, `birth_threshold` at 0,
or a zone small enough that its every interior cell has eight solid neighbours. That is what those
parameters ask for, and it is generated rather than rejected.

## The tool

`dungen` parses the command line, generates, and writes the zone to a file and to standard
output. It holds no generation of its own.

```
dungen [zone] [--algorithm maze1|grid1|cave1] [--size nx,ny]
            [--stairs-down min-max] [--stairs-up min-max] [--room-density d] [--door-probability p] [--seed n]
  maze1: [--room-dimension-base n] [--room-dimension-variation n] [--room-merge]
         [--corridor-straightness s] [--probability-dead-end-removal p]
         [--probability-loop-removal p]
  grid1: [--divisions nx,ny] [--room-dimension-min n] [--room-margin n] [--loop l]
  cave1: [--fill f] [--smoothing-passes n] [--birth-threshold n] [--survival-threshold n]
```

The zone path defaults to `Level_0.level`. A run given no `--seed` draws one, so the file names a
seed either way and any zone can be produced again; `--seed` goes through `math::to_seed`, which
reads the whole of the seed's own unsigned range, wider than a `long` on Windows. `catagen` shares
the helper.

Each option sets the group it belongs to, whichever algorithm was named, so an option may be given
before the `--algorithm` it applies to. `--room-density` and `--door-probability` are names two groups
share and set both; the chosen algorithm reads its own. `--stairs-down` and `--stairs-up` take a
closed range of counts, written in the zone file's parameters as `stairs_down` and `stairs_up`; an
option keeps the shorter "stairs" to type, where names in code and prose say "staircase".

## Reading it back

`stratum_sim::load_layout` reads the file through `stratum_format`, which `game_system_reference.md`
describes. The parameters are content the loader passes over, so three algorithms with different
parameters share one file shape.

A doorway is a cell a mover may stand in, walled toward solid on its own side and toward floor on the
floor's side alone. A stair cell is a block to the loader, closed like solid, and the floor and doorways
beside it are walled toward it.

## Known limits

- `pack_rooms`, which `maze1` calls where `room_density` reaches 1.0, is empty. That density leaves
  a zone with no rooms at all, only the maze.
- `Level::expand_room` repaints cells into a room but leaves that `Room`'s own position and size at
  what it was created with, so the record understates a merged room. `maze1`'s `door_candidates`
  reads that record and walks the four sides as straight runs, so a merged room is only offered
  doors along its original rectangle. `--room-merge` is off by default.
