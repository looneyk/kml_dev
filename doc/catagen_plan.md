# catagen --- Plan

Status: complete
Date: 2026-08-29

A second dungeon generator, sibling to `dungen`. `dungen` fills a rectangle with block oriented
levels, where a wall costs a whole cell. `catagen` generates the wall based form the `Layout`
already models --- a wall being a property of the edge between two cells --- so that the stratum
applications have thin-wall input. `game_system_principles.md` states that the thin-wall format
waits until something emits one; this is that something.

A prototype: `library/quetzal/catagen` with `tool/catagen` driving it, both wired into
the root `CMakeLists.txt`. Classes are copied from `dungen` where they are useful rather than
shared, so that neither generator constrains the other while both are being learned from.
Generalizing what they have in common into `quetzal::procgen` comes later.

`catagen_reference.md` describes what exists, the format included.

## What it produces

Every cell may be floor. Two adjacent floor cells are separated by a wall, or they are not, and
nothing is spent on the separation. Rooms therefore share walls, so a partition of the rectangle
leaves no rock at all --- the case the block format cannot express --- while sparse filling leaves rock
where the block format would have to anyway.

The grid and nothing else. No `Room`, `Door` or `Stair` records; rectangular rooms and regular
spacing only.

Parameters: the **extent** to fill, the **scale** a room aims for as a range, the **spacing** left
between rooms as a range, the **density** of the filling, a **loop** probability for doors beyond
the ones connectivity needs, and a **seed**. Spacing at zero and density at 1.0 together are what
leave no rock at all.

## Passes

1. **Partition** the extent into rooms.
2. **Select** which of them survive, the rest becoming rock.
3. **Connect** what that separated, with corridors.
4. **Realize** cells and walls from what the first three decided.
5. **Doors** enough for every room to be reachable, and `loop` more.

The first two are the algorithm and the rest are not. A second algorithm --- the roguelike pattern, or
anything else --- replaces them and leaves the rest intact.

## Stages

- [x] **1. Header row in `dungen`,** so that both generators emit the same shape.
- [x] **2. Types and the writer.** Extent, parameters, the cell and wall storage, and the file
      writer.
- [x] **3. Partition and complete filling.**
- [x] **4. Connection.** Spanning tree, doors, and the loop parameter.
- [x] **5. Sparse filling.** The selection pass and corridor carving.
- [x] **6. A reader in `stratum_sim`,** taught both the header and the thin-wall grid, and a
      generated level under `assets/level` that `stratum_terminal` loads.

## Next

Not part of this, and named where they belong:

- **The wall-aware ASCII view**, stage 5 of `work_plan.md`. Until it exists, a catagen level renders
  with its walls invisible, which is the one thing about the format a host cannot yet see.
- **Somewhere for the stairs to reach**, they being floor to the reader.
- **A second algorithm**, and with it whatever the two have in common becoming `quetzal::procgen`.
