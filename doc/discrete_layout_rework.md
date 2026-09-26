# Discrete Layout Rework

Status: built
Date: 2026-09-12, revised 2026-09-26

This document covers loading a zone from a zone file: what a zone is built from, and how the world builds it.
`discrete_layout.md` is the format, `discrete_layout_plan.md` the structures it loads into, and `addressing_plan.md`
the ids and links it builds against; this work is step 5 there. `game_system_reference.md` describes what is built.

## Loading a zone

`World` makes an empty zone, and the loader or a generator fills it. A `Document` is the file with its geometry
validated, and it is the only form a zone takes between the file and the world. The loader builds the zone directly
from it: first the layout, then an entity for everything else, with each component object decoded into a typed
component, so nothing in the world is JSON. A generator fills the zone without a file, setting typed components
directly.

    file --stratum_format::read--> Document --check against the world's types--> accepted Document
    World --create_zone(zone_id)--> empty zone --load_layout(zone, document)----> zone
                                               --Generator::generate(zone)------> zone

`World::load_zone` reads the file and checks it against the world's registered types before making the zone, so a
rejected file makes nothing. The zone's persistent id is the file's if the file has a `zone` section, and a new one
otherwise; a generated zone always has a new one.

### Loading

- **`load_layout(zone, document)`** fills the empty zone: first the layout, then every entity that the document
  places by the layout's ids. As it builds each entity, it resolves the entity's address against the layout: a wall
  address to a wall id, a flat role to a flat id, and the flat cell where an entity was authored to a placement.
- **Nothing stores a glyph.** Glyphs and entities are related only while loading and saving. A glyph set, including a
  custom one, is mapped to entities on load, and saving renders the entities as the format's glyphs.
- **`Generator::generate(zone)`** fills the empty zone it is given, as `load_layout` does, but sets typed components
  instead of writing component objects to be decoded.

### Types

A type is a prefab entity. The set of types is an asset, authored by artists, designers and mods, and `World` loads it
independently of any zone (`modding_plan.md`, `## Types`). `create_entity_type(entities, name)` makes the prefab and
the caller sets its components, in C++ or from data through the same decoder that zones load with; no JSON reaches
`World`. This is built, with the property query (`game_system_reference.md`, `### Entity types, layout entities and
properties`). A component that inherits is registered with flecs' `(OnInstantiate, Inherit)`, so an entity reads its
type's value instead of the copy flecs otherwise makes when the entity becomes an instance. A component whose state
belongs to each entity, such as `Access`, keeps the copy.

**Inheritance resolves when queried, and nothing is copied.** An entity built with a `type` is `IsA` that type and has
only the components written on it. A component that inherits declares its fields `std::optional`, and a query fills
each field from the first of these that sets it:

1. the entity's own value;
2. its type's value, through `IsA`, and then its type's type's value;
3. the value of the room that contains it;
4. the layout's value, a component on the zone's entity.

A field that the type sets is the entity's, so a door whose type is oak never looks at the room. A field that the type
leaves unset comes from the room or the layout. Each level is a lookup, not a search: the room's entity is found by the
cell's `room_id`. This is the chain that `discrete_layout.md` (`### Inheritance`) specifies, property by property: a
style's parameters merge by name, and a struct member merges field by field. Nothing is copied down the chain when the
zone is built, just as a flat's height already resolves by `DiscreteLayout` descending through cell, room and flat on
each query.

### Zones

`World` contains zones rather than layouts. **A zone is an entity** with the `Zone` tag, a `ZoneLayout` component that
owns its layout, a `ZoneNotation`, and the file's layout components. Its denizens --- its population, which are not
all monsters --- are entities in its subtree, found by query rather than listed, and its layout entities are found by
address (`addressing_plan.md`, `### Layout entities`). The tag and the two components are built
(`game_system_reference.md`, `### World`).

**The thing created and loaded is a zone, not a layout.** The operations are named for it: `create_zone` instead of
`create_layout`, and the others the same way. This is more than terminology: an operation returns or takes a zone,
and the layout is one component of it.

`load_layout` builds everything in the document in the zone's subtree:

- **The layout's components** on the zone entity, and its access points.
- **Room and flat entities.** There is one per room id and one per flat, which the zone finds by id, with their
  components. A flat's overrides for a room or a cell are entities below the flat's entity, which the flat finds by
  room id and by cell.
- **Wall entities, sparse.** A wall is a bit in the layout. A wall with components or with something sited on it has
  an entity, which the zone finds by wall id, with the wall's components and the list of entities sited on it. There are
  as many wall entities as there are walls with something written on them, not as many as there are walls
  (`decision_rationale.md`, "A wall is a record, not an entity").
- **Flat cell entities, sparse,** in the same way, found by flat id and cell.
- **The entities.** There is one per entity in the document, referring to its type, with its own components, its
  parent and its site, and the site is linked in both directions (`addressing_plan.md`, `## Links`). A door has the
  state its sides share --- `Access` --- and each door facing below it has what is reached from its side: a
  `Detection`, a keyhole, or a knocker.
- **Every component decoded.** Each component object is decoded, through flecs' reflection cursor, into the component
  registered under its name, either by the game in C++ or by a mod's schema, and the object itself is not kept. A name
  that nothing registers is an error (`modding_plan.md`, `## Components`). The reader decodes the format's own file
  components --- `site`, `parent`, `relocator` and `access_points` --- and the loader builds from them. Relocating
  through a relocator is `addressing_plan.md` step 8. Built.

`crossable(zone, wall_id, actor)` finds the wall's entity by id and reads the list of entities sited on it, and the
door functions go as `addressing_plan.md` (`### What the queries become`) lists. `World::load_zone` has `load_layout`
fill an empty zone, `World::create_zone(generator)` has `Generator::generate` fill one, and the suites load the same
way.

### What checks it

- `stratum_sim_test`: `load_layout` over data files with component objects for the layout, rooms, walls, flats, flat
  rooms and flat cells, and with entities on walls, on flat cells and on their parents, each built where the file sites
  it. Loading builds room, flat, wall and flat cell entities found by their ids, a door with its door facings linked to
  their walls and back, and a relocator with its target; a component name that nothing registers is rejected.
  `crossable` works through the links, by access and by each actor's detection. The loading, door and world suites run
  through the new path. Built: a property query that fills each field from the entity, its type, the room that contains
  it and the layout in turn, struct members field by field, for an entity placed in a cell, sited on a wall, and sited
  on its parent.
- `stratum_view_test`: its frames are unchanged.

### Order

This is `addressing_plan.md` step 5, with `modding_plan.md` step 2.

- [x] **1. Types and the property query.** `create_entity_type` and `entity_type`; a property resolved field by field
      from the entity, its type, its room and the layout; and optional fields through `register_optional`.
- [x] **2. Building everything in the document.**
  - [x] **a. Layout entities.** An entity for a room, a wall or a flat cell, found by its address, linked in both
        directions to the entities sited on it, and deleted when it has neither components nor anything sited on it.
  - [x] **b. Every entity built.** Each with its type, its parent through `Parent`, its site and its `Relocator`, and
        the file's access points made access points; the reader decoding `relocator`; the `door` type registered by
        `StratumGame`; and `create_doors`, `door_walls_type` and `door_type` deleted.
  - [x] **c. The layout's and the entities' component objects decoded,** through flecs' reflection cursor into the
        components registered under their names, with references translated and added to the world index;
        `build_error` checking every one before the zone exists; and the reader decoding `access_points`.
  - [x] **d. The rooms', walls' and flats' component objects decoded onto their layout entities.** An entity for every
        room and each flat, with a flat's overrides for a room or a cell below the flat's entity, and a layout entity's
        property chain. A wall's chain is the wall, its room and the layout. A flat cell's chain is the cell, the flat's
        override for its room, the room, the flat and the layout, with the flat's value winning at each level where
        both have a component.
  - [x] **e. A component that inherits registered with flecs' `(OnInstantiate, Inherit)`,** so an entity reads its
        type's value instead of a copy, and its decoded object is only what the file sets.
- [x] **3. Passage through the links.** `Access` and `Detection`, and `crossable` asking each entity on the wall's
      entity for the actor crossing; the view working through the links, drawing the viewer's perception; and `Door`,
      `DoorFacing`, `Openable`, `Crossable`, `Concealed`, `facing_on`, `door_of` and `other_facing` deleted.
- [x] **4. The test generator on the new path,** and `create_door` deleted.
