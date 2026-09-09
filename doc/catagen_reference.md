# catagen --- Reference

What exists, as built. `catagen_plan.md` holds the intent.

`catagen` is a wall based dungeon generator: a wall is a property of the edge between two cells
rather than a cell of its own, so rooms share walls and a partition of a rectangle leaves no cell
solid.
`dungen` remains the block oriented generator alongside it. Neither shares code with the other.

- `library/quetzal/catagen` --- the generator
- `tool/catagen` --- the driver
- `library/quetzal/stratum_sim/load_layout.cpp` --- reads both formats into a `Layout`
- `assets/level/catagen.level` --- a generated level `stratum_terminal` loads

## The level file

One header row, then the grid.

```
catagen 0.2 extent=40,20 scale=3-7 spacing=2-4 density=0.5 loop=0.1 seed=20260829
```

The generator's name, the version of the format it wrote, and the parameters of the run as
`name=value` tokens. The version is a label a reader matches rather than a number it compares, and
a leading zero says pre-release: the glyphs and the shape are still free to change, no older file is
kept readable, and every level regenerates when they do. `dungen` writes the same version, the
reader accepting one for both formats. A grid row never begins with a letter, so the first token both marks the row as
a header and says which format follows. `dungen` writes a header of the same shape naming its
own parameters.

The grid is `(2W+1)` by `(2H+1)` characters. Cell `(i, j)` sits at `(2i+1, 2j+1)`; the wall west of
it at `(2i, 2j+1)`; the wall south of it at `(2i+1, 2j)`; corners at the even positions in both. The
first line written is the highest row, the origin being the lower left cell, as in the block format.

| position | glyphs                                                       |
| -------- | ------------------------------------------------------------ |
| cell     | space open, `#` solid, `>` stairs down, `<` stairs up         |
| wall, horizontal | `-` wall, `/` door, space no wall                     |
| wall, vertical   | `\|` wall, `/` door, space no wall                    |
| corner   | `+` junction, `-` or `\|` continuing a run, space otherwise    |

Solid shows wherever every cell a position touches is closed, written `#`, so a filled area reads as
a block rather than a lattice. No wall is there, `realize` giving one only to an edge with floor
on a side of it, and the writer asserts as much rather than covering for it. A corner takes the
shape of what reaches it, which keeps a straight wall run unbroken. Corners are decoration --- the
walls either side of one already say everything --- and are not read back.

## The library

**`types.hpp`** --- `coord_type` is `int`. `Cell` is a coordinate, `Extent` a pair of counts,
`Rectangle` a half open cell range. `CellType` is `Solid`, `Open`, `StairsDown` or `StairsUp`;
`WallType` is `Open`, `Solid` or `Door`, where `Open` is the absence of a wall rather than a wall
that happens to be passable. `Direction` is the four orthogonals.

**`Level`** --- a rectangle of cells and the walls between them, and the whole of what a generator
produces. Rooms, doors and stairs are cell and wall state rather than objects of their own. Walls
are stored by the line they lie on, which is the only form that reaches the outermost line of each:
`wall_vertical(i, j)` lies between cells `(i - 1, j)` and `(i, j)`, so `i` runs to `nx` inclusive,
and `wall_horizontal(i, j)` between `(i, j - 1)` and `(i, j)`, so `j` runs to `ny` inclusive.
`wall(cell, direction)` is the convenience over them, either name reaching the one wall two cells
share. A new `Level` has no cell open and every wall solid.

**`Parameters`** --- `extent`, `scale_min` and `scale_max`, `spacing_min` and `spacing_max`,
`density`, `loop`, and `seed`, which is a
`std::optional` and empty until one is assigned. `validate` throws `std::invalid_argument` naming
the parameter at fault, an unassigned seed included, so a caller who never seeded is caught rather
than quietly handed the same level every run. `random_seed()` draws one from `std::random_device`
for a caller wanting a different level each run; `generate` itself takes the seed it is given, so a
written header reproduces its level exactly.

**`generate`** --- the algorithm below. **`write_level`** writes the header and the grid,
**`write_grid`** the grid alone.

## The algorithm

One `math::Random<>` is seeded from `Parameters::seed` and every pass draws from it in a fixed
order, so the whole run is determined by its parameters.

**1. Partition.** The extent is split recursively along its longer side while a side exceeds
`scale_max` and both halves would still reach `scale_min`, the cut falling at a uniformly chosen
position within that range.

Each cut also takes a spacing from the range, and that many cells are left along it closed rather
than taken out of either half, so the gap between two rooms is what was asked for rather than the
sum of two margins. Two leaves that touch do so across the cut of their lowest common ancestor and
no other, so spacing every cut spaces every pair --- at any spacing above zero no two rooms share a
wall, and corridors do all the joining. A side must reach `2 * scale_min + spacing` to be worth
cutting, so a large spacing suppresses splitting and leaves rooms larger than the scale asked for.

At spacing zero, which is the default, the leaves tile the rectangle exactly, no cell is spent on a
cut, and no value is drawn --- a range of one offers no choice --- so a level generated before spacing
existed still comes back from its seed unchanged.

**2. Selection.** At `density` 1.0 every leaf becomes a room. Below it, leaves are taken in a random
order until the area asked for is covered and the rest is left closed. Each room's id is its position
in the room list, and a flat cell to region map holds it. Selection is deliberately not grown from
a seed leaf: scattered rooms are what make a sparse level read as a dungeon rather than as a smaller
complete one, and corridors are what join them.

**3. Corridors.** While two rooms are out of reach of each other, the closest such pair by Manhattan
distance between centres is joined by an L shaped path, turning once, with the leading leg chosen at
random. Only a closed cell along the path is carved; a path crossing an existing room leaves it
alone. Each unbroken run of carved cells becomes a region of its own, so that a path crossing a room
does not leave one region in two disconnected pieces. Components are recounted after each corridor,
so one that happens to join more than two rooms is not followed by a redundant one. A corridor is a
room one cell wide rather than a type of its own.

**4. Realization.** Cells and walls follow from the region map: a cell is floor where it belongs to a
region, and the edge between two cells is open only where both belong to the same one. Everything
else is a solid wall, the outer boundary included.

**5. Doors.** The cell edges with a different region on each side are gathered by the pair they
separate --- that being what makes two regions adjacent, and each edge somewhere a door between them
could go. The pairs are taken in random order and a door is punched at a random edge of each pair
that joins two regions not yet joined, which is a random spanning tree over the adjacency graph and
is what makes every region reachable. Each remaining pair takes a door with probability `loop`.

**6. Stairs.** Stairs down and up at the centres of two different rooms. Corridors are passed over,
being what joins rooms rather than somewhere to arrive.

## The tool

```
catagen [level] [--extent nx,ny] [--scale min-max] [--spacing min-max] [--density d]
             [--loop l] [--seed n] [--quiet]
```

Writes the level to the named file, `catagen.level` by default, and to standard output unless
`--quiet`. `--seed` fills the parameter's own optional, and a run that asked for none takes
`random_seed()` before generating, so the header names a seed either way and any run can be
reproduced from the header it printed. The header's `name=value` tokens are the option names, so a
header reads back as a command line.

`--seed` is read by `math::to_seed` rather than through the whole-number reader the other options
use. A seed spans the whole of its own unsigned range, which is wider than a `long` where a `long`
is 32 bits, so it is read as that type and not as a signed one. `dungen` reads its seed the same
way, the helper being shared.

`dungen` was brought to the same arrangement: its generation parameters are a `Parameters`
struct rather than locals, every one of them is a command line option named after its header token,
the output path is an argument, and the seed is an optional resolved before generating and used to
seed the generator's `math::Random` through `engine().seed()`. Two runs at one seed now produce
identical files, which they could not before. Its level id and name are left as literals, being the
level's identity rather than generation inputs, and the name only feeds a `print` nothing calls.

## Reading it back

`stratum_sim::load_layout` reads both formats. It takes the header if there is one, checks the
version, and dispatches on the generator's name; a file with no header is the block format, which is
what every file written before headers existed is. Of the header's parameters it reads only the
extent, and only for the wall format; the rest are there to reproduce a level rather than to read
one, and an unrecognised token is ignored. An extent the header does not name is derived from the
grid --- the last column is a boundary wall or one of its corners and so is never blank, which means
no row can have been trimmed and the widest is the width. Given one, it is also what catches a
truncated file. Both formats reduce to the same intermediate ---
which cells are open, which of them belong together, where two of those groups meet, and which cells
the format called room floor --- before one group's walls are built, so the boundary tracing and
outline chaining are shared.

For the wall format, cells are joined into one area only across a wall that is absent. A door
therefore separates two of them, which is what leaves it expressible as a
pair of partnered walls the mover may cross. Two rooms sharing a solid wall are partnered along
its whole length and passable nowhere, which is the thing the block format cannot say: a wall with a
room behind it rather than a solid cell.

Walls are matched by the directed boundary edge they lie on rather than by searching an area's
walls for an opposed pair, an index built while the walls are created. Stairs are floor to the
reader until something reads them as more.

The wall format has one open-cell symbol and so defines no rooms: a catagen level loads with none,
where a dungen level's room and corridor symbols build one `Room` per contiguous run of room cells.
Naming rooms in the format is what would close that, and the generator already knows which cells
it laid out as rooms.

## Decisions

**`quetzal::catagen` rather than `quetzal::procgen`.** A prototype and a literal sibling to
`dungen`, both being whole generators. What the two have in common is the better basis for `procgen`
than one generator that did not yet exist. Recorded in `catagen_plan.md`.

**Nothing was copied from `dungen`.** Its vector and `Room` were looked at and left: catagen's
rooms are partition leaves with no door list, and a plain `Cell` covers what the vector was for.
The vector itself has since moved to `math::DiscreteVector` and is no longer dungen's to copy.

**The grid and nothing else.** No `Room`, `Door` or `Stair` records. Rectangular rooms and regular
spacing only.

**`>` is down and `<` is up**, matching dungen's `>` for Stair and the roguelike convention, so both
formats share the cell vocabulary.

**The seed is a parameter, not something generation invents.** A caller who wants a different level
each run asks for one, and resolving it is the caller's job rather than the library's --- which keeps
`generate` const in its parameters and pure in its seed. This is what makes the header enough to
reproduce a level, and `dungen` now does the same.

**Unassigned is `std::optional`, not a reserved value.** Every seed is a seed, zero included, and
"none yet" is said by the type rather than by spending a value on it. `validate` checks that the
optional is engaged, so nothing needs a special case for the marker.

**Loop adds doors, never corridors.** A loop is a second way between two rooms already adjacent;
carving another corridor would change the layout rather than its connectivity.

## Known limits

- **The ASCII view cannot show a wall between two cells.** `stratum_view::render` maps a cell to a
  glyph, so a catagen level renders as floor and solid with its walls invisible and a mover stopped
  by nothing it can see. The wall-aware view on a 2N+1 grid is stage 5 of `work_plan.md`.
- **A scale range narrower than twice its minimum leaves rooms above the maximum**, there being no
  cut that both halves would survive. `scale_max` at or above `2 * scale_min - 1` avoids it.
- **One character per edge can only describe a symmetric one.** A wall is per side, and the reader
  builds a partnered pair from the single glyph, giving both the same passability. A door open from
  one side only, or secret from one side, has no way to be written --- that difference wants glyphs
  the format does not have.
- **Stairs go nowhere.** They are floor to the reader, there being nothing yet for them to reach.
  It is the one thing a round trip loses: `stratum_view_test` renders a wall format level back to
  its own file, and compares against that file with `<` and `>` taken as open, because floor is all
  the view has to draw. Everything else --- walls, doors, corners, and where solid shows --- is compared
  exactly.

## Tests

`test/stratum_sim_test` gains a `load wall layout` group over two files in its `data`: `walls.level`,
two rooms of two by three cells sharing one wall with a door in the middle of it, checked wall by
wall; and `catagen.level`, a generated level checked for solid cells, mutual connections, and every open
cell being reachable from the first across passable walls alone.
