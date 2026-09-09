# Level Loader Structures --- Plan

Status: not started
Date: 2026-09-08

The structures `load_layout.hpp` declares in its `detail` namespace, which both format readers
produce and which building a `DiscreteLayout` consumes. `dungen_plan.md` and `catagen_plan.md` cover
the generators that write the two formats; what is built goes to `game_system_reference.md`.

Every type name below beginning `New` is a placeholder, standing in until the thing it names is
worth naming. `Areas` is not: area is already the word the loader's own comments use for a run of
cells nothing separates, and component is the word flecs entities are built from.

## What and why

`Map` does two jobs. It addresses a grid of cells, and it is a reader's output. `create_rooms` wants
only the first, so it builds a second `Map` whose `cell_open`, `cell_room`, `passages` and `blocks`
stay empty --- four members that mean nothing in that use, and a reader of the code cannot tell
which of the eight are live without following the call.

Its per-cell data is three `std::vector`s addressed by the same slot: `cell_open`, `cell_components`
and `cell_room`. Keeping three containers the same length and in the same order is the caller's job,
enforced nowhere, and it is why `Map::slot` is called three times over where one lookup would do.

Component is also the wrong word for what `find_components` finds. It is the word flecs entities are
built from --- `Intent` and `placement_type` are components --- while what the flood fill returns is
a run of cells nothing separates, which the loader's comments already call an area throughout. Area
is what the structures below use.

Both formats state which cells are room floor. A cell is what a file says about one square, so
`open` and `room` are two fields of one thing rather than two arrays that happen to be indexed
alike.

Nothing about behavior changes. Every case in `stratum_sim_test` passes as it did and the reference
frames in `stratum_view_test` stay byte for byte what they were.

## The structures

- **`NewCell`** is what a file states about one square: `open`, whether a mover may stand there, and
  `room`, whether it is room floor rather than corridor, doorway or anything else open. Stated
  properties only.
- **`NewGrid`** is an extent and a `NewCell` per square, with the addressing over it: `slot`,
  `cell_count`, `cell(grid_position)` and `contains`. It knows no format and holds no result.
- **`Areas`** is one partition of a grid into runs of cells nothing separates: `cell_areas`, the area
  each square belongs to, and `area_cells`, the squares of each. A grid can carry as many partitions
  as are asked for.
- **`Map`** keeps its name and its job as a reader's output: a `NewGrid`, its `Areas`, and the
  `passages` and `blocks` a format states between and within them.

### Why a partition is a value rather than a field of a cell

`find_areas` runs twice over the same squares: once joining open cells, once joining room-floor
cells. Those are two partitions of one grid, and a single area field per cell holds one of them.
Returning a partition lets both stand, and lets a format that states a third have one without
changing the cell.

That is also what removes the second `Map`. `create_rooms` runs `find_areas` over the same grid it
was given, selecting `room` where the first run selected `open`.

## Where each member of today's `Map` goes

| Goes on | Members |
|---|---|
| `NewCell` | `cell_open`, `cell_room` |
| `NewGrid` | `extent`, the cells, `slot`, `cell_count` |
| `Areas` | `cell_components`, `component_cells` |
| `Map` | `passages`, `blocks`, and the grid and partition themselves |

## What the functions become

`find_areas` takes a grid, a selector naming which stated property counts as open, and the `joined`
predicate deciding whether two adjacent squares belong together. It returns an `Areas` rather than
writing into its argument.

    Areas find_areas(const NewGrid& grid, Select select, Joined joined);

The two calls that exist become a selection: the first selects `open`, the second `room`. Both
readers keep their own `joined` --- catagen's asks what lies on the edge between two squares,
dungen's is always true, nothing separating two open cells in that format.

`trace_outlines` takes the grid and a partition rather than a `Map`, which is all it reads today.
`build` and `create_rooms` keep their shape and reach the same data through the grid.

## Order of work

Each step builds warning-free and leaves both suites passing, the 396 cases of `stratum_sim_test`
and the 96 of `stratum_view_test`, with its five reference frame files byte for byte equal.

1. **Component becomes area** wherever the loader means a run of cells: `find_components`,
   `cell_components`, `component_cells`, the local names in `trace_outlines` and `build`, and the
   comments. A rename and nothing else.
2. **`NewGrid` holding the extent and the addressing.** `Map` holds one; `slot` and `cell_count`
   move onto it. The three vectors stay as they are and stay on `Map`.
3. **`NewCell`.** `cell_open` and `cell_room` fold into the grid's cell vector. `cell_areas` stays
   on `Map`.
4. **`Areas`.** `find_areas` returns one and `trace_outlines` takes one.
5. **`create_rooms` runs over the grid it was given**, selecting `room`. The second `Map` goes.

Rooms in the wall format are separate work, unblocked by all of the above: once catagen states them,
its reader fills `room` as dungen's does and nothing else changes.

## Names to avoid

`stratum_sim::Cell` is already one square of a `Zone`'s grid as the layout records it, `open` and
`room_id`. It is the same idea one stage further on --- what the layout holds rather than what the
file stated --- so the loader's cell needs a name that does not read as that one. `GridMapping` is
grid position to world position and is not what `NewGrid` is.

`Components` is a file-local class in both `catagen/generate.cpp` and `dungen/generate_grid1.cpp`.
Neither is reachable from here and both find what this one finds, so whichever word wins should
reach them eventually.
