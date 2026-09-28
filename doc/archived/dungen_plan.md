# dungen --- Plan

Status: complete
Date: 2026-09-04

Move `dungen`'s generation out of `tool/dungen/main.cpp` into the library, one file per layout, behind a shared
`Parameters` and a `generate` that dispatches on the layout asked for, and add two more layouts. The form follows
`catagen`'s: `types` / `Parameters` / `Zone` / `generate` / `write_zone`.

`dungen_reference.md` describes what exists.

## The three layouts

| Name | Layout |
| --- | --- |
| `maze1` | Rooms dropped at random until the density is met, a maze flooded through every cell they left, doors placed in the rooms' walls, then dead ends and loops pruned back. |
| `grid1` | Rogue's. A coarse grid of sectors, a room inside most of them, orthogonally adjacent sectors joined by corridors that bend once in the solid cells between. Sparse rooms, solid surroundings, short direct corridors. |
| `cave1` | Noise smoothed into caverns by a cellular automaton, the largest connected area kept and the rest filled back in. No rooms and no doors --- open floor and solid cells. |

The names are the `--algorithm` values, the `Algorithm` enumerators (`Maze1`, `Grid1`, `Cave1`), and the suffix of the
file each is in.

## Files

In `library/quetzal/dungen`:

- `Algorithm` --- the enumeration and its spelling.
- `Parameters` --- the shared parameters, one nested group per algorithm, `random_seed` and `validate`.
- `generate` --- dispatch on `parameters.algorithm`.
- `generate_maze1`, `generate_grid1`, `generate_cave1` --- one layout each, sharing nothing beyond what is listed here.
- `place_staircases` --- turning candidate cells into staircases, each algorithm supplying the candidates its layout
  allows.
- `write_zone` --- the zone file.

`Cell`, `Cells`, `CellType`, `CellTypeProperties`, `Door`, `Zone`, `Pattern`, `Room`, `Staircase`, `Wall` and `types`
are unchanged.

Each algorithm reads the shared parameters and its own group, seeds its own `math::Random` from `parameters.seed`, and
is deterministic in its parameters.

## The file format

Unchanged. The grid glyphs keep their meaning, so the loader reads all three layouts. The generator parameters gain
`algorithm` and have only the chosen algorithm's group.

## Command line

`dungen` keeps every option, each setting the group it belongs to, and gains `--algorithm maze1|grid1|cave1`. Options of
an algorithm not chosen are read and ignored, so an option may be given before the algorithm it belongs to.

## Stages

- [x] **1. Enumerations and parameters.** `Algorithm`, `Parameters`, `write_zone`, and `generate` dispatching to three
      skeletons.
- [x] **2. `maze1` moved** out of `main.cpp` into `generate_maze1.cpp`, producing the same output, with staircase
      placement split out. `main.cpp` has the command line and file I/O only.
- [x] **3. `grid1`.**
- [x] **4. `cave1`.**
- [x] **5. `dungen_reference.md`.**
