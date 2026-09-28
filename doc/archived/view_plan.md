# The View Axis --- Plan

Status: complete, as stage 5 of `work_plan.md`
Date: 2026-09-02, revised 2026-09-22

This plan adds a wall-aware view. `cells(world, zone)` reduces a zone to one `Place` per grid cell, so a wall or a door between two
open cells reduces to `Floor` on both sides. That loses nothing for a block notation zone, where a wall exists only
against a solid cell, and makes a wall notation zone render as open ground an actor cannot cross. `game_system_reference.md`
describes what is built.

## The shape

**A second reduction beside `cells`.** `cells` remains one place per grid cell, for the cell view and the tile view.

```cpp
    // The zone reduced onto the lattice of the wall notation: cells and what lies on the edges between them,
    // (2W+1) by (2H+1) for a W by H grid
    Buffer<Place> lattice(const stratum_sim::World& world, flecs::entity zone);
```

Cell `(i, j)` is at `(2i+1, 2j+1)`, the wall west of it at `(2i, 2j+1)`, the wall south of it at `(2i+1, 2j)`, and
corners at the even positions in both --- the arithmetic of the file format, with the origin at the lower left.

**`PlaceType` gains `Wall` and `Door`.** One type over both reductions keeps the buffer one type. `lattice` produces
`Floor` at cell positions, `Wall` and `Door` at the others, and `Solid` wherever every cell the position touches is
closed. `None` is an edge or corner with an open cell beside it and nothing on it.

## Corners

A corner is structure. Where wall runs meet there is a joint, and a first-person view covers it --- a column, a post, a
pilaster, as Grimrock does. So the reduction produces the corner: `Wall` where any of its four edges has a wall or a
door. What is made of it --- a `+` in the terminal, a column in 3D --- is the view's, with the four edges beside it in
the buffer. The corner is derived from the walls, so the file's corner glyphs are not read back.

## Glyphs, and a round trip

The wall view's glyphs are the wall notation's own, so rendering a wall notation zone with no actors reproduces the file
it was loaded from.

| position | glyph |
| --- | --- |
| cell | space `Floor`, `@` an actor on one, `#` `Solid` |
| wall, horizontal | `-` `Wall`, `/` `Door` |
| wall, vertical | `\|` `Wall`, `/` `Door` |
| corner | `+`, `-` or `\|` for `Wall`, from the four edges |

A position is solid when every cell it touches is closed --- a cell touches itself, an edge the two it lies between, a
corner the four meeting at it. A wall faces open floor, so a position with one always touches an open cell; the writer
and the view assert it.

Open is a space here and `.` in the cell view, and `Solid` is `#` in both.

## The reference frames

A fourth frame plays a wall notation zone through the wall view, beside the three that check the cell view. The frames
are played in process against the two libraries, so which view the host uses does not affect them.

## The host

The terminal host moves to the wall view, which shows everything the cell view does and what lies between cells. A
31 by 31 zone becomes 63 by 63 characters, which the host's window sizing handles.

## Steps

- [x] **1. `PlaceType` gains `Wall` and `Door`, and `lattice` produces them,** tested against both notations without
      rendering.
- [x] **2. The ASCII mapping**: `lattice_glyphs` and `render_lattice`, with the round trip against a wall notation zone.
- [x] **3. The fourth reference frame**, `walls_door`: through the door, back, down a row, turn toward the wall the two
      rooms share, be stopped by it, and interact with it, compared byte for byte.
- [x] **4. The terminal host moves to it**, each edge asked of both of its sides.
- [x] **5. No `View` interface** (`decision_rationale.md`, "Presentation is a reduction and a mapping, with no view
      interface").

## Open

The open items are tracked in `work_plan.md`.
