# catagen --- Plan

Status: complete
Date: 2026-08-29

`catagen` is a second zone generator beside `dungen`. `dungen` fills a rectangle with block oriented zones, where a wall takes a
whole cell. `catagen` generates the wall based form, a wall being a property of the edge between two cells, so the
stratum applications have wall notation input.

It is `library/quetzal/catagen`, driven by `tool/catagen`, both in the root `CMakeLists.txt`. Neither generator shares
code with the other. What they have in common becomes `quetzal::procgen` later.

`catagen_reference.md` describes what exists.

## What it produces

Every cell may be floor. Two adjacent floor cells are separated by a wall or not, and the separation takes no cell.
Rooms therefore share walls, so a partition of the extent has no solid cell --- which the block notation cannot
express --- while sparse filling has solid cells as the block notation would.

It produces the grid alone, with no room, door or staircase records, and only rectangular rooms and regular spacing.

Parameters: the **extent** to fill, the **scale** a room aims for as a range, the **spacing** between rooms as a range,
the **density** of the filling, a **loop** probability for doors beyond the ones connectivity needs, and a **seed**.
Spacing at zero and density at 1.0 together leave no solid cell.

## Passes

1. **Partition** the extent into rooms.
2. **Select** which of them remain, the rest becoming solid.
3. **Connect** the rooms selection separated, with corridors.
4. **Realize** cells and walls from the first three.
5. **Doors**, enough for every room to be reachable, and `loop` more.

The first two are the algorithm. A second algorithm replaces them and keeps the rest.

## Stages

- [x] **1. Header row in `dungen`,** so both generators emit the same form.
- [x] **2. Types and the writer.** Extent, parameters, the cell and wall storage, and the file writer.
- [x] **3. Partition and complete filling.**
- [x] **4. Connection.** Spanning tree, doors, and the loop parameter.
- [x] **5. Sparse filling.** The selection pass and corridor carving.
- [x] **6. A reader in `stratum_sim`,** for the header and the wall notation grid, and a generated zone under
      `assets/level` that `stratum_terminal` loads.

## Next

These are not part of this plan, and are listed where they belong:

- **Somewhere for the staircases to lead.**
- **A second algorithm**, and with it what the two generators have in common becoming `quetzal::procgen`.
