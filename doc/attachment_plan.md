# Attachments --- Plan

Status: complete. Doors load shut, interacting opens one, a wall has no flag, and the view draws shut and open doors
differently. The links that replace the searches below are part of loading a zone (`discrete_layout_rework.md`),
`site_plan.md` has the components that entities are sited with, and `Access` and `Detection` replace the door
components (`addressing_plan.md`, `### Crossing a wall`)
Date: 2026-09-02, revised 2026-09-26

This is the first item of stage 6 in `work_plan.md`. A door is the case worked through, and the mechanism is what is
designed: everything on a wall or a flat is an attachment --- an alcove, a basin, a door, a button, a torch --- so one
mechanism handles all of them and a wall stays a bit on the cell it bounds (`sim_zone_model.md`).
`game_system_reference.md` describes what is built.

## Constraints

- **A wall is not an entity.** A wall with components or with something attached has an entity of its own, found by
  wall id (`decision_rationale.md`).
- **A wall is one side of an edge.** An actor interacts with the wall on its own side, and the other side is separate.
- **Attachments and walls link in both directions.** A door facing links to its wall, and the wall's entity links to
  the door facing, so nothing is found by searching.
- **An attachment reached from more than one side is one entity with the shared state, and per-side entities with the
  state that differs,** each referring to it.
- **A side refers to the entity it belongs to as its child,** entity to entity, whereas attachment to a wall is by id.
- **A zone's contents are its children,** so unloading and migration are subtree operations.

## The links

| From | To | Link |
|---|---|---|
| a door facing | the wall it is on | the wall id, which also encodes the wall's normal |
| a wall's entity | the entities attached to it | a link to the door facing |
| a door facing | the door it belongs to | its parent |
| a door | its door facings | a link to each |
| an attachment | its position on that surface | its own component, the same either way |

Position is separate from target: a keyhole on a door facing and a torch on a wall are both positioned along and up,
and differ only in the object they are positioned on.

## A door

**A door is its door facings and a common part.** There is a door facing for each wall it is on: two in the ordinary
case, and one where a doorway leads into a dead end. The common part is on no wall, is the door facings' parent, and
links to each of them. The zone lists its doors.

A door is one variety of **an attachment that determines passage**: a working door, a door smashed to a hole, or
blocking debris. `crossable` reads that, and `Door` is the variety with `Openable`.

```
zone                                    (Zone)
 \-- door                               Door, Openable, Lockable
      +-- door facing                   Site{On, WallAddress}, DoorFacing, Concealed
      |    \-- keyhole                  Keyhole
      \-- door facing                   Site{On, WallAddress}, DoorFacing
           \-- knocker                  Knocker
```

Everything that belongs to a door facing is a child of it, so the assembly is one subtree: it unloads with the zone,
and removing a door is one delete. The door links to each of its door facings, so reaching the other side is a matter
of following a link.

**A door facing has no direction of its own.** The wall's id encodes its normal, into the cell it bounds, and
construction puts each door facing on the right side.

### The common part

| Component | |
|---|---|
| `Door` | Tag. |
| `Openable` | `bool open`. One door has one state. |
| `Lockable` | `bool locked`, and the key that opens it. |
| `Trapped` | The trap as its own entity, with rules of its own. |

### Per door facing

| Component | |
|---|---|
| `Site` | `On` a `WallAddress`, and the position on the surface. |
| `DoorFacing` | Tag. The door is its parent. |
| `Concealed` | `bool revealed`. The door is secret from this side until revealed; a secret door is concealed on one side and an ordinary door on the other. |
| `Appearance` | The surface this side shows, once there are asset ids. |

Concealment and handles are per door facing; being open is not.

## The two walls need not be a pair

| | The walls | Crossing it |
|---|---|---|
| wall notation | the two sides of one edge | passing through the edge |
| block notation | the walls that the floor on either side of a doorway block has toward it | entering the block |

**In the block notation a doorway is a cell an actor may stand in.** A floor cell has a wall toward it, and the doorway
cell has walls toward solid cells and toward the edge of the grid but none toward floor (`discrete_layout.md`,
`### The block notation`). The door is on the walls that the floor has toward the doorway. An actor leaving the block
sideways meets the doorway's own walls toward solid, and leaving forward meets no wall.

Construction is given the walls and puts a door facing on each, so the door does not depend on the notation.

## Lifetime

A door is one subtree, so deleting a door removes its door facings and their children, and unloading the zone removes
every door in it. An entity that the door refers to instead of owning, such as its key, is referred to, not parented.

**A destroyed door is not a deleted one.** The walls remain when a door is smashed, and a wall with nothing attached is
solid, so deleting the assembly would seal the doorway.

- **Transform.** The same entity gets different components. A door smashed to a hole has `Openable` removed and
  `Crossable` added; one smashed into blocking debris has neither. Its identity survives, for a lever wired to the
  door, a key that fits it, a quest that refers to it, and a save.
- **Unlink.** The entity survives and is no longer on the wall. A door taken off its hinges and carried is an item in a
  pack, with its door facings unlinked from their walls.
- **Delete.** An editor removing a door, or the zone unloading.

Each component's reflection is registered member by member (`sim_zone_model.md`).

## The queries

**Can an actor cross this wall?** Check the door facing on the wall, then its parent. A wall with no door facing is a
plain wall. A door facing that is `Concealed` and not revealed refuses.

```
crossable(wall_id):
    door_facing = the entity the wall's entity links to     -- none: false
    if door_facing has Concealed and not revealed:          false

    door = parent(door_facing)
    if door has Openable:  return door.Openable.open        -- a working door
    return door has Crossable                               -- smashed through, or debris
```

**What is on this wall?** The entity the wall's entity links to, which a view draws and `Interact` acts on.

**What are a door's door facings?** The entities it links to.

The first question is asked per move per actor per turn, and per cell along a sight line, through the layout's
`crossable`. Finding the wall's entity by id and following a link keep it affordable.

## The seam

`crossable` needs the ECS, and `Layout` is independent of entities. `Layout` takes a predicate over wall ids,
`crossable_function_type`, and `World` supplies it on each call, bound to the zone.

**Sight is a separate question.** A portcullis lets sight through but not an actor, a curtain lets an actor through but
not sight, and a secret door from its concealed side lets neither through, since a line of sight through it would draw
the room beyond. So `World` supplies a small set of questions about a wall, and `visible` asks a different one than
`move` does.

**The actor is passed.** Some results depend on who asks --- true seeing against an illusory wall, pass wall, knock
against a lock, or a secret door found by one character instead of the whole party --- so the query takes the actor
(`addressing_plan.md` step 5).

**An overridable flag keeps it affordable.** Only attachments marked overridable consult the actor's properties, which
the actor has from a spell, a scroll or an item, with their durations; the common case never looks.

## Steps

- [x] **1. The components and the assembly**: `create_door` building the tree, and `create_doors` making one per door
      the file has.
- [x] **2. The queries**: `facing_on`, `door_of`, `other_facing` and `crossable`, with `Crossable` and `Concealed`.
- [x] **3. `Interact` opens and closes a door**: the door facing on the wall ahead, then its door, with `Openable`
      toggled.
- [x] **4. The seam**: passage determined by `crossable`, the wall flag removed, and the loader reporting each door's
      walls for a caller with a world to build it.
- [x] **5. The view shows a door as what it is**: the reduction reads the door's `Openable`, and a block notation door
      is placed in its doorway cell.

## Open

The open items are tracked in `work_plan.md`.
