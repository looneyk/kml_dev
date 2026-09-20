# Attachments --- Plan

Status: complete. Doors work: they load shut, space opens one, a wall has no flag, and the view
draws shut and open apart. The links replacing the searches below are part of loading a zone,
`discrete_layout_rework.md`
Date: 2026-09-02

Detail for the first item of stage 6 in `work_plan.md`. A door is the case being worked through, but
the mechanism is what is being designed: `sim_zone_model.md` settles that *everything* on a wall or
a flat is an attachment --- an alcove, a basin, a door, a button, a torch --- so one mechanism covers
all of them and a wall stays a bit on the cell it bounds. What is built goes to `game_system_reference.md`.

## What is already settled

Repeated here only as the constraints the shape has to satisfy; the argument is elsewhere.

- **A wall is a record, not an entity.** A wall per cell edge puts thousands in a zone and a zone
  serialises its whole subtree when it unloads. A wall with components or something attached has an
  entity of its own, found by wall id, which is sparse. `decision_rationale.md`.
- **A wall is one side of an edge.** An edge is what a level file writes as one glyph for both sides.
  A mover interacts with the wall on its own side, and the other side is separate.
- **Attachments and walls link both ways.** A facing links to its wall, and the wall's entity links
  to the facing, so nothing is found by searching.
- **An attachment reached from more than one side is one entity owning the shared fact, and per-side
  entities owning what genuinely differs,** each referencing it. Mirroring the same fact into two
  places and reconciling them is the arrangement this was chosen over.
- **The reference from a side to the entity it belongs to is entity to entity,** so it is a
  relationship rather than an id --- unlike attachment to a wall, which is an id because a wall is
  not an entity.
- **A zone's contents are its children,** so unload and migration are subtree operations.

## The links

Every link in what follows is one of these, and which it is falls out of what is on each end.

| From | To | Link |
|---|---|---|
| a facing | the wall it hangs on | the wall id, which also provides the wall's normal |
| a wall's entity | what hangs on it | a link to the facing |
| a facing | the entity it belongs to | `ChildOf`, because a door is an entity |
| a door | its facings | a link to each |
| an attachment | where it sits on that surface | its own component, the same either way |

Position is separated from target deliberately. A keyhole on a door facing and a torch on a wall are
positioned the same way --- along and up --- and differ only in what they are positioned *on*.

## A door

**A door is its facings and a common part.** A facing per wall it is on: two in the ordinary case,
one where a doorway leads into a dead end. What hangs on a wall is a *facing*; the common part hangs
on nothing, is the facings' parent, and links to each of them. The zone lists its doors.

A door is one variety of a more general attachment, and the general one is what queries ask about:
**what is attached to this wall and decides passage.** A working door is one, a door smashed to a hole is
one, blocking debris is one --- wall decoration with a passage rule. So the tag `crossable` filters
on is that, and `Door` is the variety of it that has `Openable`. This is also where the passability
coming off `Wall` belongs: on an attachment, which is an entity, rather than on a record, which is
level data.

```
zone                                    (Zone)
 \-- door                               Door, Openable, Lockable
      +-- facing                        Attachment, OnWall{wall_id}, DoorFacing, Concealed
      |    \-- keyhole                  Attachment, Keyhole
      \-- facing                        Attachment, OnWall{wall_id}, DoorFacing
           \-- knocker                  Attachment, Knocker
```

A facing is a child of its door, and what belongs to a facing is a child of that facing. The whole
assembly is therefore one subtree: it unloads with the zone, and removing a door is one delete.

`ChildOf` is how a facing references its door, rather than an id in a component beside it: "the
facing's door" is `parent()`. The door links to each of its facings, so reaching the other side's
facing is following a link rather than querying for siblings. `sim_zone_model.md` already asks for a relationship rather than an id here, on
the grounds that a door is an entity while a wall is not.

**A facing has no direction.** Which way it points is the wall's to decide, and the wall already
determines it: its id provides the normal, pointing into the cell it bounds. Construction is what puts each facing on
the right side, and after that nothing stores an orientation that could disagree with the geometry.

### The common part

| Component | |
|---|---|
| `Door` | Tag. |
| `Openable` | `bool open`. One door, one state: it cannot be open from one side and shut from the other. |
| `Lockable` | `bool locked`, and the key it answers to. |
| `Trapped` | The trap as its own entity, so a trap has rules of its own rather than being a flag. |

### Per facing

| Component | |
|---|---|
| `Attachment` | `value_type along, up` --- where on the surface it sits. |
| `OnWall` | `id_type wall_id`, the facing's link to its wall, whose id also provides the normal and so the facing's absolute direction. Only a facing has this; a keyhole hangs on its facing, not on a wall. |
| `DoorFacing` | Tag. Which door it belongs to is its parent, not a field. |
| `Concealed` | `bool revealed`. Secret from *this* side until revealed --- the classic secret door is concealed on one facing and an ordinary door on the other. |
| `Appearance` | The surface this facing shows, once there are asset ids to name. |

**Concealed is per facing and open is not**, which is the whole reason for the split. So is a
handle: a door with a knob on one side and a blank face on the other is two facings that differ,
over one state they share.

## The two walls need not be a pair

This is what makes one door shape fit both notations, and it is why each facing is on a wall of its
own rather than the door being on an edge.

| | The two walls | Crossing it |
|---|---|---|
| wall notation | the two sides of one edge | passing through the edge |
| block notation | the walls the floor either side of a doorway *block* has toward it | entering the block |

**In the block notation a doorway is a cell of wall a mover may stand in.** A floor cell has a wall
toward it, and the doorway cell has walls toward solid cells and toward the edge of the grid but none
toward floor (`discrete_layout.md`, `### The block notation`).

The walls the floor has toward the doorway are what the door hangs on, a facing on each. The doorway's
own walls toward solid are what a mover leaving the block sideways meets.

That is also what makes leaving free with nothing to ask. From inside the block there is no wall
toward the floor for `wall_toward` to find, and the mover walks out. One opening passes the whole
doorway.

Nothing in the door notices which format it came from. Construction is told which walls and puts a
facing on each --- two in the ordinary case, one where a doorway leads into a dead end.

## Lifetime

One subtree, so one rule: deleting a door takes its facings and their keyholes, and unloading the
zone takes every door in it. Nothing needs a cleanup pass and nothing can be left referring to something
that is gone.

What must not die with it is anything the door merely refers to rather than owns: the key is an item
that lives in someone's pack, so it is referenced by a relationship and not parented.

**A destroyed door is not a deleted one,** and the cascade is what makes that visible: the walls stay
when a door is smashed, and a wall with nothing attached does not open, so deleting the assembly would
seal the doorway rather than open it.

Three operations, and only the last is a delete.

- **Transform.** The same entity with different components. A door smashed to a hole is `Openable`
  removed and `Passable` added; one smashed into blocking debris is neither. What an entity *is*, in
  an ECS, is which components it has --- so becoming something else is a change of components rather than a
  swap of entities, and identity survives it. That matters for everything that references it: a
  lever wired to the door, a key that fits it, a quest referring to it, a save.
- **Unlink.** The entity survives and stops being on the wall. A door taken off its hinges and
  carried away is an item in a pack, its facings unlinked from their walls, and the doorway is empty.
- **Delete.** An editor taking a door out of a level, and the zone unloading. The only place the
  cascade is what is wanted.

Each new component needs its reflection registered member by member, per the flecs obligation in
`sim_zone_model.md` --- without it `to_json` emits names with no values and a save is silently empty.

## The queries

Three, and only the first is on a hot path.

**May a mover cross this wall?** The facing on that wall, then what its parent specifies about passage. No
facing means a plain wall, which nothing opens. A facing `Concealed` and not yet found refuses,
because a secret door is a wall until it is found.

```
crossable(wall_id):
    facing = the facing the wall's entity links to          -- no entity or no facing: false
    if facing has Concealed and not revealed:                  false

    barrier = parent(facing)
    if barrier has Openable:  return barrier.Openable.open  -- a working door
    return barrier has Passable                             -- smashed through, or debris
```

Which is one query over every variety rather than a door query with the rest bolted on: a door is
the case that has `Openable`, and everything else is the case that does not.

**What is on this wall?** What the wall's entity links to. This is what a view asks to draw a facing
and what `Interact` asks to act on one.

**What are a door's facings?** Its links to them.

The first is called from `blocked`, which feeds `move`, `neighbors` *and* `visible`, so it is asked
per step per actor per turn and again per cell along a sight line. The wall's entity found by id and a
link followed is what keeps it affordable there.

## What this replaces

`Wall::passable` goes, and with it the last mutable state on level data. `blocked` becomes: is there
a wall, and if what is attached to it is openable, is it open.

That reaches into the loader, and the reach is the interesting part. Today a passage a mover may
cross is a *wall marked passable*; `sim_zone_model.md` states that an opening is the absence of a wall
rather than a flag on one, and the two have been in contradiction since the flag was written.
Removing the flag settles it:

| What the format encodes | Today | After |
|---|---|---|
| nothing between two cells | one area, no wall | unchanged |
| a wall | wall pair, not passable | unchanged |
| a door | wall pair, passable | wall pair, a facing on each, one door |
| a dungen doorway cell | walls outlined, passable | a facing on each room-facing wall, one door |

Wall ids stay stable throughout, which every attachment depends on: an id encodes the cell and the
side, so nothing renumbers one.

**The level file has the properties.** `discrete_layout.md` gives components to rooms, walls and flats
and has an `attachments` section addressed by wall and by flat cell, which is where a door's lock,
concealment and which way it opens are written. Loading a zone builds the shape above from them
(`discrete_layout_rework.md`, `## Loading a zone`).

## The seam

`crossable` needs the ECS and `Layout` is geometry independent of entities. It is the same
seam occupancy already crosses, and `decision_rationale.md` states it as the open question this
answers.

`Layout` takes a predicate --- `std::function<bool(id_type)>` --- as it takes `obstructed_type` for
occupancy, and stays ignorant of what is behind it. `World` supplies it, owning both the layouts and
the entities.

**It is not one predicate.** `blocked` supplies three callers with one boolean --- `move` and
`neighbors` ask about movement, `visible` asks about sight --- and that holds only while passability is
a single flag. Attachments end it: a portcullis passes sight and not a mover, a curtain passes a
mover and not sight, and a secret door from its concealed side passes neither, the sight half being
the point, since a line of sight through it would draw the room beyond and give it away. So what
`World` supplies is a small set of questions about a wall, and `visible` asks a different one than
`move` does.

**Per call or held is decided by whether the result depends on who asks.** That is why
`obstructed_type` is per call and rightly so: a mover is not obstructed by itself, so the result
differs per mover and only the caller can supply it. A closed door is closed for everyone and a
portcullis is transparent to everyone --- properties of the wall rather than of the asker --- so these
belong held on the layout, and nothing threads a predicate through `neighbors` and `visible` and
every caller of both.

**Held, with the actor passed.** Some results *are* asker-dependent --- true seeing against an
illusory wall, pass wall, knock against a lock, a secret door found by a character rather than by a
party --- so the query takes the actor even though the function does not change:

```cpp
void set_crossable(crossable_type);                          // held, set once by World
bool blocked(cell, direction, flecs::entity actor) const;    // the actor per call
```

That is not `obstructed_type`, where the function itself is rebuilt per call because what counts as
an obstruction is the caller's business.

**An overrideable flag is what makes it affordable.** `blocked` is asked per step per actor per turn
and again per cell along every sight line, so consulting an actor's properties every time is the
expensive part. A flag on the wall or on what is attached to it means the common case never looks: only
attachments marked overrideable pay, and the actor supplies the overrides --- from a spell, a
scroll, an item --- with whatever duration they came with. `sim_zone_model.md` already has this shape
for destruction: a default the engine supplies, with specific entities overriding it.

The cost is that `neighbors` and `visible` take placements today rather than entities and grow an
actor parameter, which is smaller than threading a predicate through them since every caller already
has the entity.

## Steps

- [x] **1. The components and the assembly.** `Attachment`, `OnWall`, `Door`, `DoorFacing`,
      `Openable`, and `create_door(world, zone, wall_a, wall_b)` building the tree. `create_doors`
      builds one for every partnered pair the file called passable, and `Session::load` calls it,
      so a level's doors are entities from the moment it loads. Generic and closed: the formats
      have no properties yet.

      Nothing reads them, `Wall::passable` still deciding whether a mover may cross, so behaviour
      is unchanged and every reference frame is what it was. Reflection is not registered --- no
      component in `stratum_sim` is, serialisation not being wired up yet, and doing it for these
      alone would be the odd one out.
- [x] **2. The queries.** `facing_on`, `door_of`, `other_facing` and `crossable`. `Passable` and
      `Concealed` came with them so the query is whole rather than a shape that changes when a
      producer arrives --- a wall with nothing attached is solid, an openable attachment reports whether
      it is open, anything else with whether it can simply be walked through, and a concealed
      facing refuses from that side alone.
- [x] **3. `Interact` opens and closes one.** The facing on the wall ahead, its door, `Openable`
      flipped. The event still reports the wall id, so nothing observable changes and every
      reference frame is what it was --- passage still comes from the flag, which is what makes this
      safe to make before step 4 rather than with it.
- [x] **4. The seam and the flag, together.** `Layout` keeps a `crossable_type`, `World` sets it to
      `crossable` when it takes a layout, `blocked` and `move` ask it, and `Wall::passable` is gone.
      `lattice` identifies a door by its facing.

      It reached further than the step set out. `create_doors` read the flag, so the loader had to
      convey where a door goes some other way: `load_layout` takes an optional `door_type` and *reports*
      the wall pair, a caller with a world building it --- which is the answer to the open question in
      `sim_zone_model.md` about what creates an attachment at load. `Room::Type::Door` is produced
      for the first time from the same information. And `Interacted` grew what happened, because a
      host reporting "nothing happens at wall 10" while a door swings open is a lie the wall id
      could not avoid telling.

      `walls_door` is re-scripted to approach, be refused, open and walk through. `dungeon_walk`
      needed it too: a doorway cell has a door on each side, so passing through one takes two
      openings.
- [x] **5. The view shows a door as what it is** rather than as a passable wall, which is where the
      per-facing difference first has somewhere to go --- and where the block format's doorway cell is
      drawn from the door's state rather than as floor.

      The reduction emits `DoorClosed` or `DoorOpen`, taken from the door's own `Openable`, and the
      ascii mapping draws them `/` and `\`. A block format door is placed in the doorway cell rather
      than on the faces either side of it, so one doorway is one glyph however many ways in it has.
      Nothing had to be done for the round trip: a freshly loaded level has its doors shut, so the
      file's `/` is what the view draws.

      The per-facing difference still has nowhere to go --- the reduction asks the door, not the
      facing it was reached through, so the two sides of one door draw alike.

Steps 2 and 3 are done together, being the queries and the one caller that proves them. Step 4 is the
one that cannot be partial: the flag and the doors cannot both decide, and it is where the behaviour
changes.

## Open

Tracked in `work_plan.md`, which collects the open items of every plan.
