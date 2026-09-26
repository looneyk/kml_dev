# catagen --- Reference

This document describes what exists, as built. `catagen_plan.md` is the intent.

`catagen` is a wall-based zone generator: a wall is a property of the edge between two cells instead of a cell of its
own, so rooms share walls and a partition of the extent has no solid cell. `dungen` is the block-oriented generator
beside it. The two share no code.

- `library/quetzal/catagen` --- the generator
- `tool/catagen` --- the driver
- `library/quetzal/stratum_sim/load_layout.cpp` --- reads a zone file into a zone
- `assets/level/catagen.level` --- a generated zone that `stratum_terminal` loads

## The zone file

The generator writes a discrete layout file in the wall notation, through `stratum_format`; `discrete_layout.md` is
the format. `generator.parameters` has the parameters of the run as the tokens their options take --- `extent`,
`scale`, `spacing`, `density`, `loop`, `stairs_down`, `stairs_up` and `seed` --- so a file reproduces its own zone and
its parameters read as a command line.

The rows are `(2W+1)` by `(2H+1)` characters. Cell `(i, j)` is at `(2i+1, 2j+1)`, the edge west of it at `(2i, 2j+1)`,
the edge south of it at `(2i+1, 2j)`, and corners at the positions that are even in both. The first row written is the
highest, and the origin is the lower left cell.

| position | glyphs |
| --- | --- |
| cell | `.` floor, space no floor, `>` staircase down, `<` staircase up |
| edge, horizontal | `-` wall, `+` door, space no wall |
| edge, vertical | `\|` wall, `+` door, space no wall |
| corner | space |

An edge between two solid cells has no wall. A stair cell is a block: the rows write a wall on every edge between one
and floor, even where the generated wall is open, and none on its other edges. catagen writes no rooms.

Each door is a `door` entity with a `door_facing` child on each wall that `stratum_format::door_walls` finds for its
glyph: the two sides of its edge. Each staircase is a `staircase` entity sited on the wall that the cell it opens onto
--- floor or a door --- has toward its block. Its `relocator` has a `type` and `target` of `down` or `up`; an `order`
that counts from 0 among the staircases leading the same way, in the order they are written; and an `entry`, which is
the floor its open side faces, facing away from it. `layout.components.access_points` refers to every staircase. Local
ids are assigned in the order the entities are written --- each door, then its door facings, and the staircases last
--- so the same parameters give the same ids.

## The library

**`types.hpp`** --- `coord_type` is `std::int32_t`; `point_type`, `vector_type` and `box_type` are the discrete point,
vector and half-open box over it. `CellType` is `Solid`, `Open`, `StairsDown` or `StairsUp`; `WallType` is `Open`,
`Solid` or `Door`, where `Open` is the absence of a wall. Directions are `CardinalDirection`.

**`Level`** --- a rectangle of cells and the walls between them. Rooms, doors and staircases are cell and wall state,
and each staircase's facing direction is stored by `set_staircase_facing` and read by `staircase_facing`. Walls are
stored by the line they lie on, which includes the outermost lines: `wall_vertical(i, j)` lies between cells
`(i - 1, j)` and `(i, j)`, so `i` runs to `nx` inclusive, and `wall_horizontal(i, j)` lies between `(i, j - 1)` and
`(i, j)`, so `j` runs to `ny` inclusive. `wall(cell, direction)` reads either; the east wall of `(i, j)` and the west
wall of `(i + 1, j)` are the same wall. A new `Level` has no cell open and every wall solid.

**`Parameters`** --- `extent`; `scale` and `spacing`, closed ranges of cells; `density`; `loop`;
`staircase_down_count` and `staircase_up_count`, closed ranges of counts defaulting to 1-1; and `seed`, a
`std::optional` that is empty until assigned. `validate` throws `std::invalid_argument` identifying the parameter at
fault, including an unassigned seed, a count range whose maximum is below its minimum, and a staircase up count whose
minimum is zero, since a dungeon level is entered and left by a staircase leading up. `random_seed()` draws a seed from
`std::random_device`.

**`generate`** runs the algorithm below. **`write_level`** writes the zone file, and **`write_grid`** writes only its
rows.

## The algorithm

One `math::Random<>` is seeded from `Parameters::seed`, and every pass draws from it in a fixed order, so the run is
determined by its parameters.

**1. Partition.** The extent is split recursively along its longer side while a side exceeds the scale maximum and
both halves would still reach the scale minimum, with the cut at a uniformly chosen position within that range.

Each cut also draws a spacing from its range, and that many solid cells are left along the cut, so the gap between two
rooms is the spacing drawn. Two rectangles that touch do so only across the cut of their lowest common ancestor, so
spacing every cut spaces every pair: at any spacing above zero no two rooms share a wall, and corridors join them all.
A side must reach `2 * scale_min + spacing` to be cut, so a large spacing makes rooms larger than the scale asked for.
At spacing zero, the default, the rectangles tile the extent exactly, and no value is drawn, since a range of one value
offers no choice.

**2. Selection.** The rooms are the rectangles kept. At `density` 1.0 every rectangle is kept. Below it, rectangles are
taken in a random order until their cells reach the density, and the rest are solid. An area is floor that walls
separate and doors join. Each room is one area, whose area id is its position in the room list, stored in a map from
each cell to its area.

**3. Corridors.** While two rooms are unconnected, the closest such pair by Manhattan distance between centers is
joined by an L-shaped path that turns once, with the first leg chosen at random. Only solid cells along the path are
carved, and each unbroken run of carved cells becomes an area of its own, so a path crossing a room does not split one
area into disconnected pieces. Components are recounted after each corridor, so a corridor joining more than two rooms
is not followed by a redundant one. A corridor is an area one cell wide.

**4. Realization.** A cell is floor if it belongs to an area, and an edge is open only between two cells of the same
area. Every other edge with floor on a side has a solid wall, including the outer boundary.

**5. Doors.** The cell edges with a different area on each side are grouped by the pair of areas, and each is a
possible door site. The pairs are taken in random order, and a door is placed at a random site of each pair that joins
two areas not yet joined. This makes a random spanning tree over the adjacency graph, so every area is reachable. Each
remaining pair gets a door with probability `loop`.

**6. Staircases.** A count leading down is drawn from `staircase_down_count` and a count leading up from
`staircase_up_count`, with a range of one value drawing nothing. Each staircase is placed at the center of a room
chosen at random from those without one, those leading up first. A room is passed over if a block at its center would
cut a passage, with the cells across its passable sides no longer reaching one another around it, or would leave it or
a staircase beside it no passable side to face; a door counts as passable. Running out of rooms places fewer
staircases, with those leading down falling short first. A zone with no room for a staircase throws
`std::runtime_error`.

**7. Staircases off doorways.** A staircase with a door on a side of its cell moves one cell in a random direction,
across an open wall, onto floor with no door on any side where step 6 allows a block. A staircase with no such
direction stays where it is. An open wall joins two cells of one area, so the staircase stays in its own area.

**8. Facing direction.** Each staircase's facing direction is drawn from the sides of its cell that have an open wall
and floor beyond, or, if there are none, from those with a door. These last three passes draw after every other, so
nothing before them changes.

## The tool

```
catagen [zone] [--extent nx,ny] [--scale min-max] [--spacing min-max] [--density d]
             [--loop l] [--stairs-down min-max] [--stairs-up min-max] [--seed n] [--quiet]
```

The tool writes the zone to the named file, `catagen.level` by default, and to standard output unless `--quiet` is
given. A run given no `--seed` takes `random_seed()` before generating, so the file has a seed either way and any run
can be reproduced from what it printed. The option names are the parameter names, so a file's parameters read back as a
command line. `--stairs-down` and `--stairs-up` take a closed range of counts; the options keep the shorter "stairs" for
typing, where names in code and prose say "staircase".

`--seed` is read by `math::to_seed`, as the seed's own unsigned type, which is wider than a `long` where a `long` is 32
bits. `dungen` reads its seed the same way.

## Reading it back

`stratum_sim::load_layout` reads the file through `stratum_format`, which `game_system_reference.md` describes.

A door is both sides of one edge, and two rooms sharing a solid wall are walled from both sides along its whole length,
which the block notation cannot express: a wall with a room behind it instead of a solid cell. A stair cell is a block
to the loader, closed like solid, and a catagen zone loads with no rooms.

## Properties

- **`>` is down and `<` is up**, as in dungen, so both notations share the cell glyphs.
- **Loop adds doors, never corridors**: a loop is a second way between two rooms that are already adjacent.
- **A scale range narrower than twice its minimum yields rooms above the maximum**, since no cut would leave both
  halves large enough. A scale maximum at or above `2 * scale_min - 1` avoids it.
- **One glyph per edge describes both sides alike.** A wall is per side, and the loader builds a wall on each side from
  the one glyph. A door open or secret from one side only is file components on that side's door facing.

## Tests

`test/catagen_test` checks that what the generator writes now is equivalent to its reference zone files, regenerates
each from its own parameters, and checks that the reader accepts every zone it generates. Over a sweep of seeds with
ranges of staircases each way, it checks the written zone: the counts keep to their ranges, every floor cell is
reachable from every other, every edge between a stair cell and floor is a wall, no staircase has a door beside it if a
free cell does not, and every staircase is sited on one of the walls around its block and opens onto a side that
follows the rule. It takes which sides of a stair cell were open from the zone generated again.
`assets/level/catagen.level` is the `shipped` reference zone.

`test/stratum_sim_test` has a `load wall layout` group over two files in its `data`: `walls.level`, two rooms of two by
three cells sharing one wall with a door in the middle of it; and `catagen.level`, a generated zone checked for solid
cells, for edges walled from both sides counting in pairs, and for every open cell being reachable from the first
across crossable walls alone.
