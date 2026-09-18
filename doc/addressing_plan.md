# Ids and Addressing --- Plan

Status: designed, nothing built
Date: 2026-09-14, revised 2026-09-16

The design behind the spec in `addressing.md`: one list of entities in a zone file, with `attached` and
`parent` as file components the format defines and children derived from `parent`; persistent ids; links
both ways between an entity and what it is attached to; references between entities within a zone and
across zones; and an actor index for each zone. Open steps are listed in `work_plan.md`, and
`game_system_reference.md` describes the implementation.

## Decided

- **One list of entities.** Attachments section removed, attachments moved to a single `entities` section.
- **How an entity relates to a place or another entity is a file component:** `attached` for what it is
  on or in, `parent` for the entity it is a child of.
- **Children are derived.** Children are entities that share the same parent. A parent does not list its
  children. Loading gives each child a `flecs::Parent` from its `parent`, and Flecs keeps each parent's
  children, so no component of ours lists them (`## Flecs`).
- **Every entity has a place.** It is attached, has a parent, or is a parent, and every hierarchy of
  parents and children has an attached entity in it. Following `attached` and `parent` from any entity
  never leads back to it. If an entity without a place is ever needed, a `void` entity can be its parent,
  as a holding area.
- **Every door and every staircase is an entity,** since each has state, even if a staircase's never
  changes. The 0.4 reader requires an entity on every door glyph's walls and on every stair glyph's open
  side, as it requires the access points.
- **A door has a door facing on each wall of its doorway,** a child of the door attached to that wall.
- **A staircase is a block with a wall on each side.** In both notations a stair cell is a block, which
  owns no wall, and each open neighbor presents a wall to it. The staircase entity is attached to the wall
  on its open side, the side facing the room it opens into, and has whether it leads up or down, its
  relocator and its target. That wall's normal is the direction of travel from the staircase into the
  room, so a staircase has no orientation of its own. Its other sides are ordinary walls, each of which
  can have file components of its own, such as a texture. Where a stair block may go is the generators'
  concern, not the loader's: it must not cut a passage in two, and at a dead end its open side faces the
  one open neighbor. The geometry, the placement and the walls around a stair cell are built already,
  on 0.3 (`work_plan.md`, stage 8).
- **A reference is written `{ "entity": id }`,** an object with that one key, so making a zone translates
  every reference without knowing what a file component means.
- **Relationships are optional, and the caller's vocabulary.** `relationship` is written only where it
  distinguishes one way of being attached from another, as `contained_in` does from `on`. The vocabulary
  is kept small: a door facing is attached to its wall with no relationship, and an item's orientation,
  aligned with the wall it is attached to, may make `hangs_from` unnecessary.
- **Walls are barriers to movement** and define the borders of rooms. Passage through a wall comes from
  what is attached to it, and the same goes for a staircase: an actor moves into one through its open
  side, which the staircase attached there lets it cross.
- **`crossable` looks at each entity attached to the wall** and whether that entity's properties allow
  movement: its access state, open, closed or locked, and its detection state for the actor asking. A
  property is resolved from the entity, then its parent; an entity with no access state
  anywhere in that chain is not something to pass through, and so is not crossable. One rule reads the
  properties, naming no type, so `crossable` knows nothing about doors, staircases or concealment, and
  nothing on the wall or the cell marks it crossable (`## Crossing a wall`).
- **An actor keeps its `placement_type`** and is not attached to where it stands. Each zone has an actor
  index, derived from the placements, which finds the actors at a place without a search (`## Actors`).
- **The player is in a zone, and owned by the session.** It is in its zone's actor index like any
  actor, but unloading or saving the zone leaves it out (`## Migration`).
- **A dropped object is indexed in its zone,** as everything attached to a place is, and is accessible
  while the actor is in that zone. It is not accessible from another zone the actor moves to.
- **The world index has only the entities something refers to.** It is how a reference finds the zone
  its entity is in.
- **The zone file format is not Flecs's,** but it is arranged so that the ECS can be built from it. A
  saved zone is a zone file with a top-level `zone` section holding the zone's persistent id, whose
  presence makes every id in the file persistent.
- **Format 0.4 and persistent ids are built before loading a zone,** so loading is written once, against
  them.
- **A direction an entity has is its orientation.** `facing` is a noun for a door facing.
- **A wall address is a cell position and a normal,** `{ "cell_position": [i, j], "normal": [ni, nj] }`,
  the normal pointing out of the wall into the cell the wall belongs to.
- **The relocators an arriving actor can be placed at are the zone's access points,** listed in
  `layout.components.access_points`.
- **Zone files are written by generators, tools and saves.** Nothing here is intended to be written by hand,
  and being readable as text is sufficient.
- **A cell position is `cell_position`, everywhere.** Every key in the format whose value is a cell
  position is `cell_position`, and so is every name for one in code, where the rename is done.
- **A wall id is encoded from the normal,** by `encode_wall_id`, as part of 0.4.

## The format, version 0.4

### entities

One object per entity, in id order.

```json
{ "id": 2, "type": "door_facing", "components": { "parent": { "entity": 1 }, "attached": { "wall": { "cell_position": [1, 1], "normal": [-1, 0] } }, "concealed": true } }
```

| Key | |
| --- | --- |
| `id` | A whole number, unique among the file's entities |
| `type` | A string, unrestricted, types being prefabs the world has |
| `components` | An object of file components, optional |

`attachments`, a `walls` list, nested `entities` and `contains` go.

### attached

A file component whose keys the format defines. It has exactly one target.

| Key | |
| --- | --- |
| `wall` | A wall address, `{ "cell_position": [i, j], "normal": [ni, nj] }`: the cell the wall belongs to, and the normal pointing into that cell. The target, when the entity is on a wall |
| `flat`, `cell_position` | A flat cell. The target, when the entity is on a floor or ceiling |
| `target` | A reference to an entity. The target, when the entity is on or in another entity |
| `relationship` | A string, unrestricted, optional. Written where it distinguishes one way of being attached from another |
| `position` | Where on the target: along and up on a wall, xy within the cell on a flat. Centered when absent |
| `orientation` | A direction, one of the four directions defined by a cell's sides |

```json
"attached": { "wall": { "cell_position": [3, 1], "normal": [0, -1] } }
"attached": { "flat": "floor", "cell_position": [0, 2], "orientation": [1, 0] }
"attached": { "target": { "entity": 6 }, "relationship": "contained_in" }
```

### parent

A file component whose value is a reference to the direct ancestor entity:

```json
"parent": { "entity": 1 }
```

### References

A reference is an object whose only key is `entity`. A reader
recognizes a reference by that form alone; an object with an `entity` key and other keys is not one.

```json
{ "id": 5, "type": "lever", "components": { "controls": { "entity": 1 } } }
```

### access_points

`layout.components.access_points` is a list of references, at least one:

```json
"layout": { "components": { "access_points": [{ "entity": 8 }, { "entity": 9 }] } }
```

Each is a reference to an entity with a `relocator` file component, attached to the open side of a cell
written `<` or `>`.

### What the glyphs require

- **A door glyph:** one entity with a child attached to each wall of that doorway, and to no other wall.
  The walls are the two sides of the edge in the wall notation, and in the block notation the walls the
  floor on either side presents to the doorway block. `stratum_format::door_walls` is what finds them.
- **A stair glyph:** one entity with a `relocator` file component, attached to one of the walls the
  stair cell's open neighbors present to it, which is then its open side, and no other entity with a
  relocator on those walls. In the wall notation, every edge between a stair cell and a floor cell is
  written as a wall, as 0.3 already requires.

### Cell positions and wall addresses in the other sections

Every key whose value is a cell position is `cell_position`, and every wall address is an object with
`cell_position` and `normal`. A wall belongs to the cell whose side it is on, and its address is that
cell's position with the normal that points out of the wall's surface into the cell. The wall on the
other side of the same edge belongs to the neighbor and has its own address (`addressing.md`,
`### Places`).

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

- `entities` is not a list, or an entity in it is not an object.
- An id is not a whole number, or two entities have one id.
- An entity has no `type`, or its `type` is not a string.
- `attached` is not an object; has no target or more than one; has a wall address that is not a cell
  position and a normal, or a wall address or flat cell breaking the rules those have; has a `target`
  that is not a reference; has a `relationship` that is not a string; or has a `position` that is not
  two numbers or an `orientation` that is not a direction.
- `parent` is not a reference.
- An entity is not attached, has no parent and is no entity's parent; or a hierarchy of parents and
  children has no attached entity in it.
- Following `attached` targets and `parent` references from an entity leads back to it.
- In a template, a reference is not an entity's id.
- A door glyph or a stair glyph without what it requires.
- `layout.components.access_points` is absent, not a list, or empty; or has a value that is not a
  reference to an entity with a relocator attached to the open side of a cell written `<` or `>`.
- `zone.id` is not a whole number.

### The example

The cistern of `discrete_layout.md` in 0.4. The stair cells at `[3, 2]` and `[3, 0]` are blocks, and both
staircases open onto the cell between them, `[3, 1]`:

```json
"layout": {
  "components": { "name": "The Flooded Cistern", "access_points": [{ "entity": 4 }, { "entity": 5 }] }
},
"entities": [
  { "id": 1, "type": "door", "components": { "material": "oak", "open": false } },
  { "id": 2, "type": "door_facing", "components": { "parent": { "entity": 1 }, "attached": { "wall": { "cell_position": [1, 1], "normal": [-1, 0] } } } },
  { "id": 3, "type": "door_facing", "components": { "parent": { "entity": 1 }, "attached": { "wall": { "cell_position": [2, 1], "normal": [1, 0] } } } },
  { "id": 4, "type": "staircase", "components": { "attached": { "wall": { "cell_position": [3, 1], "normal": [0, -1] } }, "relocator": { "target": "level_down" } } },
  { "id": 5, "type": "staircase", "components": { "attached": { "wall": { "cell_position": [3, 1], "normal": [0, 1] } }, "relocator": { "target": "level_up" } } }
]
```

## stratum_format

`attached` and `parent` are decoded into fields of their own, being geometry and structure the reader
checks, and every other file component is kept as written. The writer puts the two back into
`components`, so a document written back is equivalent to the file.

```cpp
namespace quetzal::stratum_format
{
    // An id as a file writes it: local in a template, persistent in a saved zone
    using file_id_type = std::uint64_t;

    // An attached file component's target when it is an entity
    struct EntityTarget
    {
        file_id_type id = 0;
    };

    // The attached file component, decoded
    struct Attachment
    {
        std::variant<WallAddress, FlatCell, EntityTarget> target;
        std::optional<std::string> relationship;
        std::optional<surface_point_type> position;
        std::optional<discrete_vector_type> orientation;
    };

    struct Entity
    {
        file_id_type id = 0;
        std::string type;
        std::optional<Attachment> attached;
        std::optional<file_id_type> parent_id;
        values_type components; // Every other file component, as written
    };

    struct Document
    {
        ...
        std::optional<file_id_type> zone_id; // A saved zone's own
        std::vector<Entity> entities;
    };
}
```

A new `references.hpp`, since both the reader and making a zone walk file components for references:

```cpp
    // Every reference inside an object of file components, in the order they are written
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
- **A door** is a `door` entity and a `door_facing` child per wall, each attached to its wall.
- **A staircase** is a `staircase` entity with its `relocator`, attached to the wall on its open side.
  The open side is the side each generator writes as a staircase's `facing` in 0.3, and the `facing`
  file component goes.
- **Stair blocks, their placement and the walls around them** are built already, on 0.3.
- **The access points** are references to every staircase.

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
            std::optional<std::string> relationship;
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
merging of a glyph's default entity with what `attachments` has at the same place goes from that design.

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

        // A reference to the entity, which adds it to the index
        EntityReference make_reference(flecs::entity target);

        // The loaded entity with that id, null where it is not loaded
        flecs::entity entity(persistent_id_type id) const;

        // The zone an entity in the index is in, loaded or not
        persistent_id_type zone_of(persistent_id_type id) const;

        // The entity, re-resolved where the one last resolved is no longer alive; null where it is not loaded
        flecs::entity resolve(EntityReference& reference) const;

    private:

        std::unique_ptr<IdAllocator> m_id_allocator;
        std::unordered_map<persistent_id_type, flecs::entity> m_loaded_entities;

        // By the id of an entity something refers to: the id of the zone it is in
        std::unordered_map<persistent_id_type, persistent_id_type> m_entity_zones;
```

- **`m_loaded_entities`** has an entry for each loaded entity with a `PersistentId`, added by an
  `OnSet` observer and removed by an `OnRemove` one, so no path that creates or deletes an entity can
  leave it behind.
- **`m_entity_zones`** has an entry for each entity something refers to: added when the first
  reference to it is made, by `make_reference` or by loading a template, changed when it migrates, kept
  when its zone unloads, and removed when it is destroyed. `zone_of` asserts that the entity has one.
  `attached` targets and parents are not references in this sense, being links within one zone.

### Loading a template

`create_zone` with a `Content` whose `zone_id` is absent:

1. Allocate the zone's persistent id.
2. Allocate a persistent id for each entity, in `Content` order, into a map from local id to persistent
   id that lasts as long as the call.
3. Translate every reference in the layout's, rooms', walls', flats' and entities' file components, and
   every `attached` target and `parent`, through the map.
4. Build the entities, each with its `PersistentId` and its type, then give each child its parent, then
   attach them.
5. Add each entity that a reference in a file component points to to `m_entity_zones`.

A template loaded twice gives two sets of ids, and so two instances. The ids depend on the world's count, so tests
compare what is built and how it links rather than id values.

### Loading a saved zone

`create_zone` with a `Content` whose `zone_id` is present allocates nothing and translates nothing:
every id is already persistent, and `m_entity_zones` already has what the zone's references point to.
Loading re-registers each entity in `m_loaded_entities`, and a reference into the zone resolves again
the next time it is asked.

## Links

```cpp
namespace quetzal::stratum_sim
{
    // On an entity that is attached: its target and how
    struct Attached
    {
        flecs::entity target;       // A wall's entity, a flat cell's entity, or an entity
        std::optional<flecs::entity> relationship;
        surface_point_type position;
        std::optional<discrete_vector_type> orientation;
    };

    // On a target: everything attached to it
    struct AttachedEntities
    {
        std::vector<flecs::entity> entities;
    };

    void attach(flecs::entity entity, flecs::entity target, std::optional<flecs::entity> relationship,
        const surface_point_type& position, std::optional<discrete_vector_type> orientation);
    void detach(flecs::entity entity);

    void make_child(flecs::entity child, flecs::entity parent);
}
```

- **`attach` and `detach` keep both ends in sync.** An `OnRemove` observer on `Attached` removes an
  entity from its target's list when the entity is deleted; one on `AttachedEntities` detaches
  everything from a target being deleted. What happens to what was attached --- the contents of a
  smashed chest --- is the deleting rule's to decide, not the observer's.
- **`make_child`** sets the child's `flecs::Parent`. Flecs keeps the parent's children and deletes them
  with it, so nothing here lists them.
- **A relationship is an entity** `World` creates the first time a name is used, in a `relationships`
  scope, so `Attached::relationship` compares as an entity. Traits such as `Exclusive` are set on it
  where the vocabulary needs them.
- **An entity has at most one `Attached`.** Something on several targets is a parent whose children each
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

beside the zone's layout and actor index, the rework's `Zone` becoming the home of all three.

```cpp
        flecs::entity wall_entity(flecs::entity zone, id_type wall_id) const; // Null where it has none
        flecs::entity flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point_type& cell_position) const;
        flecs::entity ensure_wall_entity(flecs::entity zone, id_type wall_id);
        flecs::entity ensure_flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point_type& cell_position);
```

A place's entity, its list and a parent's children are derived, and rebuilt when the zone loads
(`addressing.md`, `### A saved zone`). A place has no `PersistentId`; it is referred to by its address.

### Wall ids from the normal

Encoding a wall address is a function of its own, taking the normal as it is:

```cpp
        // The id of the wall at an address, from the cell position's slot and the normal's direction index
        id_type encode_wall_id(const discrete_point_type& cell_position, const discrete_vector_type& normal) const;
```

- **The id is the slot times four plus the normal's direction index.** `load_content` encodes every
  address in a file with it.
- **`wall_toward` stays the movement query.** Moving in a direction meets the wall whose normal is the
  opposite direction, so `wall_toward` negates the direction, which is what that question means, and
  encodes with `encode_wall_id`.
- **Everything that decodes a wall id** --- the normal from an id, the cell's mask bit for the wall --- uses
  the normal's direction index.

### Crossing a wall

```cpp
namespace quetzal::stratum_sim
{
    enum class AccessState { Open, Closed, Locked };
    enum class DetectionState { Visible, Concealed, Revealed };

    // On an entity there is a way through, such as a door or a staircase
    struct Access
    {
        AccessState state = AccessState::Closed;
    };

    // On an entity that is not seen by everything, with the state of the actors that have their own
    struct Detection
    {
        DetectionState state = DetectionState::Visible;
        std::unordered_map<persistent_id_type, DetectionState> actor_states;
    };

    // A property of an entity, from the entity, then its parent, and so on. None where nothing in that
    // chain has it
    std::optional<AccessState> access(flecs::entity entity);
    std::optional<DetectionState> detection(flecs::entity entity, flecs::entity actor);

    // Whether any entity attached to the wall lets the actor through
    bool crossable(flecs::entity zone, id_type wall_id, flecs::entity actor);
}
```

- **`crossable`** takes the wall's entity, then asks each entity in its `AttachedEntities`: it lets the
  actor through when its access is open and its detection for that actor is not concealed. A wall with no
  entity, or with nothing attached that any of this is true of, cannot be crossed.
- **No access is no way through.** An entity whose chain has no `Access` at all --- a torch, a lever --- is
  not something to pass through, so nothing looks further at it.
- **A property comes from the entity or from an ancestor.** A door facing has no `Access` of its own and
  its door has one, which is what makes both sides of a door open together. A staircase's own is open and
  stays open. Nothing is read from a parent an entity does not have.
- **One rule, naming no type.** `crossable` reads `Access` and `Detection` and nothing else, so a new kind
  of way through is a matter of what it has rather than of code here.
- **The answer depends on the actor,** so `Layout::set_crossable`, `m_crossable` and the protected
  `Layout::crossable` go: `move`, `neighbors` and `visible` take the crossing predicate as `move` takes
  `obstructed`, and `World` binds it to the actor asking. `World` also gains `neighbors` and `visible` beside
  its `move`, so nothing outside it binds a predicate and `ai.cpp` stops holding a layout.
- **One wall question in the layout,** `DiscreteLayout::blocked` and the predicate's call site becoming
  `passable(cell_position, direction, crossable)`: no wall ahead is passable, a wall is what the predicate
  answers for. The grid boundary is no longer a branch of its own. An open cell at the edge has a wall, which
  the reader requires and a generator that omits it is wrong about, and a wall with nothing attached is not
  crossable, so the walk stops there. Absence of a wall asserts that the cell beyond is on the grid.

### What the queries become

- **What is on a flat cell:** the flat cell's entity, then its `AttachedEntities`.
- **Where an entity is:** its `Attached`, or where it has none, its parent's or its children's.
- **The other side of a door:** the door facing's parent, then that parent's other child.
- **Who is at a place:** the zone's actor index (`## Actors`).

`OnWall`, `DoorFacing`, `facing_on`, `door_of`, `other_facing`, `create_door`, `create_doors`,
`door_walls_type` and `load_layout`'s `door_type` go.

## Actors

An actor is located by its `placement_type`. Each zone has an actor index, of the kind its layout
supplies, so finding who is at a place is a lookup rather than a scan of the zone.

```cpp
namespace quetzal::stratum_sim
{
    // The actors in one zone by where they are, derived from their placements
    class ActorIndex
    {
    public:
        virtual ~ActorIndex() = default;

        // Adds the actor, or moves it where it is already in the index
        virtual void place(flecs::entity actor, const placement_type& placement) = 0;
        virtual void remove(flecs::entity actor) = 0;

        // Both ordered by entity id
        virtual std::vector<flecs::entity> actors_at(const placement_type& placement) const = 0;
        virtual std::vector<flecs::entity> actors_within(const placement_type& placement, value_type range) const = 0;
    };

    class DiscreteActorIndex final : public ActorIndex
    {
    public:
        explicit DiscreteActorIndex(const DiscreteLayout& discrete_layout);

        void place(flecs::entity actor, const placement_type& placement) override;
        void remove(flecs::entity actor) override;
        std::vector<flecs::entity> actors_at(const placement_type& placement) const override;
        std::vector<flecs::entity> actors_within(const placement_type& placement, value_type range) const override;

        std::vector<flecs::entity> actors_at(const discrete_point_type& cell_position) const;

    private:
        const DiscreteLayout& m_discrete_layout;
        std::unordered_map<size_t, std::vector<flecs::entity>> m_actors_by_slot; // By slot: the actors in that cell
        std::unordered_map<flecs::entity_t, size_t> m_slots_by_actor; // By actor: the slot of the cell it is in
    };
}
```

`Layout` gains:

```cpp
        virtual std::unique_ptr<ActorIndex> create_actor_index() const = 0;
```

- **The index follows the placements.** An `OnSet` observer on `placement_type` places the actor in its
  zone's index and an `OnRemove` one removes it, so no path that moves, creates or deletes an actor can
  leave the index behind. A move costs two map updates.
- **`relocate` removes the actor from the old zone's index** before changing its zone, and the observer
  places it in the new one.
- **`entities_at` and `entities_within` ask the index,** `DiscreteActorIndex::actors_within` walking the
  cells in range, and keep their ordering by entity id.
- **The index is derived,** so it is never saved, and loading a zone rebuilds it. It cannot disagree
  with the placements, which remain the one record of where an actor is.

## Migration

`World::relocate` moves an entity from one zone to another. It detaches the entity, or removes it from
the old zone's actor index if it is an actor, changes its parent to the destination zone, or for the
player the zone it records, attaches or places it at the destination, and updates its entry in
`m_entity_zones` where it has one. What is attached to it and its children move with it.

**The player is owned by the session.** Its flecs parent is the session's entity rather than a zone, and
a component on it records the zone it is in, which `relocate` changes and `World::zone_of` reads. It is
in that zone's actor index like any actor, but not in the zone's subtree, so unloading or saving the zone
leaves it out. What the player carries is attached to it, `contained_in`, and is owned by the session
too, and both are saved with the session.

## Multiplayer

Nothing to build now. What keeps it open is in `addressing.md`: `persistent_id_type` used everywhere a
persistent id is kept, allocation through `IdAllocator`, no meaning read from an id. A server-hosted
game uses `CountingIdAllocator` on the server. Several servers use an allocator that puts its own
number in the high bits. Worlds with no coordination use UUIDs, either as `persistent_id_type` or
mapped at the boundary where an entity crosses from one world to another.

## Flecs

- **`flecs::Parent`** is the non-fragmenting hierarchy of Flecs 4.1.6 (`library3/flecs`), meant for
  small structured hierarchies of a few children each, which is what a door and its door facings are.
  `ChildOf` remains how a zone contains what is in it, that being the large unstructured case.
  `test/flecs_test`'s `parent hierarchy` group checks what this design relies on:
  - a child refers to its parent, and a parent iterates its children, which Flecs keeps in a vector on the
    parent, so no component of ours lists them;
  - deleting a parent deletes its children, in bulk rather than one at a time;
  - the children of many parents are queryable together, which the view needs;
  - an entity has one parent, in one storage or the other and never both, so a child belongs to its
    zone through its parent rather than directly;
  - nesting goes as deep as it likes, a child being a parent in turn: a chest in a zone, a box in the
    chest, a tin in the box, a coin in the tin, each reading back its own parent and children, the
    whole chain reachable by path, and deleting the zone deleting all of it through both storages.
- **A `Parent` hierarchy does not survive flecs JSON.** `to_json` writes it, and `from_json` aborts the
  process on a world holding one, a bare parent and child being enough. So nothing may rely on flecs
  JSON for a hierarchy; a saved zone writes each child's `parent` itself, as the zone file format does
  for every entity.
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
and prefabs as `ChildOf` and `IsA`, which is the form chosen here: where an entity is and what its
parent is are components. NetHack's per-cell object chains are a location index outside any entity
system. Snowflake ids at Twitter and Discord divide 64 bits between a time, a machine and a count, and
Bethesda's form ids between a plugin's place in the load order and its own count.

## What checks it

- **`stratum_format_test`:** every added error from a valid 0.4 file with one change; `attached`, with
  and without a relationship, and `parent` decoded and written back; references found and translated in
  nested file components; a saved zone's references not checked; the round trip.
- **`dungen_test`, `catagen_test`:** ids in the order written and the same for the same parameters;
  every door with its door facings; every staircase attached to its open side, and in catagen a wall on
  every edge between a stair cell and an open cell; the access points as references to every staircase.
- **`flecs_test`:** `flecs::Parent` children, cascading deletion, nesting, and queries across parents,
  as step 3 checks them.
- **`stratum_sim_test`:**
  - loading a template twice gives different persistent ids and the same structure;
  - every reference translated, and one to an entity resolving to it;
  - an entity something refers to in the index, and its entry following it across zones;
  - `attach` and `detach` keeping both ends; deleting an entity or a target leaving no link behind;
  - a file's `parent` making a Flecs child, and deleting a parent deleting its children;
  - `encode_wall_id` and `wall_toward` giving the same id for the same wall, for all four normals;
  - a place's entity made when something is attached and gone when nothing is;
  - `crossable` through `Access` and `Detection`: a door open, closed and locked; a door facing taking its
    door's access, and concealed for one actor and not another; an entity with no `Access` in its chain
    never crossable; a staircase crossed from its open side and from no other;
  - the predicate passed per call: one wall crossed by one actor and not by another, in the same layout;
  - the actor index following every move, migration, creation and deletion, and `entities_at` and
    `entities_within` returning what a scan of the zone returns;
  - migration keeping the persistent id and the runtime entity, and updating the index;
  - the player in its zone's actor index, and left out when that zone is unloaded;
  - a reference re-resolving after its zone is unloaded and loaded again;
  - `CountingIdAllocator` never repeating across a save of its count.
- **`stratum_view_test`:** its frames, which print wall ids, showing 0.4's wall ids and the walls around
  stair cells, and nothing else different.

## Steps

- [ ] **1. Format 0.4 in `stratum_format`.** The document types, the reader's checks, the writer,
      `references.hpp`, `cell_position` and separate wall addresses throughout, and the format suite's
      data and mutations.
- [ ] **2. The generators and every zone file in 0.4.** Both generators, their suites and references,
      the sim and view data, the assets and the tool samples. `load_layout` still reads geometry alone,
      stair cells being blocks already (`work_plan.md`, stage 8). Each staircase's attachment moves from
      its cell's floor to the wall on its open side, and `facing` goes.
- [x] **3. `flecs::Parent` checked in `flecs_test`.** Done: `## Flecs` has what it found, including that
      a hierarchy cannot be read back from flecs JSON.
- [ ] **4. Persistent ids.** `persistent_id_type`, `IdAllocator`, `CountingIdAllocator`, `PersistentId`,
      `m_loaded_entities` and `m_entity_zones`, `EntityReference`, `make_reference` and `resolve`.
- [ ] **5. Loading a zone, against 0.4 and with persistent ids.** `discrete_layout_rework.md`'s
      `## Loading a zone`, with `Content` as above. Everything that is not geometry loads into
      `Content`, and `World` builds a zone from the layout and it: an entity for every object of file
      components, each with its type, and a property resolved when queried from the object, its room, the
      layout and its type's default. Making a zone allocates persistent ids and translates references,
      attachment links both ways through `Attached`, `AttachedEntities` and the places' entities, and a
      parent has its children, so `crossable` searches nothing, asking each attached entity's
      `Access` and `Detection` for the actor crossing. Each staircase gets its relocator with its target. Wall ids
      are encoded from the normal by `encode_wall_id`. `World` contains zones rather than layouts, and the
      door functions go.
- [ ] **6. Migration and the player's ownership.** `relocate` across zones through the links and the
      index, and the player and what it carries owned by the session.
- [ ] **7. Saved zones.** The `zone` section, unload writing a zone and load reading one back, with the
      session lifecycle of stage 7. Writing one walks the zone's subtree rather than querying its
      children, since anything with a parent of its own hangs below that parent.
- [ ] **8. Relocating through a relocator,** stage 6's item in `work_plan.md`, binding targets by
      persistent id. Resolving a target, bound or unbound, to a zone the world has or generates,
      writing the resolution back as a bound target, placing the arriving actor at one of that zone's
      unbound access points, and binding both ends to each other.
- [ ] **9. The zone actor index.** `ActorIndex`, `DiscreteActorIndex`, `Layout::create_actor_index`, the
      observers, and `entities_at` and `entities_within` asking the index. It builds against the zones as
      they are now.

**Steps 1 and 2 are one change to the build.** After step 1 alone the generators do not compile, being
written against the 0.3 document, and no checked-in zone file reads, the reader accepting 0.4 only. The
build is clean again once both are done.

Steps 1, 2 and 9 change no behavior and no frame. Step 5 is where behavior moves from the glyphs to the entities, and the frames confirm that
nothing a player sees changed apart from the wall ids they print.

## Blockers

- **Step 7 waits on stage 7 of `work_plan.md`,** the session lifecycle: a session that outlives one
  zone, and a zone added to the world when it is encountered. A zone cannot be written out and read
  back until there is something to unload it for. Where a saved zone is written is open below.
- **Step 8 waits on steps 4 and 6,** a bound target being a persistent id that resolves through the
  world index.

## Open

- **Names.** `attached`, `parent`, `door_facing`, `file_id_type`, `EntityTarget`, `PersistentId`,
  `EntityReference`, `make_reference`, `ZonePlaces`, `Access`, `Detection` and `ActorIndex` are working names. `placement` is
  not a candidate for the file's `attached`, `placement_type` already being an actor's position and
  direction.
- **Where a saved zone is written,** and how a world's zones are found by persistent id on disk. The
  session lifecycle's to settle.
