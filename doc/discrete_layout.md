# Discrete Layout File Format Specification

Status: specification --- the reader and writer are built in `stratum_format`, both generators write it, and `stratum_sim` builds layouts from it
Date: 2026-09-11, revised 2026-09-13

What a discrete layout file contains. One file is one `Zone`: the cell grid that quantizes
movement, the walls between the cells, the flats bounding it above and below, the rooms defined over
it, and everything attached to any of those.

It includes rooms and their names, surfaces with height, styles and the entities a level places,
none of which the two generator grids could express. `sim_zone_model.md` owns the model this serializes.
`discrete_layout_plan.md` has what a file loads into and the reader and writer either side of it.

## What a file is

One JSON object. `format`, `version` and `cells` are required; the rest are
omitted where a level has none.

| Key | |
| --- | --- |
| `format` | `"quetzal.discrete_layout"`. Identifies the file to a reader as this format |
| `version` | The version of this format the file was written to |
| `generator` | Which generator produced it and the parameters of the run |
| `layout` | What the layout as a whole is called and is like |
| `cells` | The grid: its notation, its extent, its origin and scale, and its rows |
| `barriers` | Where a room's fill is closed off, for every room |
| `rooms` | The areas defined over the grid |
| `walls` | Per-wall data, for the walls that have any |
| `flats` | The floor and the ceiling |
| `attachments` | Everything attached to a wall or to a flat cell |

A file is one zone. A staircase between two is an entity attached to the floor, and it has a relocator
whose target is the other. A relocator is the component that moves a mover to its target; a staircase,
a portal and a waypoint are entities that have one.

A level file is a template a zone is made from, and the file does not store the zone's identity: a zone
is an entity with a persistent id, given when it is made (`addressing.md`). `layout.components.name` is
what the level is called for a player to read.

## An example

A zone of four cells by three in the wall notation: a room of two by three on the left, a corridor
cell joining it eastward through a door, and on the right the stair head, one cell between two stair
blocks, a staircase down above it and the entrance, a staircase up, below it.

```json
{
  "format": "quetzal.discrete_layout",
  "version": "0.3",
  "generator": {
    "name": "catagen",
    "parameters": { "extent": "4,3", "scale": "3-7", "spacing": "2", "density": "1.0", "loop": "0.1", "seed": "20260829" }
  },
  "layout": {
    "components": { "name": "The Flooded Cistern", "type": "dungeon", "difficulty": 3, "style": "cistern", "entry": [[3, 2], [3, 0]] }
  },
  "cells": {
    "notation": "wall",
    "extent": [4, 3],
    "origin": [0.0, 0.0, 0.0],
    "scale": 1.0,
    "rows": [
      " - -   - ",
      "|. .| |>|",
      "     - - ",
      "|. .+. .|",
      "     - - ",
      "|. .| |<|",
      " - -   - "
    ]
  },
  "barriers": [[3, 1, 1, 0]],
  "rooms": [
    { "perimeter": [[0, 0], [2, 0], [2, 3], [0, 3]], "components": { "name": "the cistern", "style": { "terrain": "water" } } },
    { "location": [3, 1], "components": { "name": "the stair head" } }
  ],
  "walls": [
    { "wall": [1, 1, -1, 0], "components": { "style": "buttressed" } }
  ],
  "flats": [
    { "role": "floor", "height": 0.0, "components": { "terrain": "flagstone" },
      "rooms": [
        { "room": 0, "height": -0.25, "components": { "terrain": "water" } }
      ] },
    { "role": "ceiling", "height": 3.0 }
  ],
  "attachments": [
    { "walls": [[1, 1, -1, 0], [2, 1, 1, 0]], "entities": [ { "type": "door", "components": { "material": "oak", "open": false } } ] },
    { "flat": "floor", "cell": [3, 2], "entities": [ { "type": "staircase", "components": { "facing": [0, -1], "relocator": { "target": "level_down" } } } ] },
    { "flat": "floor", "cell": [3, 0], "entities": [ { "type": "staircase", "components": { "facing": [0, 1], "relocator": { "target": "level_up" } } } ] }
  ]
}
```

## Conventions

### Coordinates

Every coordinate outside `cells.rows` is a cell coordinate, whichever notation the grid uses.
`[i, j]` is the cell `i` east and `j` north of the grid's corner. The two notations therefore differ
only in the rows, and converting a level between them keeps every room, wall and attachment.

A room's `perimeter` is the exception, in junction coordinates: the corners of cells. Junction
`[u, v]` is the lower-left (SW) corner of the cell at `[u, v]`, so the cells from `[0, 0]` to
`[1, 2]` have the perimeter `[[0, 0], [2, 0], [2, 3], [0, 3]]`. A perimeter runs between cells,
which junctions can express and cells cannot.

A position in the zone is three numbers, `[x, y, z]`. The grid lies in the xy-plane and z is height.

### Addresses

A wall address is `[i, j, ni, nj]`, a cell and the normal of the wall on one of its sides, pointing
into the cell. A flat cell address is the flat's role and a cell, `"flat": "floor"` and
`"cell": [i, j]`. The file never refers to a wall by id.

What an edge, a wall, an address and every other identifier are, and the rules each follows, are
specified in `addressing.md`.

### Components

Everything in the file that is not geometry is a component object: named values under a
`components` key. The reader interprets none of it. It builds the layout from the geometry and
hands the components to its caller, which has a world to build entities in; a name the caller does
not recognize is its to ignore.

So the reader reports where a door goes and never learns what a door is, and terrain, difficulty, a
relocator's target and whatever content later adds pass through to the caller the same way.

### Inheritance

Some components are inherited: stated at one level, they apply to the levels below it until one of
those overrides them.

    layout            layout.components, and each flat's own
      room            the room's components, and each flat's components for that room
        wall, flat cell   the wall's components, and each flat's components for that cell

A wall or a flat cell inherits from the room covering it, and from the layout where no room does. A
flat states its own at every level of this hierarchy rather than having a second: a floor spans the
whole zone, so its own components belong at the layout's level, and those it specifies for a room at
the room's. Where both are specified at one level the flat's wins. `height` descends the same chain.

The nearest level wins, component by component, and a component whose value is an object merges
property by property the same way, so a level overrides part of one by stating only that part.
Merging takes no notice of what a property is. Which components inherit is the consumer's vocabulary,
as every component name is. The reader resolves no chain, and hands back each component object at
the level it was written.

`style` is one: a theme and how it is built. Its value is a name, an object of properties, or an
object with a `name` and properties overriding part of what the name refers to; `"style": "cistern"`
is `"style": { "name": "cistern" }`. A name is an asset the consumer resolves, which lets a theme be
shared across levels without being copied into each. The format defines no style property: `theme`,
`ceiling` and `segmented` are what is written so far --- what a surface is made of, flat or arched,
and whether a run is broken into bays. A generator writes the properties the caller it is written for
consumes, and nothing between them checks the pair.

### The written form

A reader accepts any valid JSON. A writer emits this form, so a level file diffs as a level:

- Two spaces of indentation.
- One grid row per line in `cells.rows`, so a changed row is a changed line. In characters rather
  than runs wherever the grid is small enough to read as a picture of the level.
- One entry per line for a room, a wall, a flat cell and an attachment, however long the line. A flat
  with `rooms` or `cells` breaks before each of them, and writes their entries one per line.
- Content written as it was read, each component object and entity inline on the line of the key it
  belongs to.
- Coordinates and coordinate arrays inline: `[3, 1, 1, 0]` on the line that uses it, never one
  number per line.
- Numbers plainly, without an exponent.

## version

A label a reader matches rather than a number it compares. `0.3`, the leading zero marking it
pre-release: no older file is kept readable, and every checked-in level regenerates when the format
moves. A reader given a version it does not know rejects the file.

## generator

Which generator produced the file and the parameters of the run, so that the file reproduces its own
level:

```json
"generator": { "name": "catagen", "parameters": { "extent": "4,3", "seed": "20260829" } }
```

Parameter values are strings. They are the generator's own option vocabulary, ranges like `3-7` and
seeds spanning an unsigned range, so each generator reads them back with the reader it has and a
parameters object reads as a command line. The format's reader uses none of them.

A file that was authored rather than generated omits `generator`.

## layout

What the layout as a whole is called and is like. A component object; nothing else.

```json
"layout": { "components": { "name": "The Flooded Cistern", "type": "dungeon", "difficulty": 3, "style": "cistern" } }
```

`name`, `type`, `difficulty` and `style` are components like any other. They are the level metadata
`sim_zone_model.md` describes, so depth, recipe, encounter and loot tables belong here too as the
content that needs them arrives.

`entry` is required: the cells a mover arriving in the zone can be placed on, at least one. Every zone
is built with them.

```json
"layout": { "components": { "entry": [[5, 5], [12, 3]] } }
```

Each entry cell is a cell written `<` or `>` with an entity attached to the floor there that has a
`relocator` component, and the reader rejects a file whose entry is absent, lists no cell, or lists
one that is not such a cell. `<` is the entrance glyph, and a waypoint or a portal is an entrance as
much as a staircase leading up is; `>` is a staircase leading down, which is the entry of a tower
entered from below. The entity's `type` is not what is checked. A relocator written on the floor cell
itself, rather than on an entity attached there, does not count.

Which entry cell an arriving mover is placed on depends on where it comes from, so the list does not
choose: a caller arriving from above looks among the staircases leading up, one arriving by waypoint
among the waypoints, and so on. A generator lists every staircase it places, and a file written by
hand lists its entries as any file must. Once a layout is read, its entries are known, which is what
an unbound `level_down` resolves to (`### A relocator's target`).

## cells

The grid: which notation the rows are written in, how many cells it has, where it is placed in the
zone, and the rows.

```json
"cells": { "notation": "wall", "extent": [4, 3], "origin": [0.0, 0.0, 0.0], "scale": 1.0, "rows": [ ... ] }
```

| Key | |
| --- | --- |
| `notation` | `"block"` or `"wall"` |
| `extent` | Cells across and up. Required, and what every row is read against |
| `origin` | The corner of cell `[0, 0]` in the zone. `[0, 0, 0]` where absent |
| `scale` | The width of one cell. `1.0` where absent |
| `encoding` | `"characters"` or `"runs"`. `"characters"` where absent |
| `rows` | The grid, the first row written being the highest |

The first row written is the highest and the grid origin is the lower left, in both notations. The
row count and the width of a row follow from `extent` and the notation.

A space is a glyph. It means no floor at a cell position, no wall at a wall position, and nothing at
a corner, and a reader reads one as it reads any other character. Every row is written to the full
width, trailing spaces included, and a row of any other width is an error.

The rows are what encode openness. A room does not decide whether a mover may stand somewhere, and
neither does terrain on a flat.

### Runs

A row may be run-compressed instead, `"encoding": "runs"` setting that for the whole grid. A count
repeats the glyph that follows it, and a glyph with no count is one of it:

    "|. .| |>|"    characters
    "|. .| |>|"    runs --- nothing in the row repeats, so it is the same row
    "         "    characters
    "9 "           runs

No notation uses a digit, so a run of digits at a position can only be a count. A count of zero is
an error, as is a row that does not expand to the grid's width.

Compression is per row, so the grid is still one row per line. It is for a large zone, where a row
is mostly one glyph repeated; a level small enough to read is written in characters.

### The block notation

One character per cell: `extent.x` characters on each of `extent.y` rows.

| glyph | |
| --- | --- |
| `.` | floor |
| `#` | wall |
| `+` | door |
| `>` | staircase down |
| `<` | entrance: a staircase up, or any other non-local transporter arrived by |

A wall is a cell here, so a door is a cell too: a block of wall a mover may stand in. Two areas
never share an edge in this notation, a block always lying between them.

A wall is a face of a block, on the side of the cell it faces. A floor cell has a wall toward a `#`, a
`+`, a stair cell or the border of the grid; a door cell has one toward a `#`, a stair cell or the border of
the grid, and none toward floor, so a mover inside a doorway meets no wall on the way out. A stair cell is
a block and has none.

### The wall notation

The edges lie between the cells, so the grid is two characters across per cell plus one:
`2 * extent.x + 1` characters on each of `2 * extent.y + 1` rows. Cell `[i, j]` is written at
`(2i + 1, 2j + 1)`, the edge west of it at `(2i, 2j + 1)`, the edge south of it at `(2i + 1, 2j)`,
and the corners at the even positions in both.

| position | glyph | |
| --- | --- | --- |
| cell | `.` | floor |
| | `>` | staircase down |
| | `<` | entrance: a staircase up, or any other non-local transporter arrived by |
| | ` ` | no floor |
| edge | `-` | wall, horizontal |
| | `\|` | wall, vertical |
| | `+` | door |
| | ` ` | no wall |
| corner | ` ` | always |

A position's parity determines which of the three it is.

A corner position is always blank, and so is a cell with no floor. The edges either side of a corner
express everything a corner could, and a solid cell is the absence of floor. A level draws its floor and the
walls around it, and nothing else.

A wall always separates a floor cell from a blank cell, from a stair cell, and from the border of the grid.
A stair cell is a block with no walls of its own, so nothing is required between one and a blank cell or
the border.

An edge glyph is one character standing for both sides, so what it writes is symmetric. A door
open from one side alone, or secret from one side, is components on the attachment.

### Staircases and entrances

A stair glyph marks where a staircase is: a block, with a wall toward it on every side where there is
floor, and a `staircase` entity attached to the floor at that cell, with a `relocator` whose target is
where it leads, as the attachments section sets out.

`<` marks more than a staircase leading up. It is the entrance glyph, the up exit or entrance point of
any non-local transporter --- a staircase, a waypoint, a portal --- as `+` is the glyph of any threshold
that opens and closes. What stands there is the entity attached at the cell, whatever its `type`.

The reader reports a staircase at every stair glyph, whether the file has an attachment there or not.
Where no staircase at that cell has a relocator with a target, the reader supplies the unbound target
the glyph implies, down for `>` and up for `<`. Every staircase leads somewhere, and a level that
specifies nothing about where its staircases go is playable as it is.

A staircase has a facing, written as a `facing` component on the staircase: the direction a mover
steps from the stair cell into the space the staircase opens into, its open side. See `### A staircase's facing`.

## rooms

A room is an area over the grid, and a room's id is its position in the array. Nothing decides
passage by a room; its cells are stored and read back.

A room is identified one of two ways, and specifies exactly one of them:

| | |
| --- | --- |
| `perimeter` | A polygon in junction coordinates, within the grid, its sides horizontal or vertical and never crossing or touching one another. Each cell edge along it is a wall or a virtual wall |
| `location` | A floor cell anywhere in the room, the barriers closing it off |

**By location.** `location` is a cell written `.`, never a doorway, which is what a room is reached
through, or a stair cell, which is a block. The fill spreads from there through floor cells, across
edges with no wall and no door, and stops at a barrier. The location alone suffices wherever a room is closed by its own walls, which is
the ordinary generated case; barriers close what is open, an egress cell or a gap where one area
runs into the next.

**By perimeter.** The polygon is the room's, and the room's cells are the floor/door cells within
it. It expresses what a fill cannot: a room whose perimeter crosses open ground, or one that
includes part of a hall.

Every cell edge along a perimeter is either a wall facing a floor/door cell on either side of it, or a
virtual wall: an edge with no wall, between two floor/door cells. An edge with no wall and a blank, solid
or stair cell on either side is an error. A pillar on a perimeter may lie inside the room however.

Either way the layout stores one result: the room's cells. Nothing that uses the layout has two
cases.

```json
{ "perimeter": [[0, 0], [2, 0], [2, 3], [0, 3]], "components": { "name": "the cistern", "style": { "terrain": "water" } } }
{ "location": [3, 1], "components": { "name": "the stair head" } }
```

A cell belongs to at most one room. Two rooms covering one cell is an error in the file.

## barriers

Where a room's fill is closed off: an array of positions beside the grid rather than a glyph in it.

```json
"barriers": [[3, 1, 1, 0], [7, 4]]
```

A wall address is a barrier across that edge; a cell address closes off the whole cell. Four numbers
is the first, two the second.

A fill does not cross a barrier, and that is all a barrier does. It is never in the layout: it builds
nothing and blocks nothing, and a mover crosses one with nothing to notice.

What survives is the shape of the cells a room covers, so a perimeter traced from them later runs
along a barrier as it runs along a wall.

The array is layout-wide and every room is filled against all of it, a barrier between two rooms
being one edge both need. A room may also have `barriers` of its own, applied to its own fill
alone, where one room needs an edge another must not have.

## walls

Per-wall data, for the walls that have any, one entry per wall. The grid creates the walls, so an
address of a wall the grid does not have is an error.

```json
{ "wall": [1, 1, -1, 0], "components": { "style": "buttressed" } }
```

Surface treatment is per side and every wall is one side, so an entry here needs no field for which side
it means.

## flats

The floor and the ceiling bounding the zone, one entry each, the floor alone for a zone with no
ceiling. Where `flats` is absent the floor is at height 0 and the ceiling at 1.

```json
{ "role": "floor", "height": 0.0, "components": { "terrain": "flagstone", "style": "cistern" },
  "rooms": [
    { "room": 0, "height": -0.25, "components": { "terrain": "water" } }
  ],
  "cells": [
    { "cell": [3, 2], "height": 0.5, "components": { "terrain": "step" } }
  ] }
```

| Key | |
| --- | --- |
| `role` | `"floor"` or `"ceiling"`. The two differ by role and nothing else |
| `height` | Where the surface is across the whole flat |
| `components` | The surface's components across the whole flat |
| `rooms` | Per room: a room id, and what that room overrides |
| `cells` | Per cell: a cell of the cell grid, and what that cell overrides |

### Height descends with components

A flat states each of the three at whichever level it is uniform over, and a level below overrides
what it specifies:

    flat
      room
        cell

A cell takes its height from the properties specified for it, failing that from those specified for
the room covering it, failing that from the flat's own. Inherited components resolve the same way,
these being three levels of the hierarchy Inheritance sets out. A sunken room costs one entry.

A flat is sampled at cell scale and has no grid of its own: a height belongs to a cell of the grid
the rows are written on. Generation is expected to work at the room level and leave the cell level
to authored detail.

A hole in the floor is a component. The cell is floor in the grid and the flat has a height there;
what makes it a hole is a component on that cell, or an attachment there, and the entity a caller
builds from either. A shaft passes a mover the way a door does, downward rather than across, and a
pit lets one in and need not let it out. What happens at the hole belongs to that entity.

A flat component that affects movement does so as an entity. Openness comes from the rows, so
terrain, rocks and a trap door affect a mover through the entity built from them and that entity's
`Blocking` tag; items and decoration are the same shape without it. The format does not distinguish
them.

## attachments

Everything attached to a wall or to a flat cell: a door, a stairway's far end, an alcove, a basin, a
button, a torch. One section covers both.

```json
{ "walls": [[1, 1, -1, 0], [2, 1, 1, 0]], "entities": [ { "type": "door", "components": { "material": "oak" } } ] }
{ "flat": "floor", "cell": [3, 2], "entities": [ { "type": "staircase", "components": { "relocator": { } } } ] }
{ "walls": [[0, 2, 0, -1]], "entities": [ { "type": "basin", "contains": [ { "type": "water" } ] } ] }
```

| Key | |
| --- | --- |
| `walls` | One or more wall addresses. The sides the attachment presents |
| `flat`, `cell` | A flat cell: the flat by role, and a cell of the cell grid |
| `position` | Where on the surface: along and up on a wall, xy within the cell on a flat. Centered where absent, and an entity may specify its own |
| `entities` | What is attached, as content the format does not check. Each is written so far as a `type`, a component object, and what it contains |

An attachment is to a flat cell and never to a flat as a whole. A flat is the entire floor or
ceiling of the zone; what a torch or a shaft is at is one cell of it.

An attachment lists every side it presents. A door is two wall addresses, one per facing; a doorway
into a dead end is one; a torch is one. The same shape works in both notations: in the wall
notation the two walls are the sides of one edge, in the block notation the walls the floor either
side has toward the doorway block.

The grid specifies where and the attachment specifies what. A door glyph creates the walls and marks
the crossing; an attachment at the same walls sets that door's material, lock and trap. A stair
glyph marks a staircase; an attachment at that cell sets the staircase's relocator and so where it
leads. An attachment may also be
the whole of an entity the grid has no glyph for.

What an entity contains is not a second entity at the same place. A basin with water in it is one
entity containing another, written in the basin's `contains` and nested as far as it goes. The
alcove that is itself a container is the same shape.

`entities` is a list because more than one entity attaches at one place: a door and the lever that
works it, a plaque beneath a torch. The caller resolves each `type`, types such as `staircase`,
`waypoint` and `portal` being prefabs the world has, and the format restricts no value of it.

### A relocator's target

A relocator is a component of the entity that moves a mover --- a staircase, a portal, a waypoint --- and
where it leads is its `target`, bound or unbound.

```json
"relocator": { "target": "level_down" }
```

**Bound: the relocator arrived at,** by its persistent id (`addressing.md`, `### Relocators and
access points`). A target is bound by having been travelled: until a way has been gone through, nothing has
fixed where it arrives. A zone long since generated still has access points nobody has used, and
every one of them is unbound. A level file is a template, so its relocators are unbound; a bound target
is written only in a saved zone, in the form `addressing_plan.md` sets out.

**Unbound: a destination.** The next level down, the way back up, the surface town, the nearest
surface access, a destination a quest specifies. It identifies one destination or asks for the
nearest of a kind. Which names it may use is the caller's vocabulary, as an entity's `type` is.

Resolving one finds a zone or makes one. Traversal passes the target to whatever owns zones, which
returns a zone it has or generates one, and chooses the cell an arriving mover is placed in. What it
resolves to is written back as a bound target, so leaving and returning arrive at the same place.

`level_down` resolves to the zone below, and `level_up` into a tower to the zone above, so either is a
real target as soon as that zone is generated or loaded. Arriving through a relocator not yet bound,
the mover is placed at one of that zone's entry points not yet bound --- a relocator it can be arrived
at, such as a staircase leading up, or leading down in a tower --- chosen among them, and going through
binds both ends: the relocator gone through is bound to that entry point, and the entry point to the
relocator it came from. The layout's `entry` lists the entry points, and every zone has at least one.

The reader interprets neither shape. A relocator is a component like any other and both are passed to
the caller as they were written.

### A staircase's facing

Which way a staircase opens, written as a `facing` component on the staircase:

```json
"facing": [0, -1]
```

The value is one of the four steps across a cell's faces, `[1, 0]`, `[0, 1]`, `[-1, 0]` or `[0, -1]`,
from the stair cell toward the space the staircase opens into, which is the way a mover arriving by it
steps off: a side with floor beyond, or a doorway where the stair cell has no floor beside it. It is a
direction on the grid rather than a wall address, a staircase being on a flat cell rather than on a wall. Like the relocator, the reader passes it on as written. A generator writes a
facing for every staircase it places.

## What a reader must do with it

Load and validate the geometry, and hand back everything else as the file wrote it. What was read,
written back, is equivalent to the file and complete, if not identical to it.
A reader has no world to build entities in, so where the file puts a door it reports the door rather
than placing one, and the same applies to every component object and every attachment.

A key the format does not define is not tracked: inside content it is kept as written, and anywhere
else it is ignored. A change to the structure of the geometry is a new version, which a reader rejects.

It resolves one structure, the file's own rather than content: a room, into the cells it covers,
however the file identified it. An inherited component is content, and its chain is the consumer's to resolve.

`discrete_layout_plan.md` has the structures and the passes.

## Errors

A file breaking any of these is rejected, stating what was wrong and where. Nothing is built until
every check has passed, so a rejected file leaves nothing behind.

- The file cannot be opened, or is not valid JSON.
- A key the format requires is absent, or a value is not the kind the format sets: an object, an
  array, a string or a number. Content is checked only for being an object, or an array of entities.
- A coordinate is not a whole number, or is outside the coordinate range.
- `format` is absent or is not this format; `version` is one the reader does not read.
- `cells` is absent, its notation or its encoding is not one of the two it offers, its extent has no
  cells, its scale is not positive, it has the wrong number of rows, or a row is not the width its
  extent sets.
- A run count of zero, or a count with no glyph after it.
- A glyph the notation does not define, or one at a position that does not take it.
- In the wall notation, a floor cell with no wall between it and a blank cell, a stair cell, or the
  border of the grid.
- An address's normal is not one of the four steps across a cell's faces, or a barrier is neither two
  numbers nor four.
- A room specifies both `perimeter` and `location`, or neither.
- A room's location is not a floor cell, or is a cell closed off by a barrier.
- A room's perimeter has fewer than four junctions, a junction off the grid, a side neither
  horizontal nor vertical, or two sides that cross or touch.
- A cell edge along a perimeter is neither a wall facing a floor/door cell nor an edge between two
  floor/door cells.
- Two rooms cover one cell.
- A barrier is at a position off the grid.
- A wall address has no wall in the grid, or its cell is not a floor/door cell.
- More than one entry in `walls` for one wall.
- A flat refers to a room that does not exist, or to a cell off the grid, or has more than one entry
  for one room or one cell.
- More than one flat in a role.
- An attachment specifies both `walls` and a flat cell, or neither, lists no wall, or is on a flat
  cell off the grid.
- `layout.components.entry` is absent, is not a list, or lists no cell; or a cell it lists is not a
  cell, is off the grid, is not written `<` or `>`, or has no entity with a relocator attached to the
  floor there.

## What this supersedes

The two grid formats, and the level file section of `catagen_reference.md` and
`dungen_reference.md` with them. Both generators write this, their parameter headers becoming
`generator.parameters` with the same tokens.

What it affects beyond the file --- what reads one, what writes one, and what each is built of ---
is in `discrete_layout_plan.md`.
