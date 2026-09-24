# Discrete Layout Rework

Status: the structures and the loader are built; loading a zone is designed
Date: 2026-09-12, revised 2026-09-22

Loading a zone from a zone file: what a zone is built from, and how the world builds it. `discrete_layout.md` is the
format, `discrete_layout_plan.md` the structures it loads into, and `addressing_plan.md` the ids, links and `Content`
this builds against; it is step 5 there. What is built is in `game_system_reference.md`.

## Loading a zone

A `Document` stays the file with its geometry validated. Loading turns it into what a zone is built from: the layout,
and `Content`, everything else placed by the layout's ids, its component objects still the document's JSON. Building
decodes them into typed components, and nothing in the world is JSON. A generator produces the same two with no file. Building
makes the zone in the world, its layout and an entity for everything in `Content`. Loading has no world; building has
no document.

    file --stratum_format::read--> Document --load_layout, load_content--> DiscreteLayout, Content
                                  Generator --generate----------------------> DiscreteLayout, Content
    World, with its types --create_zone(DiscreteLayout, Content)------------> Zone

### Loading

```cpp
namespace quetzal::stratum_sim
{
    std::unique_ptr<DiscreteLayout> load_layout(const stratum_format::Document& document);
    Content load_content(const stratum_format::Document& document, const DiscreteLayout& layout);
}
```

- **`load_layout`** takes a document rather than a file. The caller reads the document.
- **`load_content`**, in `stratum_sim/load_content.cpp`, copies every component object out of the document by id:
  `Document::components`, each room's by room id, each wall's by wall id, and each flat's for the flat, each room and
  each cell, by flat id. Every entity is copied with its site's address encoded as a wall id or a flat id and cell,
  and a flat cell where an entity was authored resolved to a placement.
- **Nothing records a glyph.** Glyphs and entities are related only while loading and saving. A glyph set, a custom one
  included, is mapped to entities on load, and saving renders the entities as the format's glyphs.
- **`Generator::generate`** fills the empty zone it is given, as `load_layout` does. Both are built for the layout, the
  notation and the doors (`game_system_reference.md`); what they build the rest from is open (`work_plan.md`).

### Types

A type is a prefab entity. The set of types is an asset, authored by artists, designers and mods, and `World` loads it
independently of any zone (`modding_plan.md`, `## Types`).

```cpp
class World
{
    flecs::entity create_type(const std::string& type, const values_type& components);
    flecs::entity type(const std::string& type) const; // Null where none is loaded
};
```

**Inheritance resolves when queried, and nothing is merged.** An entity built with a `type` is `IsA` that type and has
only the components written on it. When an object in a zone is queried for a property:

1. its own value, where it has one;
2. failing that, the room containing it, where its type inherits from its parent;
3. failing that, the layout;
4. failing that, its type's default, or nothing where the type has none.

Each is a lookup rather than a search: a component on the entity, the room's entity by the cell's `room_id`, a
component on the zone's entity, a component on the type's through `IsA`. This is the chain `discrete_layout.md`
(`### Inheritance`) specifies, property by property: a style's parameters merge by name, and a typed component that
inherits supplies the fields it sets and leaves the rest to the next level. Nothing is copied down the chain when the zone is built, as a flat's
height already resolves, `DiscreteLayout` descending cell, room and flat on each query. Where no type is loaded for an
entity, the chain ends at the layout.

### Zones

`World` contains zones rather than layouts. **A zone is an entity:** the `Zone` tag, a `ZoneLayout` component owning
its layout, a `ZoneNotation`, and the file's layout components. Its denizens --- its population, not all of them
monsters --- and its places are entities in its subtree (`addressing_plan.md`, `### Places`), found by query rather
than listed. The tag and the two components are built (`game_system_reference.md`, `### World`).

**What is created and loaded is a zone, not a layout.** The operations are named for it: `create_zone` rather than
`create_layout`, and the others the same way. This is more than terminology: an operation returns or takes a zone,
with its layout one component of it.

```cpp
flecs::entity World::create_zone(std::unique_ptr<Layout> layout, const Content& content);
```

`create_zone` builds everything in `Content` in the zone's subtree:

- **The layout's components** on the zone entity, and its access points.
- **Room and flat entities.** One per room id and one per flat, which the zone finds by id, with their components. A
  flat's overrides for a room or a cell are entities below the flat's, which it finds by room id and by cell.
- **Wall entities, sparse.** A wall is a bit in the layout. A wall with components or with something sited on it has an
  entity, which the zone finds by wall id, with the wall's components and the list of what is sited on it. There are as
  many as there are walls with something written on them, not as many as walls (`decision_rationale.md`, "A wall is a
  record, not an entity").
- **Flat cell entities, sparse,** the same way, found by flat id and cell.
- **The entities.** One per entity in `Content`, referring to its type, with its own components, its parent and its
  site, the site linked both ways (`addressing_plan.md`, `## Links`). A door has what its sides share --- `Openable`,
  `Lockable` --- and each door facing what is reached from its side, below it: `Concealed`, a keyhole, a knocker.
- **Every component decoded.** Each component object is decoded, through flecs reflection, into the component
  registered under its name, by the game in C++ or by a mod's schema, and the object itself is not kept: `open` sets
  `Openable::open`, and a `relocator` sets a `Relocator` with its target. A name nothing registers is an error
  (`modding_plan.md`, `## Components`). Relocating through a relocator is `addressing_plan.md` step 8.

`crossable(zone, wall_id)` finds the wall's entity by id and reads the list of what is sited on it, and the door
functions go as `addressing_plan.md` (`### What the queries become`) lists. `World::load_zone` reads the document, calls
`load_layout` and `load_content`, and passes both to `create_zone`. `World::create_zone(generator)` does the same with
what `Generator::generate` returns, and the suites load the same way.

### What checks it

- `stratum_sim_test`: `load_content` over data files with component objects for the layout, rooms, walls, flats, flat
  rooms and flat cells, and entities on walls, on flat cells and on their parents, the ids checked against the layout.
  `create_zone` builds room, flat, wall and flat cell entities found by their ids, a door with its door facings linked
  to their walls and back, `open` setting `Openable`, and a relocator with its target; a component name nothing
  registers is rejected. `crossable` works through the
  links. A property query resolves from the object, then the room containing it, then the layout, then its type's
  default; an object stating part of an object-valued component takes the rest from further along; a type that does not
  inherit from its parent skips the room and the layout; an object whose type is not loaded ends at the layout. The
  loading, door and world suites run through the new path.
- `stratum_view_test`: its frames unchanged.

### Order

1. `Content` and `load_content`, and `load_layout` taking a document.
2. Types in `World`, and the property query.
3. The zone's places as entities in its subtree, the operations named for zones, and `create_zone` building
   everything in `Content` with its links.
4. `crossable`, `door_of` and the view on the links, `facing_on` and `other_facing` deleted.
5. `Session`, `Generator` and the suites on the new path, and `create_doors`, `door_walls_type` and `door_type`
   deleted.
