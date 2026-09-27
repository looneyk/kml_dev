# Ids and Addressing --- Plan

Status: steps 1 to 6 built; the rest designed
Date: 2026-09-14, revised 2026-09-26

This is the design behind the spec in `addressing.md`. It covers one list of entities in a zone file, with `site` and
`parent` as file components that the format defines and children derived from `parent`; persistent ids; links in both
directions between an entity and the wall or flat cell it is sited on; references between entities within a zone and
across zones; and an actor index for each zone. `site_plan.md` describes where every entity is and what it belongs to.
Open items are listed in `work_plan.md`, and `game_system_reference.md` describes the implementation.

## Decided

- **There is one list of entities**, in a single `entities` section.
- **How an entity relates to a location or another entity is a file component:** `site` for the object it is in or
  on, and `parent` for the entity it is a child of.
- **Children are derived.** A parent does not list its children. Loading gives each child a `flecs::Parent` from its
  `parent`, and Flecs keeps each parent's children, so no component of ours lists them (`## Flecs`).
- **Every entity has a location.** It is sited, has a parent, or is a parent, and every hierarchy of parents and
  children has a sited entity in it. Following sites and parents from any entity never leads back to it.
- **Every door and every staircase is an entity,** since each has state, even a staircase whose state never changes.
  The reader requires an entity on every door glyph's walls and on every stair glyph's open side, as it requires the
  access points.
- **A door has a door facing on each wall of its doorway,** which is a child of the door sited on that wall.
- **A staircase is a block with a wall on each side,** and is sited on the wall on its open side, with its relocator
  and target. That wall's normal is the direction of travel from the staircase into the room, so a staircase has no
  orientation of its own. Its other sides are ordinary walls, each with file components of its own. Where a stair block
  may go is the generators' concern: it must not cut a passage in two, and at a dead end its open side faces the one
  open neighbor.
- **A reference is written `{ "entity": id }`,** an object with that one key, so making a zone translates every
  reference without knowing what a file component means.
- **Walls are barriers to movement** and define the borders of rooms. Passage through a wall comes from the entities
  sited on it, a staircase included: an actor moves into a staircase through its open side.
- **`crossable` reads the properties of each entity sited on the wall:** its access state, which is open, closed or
  locked, and its detection state for the actor asking, each resolved from the entity and then its ancestors. An
  entity with no access state anywhere in that chain is not crossable. One rule reads the properties, with no case for
  a door, a staircase or concealment, and nothing on the wall or the cell marks it crossable (`## Crossing a wall`).
- **Each zone has an actor index,** derived from the actors' placements, which finds the actors at a location without
  a search (`## Actors`).
- **The player is in a zone, and is owned by the game.** It is in its zone's actor index like any actor, but unloading
  or saving the zone leaves it out (`## Migration`).
- **A dropped object is indexed in its zone,** as every sited entity is, and is accessible while the actor is in that
  zone, not from another zone the actor moves to.
- **The world index has only the entities that something refers to.** A reference uses it to find the zone its entity
  is in.
- **The zone file format is not Flecs's,** but the ECS can be built from it. A saved zone is a zone file with a
  top-level `zone` section containing the zone's persistent id, and the section's presence makes every id in the file
  persistent.
- **Format 0.4 and persistent ids are built before loading a zone,** so loading is written once, against them.
- **A direction that an entity has is its orientation.**
- **The relocators at which an arriving actor can be placed are the zone's access points,** listed in
  `layout.components.access_points`.
- **Zone files are written by generators, tools and saves,** not by hand; being readable as text is enough.
- **A cell position is `cell_position`,** as every format key and every code name for one.
- **A wall id is encoded from the normal,** by `encode_wall_id` (`### Wall ids from the normal`).

## The format and the generators

Steps 1 and 2 are built. `discrete_layout.md` specifies format 0.4, `addressing.md` specifies its ids and addresses,
and `game_system_reference.md` describes `stratum_format` and the generators that write it.

## Loading a zone against 0.4

`discrete_layout_rework.md` (`## Loading a zone`) outlines loading. `load_layout` fills an empty zone from the
`Document`, and as it builds each entity it turns wall addresses into wall ids, flat roles into flat ids, and the flat
cell where an entity was authored into a placement. Every door and staircase is written in the file, so no glyph
becomes an entity.

## Persistent ids

Step 4 is built: `game_system_reference.md` (`### Persistent ids`) describes `PersistentId`, the allocators,
`EntityReference`, the loaded entities and the world index. `PersistentId` has a `std::hash` of its own, which a
128-bit form would change. Sites and parents are not references in the world index's sense, since they are links
within one zone. Loading a template is the other path that adds entries to the world index.

### Loading a template

`World::load_zone` makes the zone with a new persistent id, and `load_layout` fills it from a document that has no
`zone` section:

1. Allocate a persistent id for each entity, in the document's order, into a map from local id to persistent id that
   lasts as long as the call.
2. Translate every reference in the file components of the layout, rooms, walls, flats and entities, and every
   `parent`, through the map.
3. Build the entities, each with its `PersistentId` and its type, then give each child its parent and each entity its
   site.
4. Add each entity that a reference in a file component points to to the world index.

A template loaded twice gets two sets of ids, and so two instances. The ids depend on the world's count, so tests
compare what is built and how it links instead of id values.

### Loading a saved zone

`World::load_zone` makes the zone with the id of the document's `zone` section, and `load_layout` allocates nothing
and translates nothing: every id is already persistent, and the world index already has the entities the zone's
references point to. Setting each entity's `PersistentId` adds it to the loaded entities again, and a reference into
the zone resolves again the next time it is resolved.

## Links

A sited entity's `Site` has its address. The link in the other direction is a list on the entity of the wall or flat
cell it is sited on:

```cpp
namespace quetzal::stratum_sim
{
    // The entities sited on a wall or a flat cell, kept on the wall's or cell's entity
    struct AttachedEntities
    {
        std::vector<flecs::entity> entities;
    };
}
```

- **`reparent` and setting a site keep both ends in sync.** An `OnRemove` observer on `Site` removes an entity from the
  list it is on when the entity is deleted, and an observer on each address type clears the site of every entity on a
  wall or flat cell whose entity is being deleted. What happens to those entities --- the contents of a smashed chest
  --- is for the rule that deletes to decide, not the observer. Built (`game_system_reference.md`, `### Entity types,
  layout entities and properties`).
- **A child's `flecs::Parent`** is set by `reparent`. Flecs keeps the parent's children and deletes them with it.
- **An entity has at most one site.** An object on several walls or cells is a parent whose children each have their
  own site, so a site never has a second case.

### Layout entities

A wall's or flat cell's entity exists while it has components or something sited on it, and is deleted when it has
neither; every room and each flat has one. Each has its address as a component and is found by it, through the zone's
`AddressIndex` for its address type, beside its layout and actor index. Built (`layout_entities.hpp`).

A layout entity, its list and a parent's children are derived, and are rebuilt when the zone loads (`addressing.md`,
`### A saved zone`). A layout entity has no `PersistentId`; it is referred to by its address.

### Wall ids from the normal

```cpp
        // Returns the id of the wall at an address, from the cell position's slot and the normal's direction index
        id_type encode_wall_id(const discrete_point_type& cell_position, const discrete_vector_type& normal) const;
```

- **The id is the slot times four plus the normal's direction index.** `load_layout` encodes every address in a file
  with it.
- **`wall_toward` stays the movement query.** Moving in a direction meets the wall whose normal is the opposite
  direction, so `wall_toward` negates the direction and encodes with `encode_wall_id`.
- **Everything that decodes a wall id** --- the normal from an id, or the cell's mask bit for the wall --- uses the
  normal's direction index.

### Crossing a wall

```cpp
namespace quetzal::stratum_sim
{
    enum class AccessState { Open, Closed, Locked };
    enum class DetectionState { Visible, Concealed, Revealed };

    // The state of an entity that is a way through, such as a door or a staircase
    struct Access
    {
        AccessState state = AccessState::Closed;
    };

    // How an entity that not every actor sees is detected: its general state, and the state for each actor whose own
    // state differs
    struct Detection
    {
        DetectionState state = DetectionState::Visible;
        std::unordered_map<PersistentId, DetectionState> actor_states;
    };

    // Each is read from the entity, then its parent, and so on
    std::optional<AccessState> access(flecs::entity entity);
    DetectionState detection(flecs::entity entity, flecs::entity actor);

    // Returns true if any entity sited on the wall lets the actor through
    bool crossable(flecs::entity zone, id_type wall_id, flecs::entity actor);
}
```

- **`crossable`** takes the wall's entity and asks each entity in its `AttachedEntities`. An entity lets the actor
  through if its access is open and its detection for that actor is not concealed. A wall with no entity, or with no
  entity on it that lets the actor through, cannot be crossed.
- **No access means no way through.** An entity whose chain has no `Access` at all --- a torch, a lever --- is not
  examined further.
- **A property comes from the entity or from an ancestor.** A door facing has no `Access` of its own and its door has
  one, so both sides of a door open together. A staircase's own `Access` is open and stays open.
- **There is one rule, with no type in it.** `crossable` reads `Access` and `Detection` and nothing else, so a new type
  of way through needs only the right components, not code here.
- **The result depends on the actor.** `move`, `neighbors` and `visible` take a `crossable_function_type`, and
  `World::move`, `World::neighbors` and `World::visible` bind `crossable(zone, wall_id, actor)` for the actor moving
  or looking.
- **A component states a state, not an interpretation of one.** `Access` is what a way through is: open, closed or
  locked. A door off its hinges is open, and one smashed into rubble that blocks the doorway is closed. How a broken
  door looks is a visual state that a script sets and a view interprets, never a component that states whether the door
  can be crossed.
- **The view is the player's.** The reductions draw a way through that the viewer finds concealed as a wall, and mark
  one that the viewer has found but that is concealed from actors in general; each denizen crosses as it perceives the
  way through.

Built (`game_system_reference.md`, `### Sites and doors`). `detection` returns `Visible` if nothing in the chain has a
`Detection`, `access_entity` returns the entity whose `Access` applies, and `way_through` returns the entity on a wall
that has access. A staircase has no `Access` yet; entering one is `session_plan.md` step 6.

### What the queries become

- **The entities on a wall or flat cell:** the wall's or cell's entity, then its `AttachedEntities`.
- **Where an entity is:** its `Site`, and its parent if the site has no address.
- **The other side of a door:** the door facing's parent, then that parent's other child.
- **The actors at a location:** the zone's actor index (`## Actors`).

`Door`, `DoorFacing`, `Openable`, `Crossable`, `Concealed`, `facing_on`, `door_of`, `other_facing`, `create_door`,
`create_doors`, `door_walls_type`, and `load_layout`'s `door_type` are gone.

## Actors

An actor is located by its site's placement. Each zone has an actor index, of the type its layout supplies, so finding
the actors at a location is a lookup instead of a scan of the zone.

```cpp
namespace quetzal::stratum_sim
{
    // The actors in one zone by location, derived from their placements
    class ActorIndex
    {
    public:
        virtual ~ActorIndex() = default;

        // Adds the actor, or moves it if it is already in the index
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
        std::unordered_map<size_t, std::vector<flecs::entity>> m_actors_by_slot; // <slot, actors in that cell>
        std::unordered_map<flecs::entity_t, size_t> m_slots_by_actor; // <actor, slot of the cell it is in>
    };
}
```

`Layout` gains:

```cpp
        virtual std::unique_ptr<ActorIndex> create_actor_index() const = 0;
```

- **The index follows the placements.** An `OnSet` observer on an actor's `Site` places the actor in its zone's index
  and an `OnRemove` observer removes it, so no path that moves, creates or deletes an actor can leave the index out of
  date. A move costs two map updates.
- **Relocating removes the actor from the old zone's index** before changing its zone, and the observer places it in
  the new one.
- **`entities_at` and `entities_within` query the index,** with `DiscreteActorIndex::actors_within` walking the cells
  in range, and keep their ordering by entity id.
- **The index is derived,** so it is never saved, and loading a zone rebuilds it. It cannot disagree with the
  placements, which remain the one source of where an actor is.

## Migration

`reparent` moves an entity from one zone to another. It removes the entity from the old zone's lists and actor index,
sets its parent to the destination zone (or, for the player, the zone it is in) and its site at the destination, and
moves its entry in the world index with it if it has one. Its children move with it.

**The player is owned by the game.** `StratumGame` owns the `Player`, which exists before any zone. The player has no
parent: an exclusive `(InZone, zone)` relationship refers to the zone it is in, which relocating changes, `zone_of`
reads, and flecs removes when that zone is deleted. An entity with no parent is owned by the game, and relocating it
keeps it that way. The player is in its zone's actor index like any actor, and every query for the entities in a zone
matches it, but it is not in the zone's subtree, so unloading or saving the zone leaves it out. The objects the player
carries are its children, sited `on` it, and are owned by the game too; both are saved with the game.

**An entity that migrates is not anchored to the layout.** Nothing below an entity moving to another zone is sited on
a wall or a flat cell, since those addresses belong to the old zone.

## Flecs

- **`flecs::Parent`** is the non-fragmenting hierarchy of Flecs 4.1.6 (`library3/flecs`), meant for small structured
  hierarchies of a few children each, such as a door and its door facings. `ChildOf` remains how a zone contains its
  entities, since that is the large unstructured case. `test/flecs_test`'s `parent hierarchy` group checks what this
  design relies on:
  - a child refers to its parent, and a parent iterates its children, which Flecs keeps in a vector on the parent;
  - deleting a parent deletes its children, in bulk rather than one at a time;
  - the children of many parents can be queried together, which the view needs;
  - an entity has one parent, in one storage or the other and never both, so a child belongs to its zone through its
    parent rather than directly;
  - nesting is unbounded, with a child being a parent in turn: a chest in a zone, a box in the chest, a tin in the box,
    and a coin in the tin, each reading back its own parent and children, the whole chain reachable by path, and
    deleting the zone deleting all of it through both storages.
- **A `Parent` hierarchy does not survive flecs JSON.** `to_json` writes it, and `from_json` aborts the process on a
  world containing one; a bare parent and child are enough. A saved zone writes each child's `parent` itself, as the
  zone file format does for every entity.
- **`DontFragment`** is available for a relationship that needs a pair after all.
- **Reflection** is registered member by member for every new component, as `sim_zone_model.md` requires, or a saved
  zone is written with names and no values.

## What checks it

- **`stratum_sim_test`:**
  - loading a template twice gives different persistent ids and the same structure;
  - every reference is translated, and one to an entity resolves to it;
  - an entity that something refers to is in the index, and its entry follows it across zones;
  - a wall's or flat cell's list stays in sync with the sites on it, and deleting an entity or a layout entity leaves
    no link behind;
  - a file's `parent` makes a Flecs child, and deleting a parent deletes its children;
  - `encode_wall_id` and `wall_toward` return the same id for the same wall, for all four normals;
  - a wall's or flat cell's entity is made when something is sited on it and deleted when nothing is;
  - `crossable` works through `Access` and `Detection`: a door open, closed and locked; a door facing taking its door's
    access, and concealed for one actor and not another; an entity with no `Access` in its chain never crossable; and
    a staircase crossed from its open side and from no other;
  - one wall is crossed by one actor and not by another, in the same layout;
  - the actor index follows every move, migration, creation and deletion, and `entities_at` and `entities_within`
    return what a scan of the zone returns;
  - migration keeps the persistent id and the runtime entity, and updates the index;
  - the player is in its zone's actor index, and is left out when that zone is unloaded;
  - a reference resolves again after its zone is unloaded and loaded again;
  - `CountingIdAllocator` never repeats an id across a save and restore.
- **`stratum_view_test`:** its frames, which print wall ids, are unchanged apart from the wall ids.

## Steps

- [x] **1. Format 0.4 in `stratum_format`.**
- [x] **2. The generators and every zone file in 0.4.**
- [x] **3. `flecs::Parent` checked in `flecs_test`** (`## Flecs`).
- [x] **4. Persistent ids.** `PersistentId`, `IdAllocator`, `CountingIdAllocator`, the loaded entities and the world
      index, `EntityReference`, `make_reference` and `resolve`.
- [x] **5. Loading a zone, against 0.4 and with persistent ids.** `discrete_layout_rework.md`'s `## Loading a zone`.
      `load_layout` builds the zone from the document: an entity for every object of file components, each `IsA` its
      type, with its component objects decoded into typed components (`modding_plan.md`), and a property resolved when
      queried from the object, its type, its room and the layout. Making a zone allocates persistent ids and
      translates references, links sites in both directions through `AttachedEntities` and the layout entities, and
      gives each parent its children, so `crossable` searches nothing and asks each sited entity's `Access` and
      `Detection` for the actor crossing. Each staircase gets its relocator with its target. Wall ids are encoded from
      the normal by `encode_wall_id`. `World` contains zones rather than layouts, and the door functions are gone.
- [x] **6. Migration and the player's ownership.** Relocating across zones through the links and the index, with the
      player and the objects it carries owned by the game.
  - [x] **a. The game's entities in a zone.** `InZone`; `zone_of` reading it; `relocate` keeping an entity with no
        parent owned by the game; every query for a zone's entities matching it; and the player owned by the game, and
        scheduled, pursued and drawn as before.
  - [x] **b. Migration keeping the links.** Nothing below a migrating entity is sited on the layout; the world index
        follows the entity and everything below it; and the player and the objects it carries are left out of an
        unloaded zone.
- [ ] **7. Saved zones.** The `zone` section, unloading a zone by writing it and loading one back by reading it, with
      the session lifecycle of stage 7. Writing a zone walks its subtree instead of querying its children, since
      anything with a parent of its own is below that parent. Loading a saved zone that is already loaded is an error; a
      template makes a new zone each time it is loaded.
      Resetting to a save point is the game's operation: `StratumGame` unloads every zone, restores the player and the
      id allocator's count, and loads the zone the player was in, with the others loading as they are entered, so no
      entity is duplicated or lost.
- [ ] **8. Relocating through a relocator,** stage 6's item in `work_plan.md`, binding targets by persistent id. It
      resolves a target, bound or unbound, to a zone the world has or makes; an unbound level finds an unbound access
      point that leads back, at the target level, or makes a zone there, with `World::resolve_relocator_target`
      (`region_plan.md`, `## Relocators with a level offset`). The zone relocated from must have a level, so the hosts'
      starting zones are given one, and it draws from the gameplay engine that `World` takes from `StratumGame` (`random_engine.md`).
	  It places the arriving actor at the found relocator's access point, or at
      the new zone's unbound access point with the lowest `order`, of the type at which the relocator's type arrives,
      that leads back, and binds both ends to each other, writing each as the other's bound target; `entry_point` then skips a bound
      access point. Entering an access point, by moving into its wall or by a keystroke on its floor cell, starts it
      (`session_plan.md` step 6).
- [ ] **9. The zone actor index.** `ActorIndex`, `DiscreteActorIndex`, `Layout::create_actor_index`, the observers,
      and `entities_at` and `entities_within` querying the index. It builds against the zones as they are now.

Step 9 changes no behavior and no frame. Step 5 moves behavior from the glyphs to the entities, and the frames confirm
that nothing a player sees changed apart from the wall ids they print.

## Blockers

- **Step 7 waits on stage 7 of `work_plan.md`,** the session lifecycle: a session that outlives one zone, and a zone
  added to the world when it is encountered. A zone cannot be written out and read back until there is a reason to
  unload it.
- **Step 8 waits on `region_plan.md`,** since a level offset is resolved against zones' levels and regions, and
  nothing associates zones yet.

## Open

- **Names.** `PersistentId`, `EntityReference`, `make_reference`, `AttachedEntities`, `AddressIndex`, `Access`,
  `Detection` and `ActorIndex` are working names.
- **Where a saved zone is written,** and how a world's zones are found by persistent id on disk. The session lifecycle
  settles this.
