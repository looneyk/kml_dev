# Discrete Layout File Format Specification

Status: specification --- the reader and writer are built in `stratum_format`, both generators write it, and `stratum_sim` builds layouts from it
Date: 2026-09-11, revised 2026-09-26

This document specifies what a discrete layout file contains. One file is one `Zone`: the cell grid
that quantizes movement, the walls between the cells, the flats bounding it above and below, the rooms
defined over it, and the entities sited on any of those, with their names, heights, styles and other
components. `sim_zone_model.md` describes the model that this format serializes, and
`discrete_layout_plan.md` describes the structures a file loads into and the reader and writer on either
side of it.

## What a file is

A file is one JSON object. `format`, `version` and `cells` are required; the other keys are omitted if a
zone has none.

| Key | |
| --- | --- |
| `format` | `"quetzal.discrete_layout"`. Identifies the file to a reader as this format |
| `version` | The version of this format the file was written to |
| `zone` | The zone's own persistent id, only in a saved zone |
| `generator` | The generator that produced it and the parameters of the run |
| `layout` | The name and properties of the layout as a whole |
| `cells` | The grid: its notation, its extent, its origin and scale, and its rows |
| `barriers` | Where a room's fill is closed off, for every room |
| `rooms` | The areas defined over the grid |
| `walls` | Per-wall data, for the walls that have any |
| `flats` | The floor and the ceiling |
| `entities` | The zone's entities, and where each is sited |

A file is one zone. A staircase between two is an entity sited on the wall on its open side or on the
floor, and it has a relocator whose target is the other. A relocator is the component that moves an actor to its
target; a staircase, a portal and a waypoint are entities that have one.

A file is either a template a zone is made from or a saved zone. A template does not store the zone's
identity, a zone being an entity with a persistent id given when it is made (`addressing.md`); a saved
zone writes that id in `zone` and every id in the file is then persistent. `layout.components.name` is
what the zone is called for a player to read.

## An example

A zone of four cells by three in the wall notation: a room of two by three on the left, a corridor
cell joining it eastward through a door, and on the right the stair head, one cell between two stair
blocks, a staircase down above it and the entrance, a staircase up, below it.

```json
{
  "format": "quetzal.discrete_layout",
  "version": "0.4",
  "generator": {
    "name": "catagen",
    "parameters": { "extent": "4,3", "scale": "3-7", "spacing": "2", "density": "1.0", "loop": "0.1", "seed": "20260829" }
  },
  "layout": {
    "components": { "name": "The Flooded Cistern", "type": "dungeon", "difficulty": 3, "style": "cistern", "access_points": [ { "entity": 4 }, { "entity": 5 } ] }
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
  "barriers": [ { "cell_position": [3, 1], "normal": [1, 0] } ],
  "rooms": [
    { "perimeter": [[0, 0], [2, 0], [2, 3], [0, 3]], "components": { "name": "the cistern", "style": { "terrain": "water" } } },
    { "cell_position": [3, 1], "components": { "name": "the stair head" } }
  ],
  "walls": [
    { "wall": { "cell_position": [1, 1], "normal": [-1, 0] }, "components": { "style": "buttressed" } }
  ],
  "flats": [
    { "role": "floor", "height": 0.0, "components": { "terrain": "flagstone" },
      "rooms": [
        { "room": 0, "height": -0.25, "components": { "terrain": "water" } }
      ] },
    { "role": "ceiling", "height": 3.0 }
  ],
  "entities": [
    { "id": 1, "type": "door", "components": { "material": "oak", "open": false } },
    { "id": 2, "type": "door_facing", "components": { "parent": { "entity": 1 }, "site": { "relation": "on", "wall": { "cell_position": [1, 1], "normal": [-1, 0] } } } },
    { "id": 3, "type": "door_facing", "components": { "parent": { "entity": 1 }, "site": { "relation": "on", "wall": { "cell_position": [2, 1], "normal": [1, 0] } } } },
    { "id": 4, "type": "staircase", "components": { "site": { "relation": "on", "wall": { "cell_position": [3, 1], "normal": [0, -1] } }, "relocator": { "type": "staircase", "target": "-1", "order": 0, "placement": { "cell_position": [3, 1], "orientation": [0, -1] } } } },
    { "id": 5, "type": "staircase", "components": { "site": { "relation": "on", "wall": { "cell_position": [3, 1], "normal": [0, 1] } }, "relocator": { "type": "staircase", "target": "+1", "order": 1, "placement": { "cell_position": [3, 1], "orientation": [0, 1] } } } }
  ]
}
```

## Conventions

### Coordinates

Every coordinate outside `cells.rows` is a cell coordinate, whichever notation the grid uses.
`[i, j]` is the cell `i` east and `j` north of the grid's corner, and every key whose value is a cell
position is `cell_position`. The two notations therefore differ only in the rows, and converting a zone
between them keeps every room, wall and entity.

A room's `perimeter` is the exception, in junction coordinates: the corners of cells. Junction
`[u, v]` is the lower-left (SW) corner of the cell at `[u, v]`, so the cells from `[0, 0]` to
`[1, 2]` have the perimeter `[[0, 0], [2, 0], [2, 3], [0, 3]]`. A perimeter runs between cells,
which junctions can express and cells cannot.

A position in the zone is three numbers, `[x, y, z]`. The grid lies in the xy-plane and z is height.

### Addresses

A wall address is `{ "cell_position": [i, j], "normal": [ni, nj] }`, the cell the wall belongs to and
the normal pointing out of the wall's surface into that cell. A flat cell address is the flat's role and
a cell position, `"flat": "floor"` and `"cell_position": [i, j]`. The file never refers to a wall by id.

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

A wall or a flat cell inherits from the room containing it, and from the layout where no room does. A
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
shared across zones without being copied into each. The format defines no style property; the
generators write `theme`, what a surface is made of, `ceiling`, flat or arched, and `segmented`,
whether a run is broken into bays. A generator writes the properties the caller it is written for
consumes, and nothing between them checks the pair.

### The written form

A reader accepts any valid JSON. A writer emits this form, so a zone file diffs as a zone:

- Two spaces of indentation.
- One grid row per line in `cells.rows`, so a changed row is a changed line. In characters rather
  than runs wherever the grid is small enough to read as a picture of the zone.
- One entry per line for a room, a wall, a flat cell and an entity, however long the line. A flat
  with `rooms` or `cells` breaks before each of them, and writes their entries one per line.
- Content written as it was read, each component object inline on the line of the key it belongs to.
- Coordinates, coordinate arrays and wall addresses inline: `{ "cell_position": [3, 1], "normal": [1, 0] }`
  on the line that uses it, never one number per line.
- Numbers plainly, without an exponent.

## version

The version is a label that a reader matches, not a number it compares. It is `0.4`, and the leading
zero marks it as pre-release: no older file is kept readable, and every checked-in zone file regenerates when the format
moves. A reader given a version it does not know rejects the file.

## zone

The zone's own persistent id, written by a save:

```json
"zone": { "id": 7713 }
```

Its presence is what marks every id in the file as persistent --- entity ids, references, and bound
relocator targets --- and a template has no `zone` section. A reference in a saved zone may point to an
entity in another zone, so the reader does not check that it resolves.

## generator

The generator that produced the file and the parameters of the run, so that the file reproduces its own
zone:

```json
"generator": { "name": "catagen", "parameters": { "extent": "4,3", "seed": "20260829" } }
```

Parameter values are strings. They are the generator's own option vocabulary, ranges like `3-7` and
seeds spanning an unsigned range, so each generator reads them back with the reader it has and a
parameters object reads as a command line. The format's reader uses none of them.

A file that was authored rather than generated omits `generator`.

## layout

The name and properties of the layout as a whole. It is a component object and nothing else.

```json
"layout": { "components": { "name": "The Flooded Cistern", "type": "dungeon", "difficulty": 3, "style": "cistern" } }
```

`name`, `type`, `difficulty` and `style` are components like any other. They are the zone metadata
`sim_zone_model.md` describes, so depth, recipe, encounter and loot tables belong here too.

`level` is optional: a whole number, the zone's position relative to the surface --- 0 at the surface,
-1, -2, ... going down, and 1, 2, ... going up (`region_plan.md`, `## Level`). It is a label, used when
making connections between zones. The reader decodes it out of the layout's components, as it does
`access_points`.

```json
"layout": { "components": { "access_points": [ { "entity": 4 } ], "level": -2 } }
```

`access_points` is required: a list of at least one reference to the relocators at which an arriving
actor can be placed. Every zone is built with them.

```json
"layout": { "components": { "access_points": [ { "entity": 8 }, { "entity": 9 } ] } }
```

Each is a reference to an entity with a `relocator` component, sited `on` a surface: a wall, the floor
or the ceiling. The reader decodes the list out of the layout's components, as it does `site`, `parent`
and `relocator` out of an entity's. The reader rejects a file whose `access_points` is absent, lists nothing, or lists a
value that is not such a reference. The entity's `type` does not make it an access point. An access point on the
floor or the ceiling requires its site's `orientation`, for drawing; an access point on a wall uses the
wall's normal (`### Staircases and access points`).

The JSON defines a zone's access points; the stair glyphs `<` and `>` are visual aids. The reader warns
if they disagree --- a stair cell with no relocator on the walls around it, or a second one, or
an access point on a wall that is neither a stair cell's open side nor a door's wall --- and reads the file as the JSON has
it. Warnings go to the reader's warning callback, which by default writes them to `std::clog`.

A door facing may be an access point, with a relocator of its own, and then leads to another zone. A door between
two zones is two doors, one in each, their relocators bound to each other.

Each access point's relocator states three things:

- **`type`,** what the access point is, a name the world registers: `staircase`, `door`, `portal`, `waypoint`, or one
  a mod adds; whether it goes up, down, or across is the relocator's `target`. The type is normally the entity's own
  `type`, or for a door facing its door's, and the reader warns if it is neither.
- **`order`,** a whole, non-negative number given when it is created. The access points of each type
  are ordered from 0, without a gap, so a zone's staircases share one sequence, up and down alike.
- **`placement`,** where an arriving actor is placed: a `cell_position` in the access point's own zone that an actor
  can stand in, and an `orientation`. A staircase's placement is the floor its open side faces, facing away from it.

```json
"relocator": { "type": "staircase", "target": "+1", "order": 0, "placement": { "cell_position": [1, 1], "orientation": [0, -1] } }
```

The reader rejects an access point with no `type`, no `order`, an order repeated or past a gap within
its type, or no `placement`, or a placement on a cell an actor cannot stand in.

Which access point an arriving actor is placed at depends on where it comes from, so the list does not
choose. Each type is registered with the type it arrives at, its own, and of the access points of that type, the actor
arrives at one that leads back to the level it left: through a `-1`, at a staircase whose relocator's target is `+1`.
Of those, it is the available one with the lowest order; an access point already bound to a relocator elsewhere is
unavailable. A generator lists every staircase it places. Once a layout is read,
its access points are known, and they are what an unbound level resolves to (`### A relocator's target`).

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
| `origin` | The corner of cell `[0, 0]` in the zone. `[0, 0, 0]` if absent |
| `scale` | The width of one cell. `1.0` if absent |
| `encoding` | `"characters"` or `"runs"`. `"characters"` if absent |
| `rows` | The grid, the first row written being the highest |

The first row written is the highest and the grid origin is the lower left, in both notations. The
row count and the width of a row follow from `extent` and the notation.

A space is a glyph. It means no floor at a cell position, no wall at a wall position, and nothing at
a corner, and a reader reads one as it reads any other character. Every row is written to the full
width, trailing spaces included, and a row of any other width is an error.

The rows encode openness. A room does not determine whether an actor may stand somewhere, and neither
does terrain on a flat.

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
is mostly one glyph repeated; a zone small enough to read is written in characters.

### The block notation

One character per cell: `extent.x` characters on each of `extent.y` rows.

| glyph | |
| --- | --- |
| `.` | floor |
| `#` | wall |
| `+` | door |
| `>` | staircase down |
| `<` | entrance: a staircase up, or any other non-local transporter that an actor arrives by |

A wall is a cell here, so a door is a cell too: a block of wall that an actor may stand in. Two areas
never share an edge in this notation, since a block always lies between them.

A wall is a face of a block, on the side of the cell it faces. A floor cell has a wall toward a `#`, a
`+`, a stair cell or the border of the grid; a door cell has one toward a `#`, a stair cell or the border
of the grid, and none toward floor, so an actor inside a doorway meets no wall on the way out. A stair cell is
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
| | `<` | entrance: a staircase up, or any other non-local transporter that an actor arrives by |
| | ` ` | no floor |
| edge | `-` | wall, horizontal |
| | `\|` | wall, vertical |
| | `+` | door |
| | ` ` | no wall |
| corner | ` ` | always |

A position's parity determines which of the three it is.

A corner position is always blank, and so is a cell with no floor. The edges either side of a corner
express everything a corner could, and a solid cell is the absence of floor. A zone draws its floor and the
walls around it, and nothing else.

A wall always separates a floor cell from a blank cell, from a stair cell, and from the border of the grid.
A stair cell is a block with no walls of its own, so nothing is required between one and a blank cell or
the border.

An edge glyph is one character representing both sides, so what it writes is symmetric. A door
open from one side alone, or secret from one side, is components on that side's door facing.

### Staircases and access points

A stair glyph marks where a staircase is drawn: a block, with a wall toward it on every side where there
is floor. The glyphs are visual aids. A zone's staircases are its entities, each with a
`relocator` whose target is where it leads, sited on a surface:

- **On a wall.** The wall is the staircase's open side, the side toward the room it opens into, and the
  staircase's orientation is the wall's normal, the direction of travel from the staircase into the
  room. Interacting with the wall, as with a door, enters the staircase. The other walls around a stair cell are ordinary
  walls with components of their own, such as a texture.
- **On the floor.** Sited on a floor cell, the staircase does not impede movement. Its site's
  `orientation` is required and is used only for drawing. A keystroke by an actor on its cell enters it.

In the wall notation every staircase is sited on a wall, as a door is, including a block staircase in the
open space of a room; a staircase on the floor belongs to the block notation.

A staircase in a flat length of wall in the wall notation is the equivalent of a staircase in a 1x1
alcove in the block notation, the alcove's cell being the stair block. This is informational, and a guide
to generating zones in either notation; it does not preclude a block staircase in the open space of a
room.

`<` marks more than a staircase leading up. It is the entrance glyph, the up exit or entrance point of
any non-local transporter --- a staircase, a waypoint, a portal --- as `+` is the glyph of any threshold
that opens and closes. The object there is the entity sited on it, whatever its `type`.

A stair glyph with no entity with a `relocator` on the walls around it, or a second one, is a warning.
Every door glyph requires one entity with a child sited on each wall of its doorway, and a file missing
that is rejected.

## rooms

A room is an area over the grid, and a room's id is its position in the array. A room has no effect on
passage; its cells are stored and read back.

A room is identified one of two ways, and specifies exactly one of them:

| | |
| --- | --- |
| `perimeter` | A polygon in junction coordinates, within the grid, its sides horizontal or vertical and never crossing or touching one another. Each cell edge along it is a wall or a virtual wall |
| `cell_position` | A floor cell anywhere in the room, the barriers closing it off |

**By a cell in it.** `cell_position` is a cell written `.`, never a doorway, which is what a room is reached
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
{ "cell_position": [3, 1], "components": { "name": "the stair head" } }
```

A cell belongs to at most one room. Two rooms sharing a cell is an error in the file.

## barriers

Barriers are where a room's fill is closed off: an array of positions beside the grid, rather than glyphs
in it.

```json
"barriers": [ { "cell_position": [3, 1], "normal": [1, 0] }, { "cell_position": [7, 4] } ]
```

A cell position with a normal is a barrier across that edge; a cell position alone closes off the whole
cell.

A fill does not cross a barrier, and that is all a barrier does. It is never in the layout: it builds
nothing and blocks nothing, and an actor crosses one with nothing to notice.

What remains is the room's cells, so a perimeter traced from them later runs
along a barrier as it runs along a wall.

The array is layout-wide and every room is filled against all of it, a barrier between two rooms
being one edge both need. A room may also have `barriers` of its own, applied to its own fill
alone, where one room needs an edge another must not have.

## walls

Per-wall data for the walls that have any, with one entry per wall. The grid creates the walls, so an
address of a wall the grid does not have is an error.

```json
{ "wall": { "cell_position": [1, 1], "normal": [-1, 0] }, "components": { "style": "buttressed" } }
```

Surface treatment is per side and every wall is one side, so an entry here needs no field for which side
it means.

## flats

The floor and the ceiling that bound the zone, with one entry each, or only the floor for a zone with
no ceiling. If `flats` is absent, the floor is at height 0 and the ceiling at 1.

```json
{ "role": "floor", "height": 0.0, "components": { "terrain": "flagstone", "style": "cistern" },
  "rooms": [
    { "room": 0, "height": -0.25, "components": { "terrain": "water" } }
  ],
  "cells": [
    { "cell_position": [3, 2], "height": 0.5, "components": { "terrain": "step" } }
  ] }
```

| Key | |
| --- | --- |
| `role` | `"floor"` or `"ceiling"`. The two differ by role and nothing else |
| `height` | Where the surface is across the whole flat |
| `components` | The surface's components across the whole flat |
| `rooms` | Per room: a room id, and what that room overrides |
| `cells` | Per cell: a cell position of the cell grid, and what that cell overrides |

### Height descends with components

A flat states each of the three at whichever level it is uniform over, and a level below overrides
what it specifies:

    flat
      room
        cell

A cell takes its height from the properties specified for it, otherwise from those specified for the
room that contains it, and otherwise from the flat's own. Inherited components resolve the same way,
these being three levels of the hierarchy Inheritance sets out. A sunken room costs one entry.

A flat is sampled at cell scale and has no grid of its own: a height belongs to a cell of the grid
the rows are written on. Generation is expected to work at the room level and leave the cell level
to authored detail.

A hole in the floor is a component. The cell is floor in the grid and the flat has a height there;
what makes it a hole is a component on that cell, or an entity sited there, and the entity a caller
builds from either. A shaft lets an actor through the way a door does, downward rather than across, and
a pit lets one in and need not let it out. What happens at the hole belongs to that entity.

A flat component that affects movement does so as an entity. Openness comes from the rows, so
terrain, rocks and a trap door affect an actor through the entity built from them and that entity's
`Blocking` tag; items and decoration are entities without it. The format does not distinguish
them.

## entities

The zone's entities, in one list with one object per entity, in id order: a door, a staircase, an alcove, a
basin, a button, a torch, the key inside the basin.

```json
{ "id": 1, "type": "door", "components": { "material": "oak" } }
{ "id": 2, "type": "door_facing", "components": { "parent": { "entity": 1 }, "site": { "relation": "on", "wall": { "cell_position": [1, 1], "normal": [-1, 0] } } } }
{ "id": 6, "type": "basin", "components": { "site": { "relation": "on", "flat": "floor", "cell_position": [0, 2] } } }
{ "id": 7, "type": "water", "components": { "parent": { "entity": 6 }, "site": { "relation": "in" } } }
```

| Key | |
| --- | --- |
| `id` | A whole number, unique among the file's entities: a local id in a zone template, a persistent id in a saved zone |
| `type` | A string the format does not restrict, types being prefabs the world has |
| `components` | The entity's file components, optional |

Three file components are defined here, which the reader validates and decodes: `site`, where the
entity is, `parent`, whose child it is, and `relocator`, where it leads (`### A relocator's target`).
Every other file component is content the reader passes to its caller as it was written.

### site

The object the entity is in or on, and where on it.

| Key | |
| --- | --- |
| `relation` | `in` or `on`. Required |
| `wall` | A wall address, if the entity is sited on a wall |
| `flat`, `cell_position` | A flat cell: the flat by role, and a cell position of the cell grid, if the entity is sited on a floor or ceiling |
| `cell_position` alone | A cell of the zone, if the entity is in the zone at that cell, as an actor is |
| `position` | Where on it: along and up on a wall, xy within the cell on a flat, or in the parent's frame if there is no address. Centered if absent |
| `orientation` | A direction, one of the four a cell's sides define |

`wall`, `flat` and a `cell_position` alone are the three addresses, and never more than one. **None is
written if the entity is sited on its parent**, which is what an entity in or on another entity is: a
key in a chest, coins in a pouch, a mug on a table. An entity sited on its parent must have one.

An entity in a cell of the zone stands or rests there rather than being fixed to a surface: an actor, or
anything that moves or is moved. Its relation is `in`, its `orientation` is required, and its cell is
one an actor can stand in, a floor/door cell.

```json
{ "id": 8, "type": "rat", "components": { "site": { "relation": "in", "cell_position": [0, 0], "orientation": [1, 0] } } }
```

An entity is sited on a flat cell and never on a flat as a whole. A flat is the entire floor or
ceiling of the zone; what a torch or a shaft is at is one cell of it.

An entity has at most one `site`. Something that presents more than one side is a parent whose
children each have their own: a door is a `door` entity with a `door_facing` child on each wall of its
doorway. In the wall notation those two walls are the sides of one edge; in the block notation they are
the walls the floor either side has toward the doorway block.

### parent

A reference to the entity this one is a child of.

```json
"parent": { "entity": 1 }
```

A parent does not list its children; they are found on reading, from each child's `parent`. A parent
owns its children, so deleting a door deletes its door facings, and a key in a chest is a child of the
chest and sited `in` it.

Every entity has a place: it is sited on the geometry, has a parent, or is a parent, and every hierarchy
of parents and children is sited on the geometry somewhere in it. Following `parent` from any entity
never leads back to it.

### References

A reference to an entity is an object whose only key is `entity`, written anywhere in a file component.
`parent` is a reference in this form too, so a reader finds every reference by its form alone, without
knowing what a file component means.

```json
{ "id": 5, "type": "lever", "components": { "controls": { "entity": 1 } } }
```

An object with an `entity` key and anything beside it is not a reference. In a template every reference
is an entity's id in the same file; in a saved zone one may point into another zone, and is not checked.

### What the grid and the entities each specify

The grid specifies where and an entity specifies what. A door glyph creates the walls and marks the
crossing; the door entity on those walls sets its material, lock and trap. A stair glyph marks a
staircase; the entity on its open side sets the relocator and so where it leads. An entity may also be
the whole of something the grid has no glyph for.

More than one entity may be sited at one place: a door facing and the lever that works it, a plaque beneath
a torch. The caller resolves each `type`, types such as `staircase`, `waypoint` and `portal` being
prefabs the world has, and the format restricts no value of it.

### A relocator's target

A relocator is a component of an entity that moves an actor --- a staircase, a portal, a waypoint --- and
where it leads is its `target`, bound or unbound.

```json
"relocator": { "target": "-1" }
```

**Bound: the relocator arrived at,** as a reference (`addressing.md`, `### Relocators and access
points`). A target is bound by having been traveled: until a way has been gone through, nothing has
fixed where it arrives. A zone long since generated still has access points nobody has used, and
every one of them is unbound. A zone template's relocators are all unbound; a bound target
is written only in a saved zone, `"target": { "entity": 90412 }`.

**Unbound: a string, `[region/]zone[@position]`.** The region and the position are optional.

```
target   = [region "/"] zone ["@" position]
region   = name
zone     = relative | absolute | name
relative = ("+" | "-") digits
absolute = "=" ["-"] digits
name     = letter {letter | digit | "_"}
position = one or more characters other than "/" and "@"
```

| Target | Destination |
| --- | --- |
| `-1` | One level down, from the relocator's region |
| `+2` | Two levels up |
| `+0` | Another zone at the same level |
| `=0` | Level 0 |
| `=-3` | Level -3 |
| `sewers/=-1` | Level -1 of the region `sewers` |
| `town/market@fountain` | The zone `market` in the region `town`, at the position `fountain` |

A relative zone is a number of levels from the relocator's zone (`## layout`), and always has its sign. An absolute
zone is a level, marked with `=`. A named zone is a zone of pregenerated content, and the position a place in it.
An omitted region is the relocator's own region, from which the
caller's rules for moving down, up, and across apply, and a written region is the only one searched. Region names,
zone names, and positions are the caller's vocabulary.

Resolving one finds a zone or makes one. Traversal passes the target to whatever owns zones, which
returns a zone it has or generates one, and chooses the cell an arriving actor is placed in. What it
resolves to is written back as a bound target, so leaving and returning arrive at the same place.

A level resolves to a zone at that level (`region_plan.md`), so it is a real target as soon as that zone is generated
or loaded. An actor arriving through a relocator that is not yet bound is placed at one of that zone's unbound access
points that leads back --- through a `-1`, at one whose relocator's target is `+1`, or `=`*L* for the level *L* left
--- and going through binds both ends: the relocator gone through is bound to that access point, and the access
point to the relocator it came from. The layout's `access_points` lists them, and every zone has at least one.

The reader decodes a relocator: its `target`, required, a string that matches the grammar or a reference, and the
`type`, `order` and `placement` an access point requires. It keeps a region, a zone name, and a position as written,
and the writer writes a target back as the same string. A bound target is a reference, and the reader rejects one in
a template.

## What a reader must do with it

Load and validate the geometry, and hand back everything else as the file wrote it. What was read,
written back, is equivalent to the file and complete, if not identical to it.
A reader has no world to build entities in, so where the file puts a door it reports the door rather
than placing one, and the same applies to every component object and every entity.

A key the format does not define is not tracked: inside content it is kept as written, and anywhere
else it is ignored. A change to the structure of the geometry is a new version, which a reader rejects.

It resolves one structure, the file's own rather than content: a room, into its cells, however the
file identified it. An inherited component is content, and its chain is the consumer's to resolve.

`discrete_layout_plan.md` has the structures and the passes.

## Errors

A file breaking any of these is rejected, stating what was wrong and where. Nothing is built until
every check has passed, so a rejected file leaves nothing behind.

- The file cannot be opened, or is not valid JSON.
- A key the format requires is absent, or a value is not the JSON type the format sets: an object, an
  array, a string or a number. Content is checked only for being an object.
- A coordinate is not a whole number, or is outside the coordinate range.
- `format` is absent or is not this format; `version` is one the reader does not read.
- `zone.id` is not a whole, non-negative number.
- `layout.components.level` is not a whole number.
- `cells` is absent, its notation or its encoding is not one of the two it offers, its extent has no
  cells, its scale is not positive, it has the wrong number of rows, or a row is not the width its
  extent sets.
- A run count of zero, or a count with no glyph after it.
- A glyph the notation does not define, or one at a position that does not take it.
- In the wall notation, a floor cell with no wall between it and a blank cell, a stair cell, or the
  border of the grid.
- A normal or an orientation is not one of the four directions a cell's sides define.
- A room specifies both `perimeter` and `cell_position`, or neither.
- A room's cell is not a floor cell, or is a cell closed off by a barrier.
- A room's perimeter has fewer than four junctions, a junction off the grid, a side neither
  horizontal nor vertical, or two sides that cross or touch.
- A cell edge along a perimeter is neither a wall facing a floor/door cell nor an edge between two
  floor/door cells.
- Two rooms share a cell.
- A barrier is at a position off the grid.
- A wall address has no wall in the grid, or its cell is not a floor/door cell.
- More than one entry in `walls` for one wall.
- A flat refers to a room that does not exist, or to a cell off the grid, or has more than one entry
  for one room or one cell.
- More than one flat in a role.
- `entities` is not a list, or an entity in it is not an object.
- An entity's id is not a whole, non-negative number, or two entities have one id.
- An entity has no `type`, or its `type` is not a string.
- A `site` has no `relation` or one the format does not define, gives both a `wall` and a `flat`, or has
  a `position` that is not two numbers.
- A `parent` is not a reference.
- A `site` has no address and the entity has no parent.
- A `site` in a cell of the zone has a relation other than `in`, has no `orientation`, or is in a cell off the
  grid or one an actor cannot stand in.
- An entity is not sited on the geometry, is no entity's child and is no entity's parent; or a hierarchy
  of parents and children is sited on the geometry nowhere in it.
- Following `parent` references from an entity leads back to it.
- In a template, a reference is not an entity's id.
- A door glyph whose doorway has no one entity with a child sited on each of its walls, or has more
  than one; or a door with a child sited on a wall outside its doorway.
- `layout.components.access_points` is absent, is not a list, or lists nothing; or a value it lists is
  not a reference to an entity with a relocator sited on a surface.
- An access point on the floor or the ceiling has no site `orientation`.
- An access point's relocator has no `type`, no `order`, an order repeated or past a gap within its type,
  or no `placement`, or a placement on a cell an actor cannot stand in.
- A relocator has no `target`, or one that is neither a string nor a reference; a string that does not match
  `[region/]zone[@position]`; or a template has a bound target.
- `layout.components.level` is not a whole number.

A stair glyph with no entity with a relocator on the walls its open neighbors present to it, or with a
second one, an access point on a wall that is neither a stair cell's open side nor a door's wall, and an access
point's relocator type that is neither its entity's type nor its parent's, are warnings, not errors.
