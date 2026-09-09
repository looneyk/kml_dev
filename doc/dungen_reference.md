# dungen --- Reference

What exists, as built. `dungen_plan.md` holds the intent.

`dungen` is a block oriented dungeon generator: a wall costs a whole cell, so solid cells lie between
rooms and a door is one of those cells a mover may pass through. `catagen` is the wall based
generator alongside it, where a wall is a property of the edge between two cells. Neither shares
code with the other.

- `library/quetzal/dungen` --- the structures and the generators
- `tool/dungen` --- the driver
- `library/quetzal/stratum_sim/load_layout.cpp` --- reads both formats into a `Layout`

## The level file

One header row, then the grid.

```
dungen 0.2 algorithm=grid1 size=60,30 stair_placements=paired divisions=3,3 room_density=1 room_dimension_min=3 room_margin=1 loop=0.15 seed=3
```

The header names the generator, the format version, and the parameters of the run: the ones every
algorithm shares, then the group belonging to the algorithm named, then the seed. The seed is last
and is what reproduces the level exactly. A grid row never begins with a letter, so a reader tells
the two apart by the first character; a file with no header row at all is a grid alone.

One character per cell, written by `Cell::render` from `cell_types`:

| Glyph | Cell type | |
| ----- | --------- | - |
| `#` | `Solid` | filled, with no way into it |
| ` ` | `Room` | room floor |
| `.` | `Corridor` | corridor floor |
| `\|` | `Door` | a solid cell a mover may pass through |
| `>` | `Stair` | a way off the level |
| `0` | `Null` | a cell no generator assigned; none of the three leave any |

The origin is the lower left cell, and the highest row is written first, so the file reads as a map
and is oriented the same way up as the display. `catagen` writes its grid the same way round, and
`load_layout` reads the first line of either as the highest row, so a level round trips unmirrored.

## The library

Structures, unchanged by which algorithm fills them:

- `Cell` --- a type, the id of the room it belongs to, and four `Wall`s.
- `Cells` --- a rectangle of them, addressed by `position_type`.
- `Level` --- the cells plus the `Room`, `Door` and `Stair` records over them, and the renderer.
  `create_room`, `create_door` and `create_stair` each set the cell type and add the record.
- `Pattern` --- a `Cells` with an origin and a shift, matched and replaced over a level by `match`
  and `match_replace`. `maze1` prunes with these; nothing else uses them.
- `types.hpp` --- `coord_type`, `vector_type` and its `position_type` and `dimensions_type` roles,
  and `random_type`, which is what a run draws from.

Generation:

- `Algorithm` --- `Maze1`, `Grid1`, `Cave1`, with `to_string` and `to_algorithm` for the command line
  and the header.
- `StairPlacements` --- `None`, `Paired`, `Multiple`, likewise.
- `Parameters` --- `algorithm`, `size`, `stair_placements` and `seed` in common, then one group per
  algorithm: `Maze1Parameters`, `Grid1Parameters`, `Cave1Parameters`. `validate` checks the shared
  fields and the chosen algorithm's group; the other groups go unread, so they may hold anything a
  command line or a header put there. `random_seed` draws one from the platform.
- `generate` --- dispatches on `parameters.algorithm` to `generate_maze1`, `generate_grid1` or
  `generate_cave1`, one file each. Each asserts the algorithm it was given, validates, seeds its own
  `random_type` from `parameters.seed`, and is deterministic in what it was handed.
- `place_stairs` --- shared. Each algorithm hands it the cells its own layout allows a stairway to
  occupy; it shuffles them and takes two for `Paired`, or three plus one per thousand cells for
  `Multiple`. Running out of candidates places fewer.
- `write_level` --- the header row and the grid.

## maze1

Rooms dropped at random, a maze flooded through everything they left, then pruned back. Both
dimensions are rounded down to odd, the rooms and the maze running along the odd cells.

1. **Rooms.** A count drawn from the level area and `room_density` decides how many placements are
   attempted. Each draws a size from `room_dimension_base` plus a value below
   `room_dimension_variation`, forced odd, and a position on the odd lattice. A placement
   overlapping two or more rooms is dropped; one overlapping a single room is dropped as well,
   unless `room_merge` grows that room to cover it.
2. **Doors.** Every other cell along a room's walls is a candidate. The count comes from the room's
   size, and a door between two rooms is only ever placed once, so no two rooms are joined twice.
3. **Corridors.** A recursive backtracker floods every solid odd cell.
   `corridor_straightness` is the chance it keeps its heading where it can.
4. **Stairs,** at the corridor dead ends, before they are pruned.
5. **Pruning.** `probability_dead_end_removal` walks each dead end back a cell at a time;
   `probability_loop_removal` closes the loops the flood left.

## grid1

The level cut into `divisions` sectors, a room inside most of them, adjacent sectors joined.

1. **Sectors.** Sector bounds divide the level, an uneven division spreading its remainder across
   the sectors rather than leaving it all at one end.
2. **Rooms.** `room_density` fixes how many of the sectors hold a room rather than the chance of any
   one of them doing so, so a level does not come out empty. A room takes a random size from
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
5. **Stairs,** in the rooms, or in the corridors where the density left no rooms at all.

The orientation of an adjacency is stored on it rather than read back out of the difference between
the two sector slots: a single column of sectors puts its neighbours one slot apart just as a row
does.

## cave1

Noise smoothed into caverns. No rooms and no doors --- open floor is corridor, and the border is left
solid so that no cavern opens onto the edge of the level.

1. **Noise.** Every cell off the border drawn solid with probability `fill`.
2. **Smoothing.** `smoothing_passes` passes of a cellular automaton over the eight neighbours of a
   cell: a solid cell stays solid while at least `survival_threshold` of them are solid, and open
   ground fills in where at least `birth_threshold` are. A pass reads the grid it was given
   throughout, so every cell turns on the same state.
3. **One region.** The largest region of open ground is kept and the rest left solid, so everything
   the level holds is reachable from everywhere else it holds.
4. **Stairs,** anywhere on that region.

Parameters at their extremes leave a level solid throughout --- `fill` at 1.0, `birth_threshold` at 0,
or a level small enough that its every interior cell has eight solid neighbours. That is what those
parameters ask for, and it is generated rather than rejected.

## The tool

`dungen` parses the command line, generates, and writes the level to a file and to standard
output. It holds no generation of its own.

```
dungen [level] [--algorithm maze1|grid1|cave1] [--size nx,ny]
            [--stair-placements none|paired|multiple] [--room-density d] [--seed n]
  maze1: [--room-dimension-base n] [--room-dimension-variation n] [--room-merge]
         [--corridor-straightness s] [--probability-dead-end-removal p]
         [--probability-loop-removal p]
  grid1: [--divisions nx,ny] [--room-dimension-min n] [--room-margin n] [--loop l]
  cave1: [--fill f] [--smoothing-passes n] [--birth-threshold n] [--survival-threshold n]
```

The level path defaults to `Level_0.level`. A run given no `--seed` draws one, so the header names a
seed either way and any level can be produced again; `--seed` goes through `math::to_seed`, which
reads the whole of the seed's own unsigned range, wider than a `long` on Windows. `catagen` shares
the helper.

Each option sets the group it belongs to, whichever algorithm was named, so an option may be given
before the `--algorithm` it applies to. `--room-density` is the one name two groups share and sets
both; the chosen algorithm reads its own.

## Reading it back

`stratum_sim/load_layout.cpp` accepts a `dungen 0.2` header, or a file with no header, and hands the
grid to `load_layout_dungen.cpp`. The header is parsed as a generator, a version, and `key=value`
tokens into a map, so a key it does not know is kept rather than refused --- which is what lets one
header format serve three algorithms with different parameters.

Solid is `#` and `0`; room and corridor are both open floor, differing only in what the level calls
them; a doorway or a stairway is a cell of wall a mover may stand in, and becomes a block of its own
whose faces adjoin the areas either side.

## Known limits

- `pack_rooms`, which `maze1` calls where `room_density` reaches 1.0, is empty. That density leaves
  a level with no rooms at all, only the maze.
- A `Stair` stores no direction. `place_stairs` decides which way each leads and has nowhere to put
  it, so up and down are indistinguishable in the file and in the level.
- `Level::expand_room` repaints cells into a room but leaves that `Room`'s own position and size at
  what it was created with, so the record understates a merged room. `maze1`'s `door_candidates`
  reads that record and walks the four sides as straight runs, so a merged room is only offered
  doors along its original rectangle. `--room-merge` is off by default.
