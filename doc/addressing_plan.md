# Ids and Addressing --- Plan

Status: designed, nothing built
Date: 2026-09-14

The design the spec in `addressing.md`: one list of entities in a zone file,
with `attached` and `parent` as components the format defines and parts derived from `parent`;
persistent ids; links both ways between an entity and what it is attached to; and references between
entities within a zone and across zones. Open steps are listed in `work_plan.md`, and `game_system_reference.md` describes the implementation.

## Decided

- **One list of entities.** Attachments section removed, attachments moved to a single `entities` section.
- **How an entity relates to a place or another entity is a component:** `attached` for what it is on
  or in, `parent` for the entity it is a part of.
- **Parts are derived.** Parts are entities that share the same parent. A parent does not list its
  parts; the list is built from the parts' `parent` components when the zone loads.
- **Zone files are written by generators, tools and saves.** Nothing here is intended to be written by hand,
  and being readable as text is sufficient.
- **A cell position is `cell_position`, everywhere.** Every key in the format that holds a cell position
  is `cell_position`, and so is every name for one in code, where the rename is done.
- **A wall id is encoded from the normal,** by `encode_wall_id`, as part of 0.4.

## Choices made here, for review

Designing the decided model needed these as well, and none has been agreed.

| | Proposed | Instead |
| --- | --- | --- |
| 1 | A reference in content is written `{ "entity": id }`, an object with that one key, so making a zone translates every reference without knowing what a component means | Each type declaring which of its fields are references, in the types asset |
| 2 | The 0.4 reader requires an entity at every door glyph's walls and every stair glyph's cell, as it requires the entry | Glyphs supplying a default entity where the file has none, as the 0.3 loading design does |
| 3 | Every entity is placed: attached, or the parent of a part that is placed; and following `attached` and `parent` from any entity never leads back to it | An entity allowed to be neither, for whatever has no place |
| 4 | A staircase's facing direction is its `attached` component's `orientation` rather than a `facing` component | `facing` stays a component, and `orientation` is for what has no component of its own |
| 5 | Relationship names: `fills` for a door facing in its doorway, `on` for a fixture on a flat cell or on an entity, `hangs_from` for a wall fixture, `lying_on` for a loose item, `contained_in` | Other names; the vocabulary is the caller's either way |
| 6 | A door always has door facings as parts, one where a doorway leads into a dead end | A door on one wall attached to it directly, with no part |
| 7 | A part is a child by `flecs::Parent`, the non-fragmenting parent in Flecs 4.1.6, and its parent has a `Parts` list made at load | `ChildOf`, which puts each parent's parts in a table of their own; or iterating Flecs's children of a parent in place of `Parts` |
| 8 | The world index covers every entity in a zone, loaded or not | Only entities something refers to, which is smaller and has to be kept as references are made |
| 9 | A saved zone is the zone file format with a top-level `zone` section holding the zone's persistent id, whose presence makes every id in the file persistent | Flecs's own JSON for the entities beside a zone file for the geometry |
| 10 | What decides passage on a wall is what is attached to it `fills`, and that entity's parent | A tag on the entity, as `Door` is today |
| 11 | Format 0.4 and persistent ids are built before loading a zone, so loading is written once against them | Loading a zone first, against 0.3, then reworked |
| 12 | An actor keeps its `placement_type` and is found by `entities_at` asking every placed entity in its zone, as now | An actor also attached to the cell it stands on, `standing_on`, so what is at a cell is one lookup for actors as for everything else, at the cost of changing two cells' lists, and making or deleting a cell's entity, on every step; and `placement_type` working for a `ContinuousLayout`, which has no cells |

## The format, version 0.4

### entities

One entry per entity, in id order.

```json
{ "id": 2, "type": "door_facing", "components": { "parent": { "entity": 1 }, "attached": { "wall": { "cell_position": [1, 1], "normal": [-1, 0] }, "relationship": "fills" }, "concealed": true } }
```

| Key | |
| --- | --- |
| `id` | A whole number, unique among the file's entities |
| `type` | A string, unrestricted, types being prefabs the world has |
| `components` | A component object, optional |

`attachments`, a `walls` list, nested `entities` and `contains` go.

### attached

A component whose keys the format defines. It has exactly one target.

| Key | |
| --- | --- |
| `wall` | A wall address, `{ "cell_position": [i, j], "normal": [ni, nj] }`. The target, when the entity is on a wall |
| `flat`, `cell_position` | A flat cell. The target, when the entity is on a floor or ceiling |
| `target` | A reference to an entity. The target, when the entity is on or in another entity |
| `relationship` | A string, unrestricted |
| `position` | Where on the target: along and up on a wall, xy within the cell on a flat. Centered when absent |
| `orientation` | A direction, one of the four directions defined by a cell's sides |

```json
"attached": { "flat": "floor", "cell_position": [3, 2], "relationship": "on", "orientation": [0, -1] }
"attached": { "target": { "entity": 6 }, "relationship": "contained_in" }
```

### parent

A component whose value is a reference to the direct ancestor entity:

```json
"parent": { "entity": 1 }
```

### References

A reference is an object whose only key is `entity`. A reader
recognizes a reference by that form alone; an object with an `entity` key and other keys is not one.

```json
{ "id": 5, "type": "lever", "components": { "controls": { "entity": 1 } } }
```

### entry

`layout.components.entry` is a list of references, at least one:

```json
"layout": { "components": { "entry": [{ "entity": 8 }, { "entity": 9 }] } }
```

Each is a reference to an entity with a `relocator` component, attached to a flat cell of the floor
whose cell is written `<` or `>`.

### What the glyphs require

- **A door glyph:** an entity with two children attached to the two facings of the door on that wall edge.
`stratum_format::door_walls` finds for that door, and to no other wall. (no idea what you're saying here ...)
- **A stair glyph:** an entity with a `relocator` component, attached to the floor at that cell
  position.

### Cell positions and wall addresses in the other sections

Every key holding a cell position is `cell_position`, and every wall address is an object with
`cell_position` and `normal`:

| Section | 0.4 |
| --- | --- |
| `rooms` | `{ "cell_position": [3, 1], "components": { ... } }` for a room identified by a cell in it |
| `walls` | `{ "wall": { "cell_position": [1, 1], "normal": [-1, 0] }, "components": { ... } }` |
| `flats`, per-cell entries | `{ "cell_position": [3, 2], "height": 0.5, "components": { ... } }` |
| `barriers` | `{ "cell_position": [3, 1], "normal": [1, 0] }` across an edge, `{ "cell_position": [7, 4] }` closing off a cell |

A room's perimeter is still a list of junctions.

### A saved zone

The same format, with the exception that the former local ids are replaced with persistent ids, and with
a top-level section a template does not have:

```json
"zone": { "id": 7713 }
```

This section's presence is what marks every id in the file as persistent: entity ids, references ---
`attached` targets and `parent` among them --- and bound relocator targets, which are references,
`"target": { "entity": 90412 }`. A reference in a saved zone may point to an entity in another zone, so
the reader does not check that it resolves.

### Errors added

- `entities` is not a list, or an entry is not an object.
- An id is not a whole number, or two entities have one id.
- An entity has no `type`, or its `type` is not a string.
- `attached` is not an object; has no target or more than one; has a wall address that is not a cell
  position and a normal, or a wall address or flat cell breaking the rules those have; has a `target`
  that is not a reference; has no `relationship` or one that is not a string; or has a `position` that
  is not two numbers or an `orientation` that is not a direction.
- `parent` is not a reference.
- An entity is neither attached nor the parent of a part that is placed.
- Following `attached` targets and `parent` references from an entity leads back to it.
- In a template, a reference is not an entity's id.
- A door glyph or a stair glyph without what it requires.
- `layout.components.entry` is absent, not a list, or empty; or an entry is not a reference to an
  entity with a relocator attached to a floor cell written `<` or `>`.
- `zone.id` is not a whole number.

### The example

The cistern of `discrete_layout.md` in 0.4:

```json
"layout": {
  "components": { "name": "The Flooded Cistern", "entry": [{ "entity": 4 }, { "entity": 5 }] }
},
"entities": [
  { "id": 1, "type": "door", "components": { "material": "oak", "open": false } },
  { "id": 2, "type": "door_facing", "components": { "parent": { "entity": 1 }, "attached": { "wall": { "cell_position": [1, 1], "normal": [-1, 0] }, "relationship": "fills" } } },
  { "id": 3, "type": "door_facing", "components": { "parent": { "entity": 1 }, "attached": { "wall": { "cell_position": [2, 1], "normal": [1, 0] }, "relationship": "fills" } } },
  { "id": 4, "type": "staircase", "components": { "attached": { "flat": "floor", "cell_position": [3, 2], "relationship": "on", "orientation": [0, -1] }, "relocator": { "target": "level_down" } } },
  { "id": 5, "type": "staircase", "components": { "attached": { "flat": "floor", "cell_position": [3, 0], "relationship": "on", "orientation": [0, 1] }, "relocator": { "target": "level_up" } } }
]
```

## stratum_format

`attached` and `parent` are decoded into fields of their own, being geometry and structure the reader
checks, and every other component is kept as written. The writer puts the two back into `components`,
so a document written back is equivalent to the file.

```cpp
namespace quetzal::stratum_format
{
    // An id as a file writes it: local in a template, persistent in a saved zone
    using file_id_type = std::uint64_t;

    // An attached component's target when it is an entity
    struct EntityTarget
    {
        file_id_type id = 0;
    };

    // The attached component
    struct Attachment
    {
        std::variant<WallAddress, FlatCell, EntityTarget> target;
        std::string relationship;
        std::optional<surface_point_type> position;
        std::optional<discrete_vector_type> orientation;
    };

    struct Entity
    {
        file_id_type id = 0;
        std::string type;
        std::optional<Attachment> attached;
        std::optional<file_id_type> parent_id;
        values_type components; // Every other component, as written
    };

    struct Document
    {
        ...
        std::optional<file_id_type> zone_id; // A saved zone's own
        std::vector<Entity> entities;
    };
}
```

A new `references.hpp`, since both the reader and making a zone walk content for references:

```cpp
    // Every reference inside a component object, in the order they are written
    std::vector<file_id_type> references(const values_type& components);

    // Each reference replaced by what translate returns for it
    void translate_references(values_type& components, const std::function<file_id_type(file_id_type)>& translate);
```

`grid.hpp` keeps `door_walls` and `stair_cell`, which the glyph checks use. The reader's checks run
after the geometry's, in the order the errors are listed, so a file's first fault is reported.

## The generators

Both write 0.4 through `stratum_format` as they write 0.3 now.

- **Ids in the order written.** Each door in the order `door_walls` returns them, followed by its door
  facings in the order of its walls, then the staircases in the order the generator placed them. The
  same parameters give the same ids, which the reference zone files compare.
- **A door** is a `door` entity and a `door_facing` part per wall, each attached `fills`.
- **A staircase** is a `staircase` entity with its `relocator`, attached to its floor cell `on`, with its
  facing direction as `orientation`.
- **The entry** is a reference to every staircase.

`staircases_placed_and_faced` and `doors_have_entities` in the generator, sim and view suites read the
new list. The sim and view suites' zone files, the assets and the tool samples are converted.

## Loading a zone against 0.4

`discrete_layout_rework.md` (`## Loading a zone`) outlines loading against 0.3. Against 0.4 its
`Content` becomes:

```cpp
namespace quetzal::stratum_sim
{
    struct WallTarget
    {
        id_type wall_id = nullid;
    };

    struct Content
    {
        struct Attachment
        {
            std::variant<WallTarget, FlatCell, stratum_format::EntityTarget> target;
            std::string relationship;
            std::optional<surface_point_type> position;
            std::optional<discrete_vector_type> orientation;
        };

        struct Entity
        {
            stratum_format::file_id_type id = 0;
            std::string type;
            std::optional<Attachment> attached;
            std::optional<stratum_format::file_id_type> parent_id;
            values_type components;
        };

        std::optional<stratum_format::file_id_type> zone_id;
        values_type layout;
        std::vector<values_type> rooms; // By room id
        std::map<id_type, values_type> walls; // By wall id
        std::map<id_type, FlatContent> flats; // By flat id
        std::vector<Entity> entities;
    };
}
```

`load_content` turns wall addresses into wall ids and flat roles into flat ids, and copies the rest.
Glyphs no longer become attachments, 0.4 requiring every door and staircase to be written, so the
merging of a glyph's default entity with an `attachments` entry at the same place goes from that design.

## Persistent ids

```cpp
namespace quetzal::stratum_sim
{
    using persistent_id_type = std::uint64_t;

    // An entity's identity in a world, for as long as the entity exists
    struct PersistentId
    {
        persistent_id_type id = 0;
    };

    class IdAllocator
    {
    public:
        virtual ~IdAllocator() = default;
        virtual persistent_id_type allocate() = 0;
    };

    // Ids from a count, which is saved with the world
    class CountingIdAllocator final : public IdAllocator
    {
    public:
        explicit CountingIdAllocator(persistent_id_type next);
        persistent_id_type allocate() override;
        persistent_id_type next() const;

    private:
        persistent_id_type m_next;
    };

    // A reference to an entity that may be in another zone, or in none loaded
    struct EntityReference
    {
        persistent_id_type id = 0;
        flecs::entity entity; // The runtime entity last resolved, not alive once its zone has been unloaded
    };
}
```

`persistent_id_type` has a `std::hash` of its own, which a 128-bit form would replace.

`World` gains:

```cpp
        persistent_id_type allocate_id();

        // The loaded entity with that id, null where it is not loaded
        flecs::entity entity(persistent_id_type id) const;

        // The zone an entity is in, loaded or not, and none for an entity in no zone
        std::optional<persistent_id_type> zone_of(persistent_id_type id) const;

        // The entity, re-resolved where the one last resolved is no longer alive; null where it is not loaded
        flecs::entity resolve(EntityReference& reference) const;

    private:

        std::unique_ptr<IdAllocator> m_id_allocator;
        std::unordered_map<persistent_id_type, flecs::entity> m_loaded_entities;
        std::unordered_map<persistent_id_type, std::optional<persistent_id_type>> m_entity_zones;
```

- **`m_loaded_entities`** has an entry for each loaded entity with a `PersistentId`, added by an
  `OnSet` observer and removed by an `OnRemove` one, so no path that creates or deletes an entity can
  leave it behind.
- **`m_entity_zones`** has an entry for every persistent entity: set when it is made, changed when it
  migrates, kept when its zone unloads, removed when it is destroyed.

### Loading a template

`create_zone` with a `Content` whose `zone_id` is absent:

1. Allocate the zone's persistent id.
2. Allocate a persistent id for each entity, in `Content` order, into a map from local id to persistent
   id that lasts as long as the call.
3. Translate every reference in the layout's, rooms', walls', flats' and entities' component objects,
   and every `attached` target and `parent`, through the map.
4. Build the entities, each with its `PersistentId` and its type, then the parts, then the attachments.

A template loaded twice gives two sets of ids, and so two instances. The ids depend on the world's count, so tests
compare what is built and how it links rather than id values.

### Loading a saved zone

`create_zone` with a `Content` whose `zone_id` is present allocates nothing and translates nothing:
every id is already persistent. Loading re-registers each entity in `m_loaded_entities`, and a
reference into the zone resolves again the next time it is asked.

## Links

```cpp
namespace quetzal::stratum_sim
{
    // On an entity that is attached: its target and how
    struct Attached
    {
        flecs::entity target;       // A wall's entity, a flat cell's entity, or an entity
        flecs::entity relationship;
        surface_point_type position;
        std::optional<discrete_vector_type> orientation;
    };

    // On a target: everything attached to it
    struct AttachedEntities
    {
        std::vector<flecs::entity> entities;
    };

    // On a parent: its parts, built when the zone loads
    struct Parts
    {
        std::vector<flecs::entity> entities;
    };

    void attach(flecs::entity entity, flecs::entity target, flecs::entity relationship, const surface_point_type& position,
        std::optional<discrete_vector_type> orientation);
    void detach(flecs::entity entity);

    void make_part(flecs::entity part, flecs::entity parent);
}
```

- **`attach` and `detach` keep both ends in step.** An `OnRemove` observer on `Attached` removes an
  entity from its target's list when the entity is deleted; one on `AttachedEntities` detaches
  everything from a target being deleted. What happens to what was attached --- the contents of a
  smashed chest --- is the deleting rule's to decide, not the observer's.
- **`make_part`** sets the part's `flecs::Parent` and adds the part to the parent's `Parts`. Flecs
  deletes a parent's children with it, so deleting a parent deletes its parts.
- **A relationship is an entity** `World` creates the first time a name is used, in a `relationships`
  scope, so `Attached::relationship` compares as an entity. Traits such as `Exclusive` are set on it
  where the vocabulary needs them.
- **An entity has at most one `Attached`.** Something on several targets is a parent whose parts each
  have their own, so attachment never has a second case.

### Places

A place's entity exists where it has components or something attached, and goes when it has neither.
`World` keeps, per zone:

```cpp
    struct ZonePlaces
    {
        std::unordered_map<id_type, flecs::entity> wall_entities; // By wall id
        std::vector<std::unordered_map<size_t, flecs::entity>> flat_cell_entities; // By flat id, then slot
    };
```

beside the zone's layout, the rework's `Zone` becoming the home of both.

```cpp
        flecs::entity wall_entity(flecs::entity zone, id_type wall_id) const; // Null where it has none
        flecs::entity flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point_type& cell_position) const;
        flecs::entity ensure_wall_entity(flecs::entity zone, id_type wall_id);
        flecs::entity ensure_flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point_type& cell_position);
```

A place's entity, its list and a parent's `Parts` are derived, and rebuilt when the zone loads
(`addressing.md`, `### A saved zone`). A place has no `PersistentId`; it is referred to by its address.

### Wall ids from the normal

Encoding a wall address is a function of its own, taking the normal as it is:

```cpp
        // The id of the wall at an address, from the cell position's slot and the normal's direction index
        id_type encode_wall_id(const discrete_point_type& cell_position, const discrete_vector_type& normal) const;
```

- **The id is the slot times four plus the normal's direction index.** `load_content` encodes every
  address in a file with it.
- **`wall_toward` stays the movement query.** Stepping in a direction meets the wall whose normal is the
  opposite direction, so `wall_toward` negates the direction, which is what that question means, and
  encodes with `encode_wall_id`.
- **Everything that decodes a wall id** --- the normal from an id, the cell's mask bit for the wall --- uses
  the normal's direction index.

### What the queries become

- **`crossable(zone, wall_id)`:** the wall's entity, then the entity in its `AttachedEntities` that
  `fills`, then that entity's parent, or the entity itself if it has no parent, then `Openable` or
  `Passable` as now. A door facing `Concealed` and not found refuses as now.
- **What is on a flat cell:** the flat cell's entity, then its `AttachedEntities`.
- **Where an entity is:** its `Attached`, or for a parent, its parts' `Attached`.
- **The other side of a door:** the door facing's parent, then its `Parts`.

`OnWall`, `DoorFacing`, `facing_on`, `door_of`, `other_facing`, `create_door`, `create_doors`,
`door_walls_type` and `load_layout`'s `door_type` go.

## Migration

`World::relocate` with an entity attached in one zone and a destination in another detaches it,
changes its parent to the destination zone, attaches it at the destination, and sets its entry in
`m_entity_zones`. What is attached to it and its parts move with it.

An entity in no zone --- the player, and what it carries --- has `std::nullopt` in `m_entity_zones` and is
saved with the session.

## Multiplayer

Nothing to build now. What keeps it open is in `addressing.md`: `persistent_id_type` used everywhere a
persistent id is kept, allocation through `IdAllocator`, no meaning read from an id. A server-hosted
game uses `CountingIdAllocator` on the server. Several servers use an allocator that puts its own
number in the high bits. Worlds with no coordination use UUIDs, either as `persistent_id_type` or
mapped at the boundary where an entity crosses from one world to another.

## Flecs

- **`flecs::Parent`** is declared in Flecs 4.1.6 (`library3/flecs`) as a non-fragmenting `ChildOf`.
  `test/flecs_test` checks what this design relies on before it is used: deletion cascading to
  children, iterating a parent's children, and a JSON round trip keeping the parent.
- **`DontFragment`** is available for a relationship that needs a pair after all.
- **Reflection** is registered member by member for every new component, `sim_zone_model.md`'s
  obligation, or a saved zone is written with names and no values.

## Other systems

Tiled (a general 2D map editor, TMX or JSON) places objects by position in object layers and refers
between them by integer object id, keeping the next id in the map. LDtk (a 2D level editor from the
studio behind Dead Cells, JSON) has entity layers with typed fields and refers by UUID. Hammer (Valve's
editor for Half-Life, Counter-Strike and Portal) has point entities and entities tied to brushes,
wired by name through inputs and outputs. IFC (buildingSMART's ISO 16739 model for building data)
makes each relationship an object --- an opening voids a wall and a door fills the opening --- and gives
each object a GUID. Unity keeps an object's place as its `Transform` component, and Flecs its hierarchy
and prefabs as `ChildOf` and `IsA`, which is the form chosen here: where an entity is and what it is
part of are components. NetHack's per-cell object chains are a location index outside any entity
system. Snowflake ids at Twitter and Discord divide 64 bits between a time, a machine and a count, and
Bethesda's form ids between a plugin's place in the load order and its own count.

## What checks it

- **`stratum_format_test`:** every added error from a valid 0.4 file with one change; `attached` and
  `parent` decoded and written back; references found and translated in nested components; a saved
  zone's references not checked; the round trip.
- **`dungen_test`, `catagen_test`:** ids in the order written and the same for the same parameters;
  every door with its door facings and every staircase attached; the entry as references to every
  staircase.
- **`flecs_test`:** `flecs::Parent` cascading, iterated and round-tripped.
- **`stratum_sim_test`:**
  - loading a template twice gives different persistent ids and the same structure;
  - every reference translated, and one to an entity resolving to it;
  - `attach` and `detach` keeping both ends; deleting an entity or a target leaving no link behind;
  - `Parts` built from `parent`, and deleting a parent deleting its parts;
  - `encode_wall_id` and `wall_toward` giving the same id for the same wall, for all four normals;
  - a place's entity made when something is attached and gone when nothing is;
  - `crossable` through the links, a door with one door facing and with two, concealed from one side;
  - migration keeping the persistent id and the runtime entity, and updating the index;
  - a reference re-resolving after its zone is unloaded and loaded again;
  - `CountingIdAllocator` never repeating across a save of its count.
- **`stratum_view_test`:** its frames, which print wall ids, showing 0.4's wall ids and nothing else
  different.

## Steps

- [ ] **1. Format 0.4 in `stratum_format`.** The document types, the reader's checks, the writer,
      `references.hpp`, `cell_position` and separate wall addresses throughout, and the format suite's
      data and mutations. Choices 1 to 6 and 9 settled first.
- [ ] **2. The generators and every zone file in 0.4.** Both generators, their suites and references,
      the sim and view data, the assets and the tool samples. `load_layout` still reads geometry alone, so
      the sim and view suites pass unchanged.
- [ ] **3. `flecs::Parent` checked in `flecs_test`.** Choice 7 settled by what it finds.
- [ ] **4. Persistent ids.** `persistent_id_type`, `IdAllocator`, `CountingIdAllocator`, `PersistentId`,
      `m_loaded_entities` and `m_entity_zones`, `EntityReference` and `resolve`.
- [ ] **5. Loading a zone, against 0.4 and with persistent ids.** The rework's step 5 with `Content` as
      above: making a zone allocates and translates, builds entities, parts, `Attached`,
      `AttachedEntities` and places, and `crossable` answers through them. Wall ids are encoded from the
      normal by `encode_wall_id`. The door functions go. Choices 10 and 12 settled first.
- [ ] **6. Migration and entities in no zone.** `relocate` across zones through the links and the index.
- [ ] **7. Saved zones.** The `zone` section, unload writing a zone and load reading one back, with the
      session lifecycle of stage 7.
- [ ] **8. Relocating through a relocator,** stage 6's item, binding targets by persistent id.

Steps 1 and 2 change no behavior and no frame. Step 5 is where behavior moves from the glyphs to the
entities, and the frames confirm that nothing a player sees changed apart from the wall ids they print.

## Open

- **Names.** `attached`, `parent`, `door_facing`, `file_id_type`, `EntityTarget`, `PersistentId`,
  `EntityReference`, `Parts` and `ZonePlaces` are working names. `placement` is not a candidate for the
  file's `attached`, `placement_type` already being an actor's position and facing direction.
- **Where a saved zone is written,** and how a world's zones are found by persistent id on disk. The
  session lifecycle's to settle.
- **What the index costs at a world's scale.** An entry per entity in every zone ever made. Measured
  once zones are saved, against choice 8's alternative.
