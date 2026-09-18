# catagen --- Reference

What exists, as built. `catagen_plan.md` holds the intent.

`catagen` is a wall based dungeon generator: a wall is a property of the edge between two cells
rather than a cell of its own, so rooms share walls and a partition of a rectangle leaves no cell
solid.
`dungen` remains the block oriented generator alongside it. Neither shares code with the other.

- `library/quetzal/catagen` --- the generator
- `tool/catagen` --- the driver
- `library/quetzal/stratum_sim/load_layout.cpp` --- reads a level file into a `Layout`
- `assets/level/catagen.level` --- a generated level `stratum_terminal` loads

## The level file

A discrete layout file in the wall notation, written through `stratum_format`; `discrete_layout.md`
is the format. `generator.parameters` has the parameters of the run as the tokens its options take ---
`extent`, `scale`, `spacing`, `density`, `loop`, `stairs_down`, `stairs_up` and `seed` --- so a file
reproduces its own level and its parameters read as a command line.

The rows are `(2W+1)` by `(2H+1)` characters. Cell `(i, j)` sits at `(2i+1, 2j+1)`; the edge west of
it at `(2i, 2j+1)`; the edge south of it at `(2i+1, 2j)`; corners at the even positions in both. The
first row written is the highest, the origin being the lower left cell.

| position | glyphs                                                            |
| -------- | ----------------------------------------------------------------- |
| cell     | `.` floor, space no floor, `>` staircase down, `<` staircase up    |
| edge, horizontal | `-` wall, `+` door, space no wall                          |
| edge, vertical   | `\|` wall, `+` door, space no wall                         |
| corner   | space                                                             |

An edge between two solid cells has no wall, `realize` giving one only to an edge with floor on a side
of it. A stair cell is a block: the rows write a wall on every edge between one and floor, even where
the level's wall is open, and none on its other edges. catagen writes no rooms, though generation knows which cells it laid out as rooms and the format
has a section for them.

Each door has an `attachments` entry at the walls `stratum_format::door_walls` finds for its glyph, the
two sides of its edge, a `door` entity. Each staircase has an `attachments` entry at its cell, a `staircase` entity whose `facing` is the side
of the cell it opens onto, always floor or a door and whose `relocator` has the `target` `level_down` or `level_up`.
`layout.components.entry` lists the cells of every staircase, each being an entry.

## The library

**`types.hpp`** --- `coord_type` is `int`. `Cell` is a coordinate, `Extent` a pair of counts,
`Rectangle` a half open cell range. `CellType` is `Solid`, `Open`, `StairsDown` or `StairsUp`;
`WallType` is `Open`, `Solid` or `Door`, where `Open` is the absence of a wall rather than a wall
that happens to be passable. `Direction` is the four orthogonals.

**`Level`** --- a rectangle of cells and the walls between them, and the whole of what a generator
produces. Rooms, doors and staircases are cell and wall state rather than objects of their own, with
a staircase's facing kept beside the cells by `staircase_facing` and `set_staircase_facing`. Walls
are stored by the line they lie on, which is the only form that reaches the outermost line of each:
`wall_vertical(i, j)` lies between cells `(i - 1, j)` and `(i, j)`, so `i` runs to `nx` inclusive,
and `wall_horizontal(i, j)` between `(i, j - 1)` and `(i, j)`, so `j` runs to `ny` inclusive.
`wall(cell, direction)` is the convenience over them, either name reaching the one wall two cells
share. A new `Level` has no cell open and every wall solid.

**`Parameters`** --- `extent`, `scale_min` and `scale_max`, `spacing_min` and `spacing_max`,
`density`, `loop`, `staircase_down_count` and `staircase_up_count`, closed ranges of `size_t` that
default to 1-1, and `seed`, which is a
`std::optional` and empty until one is assigned. `validate` rejects a staircase count whose maximum is
below its minimum, and counts whose minimums are both zero, every staircase being an entry and a level
needing one. `validate` throws `std::invalid_argument` naming
the parameter at fault, an unassigned seed included, so a caller who never seeded is caught rather
than quietly handed the same level every run. `random_seed()` draws one from `std::random_device`
for a caller wanting a different level each run; `generate` itself takes the seed it is given, so
written parameters reproduce their level exactly.

**`generate`** --- the algorithm below. **`write_level`** writes the level file, **`write_grid`** its
rows alone.

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
distance between centers is joined by an L shaped path, turning once, with the leading leg chosen at
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

**6. Staircases.** A count leading down is drawn from `staircase_down_count` and a count leading up
from `staircase_up_count`, a range of one drawing nothing, and each staircase takes the center of a
room chosen at random from those not yet holding one, those leading up first. Corridors are passed
over, being what joins rooms rather than somewhere to arrive, and so is a room whose center would cut a
passage, the cells across its passable sides no longer reaching one another around it, or leave it or a
staircase beside it no passable side to face, a door counting as passable. Running out of rooms places
fewer, those leading down going short first. A level with no room to place one in is an error:
`generate` throws `std::runtime_error`.

**7. Staircases off doorways.** A staircase with a door on a side of its cell moves one cell, in a
random direction across an open wall onto floor with no door on any side that takes a block as step 6
allows one, and stays where there is no such direction. An open wall joins two cells of one region, so
it stays in its own space.

**8. Facing.** Each staircase's facing is drawn from the sides of its cell with an open wall and floor
beyond, or where there are none, from those with a door. These last three passes draw after every other, so nothing
before them changes.

## The tool

```
catagen [level] [--extent nx,ny] [--scale min-max] [--spacing min-max] [--density d]
             [--loop l] [--stairs-down min-max] [--stairs-up min-max] [--seed n] [--quiet]
```

Writes the level to the named file, `catagen.level` by default, and to standard output unless
`--quiet`. `--seed` fills the parameter's own optional, and a run that asked for none takes
`random_seed()` before generating, so the file names a seed either way and any run can be
reproduced from what it printed. The parameter names are the option names, so a file's parameters
read back as a command line. `--stairs-down` and `--stairs-up` take a closed range of counts; an option
keeps the shorter "stairs" to type, where names in code and prose say "staircase".

`--seed` is read by `math::to_seed` rather than through the whole-number reader the other options
use. A seed spans the whole of its own unsigned range, which is wider than a `long` where a `long`
is 32 bits, so it is read as that type and not as a signed one. `dungen` reads its seed the same
way, the helper being shared.

`dungen` was brought to the same arrangement: its generation parameters are a `Parameters`
struct rather than locals, every one of them is a command line option named after its parameter,
the output path is an argument, and the seed is an optional resolved before generating and used to
seed the generator's `math::Random` through `engine().seed()`. Two runs at one seed now produce
identical files, which they could not before. Its level id and name are left as literals, being the
level's identity rather than generation inputs, and the name only feeds a `print` nothing calls.

## Reading it back

`stratum_sim::load_layout` reads the file through `stratum_format`, which `game_system_reference.md`
describes. The parameters are there to reproduce a level rather than to read one.

A door is both sides of one edge, and two rooms sharing a solid wall are walled from both sides along
its whole length and passable nowhere, which is the thing the block notation cannot say: a wall with a
room behind it rather than a solid cell. A stair cell is a block to the loader, closed like solid, and a
catagen level loads with no rooms.

## Decisions

**`quetzal::catagen` rather than `quetzal::procgen`.** A prototype and a literal sibling to
`dungen`, both being whole generators. What the two have in common is the better basis for `procgen`
than one generator that did not yet exist. Recorded in `catagen_plan.md`.

**Nothing was copied from `dungen`.** Its vector and `Room` were looked at and left: catagen's
rooms are partition leaves with no door list, and a plain `Cell` covers what the vector was for.
The vector itself has since moved to `math::DiscreteVector` and is no longer dungen's to copy.

**The grid, and a facing per staircase.** No `Room`, `Door` or staircase records. The one thing a
cell and its walls cannot hold is which side a staircase opens onto, so `Level` keeps that beside the
cells, keyed by the staircase's cell, rather than growing a record for it. Rectangular rooms and
regular spacing only.

**`>` is down and `<` is up**, matching dungen and the roguelike convention, so both notations share
the cell vocabulary.

**The seed is a parameter, not something generation invents.** A caller who wants a different level
each run asks for one, and resolving it is the caller's job rather than the library's --- which keeps
`generate` const in its parameters and pure in its seed. This is what makes the written parameters
enough to reproduce a level, and `dungen` now does the same.

**Unassigned is `std::optional`, not a reserved value.** Every seed is a seed, zero included, and
"none yet" is said by the type rather than by spending a value on it. `validate` checks that the
optional is engaged, so nothing needs a special case for the marker.

**Loop adds doors, never corridors.** A loop is a second way between two rooms already adjacent;
carving another corridor would change the layout rather than its connectivity.

## Known limits

- **A scale range narrower than twice its minimum leaves rooms above the maximum**, there being no
  cut that both halves would survive. `scale_max` at or above `2 * scale_min - 1` avoids it.
- **One character per edge can only describe a symmetric one.** A wall is per side, and the loader
  builds a wall on each side from the single glyph. A door open from one side only, or secret from one
  side, is components on the attachment rather than anything the rows say.
- **Staircases go nowhere.** They are blocks to the loader, there being nothing yet to let an actor
  through the open side or for them to reach. `stratum_view_test` renders a wall notation level back to
  its own rows with `<` and `>` taken as blank, because solid is all the view has to draw there.

## Tests

`test/catagen_test` checks that what the generator writes now is equivalent to its reference levels,
regenerates each from its own parameters, and checks every level it generates is one the reader
accepts. Over a sweep of seeds with ranges of staircases each way, it checks from the written level
that the counts keep to their ranges, that every floor cell is reachable from every other, that every
edge between a stair cell and floor is a wall, that no staircase has a door beside it while a free cell
does not, and that every staircase has one facing that follows the rule, which sides of a stair cell were
open coming from the level generated again. `assets/level/catagen.level`
is the `shipped` reference level. `test/stratum_sim_test` has a `load wall layout` group over two files in its `data`:
`walls.level`, two rooms of two by three cells sharing one wall with a door in the middle of it; and
`catagen.level`, a generated level checked for solid cells, edges walled from both sides counting in
pairs, and every open cell being reachable from the first across passable walls alone.
