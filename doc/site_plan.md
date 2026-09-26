# Sites --- Plan

Status: complete
Date: 2026-09-21, revised 2026-09-26

This plan describes where every entity is and what it belongs to. Two links do it: the **parent**, which determines
lifetime and which entities travel together, and the **site**, which is the object the entity is in or on and where on
it. `attachment_plan.md` built the door assembly on `Attachment` and `OnWall`; this plan generalizes both to every
entity and replaces them. `game_system_reference.md` describes what is built.

## The two links

| Link | Means | Present |
|---|---|---|
| parent | belongs to, travels with, is deleted with | every entity except the game's own; the chain ends at the zone |
| site | the object it is in or on, and where on it | every entity that is anywhere |

An entity may belong without being sited, so the site is optional. Nothing is sited without belonging.

**One component is used, whatever an entity is placed on.** A site is a relation, an optional address and a
placement. The placement is relative to the address --- the wall or flat cell that the address refers to --- or to the
parent if there is no address. An actor is `In` its zone with no address, a key is `In` its parent chest with no
address, and a torch is `On` a wall with one. Picking a mug up off a table and dropping it on the floor changes fields
of one component.

**The relation is always stated,** including the `In` of every child of a zone.

**An entity in or on another entity has that entity as its parent.** A site never refers to an entity; that link is
the parent. A key in a chest, coins in a pouch, a sword an actor carries, and a mug on a table each travel with the
entity they are in or on, so one link states both.

**A door facing has two separate links.** It belongs to its door, which it is deleted with and inherits from, and it is
on a wall, which is geometry rather than an entity. This is the only case where the two links differ.

## Relation

The relation is `In` or `On`, and it qualifies the site whether the site refers to geometry or to the parent.

| | Against geometry | Against the parent |
|---|---|---|
| `In` | an alcove in a wall | a key in a chest, coins in a pouch |
| `On` | a torch on a wall, a lamp on a ceiling cell | a mug on a table, a sword an actor carries |

## The address

The site's address is the wall or the flat cell that the entity is in or on. **No address means the parent's own
space**, which for a child of a zone is the zone. A torch in a sconce is `In` its parent sconce, and the sconce is `On`
a wall.

**An address means the entity is fixed to a surface; an object resting on a floor has a placement and no address.** A
wall has to be addressed, since a wall id is its identity and a wall has no position of its own. A cell of a flat is a
position, so an address to one would be a position written a second way, rewritten whenever the entity is picked up
and put down, in a frame that the layout's queries do not take. So a chest, a dropped item and an actor are all `In`
their zone with a placement, and a zone with no cell grid places them the same way. A flat cell in the file is where
an entity was authored, which loading resolves to a placement.

## The data

### `stratum_format`

```cpp
enum class Relation { In, On };

//--------------------------------------------------------------------------
// The site file component: the object the entity is in or on, and where on it. With no address, the entity is sited on
// its parent
struct Site
{
    Relation relation = Relation::In;
    std::optional<std::variant<WallAddress, FlatCell>> address;
    std::optional<surface_point_type> position;
    std::optional<discrete_vector_type> orientation;
};

//--------------------------------------------------------------------------
// One of the zone's entities. The site and parent file components are decoded into fields, which the reader validates;
// components contains the rest as written
struct Entity
{
    stored_entity_id_type id = 0;
    std::string type;
    std::optional<Site> site;
    std::optional<stored_entity_id_type> parent_id;
    raw_values_type components;
};

std::string_view name(Relation relation);
```

### `stratum_sim`

```cpp
enum class Relation { In, On };

//--------------------------------------------------------------------------
struct WallAddress
{
    id_type wall_id = nullid;
};

//--------------------------------------------------------------------------
struct FlatCellAddress
{
    id_type flat_id = nullid;
    discrete_point_type cell_position;
};

using address_type = std::variant<WallAddress, FlatCellAddress>;

//--------------------------------------------------------------------------
// Where an entity is. The placement is relative to the address, or to the parent if there is no address
struct Site
{
    Relation relation = Relation::In;
    std::optional<address_type> address;
    placement_type placement;
};

const placement_type& placement_of(flecs::entity e);
void set_placement(flecs::entity e, const placement_type& placement);
```

### The zone file

```json
"entities": [
  { "id": 1, "type": "door", "components": { "material": "oak", "open": false } },
  { "id": 2, "type": "door_facing", "components": { "parent": { "entity": 1 }, "site": { "relation": "on", "wall": { "cell_position": [1, 1], "normal": [-1, 0] } } } },
  { "id": 3, "type": "door_facing", "components": { "parent": { "entity": 1 }, "site": { "relation": "on", "wall": { "cell_position": [2, 1], "normal": [1, 0] } }, "concealed": true } },
  { "id": 4, "type": "staircase", "components": { "site": { "relation": "on", "wall": { "cell_position": [3, 1], "normal": [0, -1] } }, "relocator": { "target": "down" } } },
  { "id": 5, "type": "chest", "components": { "site": { "relation": "on", "flat": "floor", "cell_position": [0, 2], "orientation": [1, 0] } } },
  { "id": 6, "type": "key", "components": { "parent": { "entity": 5 }, "site": { "relation": "in" }, "opens": { "entity": 1 } } },
  { "id": 7, "type": "table", "components": { "site": { "relation": "on", "flat": "floor", "cell_position": [4, 2], "orientation": [0, 1] } } },
  { "id": 8, "type": "mug", "components": { "parent": { "entity": 7 }, "site": { "relation": "on", "position": [0.3, 0.5] } } }
]
```

## The tree

```
zone
 +-- door                    Access
 |    \-- door_facing        On wall 12        belongs to the door, placed on a wall
 +-- staircase               On wall 30, relocator
 +-- torch                   On wall 7
 +-- alcove                  In wall 9
 +-- lamp                    On the ceiling flat, cell [4, 6]
 +-- chest                   In the zone, a placement and no address
 |    \-- key                In its parent
 +-- table                   In the zone, a placement and no address
 |    \-- mug                On its parent
 \-- actor                   In the zone, a placement and no address
      +-- sword              On its parent
      \-- pouch              On its parent
           \-- coins         In their parent
```

An entity's type is a component that queries filter on, so there are no grouping nodes below the zone. Nesting is
unbounded --- coins in a pouch that an actor carries --- so nothing calls `parent()` a fixed number of times.

## Operations

- **Pick up.** Reparent the mug to the actor and set its site: `On`, no address, and a placement in the actor's frame.
  Everything the mug contains comes with it, since it is the mug's subtree.
- **Leave a zone.** Reparent the actor to the zone it arrives in. It is the same operation at a higher root, and the
  objects the actor carries travel with it.
- **Unload.** Delete the zone's subtree.
- **Find the entities on a wall.** A query over `Site` by address.
- **Find the zone an entity is in.** The root of its parent chain.
- **Find where an actor is.** Its site's placement, in the zone's frame, since its site has no address.

## Steps

- [x] **1. `Relation` and `Site` in `stratum_format`.** The reader decodes the `site` file component, the writer emits
      it, and an entity that a file places in or on another entity is read as the parent file component.
- [x] **2. `Site` in `stratum_sim`,** replacing `Attachment` and `OnWall`. `create_door` builds a door facing with a
      wall address, and `facing_on` and `crossable` query it.
- [x] **3. `zone_of` and `facing_on` walk the parent chain** to its root instead of calling `parent()` a fixed number
      of times.
- [x] **4. `placement_type` folds into `Site`,** so an actor is `In` its zone with a placement and no address, and one
      component states where every entity is.
- [x] **5. An object resting on a floor has a placement and no address,** since an address means fixed to a surface.
      The file's flat cell is where an entity was authored, which loading resolves to a placement.
- [x] **6. Reparenting is one operation** that sets the parent and the site together, and picking up, putting down and
      relocating between zones all call it.

## Open

The open items are tracked in `work_plan.md`.

## Appendix: changes from the attached file component

| | Before | After |
|---|---|---|
| component name | `attached` | `site` |
| relation | `relationship`, an optional string | `relation`, `In` or `On` |
| an entity placed in or on another entity | `"target": { "entity": 6 }` | the `parent` file component, and a site with no address |
| `part_of`, `contained_in`, `carried_by` | values of `relationship` | the parent link, qualified `In` or `On` |
| a wall in the sim | `OnWall`, beside `Attachment` | the site's address |
| an actor's placement | `placement_type`, its own component | the site's placement |
