# The View Axis --- Plan

Status: complete. Stage 5 of `work_plan.md` is done
Date: 2026-09-02

Detail for stage 5 of `work_plan.md`, as `layout_plan.md` held stage 4. What is built goes to
`game_system_reference.md`. `Interact` asking the layout what is in front was the third item and is
already done.

## What and why

`cells(world, zone)` reduces a zone to one `Place` per grid position. What lies on the edges
between cells is not in it, so a wall or a door between two open cells reduces to `Floor` on both
sides. For a dungen level that loses nothing --- in the block format a wall exists only where open
space meets a closed cell, and a closed cell has a position of its own. For a catagen level it loses
the level: it emits walls and doors between open cells, so it renders as open ground a mover cannot
cross, with the reason on screen nowhere.

So this is not validation of a design any more. A catagen level is unplayable without it, and the
per-edge wall model --- the one `sim_zone_model.md` calls settled and the loader builds from both
formats --- is checked by no reference frame at all.

## The shape

**A second reduction beside `cells`, not a replacement.** `cells` is honest about being one cell per
grid position, both current views consume it, and the Godot tile view has no use for edges yet. The
new one is the third consumer the `View` interface is meant to be derived from, so it exists alongside
the other two rather than absorbing one.

```cpp
    // The zone reduced onto the lattice the wall format writes: cells and what lies on the
    // edges between them. (2W+1) by (2H+1) for a W by H grid
    Buffer<Place> lattice(const stratum_sim::World& world, flecs::entity zone);
```

Cell `(i, j)` is at `(2i+1, 2j+1)`, the wall west of it at `(2i, 2j+1)`, the wall south of it at
`(2i+1, 2j)`, and corners at the even positions in both --- the same arithmetic
`catagen_reference.md` states for the file format, and the same origin at the lower left.

`lattice` is the name for the shape rather than the content, cells and edges having no shared word.
`surfaces` and `structure` were the alternatives; neither conveys what the buffer is indexed by.

**`PlaceType` grows by exactly two.** One enum over both reductions keeps the buffer one type and a
consumer's switch in one place. `lattice` emits `Floor` at cell positions and `Wall` and
`Door` at the others, and `Solid` wherever every cell the position touches is closed.

The two absences are `Solid` and `None`, and one rule separates them. `Solid` is where every cell
the position touches is closed, which covers three of the four places an absence can appear --- a
closed cell, an edge between two closed cells, a corner with four around it. `None` is the fourth,
an edge or corner with an open cell beside it and nothing on the position.

## Doors, before there are doors

A door in a catagen level loads as a partnered pair of walls the loader set passable. There is no
door record and no attachment mechanism --- stage 6 owns both. So the reduction distinguishes `Door`
from `Wall` by `Wall::passable`, the flag `decision_rationale.md` expects to come off the record when
a door is an attachment.

That is the right answer twice over: it is what the data encodes today, and when the flag goes the
reduction is the one place that has to change. Recording it here so that the dependency is stated
rather than discovered.

## Corners

A corner is structure, not decoration. Where wall runs meet at an angle there is a joint, and a
first-person view puts something there to cover it --- a column, a post, a pilaster; Grimrock does
exactly this. So the reduction emits a corner rather than leaving it blank for the ASCII mapping
to invent.

It emits `Wall` where any of the four edges touching it has a wall or a door on it, and an absence
otherwise. What is *made* of that --- a `+` in the terminal, a column in 3D, nothing where a run
passes straight through --- is the view's, and every view has what it needs to decide: the four edges
around the corner are in the same buffer, adjacent to it.

That is a derivation rather than a second store, and it is why `catagen_reference.md` can state that a
corner is not read back: the walls determine it, so the file's corner glyphs are a convenience for a
human reading the file. Significant to a view and redundant to the model are not in conflict.

## Glyphs, and a round trip worth having

The wall view's default glyphs should be catagen's own, so that rendering a catagen level reproduces
the file it was loaded from. The block format already has that check --- "corridor renders back to its
own file" in `stratum_view_test` --- and the wall format has none.

| position | glyph |
| --- | --- |
| cell | space `Floor`, `@` an actor on one, `#` `Solid` |
| wall, horizontal | `-` `Wall`, `/` `Door` |
| wall, vertical | `\|` `Wall`, `/` `Door` |
| corner | `+`, `-` or `\|` for `Wall`, shaped from the four edges |

`Solid` is the one that needs its neighbours, and it is one rule rather than a case per position: a
position is solid when every cell it touches is closed --- a cell touches itself, an edge the two it
lies between, a corner the four that meet at it. Everywhere else it is an opening. The mapping is
already reading neighbours to shape a corner, and this is the same look.

It never competes with a wall: **a wall faces open floor on the side it belongs to**, so a position
with one always touches an open cell. `load_layout` traces the outline of every open component,
so an edge between two closed cells has no wall at all, and `catagen`'s `realize` assigns a wall
only to an edge with floor on a side of it. Both the writer and the view assert it rather than
handling it.

One condition the round trip does need: the level must have no actors, as the existing round-trip cases
already arrange.

Note that open is a space here and `.` in the cell view, and that `Solid` is `#` in both. The two
views are not the same picture at a different scale and should not pretend to be: this one is the
file, and that is what makes it checkable.

## The reference frames

**Add rather than move.** The three existing frames check `render` and the command sequences behind
it at one character per cell; that is still worth checking, and moving them would trade one
covered view for another. A fourth frame plays a catagen level through the wall view, which is the
combination with no coverage at all today.

The frames are played in process against the two libraries rather than through the host, so which
view the host uses does not touch them.

## The host

The terminal host moves to the wall view. It is strictly more informative --- everything the cell view
shows plus what lies between cells --- and the cell view stays for the tile view and the frames.

The consequence to expect is size: a 31 by 31 dungeon becomes 63 by 63 characters. The host already
grows its window on the first frame that would not otherwise fit and reports the size needed when it
cannot, so this is a risk to watch rather than work to do.

A `--cells` flag to see the old view is an option, not a plan.

## The View interface

**There is none to derive, and the third consumer is what settles it.**

What the three share is a two-stage convention, and it is already explicit in the code: a
*reduction* takes a world and a zone to a `Buffer<Place>` --- `cells` and `lattice` --- and a *mapping*
takes that to a `Buffer<T>` a device can emit, which is `render` and `render_lattice`. The Godot
tile view consumes a reduction and stops; the terminal consumes a mapping.

Nothing dispatches. Neither host owns a view and asks it to draw --- each is written against the one
it needs --- so an interface would be a vtable with no call site. What differs between the three is
the payload type and the shape of the buffer, and those are a template parameter and a size, not
something a virtual call resolves.

The duplication that was real got removed rather than abstracted: both reductions walked the zone's
entities to place actors, which is now one `each_actor_cell`.

What would earn an interface is a consumer whose output is not a buffer at all. That is the
first-person 3D view of Phase 3b, which produces mesh instances, and it is the case that tests
whether `Buffer<T>` is the shared shape or just the shape the two flat views happen to have. Asking
before then would be deriving it from two examples of the same shape.

## Steps

- [x] **1. `PlaceType` gains `Wall` and `Door`, and `lattice` produces them.** The reduction, its
      arithmetic, and the cell, edge and corner cases, tested against both formats without any
      rendering. `Glyphs` gains the two new types and maps neither, `cells` having nowhere to put
      an edge --- the wall-aware mapping is step 2.
- [x] **2. The ASCII mapping.** `Glyphs` gained the wall set, an `opening` distinct from `floor`,
      and `lattice_glyphs` as the set the wall format writes. `render_lattice` rather than an
      overload: `Cells` and `Lattice` are the same type. Solid is one rule over all three positions
      rather than a case for each. The round trip against a wall format level passes byte for byte,
      so the corner shaping and where solid shows agree with the generator without either depending
      on the other.
- [x] **3. The fourth reference frame.** `walls_door` plays a wall format level through the wall
      view: through the door, back, down a row, turn to face the wall the two rooms share, be
      stopped by it, and interact with it. The block reports the same wall the interact does, and it
      is drawn beside the actor --- which is the whole of what the cell view could
      not show, compared byte for byte. A scenario selects which view it is played through, the two
      being checked separately.
- [x] **4. The terminal host moves to it.** Which found a real bug: the reduction asked one side
      of each edge, and a wall exists only on the side facing open space, so every wall with its
      closed cell to the east or north went undrawn. A catagen level rendered with half its rooms
      unoutlined while a mover was stopped by walls that were not there. Both sides are asked now,
      and the case that would have caught it is in `stratum_view_test`.
- [x] **5. There is no `View` interface to derive.** Recorded above.

Steps 1 and 2 are one change split for review. Step 5 is independent of the host.

## Open

Tracked in `work_plan.md`, which collects the open items of every plan.
