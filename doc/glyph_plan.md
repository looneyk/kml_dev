# Glyph mapping --- Plan

Status: complete
Date: 2026-09-03

Detail for the glyph mapping item of stage 6 in `work_plan.md`. What is built goes to
`game_system_reference.md`.

The reduction currently produces one `PlaceType` per position, and `PlaceType` gained `DoorClosed`
and `DoorOpen` so that a shut door could be drawn apart from an open one. That does not scale: a
locked, concealed or broken door multiplies the enum rather than adding to it, and every other
attachment would do the same again. State belongs beside the type, not inside it.

The mapping to characters is hardcoded twice over --- a struct of named glyphs, and a chain of
conditions in `render_lattice` choosing between them. A glyph set that can express more than ASCII can,
or one that produces tile indices rather than characters, has nowhere to be expressed.

## What a position reduces to

`PlaceType` gives the fixed structure at a position and nothing else: `None`, `Solid`, `Floor`,
`Wall`, `Door`. One entry per type of structure, not per state it can be in, and nothing that moves.

State travels beside it. A reduction produces a small record rather than a scalar, and `Cells` and
`Lattice` are buffers of it:

```cpp
struct Place
{
    PlaceType type = PlaceType::None;
    state_type state {};
};
```

## The state vocabulary

`state` is a set of named flags, shared across types rather than meaning something different under
each. The names are the component data's own, choosing whichever of a component or its field reads
as the state a table would talk about:

| component | name | |
|---|---|---|
| `Openable{open}` | `open` | the field, which is already the adjective |
| `Passable` | `passable` | the tag |
| `Concealed{found}` | `concealed` | the component, the field being its inverse |

Nothing is coined. A component renamed makes glyph files stale, and they fail at load against a name
that is not declared rather than drawing the wrong glyph.

Two rules keep the vocabulary from becoming a drawer of everything:

- **Only what changes the drawing becomes a name.** A lock enters the vocabulary when a locked door
  is drawn differently and not before. The width is bounded by what glyph sets distinguish, not by
  what the simulation models.
- **Each name declares the types it applies to.** `open` is declared for the types that can have
  `Openable`, so `wall open` is rejected at load rather than accepted as a line that can never
  match.

The reduction is where components become names. Nothing downstream of it sees flecs.

## The shape key

A wall or a corner is drawn from what reaches it, which the lattice already has and currently
collapses to two booleans and three characters --- all ASCII can express. The key covers the whole of it
instead: four bits, one per direction, set where something is drawn that way.

Sixteen patterns is exactly the box drawing set, so a Unicode set fills them in and an ASCII set
maps many of them onto `+`. Duplication in the ASCII table is the expected shape of a set that
cannot express as much, not a sign the key is too wide.

It removes a case rather than adding one: a wall segment's own mask determines vertical or horizontal, so
walls and corners resolve through one key instead of the renderer branching on position parity
first.

## The lookup

A glyph set is a table from `(type, state, mask)` to whatever the device draws --- a character for
ASCII, an index for tiles. Most specific wins: an entry specifying a state is preferred to one that does
not, so a set with `door open` draws it apart and a set with only `door` draws both alike.

That is what makes differing capabilities a property of the data. The renderer asks the same
question of every set.

## Loading

A glyph set is a file rather than a struct of named members, read into the table at startup. The
ASCII set becomes the default one shipped as data, and it is the only set until something needs a
second.

The file lives with the other assets, and the reader follows the level readers: line oriented,
binary mode with line endings filtered centrally, a header naming the format and its version.

## Steps

- [x] **1. `Place` and the state vocabulary.** `PlaceType` back to one entry per type, `Place`
      beside it, the flags and their per-type declaration, and `Cells` and `Lattice` as buffers of
      `Place`. The reduction fills state from the components. The renderers keep their conditions
      and read `place.type`, so nothing drawn changes and every reference frame is what it was.
- [x] **2. The mask.** The lattice reduction computes the four-bit mask per position. The renderer
      still collapses it, so again nothing drawn changes.
- [x] **3. The table.** Lookup by `(type, state, mask)`, most specific first, with the ASCII set
      built in as data rather than as a struct. `render_lattice`'s condition chain goes.

      Telling `Solid` from `None` came with it. The two absences differ by whether every cell the
      position touches is closed, and the renderer was what decided it --- so the table could
      not have been the whole mapping while the rule stood outside it. The reduction decides it now,
      from the layout rather than from a component, and both absences are types like any other.
- [x] **4. Loading.** `read_glyphs`, the two files in `assets/glyphs/`, and a check against the
      built-in sets so neither can drift from the other.

Steps 1 and 2 are the seam and change nothing visible, which is what made step 3 a comparison
against frames that already existed rather than a rewrite of them. Every reference frame is
byte for byte what it was before any of this.

## Open

- Whether `cells` has state at all. Its consumer draws one glyph per cell and has no edges, so
  it may need the type alone --- but the two reductions producing different shapes is its own cost.
- What a tile set produces, which decides whether the table is templated on what it maps to or
  whether characters and indices are two tables. Nothing needs it until a tile view does.
- Nothing loads a set from a file yet. The hosts take the built-in ones, a file being the way a
  set that is not built in arrives rather than how these two do.
