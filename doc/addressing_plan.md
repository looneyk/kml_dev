# Ids and Addressing --- Plan

Status: steps 1, 2 and 3 built; the rest designed
Date: 2026-09-14, revised 2026-09-22

The design behind the spec in `addressing.md`: one list of entities in a zone file, with `site` and `parent` as file
components the format defines and children derived from `parent`; persistent ids; links both ways between an entity
and the place it is sited on; references between entities within a zone and across zones; and an actor index for each
zone. `site_plan.md` is where every entity is and what it belongs to. Open items are listed in `work_plan.md`, and
`game_system_reference.md` describes the implementation.

## Decided

- **One list of entities**, in a single `entities` section.
- **How an entity relates to a place or another entity is a file component:** `site` for what it is in or on, `parent`
  for the entity it is a child of.
- **Children are derived.** A parent does not list its children. Loading gives each child a `flecs::Parent` from its
  `parent`, and Flecs keeps each parent's children, so no component of ours lists them (`## Flecs`).
- **Every entity has a place.** It is sited, has a parent, or is a parent, and every hierarchy of parents and children
  has a sited entity in it. Following sites and parents from any entity never leads back to it.
- **Every door and every staircase is an entity,** each having state, even a staircase whose state never changes. The
  reader requires an entity on every door glyph's walls and on every stair glyph's open side, as it requires the access
  points.
- **A door has a door facing on each wall of its doorway,** a child of the door sited on that wall.
- **A staircase is a block with a wall on each side,** sited on the wall on its open side, with its relocator and
  target. That wall's normal is the direction of travel from the staircase into the room, so a staircase has no
  orientation of its own. Its other sides are ordinary walls, each with file components of its own. Where a stair block
  may go is the generators' concern: it must not cut a passage in two, and at a dead end its open side is toward the one
  open neighbor.
- **A reference is written `{ "entity": id }`,** an object with that one key, so making a zone translates every
  reference without knowing what a file component means.
- **Walls are barriers to movement** and define the borders of rooms. Passage through a wall comes from what is sited
  on it, a staircase included: an actor moves into one through its open side.
- **`crossable` reads the properties of each entity sited on the wall:** its access state, open, closed or locked, and
  its detection state for the actor asking, each resolved from the entity, then its parent. An entity with no access
  state anywhere in that chain is not crossable. One rule reads the properties, with no case for a door, a staircase or
  concealment, and nothing on the wall or the cell marks it crossable (`## Crossing a wall`).
- **Each zone has an actor index,** derived from the actors' placements, which finds the actors at a place without a
  search (`## Actors`).
- **The player is in a zone, and owned by the session.** It is in its zone's actor index like any actor, but unloading
  or saving the zone leaves it out (`## Migration`).
- **A dropped object is indexed in its zone,** as everything sited on a place is, and is accessible while the actor is
  in that zone, not from another zone the actor moves to.
- **The world index has only the entities something refers to.** It is how a reference finds the zone its entity is in.
- **The zone file format is not Flecs's,** but the ECS can be built from it. A saved zone is a zone file with a
  top-level `zone` section containing the zone's persistent id, whose presence makes every id in the file persistent.
- **Format 0.4 and persistent ids are built before loading a zone,** so loading is written once, against them.
- **A direction an entity has is its orientation.**
- **The relocators an arriving actor can be placed at are the zone's access points,** listed in
  `layout.components.access_points`.
- **Zone files are written by generators, tools and saves,** not by hand; being readable as text suffices.
- **A cell position is `cell_position`,** as every format key and every code name for one.
- **A wall id is encoded from the normal,** by `encode_wall_id` (`### Wall ids from the normal`).

## The format and the generators

Built (steps 1 and 2): `discrete_layout.md` specifies format 0.4, `addressing.md` its ids and addresses, and
`game_system_reference.md` describes `stratum_format` and the generators that write it.

## Loading a zone against 0.4

`discrete_layout_rework.md` (`## Loading a zone`) outlines loading, with this `Content`:

```cpp
namespace quetzal::stratum_sim
{
    struct Content
    {
        struct Entity
        {
            stratum_format::stored_entity_id_type id = 0;
            std::string type;
            std::optional<Site> site; // The wall address encoded as a wall id, the flat role as a flat id
            std::optional<stratum_format::stored_entity_id_type> parent_id;
            values_type components;
        };

        std::optional<stratum_format::stored_entity_id_type> zone_id;
        values_type layout;
        std::vector<values_type> rooms; // By room id
        std::map<id_type, values_type> walls; // By wall id
        std::map<id_type, FlatContent> flats; // By flat id
        std::vector<Entity> entities;
    };
}
```

`load_content` turns wall addresses into wall ids, flat roles into flat ids, and a flat cell where an entity was
authored into a placement, and copies the rest. Every door and staircase is written in the file, so no glyph becomes an
entity.

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

- **`m_loaded_entities`** has an entry for each loaded entity with a `PersistentId`, added by an `OnSet` observer and
  removed by an `OnRemove` one, so no path that creates or deletes an entity can leave it behind.
- **`m_entity_zones`** has an entry for each entity something refers to: added when the first reference to it is made,
  by `make_reference` or by loading a template, changed when it migrates, kept when its zone unloads, and removed when
  it is destroyed. `zone_of` asserts that the entity has one. Sites and parents are not references in this sense, being
  links within one zone.

### Loading a template

`create_zone` with a `Content` whose `zone_id` is absent:

1. Allocate the zone's persistent id.
2. Allocate a persistent id for each entity, in `Content` order, into a map from local id to persistent id that lasts
   as long as the call.
3. Translate every reference in the layout's, rooms', walls', flats' and entities' file components, and every `parent`,
   through the map.
4. Build the entities, each with its `PersistentId` and its type, then give each child its parent and each entity its
   site.
5. Add each entity that a reference in a file component points to to `m_entity_zones`.

A template loaded twice gets two sets of ids, and so two instances. The ids depend on the world's count, so tests
compare what is built and how it links rather than id values.

### Loading a saved zone

`create_zone` with a `Content` whose `zone_id` is present allocates nothing and translates nothing: every id is already
persistent, and `m_entity_zones` already has what the zone's references point to. Loading re-registers each entity in
`m_loaded_entities`, and a reference into the zone resolves again the next time it is resolved.

## Links

A sited entity's `Site` has its address. The other direction is a list on the place's entity:

```cpp
namespace quetzal::stratum_sim
{
    // On a place's entity: everything sited on it
    struct AttachedEntities
    {
        std::vector<flecs::entity> entities;
    };
}
```

- **`reparent` and setting a site keep both ends in sync.** An `OnRemove` observer on `Site` removes an entity from its
  place's list when the entity is deleted; one on `AttachedEntities` clears the site of everything on a place being
  deleted. What happens to what was on it --- the contents of a smashed chest --- is the deleting rule's to decide, not
  the observer's.
- **A child's `flecs::Parent`** is set by `reparent`. Flecs keeps the parent's children and deletes them with it.
- **An entity has at most one site.** Something on several places is a parent whose children each have their own, so a
  site never has a second case.

### Places

A place's entity exists where it has components or something sited on it, and goes when it has neither. `World` keeps,
per zone:

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

A place's entity, its list and a parent's children are derived, and rebuilt when the zone loads (`addressing.md`,
`### A saved zone`). A place has no `PersistentId`; it is referred to by its address.

### Wall ids from the normal

```cpp
        // The id of the wall at an address, from the cell position's slot and the normal's direction index
        id_type encode_wall_id(const discrete_point_type& cell_position, const discrete_vector_type& normal) const;
```

- **The id is the slot times four plus the normal's direction index.** `load_content` encodes every address in a file
  with it.
- **`wall_toward` stays the movement query.** Moving in a direction meets the wall whose normal is the opposite
  direction, so `wall_toward` negates the direction and encodes with `encode_wall_id`.
- **Everything that decodes a wall id** --- the normal from an id, the cell's mask bit for the wall --- uses the
  normal's direction index.

### Crossing a wall

```cpp
namespace quetzal::stratum_sim
{
    enum class AccessState { Open, Closed, Locked };
    enum class DetectionState { Visible, Concealed, Revealed };

    // On an entity that is a way through, such as a door or a staircase
    struct Access
    {
        AccessState state = AccessState::Closed;
    };

    // On an entity that is not seen by every actor, with the states of the actors that have their own
    struct Detection
    {
        DetectionState state = DetectionState::Visible;
        std::unordered_map<persistent_id_type, DetectionState> actor_states;
    };

    // A property of an entity, from the entity, then its parent, and so on. None where nothing in that chain has it
    std::optional<AccessState> access(flecs::entity entity);
    std::optional<DetectionState> detection(flecs::entity entity, flecs::entity actor);

    // Whether any entity sited on the wall lets the actor through
    bool crossable(flecs::entity zone, id_type wall_id, flecs::entity actor);
}
```

- **`crossable`** takes the wall's entity and asks each entity in its `AttachedEntities`: one lets the actor through
  when its access is open and its detection for that actor is not concealed. A wall with no entity, or with nothing on
  it that lets the actor through, cannot be crossed.
- **No access is no way through.** An entity whose chain has no `Access` at all --- a torch, a lever --- is not
  examined further.
- **A property comes from the entity or from an ancestor.** A door facing has no `Access` of its own and its door has
  one, so both sides of a door open together. A staircase's own is open and stays open.
- **One rule, with no type in it.** `crossable` reads `Access` and `Detection` and nothing else, so a new kind of way
  through is a matter of the components it has rather than of code here.
- **The result depends on the actor.** `move`, `neighbors` and `visible` take a `crossable_function_type`, and
  `World::move`, `World::neighbors` and `World::visible` bind `crossable(zone, wall_id)`; each binds
  `crossable(zone, wall_id, actor)` instead, for the actor moving or looking.

### What the queries become

- **What is on a wall or flat cell:** the place's entity, then its `AttachedEntities`.
- **Where an entity is:** its `Site`, and its parent where the site has no address.
- **The other side of a door:** the door facing's parent, then that parent's other child.
- **Who is at a place:** the zone's actor index (`## Actors`).

`DoorFacing`, `facing_on`, `door_of`, `other_facing`, `create_door`, `create_doors`, `door_walls_type` and
`load_layout`'s `door_type` go.

## Actors

An actor is located by its site's placement. Each zone has an actor index, of the kind its layout supplies, so finding
who is at a place is a lookup rather than a scan of the zone.

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

- **The index follows the placements.** An `OnSet` observer on an actor's `Site` places the actor in its zone's index
  and an `OnRemove` one removes it, so no path that moves, creates or deletes an actor can leave the index behind. A
  move costs two map updates.
- **Relocating removes the actor from the old zone's index** before changing its zone, and the observer places it in
  the new one.
- **`entities_at` and `entities_within` query the index,** `DiscreteActorIndex::actors_within` walking the cells in
  range, and keep their ordering by entity id.
- **The index is derived,** so it is never saved, and loading a zone rebuilds it. It cannot disagree with the
  placements, which remain the one record of where an actor is.

## Migration

`reparent` moves an entity from one zone to another: it removes the entity from the old zone's place list or actor
index, sets its parent to the destination zone, or for the player the zone it is in, and its site at the destination,
and updates its entry in `m_entity_zones` where it has one. Its children move with it.

**The player is owned by the session.** Its flecs parent is the session's entity rather than a zone, and a component
on it refers to the zone it is in, which relocating changes and `World::zone_of` reads. It is in that zone's actor
index like any actor, but not in the zone's subtree, so unloading or saving the zone leaves it out. What the player
carries is its child, sited `on` it, and is owned by the session too; both are saved with the session.

## Flecs

- **`flecs::Parent`** is the non-fragmenting hierarchy of Flecs 4.1.6 (`library3/flecs`), meant for small structured
  hierarchies of a few children each, as a door and its door facings are. `ChildOf` remains how a zone contains what is
  in it, that being the large unstructured case. `test/flecs_test`'s `parent hierarchy` group checks what this design
  relies on:
  - a child refers to its parent, and a parent iterates its children, which Flecs keeps in a vector on the parent;
  - deleting a parent deletes its children, in bulk rather than one at a time;
  - the children of many parents are queryable together, which the view needs;
  - an entity has one parent, in one storage or the other and never both, so a child belongs to its zone through its
    parent rather than directly;
  - nesting is unbounded, a child being a parent in turn: a chest in a zone, a box in the chest, a tin in the box, a
    coin in the tin, each reading back its own parent and children, the whole chain reachable by path, and deleting the
    zone deleting all of it through both storages.
- **A `Parent` hierarchy does not survive flecs JSON.** `to_json` writes it, and `from_json` aborts the process on a
  world containing one, a bare parent and child being enough. A saved zone writes each child's `parent` itself, as the
  zone file format does for every entity.
- **`DontFragment`** is available for a relationship that needs a pair after all.
- **Reflection** is registered member by member for every new component, `sim_zone_model.md`'s obligation, or a saved
  zone is written with names and no values.

## What checks it

- **`stratum_sim_test`:**
  - loading a template twice gives different persistent ids and the same structure;
  - every reference translated, and one to an entity resolving to it;
  - an entity something refers to in the index, and its entry following it across zones;
  - a place's list kept in sync with the sites on it; deleting an entity or a place leaving no link behind;
  - a file's `parent` making a Flecs child, and deleting a parent deleting its children;
  - `encode_wall_id` and `wall_toward` returning the same id for the same wall, for all four normals;
  - a place's entity made when something is sited on it and gone when nothing is;
  - `crossable` through `Access` and `Detection`: a door open, closed and locked; a door facing taking its door's
    access, and concealed for one actor and not another; an entity with no `Access` in its chain never crossable; a
    staircase crossed from its open side and from no other;
  - one wall crossed by one actor and not by another, in the same layout;
  - the actor index following every move, migration, creation and deletion, and `entities_at` and `entities_within`
    returning what a scan of the zone returns;
  - migration keeping the persistent id and the runtime entity, and updating the index;
  - the player in its zone's actor index, and left out when that zone is unloaded;
  - a reference re-resolving after its zone is unloaded and loaded again;
  - `CountingIdAllocator` never repeating across a save of its count.
- **`stratum_view_test`:** its frames, which print wall ids, unchanged apart from the wall ids.

## Steps

- [x] **1. Format 0.4 in `stratum_format`.**
- [x] **2. The generators and every zone file in 0.4.**
- [x] **3. `flecs::Parent` checked in `flecs_test`** (`## Flecs`).
- [ ] **4. Persistent ids.** `persistent_id_type`, `IdAllocator`, `CountingIdAllocator`, `PersistentId`,
      `m_loaded_entities` and `m_entity_zones`, `EntityReference`, `make_reference` and `resolve`.
- [ ] **5. Loading a zone, against 0.4 and with persistent ids.** `discrete_layout_rework.md`'s `## Loading a zone`,
      with `Content` as above. Everything that is not geometry loads into `Content`, and `World` builds a zone from the
      layout and it: an entity for every object of file components, each `IsA` its type, its component objects decoded
      into typed components (`modding_plan.md`), and a property resolved when queried from the object, its room, the
      layout and its type's default. Making a zone allocates persistent ids and
      translates references, links sites both ways through `AttachedEntities` and the places' entities, and gives each
      parent its children, so `crossable` searches nothing, asking each sited entity's `Access` and `Detection` for the
      actor crossing. Each staircase gets its relocator with its target. Wall ids are encoded from the normal by
      `encode_wall_id`. `World` contains zones rather than layouts, and the door functions go.
- [ ] **6. Migration and the player's ownership.** Relocating across zones through the links and the index, and the
      player and what it carries owned by the session.
- [ ] **7. Saved zones.** The `zone` section, unload writing a zone and load reading one back, with the session
      lifecycle of stage 7. Writing one walks the zone's subtree rather than querying its children, since anything with
      a parent of its own is below that parent.
- [ ] **8. Relocating through a relocator,** stage 6's item in `work_plan.md`, binding targets by persistent id.
      Resolving a target, bound or unbound, to a zone the world has or generates, writing the resolution back as a bound
      target, placing the arriving actor at one of that zone's unbound access points, and binding both ends to each
      other.
- [ ] **9. The zone actor index.** `ActorIndex`, `DiscreteActorIndex`, `Layout::create_actor_index`, the observers,
      and `entities_at` and `entities_within` querying the index. It builds against the zones as they are now.

Step 9 changes no behavior and no frame. Step 5 moves behavior from the glyphs to the entities, and the frames confirm
that nothing a player sees changed apart from the wall ids they print.

## Blockers

- **Step 7 waits on stage 7 of `work_plan.md`,** the session lifecycle: a session that outlives one zone, and a zone
  added to the world when it is encountered. A zone cannot be written out and read back until there is something to
  unload it for.
- **Step 8 waits on steps 4 and 6,** a bound target being a persistent id that resolves through the world index.

## Open

- **Names.** `PersistentId`, `EntityReference`, `make_reference`, `AttachedEntities`, `ZonePlaces`, `Access`,
  `Detection` and `ActorIndex` are working names.
- **Where a saved zone is written,** and how a world's zones are found by persistent id on disk. The session
  lifecycle's to settle.
