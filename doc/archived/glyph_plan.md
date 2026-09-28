# Glyph mapping --- Plan

Status: complete
Date: 2026-09-03, revised 2026-09-22

This is the glyph mapping item of stage 6 in `work_plan.md`: state beside the place type, and a glyph set as a table
that a file can supply. `game_system_reference.md` describes what is built.

Drawing a shut door differently from an open one should not need a `PlaceType` per state: a locked, concealed or broken door, and
every other attachment, would multiply the enum. And the mapping to characters, a struct of named glyphs and a chain of
conditions in `render_lattice`, leaves nowhere to express a glyph set with more than ASCII, or one producing tile indices.

## What a position reduces to

`PlaceType` is the fixed structure at a position and nothing else: `None`, `Solid`, `Floor`, `Wall`, `Door`. State is
beside it, in a `Place`, and `Cells` and `Lattice` are buffers of `Place`:

```cpp
struct Place
{
    PlaceType type = PlaceType::None;
    state_type state {};
};
```

## The state vocabulary

`state` is a set of named flags shared across types, each named after the component data it is read from:

| component | name | |
|---|---|---|
| `Openable{open}` | `open` | the field |
| `Crossable` | `crossable` | the tag |
| `Concealed{revealed}` | `concealed` | the component, `revealed` being its inverse |

A renamed component makes a glyph file fail at load against an undeclared name.

- **Only what changes the drawing becomes a name.** A lock becomes one when a locked door is drawn differently.
- **Each name declares the types it applies to**, so `wall open` is rejected at load.

The reduction is where components become names; nothing downstream of it sees flecs.

## The mask

A wall or a corner is drawn from the directions its wall line continues in: four bits, one per direction. Sixteen
patterns is the box drawing set, so a Unicode set fills them in and an ASCII set maps many of them onto `+`. A wall's own
mask determines whether it is vertical or horizontal, so walls and corners resolve through one key.

## The lookup

A glyph set is a table from `(type, state, mask)` to what the device draws --- a character for ASCII, an index for tiles.
The most specific entry wins: a set with `door open` draws an open door differently, and a set with only `door` draws
both the same.

## Loading

A glyph set is a file, read into the table: line oriented, binary mode with line endings filtered centrally, a header
giving the format and its version. The files are in `assets/glyphs/`.

## Steps

- [x] **1. `Place` and the state vocabulary**, filled by the reduction from the components, with nothing drawn changing.
- [x] **2. The mask**, computed per position by the lattice reduction.
- [x] **3. The table**, looked up by `(type, state, mask)`, most specific first, with the ASCII sets built in. The
      reduction decides `Solid` against `None`, from the layout.
- [x] **4. Loading**: `read_glyphs`, the two files in `assets/glyphs/`, and a check of each against the built-in set.

Every reference frame is byte for byte what it was before.

## Open

The open items are tracked in `work_plan.md`.
