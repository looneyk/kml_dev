# Ids and Addressing

This document specifies how stratum identifies places and entities, and how one refers to another. `discrete_layout.md` specifies the rest of
the zone file format, `addressing_plan.md` is the detailed design of what is not built yet, and `decision_rationale.md`
explains the decisions.

This document describes format version 0.4, which supersedes 0.3. The changes from 0.3 are listed in
the appendix.

The document has four parts:

1. **Places and entities** describes what is addressed, and what each is.
2. **In a file** describes how a zone file writes them.
3. **In a layout** describes the ids a layout encodes places as.
4. **In the world** describes the data structures a loaded zone has, and how lookups work.

Date: 2026-09-14, revised 2026-09-26

## Implementation status

| | |
| --- | --- |
| Cell positions, junctions, positions, wall addresses, flat cells, rooms and flats in a file | Built, in the 0.4 form |
| Wall ids, cell slots, room ids and flat ids in a layout | Built: `DiscreteLayout` |
| One list of entities in a file, with `site` and `parent` file components | Built in `stratum_format`, both generators, and the loader |
| Runtime entities, and migration between zones keeping them | Built |
| Stair cells as blocks, and staircases placed where their blocks cut no passage | Built, with the staircase sited on the wall on its open side |
| Persistent ids, the id allocator and the world index | Built |
| Links in both directions between an entity and the place it is sited on; location maps | Built |
| References between entities | Built |
| Relocator targets bound to relocators | Built |
| Saved zones | Built |
| A placement index for each zone | Built |

## Every identifier at a glance

| What | In a file | In a layout | In the world |
| --- | --- | --- | --- |
| A position | `[x, y, z]` | a point | a point |
| A cell | its cell position, `[i, j]` | its slot | its entity, if it has one |
| A junction | `[u, v]`, in a room's perimeter only | not stored | not stored |
| A wall | its address: a cell position and a normal | its wall id | its entity, if it has one |
| A flat | its role, `"floor"` or `"ceiling"` | its flat id | its entity |
| A flat cell | its flat's role and a cell position | its flat id and the cell's slot | its entity, if it has one |
| A room | its position in `rooms` | its room id | its entity |
| An entity | its local id, or its persistent id in a saved zone | not stored | its persistent id, and its runtime entity while loaded |
| A zone | its persistent id, in a saved zone only | not stored | its persistent id, and its runtime entity while loaded |

## Part 1: Places and entities

### Places

A place is part of a zone's geometry: a cell, a wall, a flat cell or a room. Places are not entities.
They have no entity ids and are never saved as objects. A file refers to a place by its address, and a
layout by the id it encodes the address as.

**Cells and junctions.** A cell is one square of the grid, and a cell position is where a cell is. A
junction is a corner where cells meet.

**Edges and walls.** An edge is the boundary between two cells. The wall notation writes an edge as one
glyph for both sides. A wall is one side of an edge, and it belongs to the cell on that side: its surface
is toward that cell, and an actor in the cell sees that side. The two sides of an edge are two separate
walls, one belonging to each cell, so a room's walls are the sides of its own cells, and a room of a
single cell has all four of its walls on that cell. An actor interacts only with the wall on its own
side of an edge. Everything is per wall: surface treatment, what is sited on it, a door facing.

A wall exists only where open space meets something. The side of an edge toward a solid cell has no
wall.

**Stair cells.** A stair cell is a block with no wall of its own: each open neighbor
presents a wall to it. The staircase is sited on the wall on its open side, the side toward the room
it opens into. Its other sides are ordinary walls, each with its own surface treatment.

**Flats and flat cells.** A zone is bounded by two flats, a floor and a ceiling. A flat cell is one cell
of one flat. Anything on a floor or ceiling is on a flat cell, never on the flat as a whole.

**Rooms.** A room is a set of cells.

**Directions.** A direction on the grid is one of the four directions defined by a cell's sides:
`[1, 0]`, `[0, 1]`, `[-1, 0]` and `[0, -1]`. A wall's normal is a direction, and so is an entity's
orientation. The normal of the wall a staircase is sited on is the direction of travel from the
staircase into the room, so a staircase has no orientation of its own.

**Places depend on the layout.** Cells, walls and flat cells are `DiscreteLayout`'s kinds of place. A
`ContinuousLayout` will have places of its own kind, and an entity is sited on a place as its zone's
layout defines one.

### Entities

An entity is anything with identity or physical presence: a door, a door facing, a staircase, a
torch, a key, a monster. Topology, heights and surface appearance are geometry data, not entities
(`sim_zone_model.md`, `## Entities and zone data`).

**Everything has a site.** A door facing is sited on a wall of its doorway, a torch on a wall, a chest
on a floor cell, and a key on the floor cell it lies on or in the chest it is inside. An entity has at
most one site, and its relation is `in` or `on`. A site whose address is a wall or a flat cell is on the
zone's geometry; a site with no address is on the entity's parent, which is what a key in a chest is.

**Passage comes from what is sited on a wall.** Walls are barriers to movement and define the borders of
rooms. An actor can cross a wall when an entity sited on it can be crossed, such as the door facing of an
open door. The same goes for a staircase: an actor moves into one through its open side, which the
staircase sited there lets it cross.

**Children are entities that share the same parent.** A door's children are its door facings, one on
each wall of its doorway. Each door facing is sited on its own wall, and has what is reached from its
side, such as concealment, or a keyhole or a knocker sited on it. The door itself has what both sides
share, such as whether it is open or locked. Anything presenting more than one side is made of children
in the same way. A parent owns its children: deleting a door deletes its door facings.

**Contents are children.** A key in a chest is a child of the chest, sited `in` it, and travels with it.
Taking the key out is a change of parent and of site together.

**Every entity has a place.** It is sited on the geometry, has a parent, or is a parent, and every
hierarchy of parents and children is sited on the geometry somewhere in it.

**An entity can refer to another.** A lever refers to the door it controls, a summoned monster to its
summoner, a quest to its object. A generator makes these references when it creates both entities: it
places a lever beside a door and writes the door's id into the lever. Script decides what a reference
does, such as a triggered script that opens the door a lever controls.

**A link to something that does not exist yet uses a shared value, not a reference.** A key in one
zone for a door in a zone not yet generated has no id to refer to. The key and the lock have the same
lock value, and they match whenever both exist.

### Zone files

A zone file is a template, written by a generator or a tool, or a saved zone with its state at the time
of saving.

### Ids

An entity has three ids over its life, and they are never interchangeable.

- **Its local id** is its id in a zone file that is a template. Local ids are unique within the file and
  assigned in the order entities are written, so a generator writes the same ids for the same
  parameters. A local id is mapped to a persistent id on loading, so a zone loaded twice receives
  distinct persistent ids for each local id, resulting in two instances.
- **Its persistent id** is its identity in a world, from when it is made until it is destroyed. Loading a
  template gives each entity a new persistent id, and an entity made during play gets one when it is
  made. Moving to another zone keeps it, and saving writes it. A zone is an entity and has one too.
- **Its runtime entity** is the `flecs::entity` it is while its zone is loaded. Flecs assigns and recycles
  these, so the same entity is a different runtime entity after its zone is unloaded and loaded again.
  It is never written.

Layout ids --- wall ids, slots, room ids and flat ids --- identify places, and only within one layout.

## Part 2: In a file

### Places

**Cell positions and positions.** A cell position is `[i, j]`: `i` cells east and `j` cells north of the
grid's corner, in either notation. A position in the zone is `[x, y, z]`, with the grid in the xy-plane
and z as height.

**Junctions.** Junction `[u, v]` is the lower-left (SW) corner of the cell at `[u, v]`. Only a room's
perimeter uses junctions, because a perimeter runs between cells.

**Wall addresses.** A wall address is a cell position and a normal, written as two separate values:

```json
"wall": { "cell_position": [1, 1], "normal": [-1, 0] }
```

The cell position is the cell the wall belongs to, and the normal points out of the wall's surface,
into that cell. So the wall lies on the side of the cell the normal points away from, on the edge
between that cell and its neighbor at `cell_position - normal`. The normal is a unit normal vector
in one of the cardinal directions.

The wall on the other side of the same edge belongs to the neighbor and has an address of its own, with
that neighbor's cell position and the opposite normal. A door on the edge between the cells at `[1, 1]`
and `[2, 1]` is on two walls: `{ "cell_position": [1, 1], "normal": [-1, 0] }` and
`{ "cell_position": [2, 1], "normal": [1, 0] }`.

The cell at an address's cell position is a floor or door cell; door cells exist only in the block
notation. Solid cells and stair cells have no walls, a wall existing only on the open side of an edge, so
an address can never be written from their side. Where only the edge matters, as for a barrier, the
address from either side identifies it. (We may need to review this to make sure it is consistent
with neighbor walls.)

**Flat cells.** A flat cell is written as its flat's role and a cell position:
`"flat": "floor", "cell_position": [i, j]`.

**Rooms.** A room's id is its position in the file's `rooms` array.

A file never contains a wall id or any other layout id.

### Entities

A zone's entities are one list. Each has an `id`, a `type` and `components`. How an entity relates to a
place or another entity is written as a file component. Two file components are defined in the format,
which validates them and uses them to build the zone: `site` and `parent`. The reader does not
interpret any other file component.

**`site`** is where the entity is: a relation, `in` or `on`; an address, which is a wall or a flat cell if
the entity is sited on the zone's geometry, and is absent if it is sited on its parent; and a position
and orientation on that, if it needs them.

**`parent`** is a reference to the entity this one is a child of. A parent does not list its children in
the file; they are found on reading, from each child's `parent`.

**A reference** to another entity is written `{ "entity": id }`, anywhere in a file component. `parent`
is a reference in this form too. Loading finds every reference by this form, without interpreting the
file component.

```json
"entities": [
  { "id": 1, "type": "door", "components": { "material": "oak" } },
  { "id": 2, "type": "door_facing", "components": { "parent": { "entity": 1 }, "site": { "relation": "on", "wall": { "cell_position": [1, 1], "normal": [-1, 0] } } } },
  { "id": 3, "type": "door_facing", "components": { "parent": { "entity": 1 }, "site": { "relation": "on", "wall": { "cell_position": [2, 1], "normal": [1, 0] } }, "concealed": true } },
  { "id": 4, "type": "keyhole", "components": { "parent": { "entity": 2 }, "site": { "relation": "on" } } },
  { "id": 5, "type": "lever", "components": { "site": { "relation": "on", "wall": { "cell_position": [1, 2], "normal": [0, -1] }, "position": [0.5, 1.2] }, "controls": { "entity": 1 } } },
  { "id": 6, "type": "chest", "components": { "site": { "relation": "on", "flat": "floor", "cell_position": [0, 2] } } },
  { "id": 7, "type": "key", "components": { "parent": { "entity": 6 }, "site": { "relation": "in" }, "lock": "vault" } },
  { "id": 8, "type": "staircase", "components": { "site": { "relation": "on", "wall": { "cell_position": [3, 1], "normal": [0, -1] } }, "relocator": { "target": "-1" } } }
]
```

`discrete_layout.md` (`## entities`) has the full set of keys, the rules the reader checks, and what door and stair
glyphs require.

### Relocators and access points

A relocator is the component of an entity, such as a staircase, a portal or a waypoint, that moves an
actor (`discrete_layout.md`, `### A relocator's target`). Its target is unbound or bound:

- **Unbound:** a string, `[region/]zone[@position]`, such as `-1` for the next level down, `=0` for level 0, or
  `town/market@fountain` for a place in pregenerated content. Every relocator in a template is unbound.
- **Bound:** a reference to the relocator it arrives at. A relocator becomes bound when it is first gone
  through, and both relocators are then bound to each other. Bound targets appear only in saved zones.

A zone's **access points** are the relocators an arriving actor can be placed at; a zone has at least
one. `layout.components.access_points` is a list of references to them. Each is an entity with a
relocator, sited on the open side of a cell written `<` or `>`.

### A saved zone

A saved zone is written in the same format, with the exception that the former local ids are replaced
with persistent ids. That includes references, so a reference can point to an entity in another zone.
The file also has the zone's own persistent id, which a template does not have.

A zone saves its geometry, the file components of its places, and its entities with all of theirs. Each fact is saved once, in the place it comes from. That a chest lies on a flat cell is saved in
the chest's `site`, and a wall's style in the zone's `walls` section. Nothing derived is saved: not
layout entities, not the lists of what is sited on something, not a parent's children, not the
placement index. They are all rebuilt when the zone loads. The world separately saves its id allocator's
count and its index of which zone each entity that something refers to is in.

## Part 3: In a layout

**Slots.** A cell's slot is its index in the layout's table of cells: `j * nx + i` for the cell at
`[i, j]` in a grid `nx` cells wide (`DiscreteLayout::slot`).

**Wall ids.** A wall id is a wall address encoded by the layout, from the cell's slot and the wall's
direction. Today that is the slot times four, plus the index of the direction from the cell to the
wall among east, north, west and south, and `load_layout` gets it by passing the negated normal to
`DiscreteLayout::wall_toward`. `addressing_plan.md` has the change to encoding from the normal directly.

Encoding needs the grid's extent, so wall ids exist only once there is a layout. A wall id stays valid
without anything stored for it, as long as the grid it was encoded against does not change. Changing
that grid would silently point every stored wall id at a different wall. Mining does not change the
grid: a minable cell is a phantom cell carved out at generation, which mining reveals.

**Flat ids.** The layout identifies its floor and ceiling by flat id (`DiscreteLayout::floor_id()`,
`ceiling_id()`). A flat cell is its flat id and its cell's slot.

**Room ids.** Each cell has the `room_id` of the room it is a part of, or `nullid` if it is not part of
any room.

## Part 4: In the world

### Entities and their ids

**Persistent ids.**

- Their type is `PersistentId`, used everywhere a persistent id is kept: the component on an
  entity, references, the world index, and saves. It is 64 bits.
- A world has one allocator, and it is the only source of persistent ids. An id from the
  allocator is saved with the world, and the restored allocator continues from it.
- No code relies on ids being ordered, dense or small, or reads bits out of one, except the allocator
  that created it.
- A missing id is `std::optional`, not a reserved value.

**Runtime entities.** A loaded zone uses runtime entities for everything it does. Persistent ids are
used only when a zone is loaded, saved or unloaded, and when a reference is resolved. The world keeps a
map from persistent id to runtime entity for everything loaded.

**Migration.** Moving an entity between two loaded zones keeps its runtime entity, because the move is
only a change of parent zone (`sim_zone_model.md`, `### Entity ownership and migration`).

### Sites

**Links run both ways.** A sited entity has its address, and the place at that address has a list of
everything sited on it. Neither direction needs a search.

**Places get entities only when needed.** A wall, flat cell or room has an entity only if it has
components or something sited on it. Each zone has maps from wall id to wall entity, and from flat id
and slot to flat cell entity.

**Lookups.**

- **What is on this wall or flat cell?** Look up the wall's or cell's entity in the zone's index, then
  read its list.
- **What is in that chest?** Read the chest's children, keeping those sited `in` it.
- **Where is an entity?** Read its `site`, and its parent if the site has no address.
- **Can an actor cross a wall?** Check each entity on the wall's list for open access that is not
  concealed from the actor, reading each property from the entity or, if it has none, from its
  nearest ancestor that has one. An entity with no access at all, a torch or a lever, is no way through. `crossable` has
  no case for a door, a staircase or concealment.

### Children

A file's `parent` makes the entity a Flecs child of that parent, so Flecs keeps each parent's children
and nothing here lists them. A door and its door facings are one subtree: deleting the door deletes
them, and reading a door's door facings, or the other side of a door, is reading its children.

A zone's entities are children of the zone, except the ones that are children of another entity, which
belong to the zone through their parent. So **a zone's contents are its subtree, not its children**: a
query for the zone's children finds what is placed directly in it, which is what occupancy and the view
query, and whatever has to reach all of a zone --- saving it, unloading it, dumping it --- walks the
subtree instead. Deleting the zone already reaches all of it. The character is in a zone without being in
its subtree (`### The character`).

### References across zones

**A reference stores a persistent id,** because the entity it refers to may be in another zone, which
may not be loaded. The world keeps an index of which zone each entity that something refers to is in,
updated whenever such an entity moves between zones, and resolves references through it. An entity
that nothing refers to is not in the index. A dropped object is indexed in its zone, as everything
sited on a place is, and is accessible while the actor is in that zone, but not from another zone
the actor moves to. A reference also caches the runtime entity it last resolved to, and resolves
again once that runtime entity is gone.

**Bound relocator targets** are references, so arriving through one finds the target relocator and its
zone through the index.

### Actors

An actor, such as the character or a denizen, is `In` its zone, its site having a placement --- a position and a
direction --- and no address.

**Each zone has a placement index,** which finds the actors, and anything else placed in a cell of the zone, at a
place with a lookup. The index is derived from the placements: it is updated as they move and rebuilt when the zone
loads, so it cannot disagree with them. Each type of layout supplies its own type of index, and a `DiscreteLayout`'s
finds entities by cell.

### The character

The character is in a zone, and is owned by the game. It is in its zone's placement index and
moves between zones, but it is not in the zone's subtree, so unloading or saving the
zone leaves it out. What the character carries is a child of it, sited `on` it, and is owned by the
game too. Both are saved with the game.

`addressing_plan.md` has the structures and components for all of this.

## Appendix: Changes from 0.3

- **Entities are one list.** 0.3 writes an `attachments` section, each entry a place with the entities
  attached there nested inside it. 0.4 writes one `entities` list, and where an entity is is its `site`
  file component.
- **Entities have ids.** 0.3 entities have none.
- **A relation is written.** 0.3 implies it from the place.
- **A door is a parent with door facings as children.** 0.3 writes a door as one entry listing both
  walls.
- **A staircase is sited on the wall on its open side.** 0.3 attaches it to its stair cell's floor,
  with a `facing` file component for the open side.
- **Contents are children, sited on their parent.** 0.3 nests an entity's contents in its `contains`,
  which goes.
- **Wall addresses are a cell position and a normal, written separately.** 0.3 writes one array,
  `[i, j, ni, nj]`, in `walls`, `attachments` and `barriers`.
- **Every cell position is written `cell_position`.** 0.3 writes `cell` in flat cells and in the flats'
  per-cell entries, `location` in rooms, and a bare `[i, j]` in barriers.
- **Access points replace the entry, and are references.** 0.3's `layout.components.entry` is a list of
  cell positions; 0.4's `layout.components.access_points` is a list of references to relocators.
- **A bound relocator target is a reference.** 0.3 specifies a zone name and a cell.
- **A saved zone has persistent ids and its own zone id.** 0.3 has no saved zone.
