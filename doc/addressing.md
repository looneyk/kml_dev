# Ids and Addressing

How stratum identifies places and entities, and how one refers to another. This is the one place
that is specified. `discrete_layout.md` is the rest of the zone file format, `addressing_plan.md` is
the detailed design of what is not built yet, and `decision_rationale.md` explains the decisions.

This document describes format version 0.4, which supersedes 0.3. The changes from 0.3 are listed in
the appendix.

The document has four parts:

1. **What is addressed:** places and entities, and what each is.
2. **In a file:** how a zone file writes them.
3. **In a layout:** the ids a layout encodes places as.
4. **In the world:** the data structures a loaded zone has, and how lookups work.

Date: 2026-09-14, revised 2026-09-16

## What is built

| | |
| --- | --- |
| Cell positions, junctions, positions, wall addresses, flat cells, rooms and flats in a file | Built, in the form the appendix lists as 0.3's |
| Wall ids, cell slots, room ids and flat ids in a layout | Built: `DiscreteLayout` |
| Entities attached to places | Built in the 0.3 form in the format and both generators; nothing in `stratum_sim` reads them yet |
| Runtime entities, and migration between zones keeping them | Built |
| One list of entities in a file, with `attached` and `parent` file components | Designed |
| Persistent ids, the id allocator and the world index | Designed |
| Links in both directions between an entity and what it is attached to; location maps | Designed |
| References between entities, and relocator targets that are entities | Designed |
| An actor index for each zone | Designed |

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

## Part 1: What is addressed

### Places

A place is part of a zone's geometry: a cell, a wall, a flat cell or a room. Places are not entities.
They have no entity ids and are never saved as objects. A file refers to a place by its address, and a
layout by the id it encodes the address as.

**Cells and junctions.** A cell is one square of the grid, and a cell position is where a cell is. A
junction is a corner where cells meet.

**Edges and walls.** An edge is the boundary between two cells. The wall notation writes an edge as one
glyph covering both sides. A wall is one side of an edge, and it belongs to the cell on that side: its
surface faces into that cell, and it is that side that an actor in the cell sees. The two sides of an
edge are two separate walls, one belonging to each cell. So a room's walls are the sides of its own
cells, and a room of a single cell has all four of its walls on that cell. An actor can
always tell which wall it can interact with. The wall on the other side of the same edge is out of its
reach until the actor is on that side. Everything is per wall: surface treatment, what is attached, a
door facing.

A wall exists only where open space meets something. The side of an edge toward a solid cell has no
wall.

**Stair cells.** A stair cell is a block, and like a solid cell it owns no wall: each open neighbor
presents a wall to it. The staircase is attached to the wall on its open side, the side facing the room
it opens into. Its other sides are ordinary walls, each with its own surface treatment.

**Flats and flat cells.** A zone is bounded by two flats, a floor and a ceiling. A flat cell is one cell
of one flat. Anything on a floor or ceiling is on a flat cell, never on the flat as a whole.

**Rooms.** A room is a set of cells.

**Directions.** A direction on the grid is one of the four directions defined by a cell's sides:
`[1, 0]`, `[0, 1]`, `[-1, 0]` and `[0, -1]`. A wall's normal is a direction, and so is an entity's
orientation. The normal of the wall a staircase is attached to is the direction of travel from the
staircase into the room, so a staircase has no orientation of its own.

**Places depend on the layout.** Cells, walls and flat cells are `DiscreteLayout`'s kinds of place. A
`ContinuousLayout` will have places of its own kind, and an entity is attached to a place as its zone's
layout defines one.

### Entities

An entity is anything with identity or physical presence: a door, a door facing, a staircase, a
torch, a key, a monster. Topology, heights and surface appearance are geometry data, not entities
(`sim_zone_model.md`, `## Entities and level data`).

**Everything is attached to something.** A door facing is attached to a wall of its doorway, a torch to
a wall, a chest to a floor cell, and a key to the floor cell it lies on or to the chest it is inside. An
entity is attached to at most one target, which is a place or another entity. Where an entity can be
attached to a target in more than one way, a relationship distinguishes them, such as `contained_in` for
a key inside a chest rather than `on` it. Relationships are optional, and the set of them is the game's
vocabulary, as entity types are.

**Passage comes from what is attached.** Walls are barriers to movement and define the borders of rooms.
An actor can cross a wall when an entity attached to it can be crossed, such as the door facing of an
open door. The same goes for a staircase: an actor moves into one through its open side, which the
staircase attached there lets it cross.

**Children are entities that share the same parent.** A door's children are its door facings, one on
each wall of its doorway. Each door facing is attached to its own wall, and has what is reached from its
side, such as concealment, or a keyhole or a knocker attached to it. The door itself has what both sides
share, such as whether it is open or locked. Anything attached to more than one target is made of
children in the same way. A parent owns its children: deleting a door deletes its door facings.

**Contents are attached, not children.** A key in a chest is attached to the chest, `contained_in`. It
can be taken out, and the chest does not own it.

**Every entity has a place.** It is attached, has a parent, or is a parent, and every hierarchy of
parents and children has an attached entity in it.

**An entity can refer to another.** A lever refers to the door it controls, a summoned monster to its
summoner, a quest to its object. A generator makes these references while both entities are in front
of it: it places a lever beside a door and gives the lever the door's id. What a reference does is up
to script, such as a triggered script that opens the door a lever controls.

**A link to something that does not exist yet uses a shared value, not a reference.** A key in one
zone for a door in a zone not yet generated has no id to refer to. The key and the lock carry the same
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
notation. Solid cells and stair cells own no walls, a wall existing only on the open side of an edge, so
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
which validates them and uses them to build the zone: `attached` and `parent`. The reader does not
interpret any other file component.

**`attached`** has one target --- a wall address, a flat cell, or a reference to an entity --- and a
relationship where one is needed. It has a position and orientation on the target where the attachment
needs them.

**`parent`** is a reference to the entity this one is a child of. A parent does not list its children in
the file; they are found on reading, from each child's `parent`.

**A reference** to another entity is written `{ "entity": id }`, anywhere in a file component. `attached`
targets that are entities and `parent` are references in this form too. Loading finds every reference
by this form, without needing to know what the file component means.

```json
"entities": [
  { "id": 1, "type": "door", "components": { "material": "oak" } },
  { "id": 2, "type": "door_facing", "components": { "parent": { "entity": 1 }, "attached": { "wall": { "cell_position": [1, 1], "normal": [-1, 0] } } } },
  { "id": 3, "type": "door_facing", "components": { "parent": { "entity": 1 }, "attached": { "wall": { "cell_position": [2, 1], "normal": [1, 0] } }, "concealed": true } },
  { "id": 4, "type": "keyhole", "components": { "attached": { "target": { "entity": 2 }, "relationship": "on" } } },
  { "id": 5, "type": "lever", "components": { "attached": { "wall": { "cell_position": [1, 2], "normal": [0, -1] }, "position": [0.5, 1.2] }, "controls": { "entity": 1 } } },
  { "id": 6, "type": "chest", "components": { "attached": { "flat": "floor", "cell_position": [0, 2] } } },
  { "id": 7, "type": "key", "components": { "attached": { "target": { "entity": 6 }, "relationship": "contained_in" }, "lock": "vault" } },
  { "id": 8, "type": "staircase", "components": { "attached": { "wall": { "cell_position": [3, 1], "normal": [0, -1] } }, "relocator": { "target": "level_down" } } }
]
```

`addressing_plan.md` has the full set of keys, the rules the reader checks, what door and stair glyphs
require, and the design choices still open.

### Relocators and access points

A relocator is the component of an entity, such as a staircase, a portal or a waypoint, that moves an
actor (`discrete_layout.md`, `### A relocator's target`). Its target is one of two kinds:

- **Unbound:** a destination, such as `level_down`, the surface town, or a place a quest specifies.
  Every relocator in a template is unbound.
- **Bound:** a reference to the relocator it arrives at. A relocator becomes bound when it is first gone
  through, and both relocators are then bound to each other. Bound targets appear only in saved zones.

A zone's **access points** are the relocators an arriving actor can be placed at; a zone has at least
one. `layout.components.access_points` is a list of references to them. Each is an entity with a
relocator, attached to the open side of a cell written `<` or `>`.

### A saved zone

A saved zone is written in the same format, with the exception that the former local ids are replaced
with persistent ids. That includes references, so a reference can point to an entity in another zone.
The file also has the zone's own persistent id, which a template does not have.

What a zone saves is its geometry, the file components of its places, and its entities with all of
theirs. Each fact is saved once, in the place it comes from. That a chest lies on a flat cell is saved in
the chest's `attached`, and a wall's style in the zone's `walls` section. Nothing derived is saved: not
places' entities, not the lists of what is attached to something, not a parent's children, not the
actor index. They are all rebuilt when the zone loads. The world separately saves its id allocator's
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

- Their type is `persistent_id_type`, used everywhere a persistent id is kept: the component on an
  entity, references, the world index, and saves. It is 64 bits.
- A world has one allocator, and it is the only thing that creates persistent ids. The allocator's
  count is saved with the world.
- No code relies on ids being ordered, dense or small, or reads bits out of one, except the allocator
  that created it.
- A missing id is `std::optional`, not a reserved value.

**Runtime entities.** A loaded zone uses runtime entities for everything it does. Persistent ids are
used only when a zone is loaded, saved or unloaded, and when a reference is resolved. The world keeps a
map from persistent id to runtime entity for everything loaded.

**Migration.** Moving an entity between two loaded zones keeps its runtime entity, because the move is
only a change of parent zone (`sim_zone_model.md`, `### Entity ownership and migration`).

### Attachment

**Links run both ways.** An attached entity has its target, and its relationship where it has one. The
target has a list of everything attached to it. Neither direction needs a search.

**Places get entities only when needed.** A wall, flat cell or room has an entity only if it has
components or something attached to it. Each zone has maps from wall id to wall entity, and from flat id
and slot to flat cell entity.

**Lookups.**

- **What is on this wall or flat cell:** look up the place's entity in the zone's map, then read its
  list.
- **What is in that chest:** read the chest's list, keeping those attached `contained_in`.
- **Where an entity is:** read its `attached`.
- **Whether a wall can be crossed:** ask each entity on the wall's list whether its properties let the
  actor through.
  `crossable` knows nothing about doors or concealment; each entity decides for itself.

### Children

A file's `parent` makes the entity a Flecs child of that parent, so Flecs keeps each parent's children
and nothing here lists them. A door and its door facings are one subtree: deleting the door deletes
them, and reading a door's door facings, or the other side of a door, is reading its children.

A zone's entities are children of the zone, except the ones that are children of another entity, which
belong to the zone through their parent. So **a zone's contents are its subtree, not its children**: a
query for the zone's children finds what is placed directly in it, which is what occupancy and the view
ask for, and whatever has to cover a zone entirely --- saving it, unloading it, dumping it --- walks the
subtree instead. Deleting the zone already reaches all of it. The player is in a zone without being in
its subtree (`### The player`).

### References across zones

**A reference stores a persistent id,** because the entity it refers to may be in another zone, which
may not be loaded. The world keeps an index of which zone each entity that something refers to is in,
updated whenever such an entity moves between zones, and resolves references through it. An entity
that nothing refers to is not in the index. A dropped object is indexed in its zone, as everything
attached to a place is, and is accessible while the actor is in that zone, but not from another zone
the actor moves to. A reference also caches the runtime entity it last resolved to, and resolves
again once that runtime entity is gone.

**Bound relocator targets** are references, so arriving through one finds the target relocator and its
zone through the index.

### Ids created in more than one place

One allocator per world is enough for a single player, and for a game hosted by one server, since the
server is then the only place ids are created. A client that predicts a spawn uses a temporary id,
which the server replaces. Several servers allocating at the same time can split the 64 bits between
which server created an id and that server's own count. Where there is no coordination at all --- peer
to peer, merging worlds saved separately, carrying an entity from one save to another --- UUIDs are
needed, either as `persistent_id_type` itself or only at the boundary between worlds, mapped there to
each world's own ids. The rules for persistent ids above are what keep each of these a change to the
allocator or the id type, and not to the code that uses ids.

### Actors

An actor, such as the player or a monster, is located by its `placement_type`: a position and a
direction. An actor is not attached to what it stands on.

**Each zone has an actor index,** which finds the actors at a place with a lookup. The index is derived
from the actors' placements: it is updated as they move and rebuilt when the zone loads, so it cannot
disagree with them. Each kind of layout supplies its own kind of index, and a `DiscreteLayout`'s finds
actors by cell. Until the index is built, `World::entities_at` checks every placed entity in the zone.

### The player

The player is in a zone, and owned by the session. It is in its zone's actor index like any actor, and
moves between zones as any actor does, but it is not in the zone's subtree, so unloading or saving the
zone leaves it out. What the player carries is attached to it, `contained_in`, and is owned by the
session too. Both are saved with the session.

`addressing_plan.md` has the structures and components for all of this.

## Appendix: Changes from 0.3

- **Entities are one list.** 0.3 writes an `attachments` section, each entry a place with the entities
  attached there nested inside it. 0.4 writes one `entities` list, and where an entity is attached is
  its `attached` file component.
- **Entities have ids.** 0.3 entities have none.
- **A relationship can be written.** 0.3 implies it from the place.
- **A door is a parent with door facings as children.** 0.3 writes a door as one entry listing both
  walls.
- **A staircase is a block, attached to the wall on its open side.** 0.3 writes a stair cell as open
  floor, and the staircase attached to the floor with a `facing` file component.
- **Contents are attached.** 0.3 nests an entity's contents in its `contains`, which goes.
- **Wall addresses are a cell position and a normal, written separately.** 0.3 writes one array,
  `[i, j, ni, nj]`, in `walls`, `attachments` and `barriers`.
- **Every cell position is written `cell_position`.** 0.3 writes `cell` in flat cells and in the flats'
  per-cell entries, `location` in rooms, and a bare `[i, j]` in barriers.
- **Access points replace the entry, and are references.** 0.3's `layout.components.entry` is a list of
  cell positions; 0.4's `layout.components.access_points` is a list of references to relocators.
- **A bound relocator target is a reference.** 0.3 specifies a zone name and a cell.
- **A saved zone has persistent ids and its own zone id.** 0.3 has no saved zone.
