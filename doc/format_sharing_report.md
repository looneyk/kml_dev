# Format Sharing --- Report

What the two level formats actually share, what separating them would cost, and which
format/view combinations exist. Written to answer whether the sharing is carrying its weight.

## The short answer

The two formats are already separate everywhere they differ. They meet at one seam --- `Map` ---
and nothing downstream of it depends on which format it came from. So there is no tangle to cut:
separating further means *duplicating* the format-neutral half, not untangling it.

But the seam itself is not clean. `Map` holds two mutually exclusive fields, and that is the
overlap worth acting on.

## What is format-specific today

| Piece | Lines | Formats |
| --- | --- | --- |
| `read_block` in `load_layout_dungen.cpp` | 158 | block only |
| `read_walls`, `wall_extent` in `load_layout_catagen.cpp` | 199 | wall only |
| generator dispatch in `load_layout()` | ~12 | picks one of the two |

That is the whole of it. Each reader is a file of its own, neither includes the other, and
each is reached through one `if` on the header's generator name.

## What is format-neutral

| Piece | Lines | Why it does not care |
| --- | --- | --- |
| header row reading | ~80 | both formats write the same header shape |
| `find_components` | ~60 | takes the format's own predicates |
| `trace_outlines`, `build`, `create_rooms` | ~330 | work from `Map`, which is already reduced |
| `DiscreteLayout` | whole file | one runtime model, not two |
| `cells`, `lattice`, `Place`, `Glyphs`, `render*` | whole library | consume a layout, never a file |
| `Clock`, `commands`, `ai`, `attachments`, `Room`, `World`, `Session` | whole library | never see a format at all |

The reason this holds is the model, not luck: a block layout is a wall layout in which no two
areas share an edge, so the per-side wall records express both. The wall format can express one
case the block format cannot --- a wall with a room behind it rather than a solid cell --- and
that is a value the block reader never produces, not a second structure.

## What separating them would take

Everything in the second table would be duplicated or made a template. Concretely:

- **Two `Map` types.** Cheap, and worth doing on its own account --- see below.
- **Two of `trace_outlines`/`build`/`create_rooms`** (~330 lines duplicated), or one templated
  on the map type, which is the same coupling with more syntax.
- **Two `DiscreteLayout`s**, or one shared and the separation stops at the loader.
- **Two view stacks** if the separation is carried through: `cells`/`lattice`/`Glyphs`/`render`
  all key on a layout, so two layout types means two of each, and the glyph file format forks
  with them.
- **`Session`, `World` and the hosts become generic over which pair they own**, since a session
  currently has one `Layout` interface and does not care.

What constrains this is the test suites: `stratum_view_test` renders a *block* format level onto
the lattice and a *wall* format level onto it too, and both round-trip byte for byte against
their own files. Those cases exist because the shared path is what makes them comparable.
Duplicating the path means duplicating the reference frames.

**Assessment: the sharing is carrying its weight.** ~360 format-specific lines against ~470
shared loader lines plus two whole libraries. Splitting trades one seam for two of everything.

## The overlap that is real

`Map` is a union pretending to be a struct:

| Field | Block format | Wall format |
| --- | --- | --- |
| `w`, `h`, `cell_open`, `cell_components`, `component_cells` | used | used |
| `blocks` | used | **always empty** --- the wall format spends no cell on a doorway |
| `passages` | **always empty** --- nothing separates two open cells but a block | used |
| `cell_room` | used | empty where the format defines no rooms |

`blocks` and `passages` are mutually exclusive and nothing in the type expresses it. A reader of
`build()` has to trace which format filled the map to tell which loop does anything. That is where "block-ness"
and "wall-ness" leak into shared code, and it is what reads as too much reuse.

Worth doing, and small: make the exclusion explicit rather than implied by which reader ran.

## Format and view combinations

Two formats, two reductions, all four running today. Three show every wall the layout has;
one cannot.

| | cell view (`cells`, WxH) | lattice view (`lattice`, 2N+1) |
| --- | --- | --- |
| **block format** | shows every wall --- one exists exactly where open meets solid, and the solid cell is drawn, so the wall is read off it. **Default.** | shows every wall, drawn on the edges rather than implied. `corridor.level` through `lattice()` in `stratum_view_test` |
| **wall format** | **loses walls** --- one between two open cells has no position here and cannot be inferred, so it renders as open ground a mover cannot cross | shows every wall. **Default.** |

`corridor.level`, which is `#.....#`, through each:

    block x lattice          block x cell

    ###############
    ###############          #######
    ##+---------+##          #@...@#
    ##|@       @|##          #######
    ##+---------+##
    ###############
    ###############

Whether a view is byte-exact against the file it was loaded from is a separate property, and
only one combination has it:

| | byte-exact against its source file |
| --- | --- |
| block x cell | no --- dungen writes a space for room floor and a dot for corridor, and both render as a dot. Checked as a per-cell mapping instead |
| block x lattice | no --- no dungen file has this shape |
| wall x cell | no |
| wall x lattice | yes, which is what `lattice_glyphs()` draws floor as a space for |

**What no view draws:** the block format's room-versus-corridor labelling. `cell_room` reaches
the layout and creates rooms, but `Place` has no field for it and neither glyph set tells the
two apart, so both reductions drop it. Equal in both views, so it does not separate them.

The defaults are picked in [Application.cpp:52](application/stratum_terminal/Application.cpp#L52) from
`Session::format()`, and `--cell` overrides them. That override is the only use `Format` has
anywhere.

### By consumer

| Consumer | Reduction | Status |
| --- | --- | --- |
| `stratum_terminal` cell view | `cells` -> `render` | built |
| `stratum_terminal` lattice view | `lattice` -> `render_lattice` | built |
| `stratum_godot` tile view | `cells` only | built --- so a wall format zone is lossy here, the one gap in the matrix |
| first-person 3D | neither; needs mesh instances | not built. The consumer that would settle whether `Buffer<T>` is the shared shape |

## Recommendation

1. **Do not separate the formats.** The runtime model genuinely is one model, and the split
   would duplicate the two libraries that never mention a format.
2. **Split `Map`.** Make `blocks` and `passages` exclusive in the type rather than by
   convention. This is the overlap that actually reads as overcomplication, and it is contained
   to the loader.
3. **Give the Godot view the lattice**, or accept in writing that it shows block format zones
   only. It consumes `cells`, which is the one combination in the matrix that loses walls, and
   it loses them silently.
