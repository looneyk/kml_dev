# Session --- Plan

Status: steps 1 to 3 and 5 built; steps 4, 6 and 7 designed
Date: 2026-09-22, revised 2026-09-24

How the parts of `StratumGame` divide the work of creating, loading, populating and playing zones. `World` does the
zone management; `Session` keeps the state of play and calls into `World`. `discrete_layout_rework.md` is loading a zone
from its document; `sim_zone_model.md` (`### Residency, simulation, and persistence`) is when zones
unload. What is built is in `game_system_reference.md`, and open items are in `work_plan.md` (`### Session`).

## Responsibilities

| Part | Responsible for |
| --- | --- |
| `World` | Zones: creating, loading, populating and unloading them, which are loaded, and which unload as zones are entered; entities and persistent ids |
| `Session` | Play: the zone the player is in, entering a zone, the turn order, the clock and its driver, the player's intent and pending command, and advancing |
| `Player` | The entity the host controls |
| `StratumGame` | Owning the other three and the id allocator |

## World

```cpp
flecs::entity World::create_zone(const Generator& generator);
flecs::entity World::load_zone(const std::filesystem::path& filepath);
void World::populate(flecs::entity zone);
void World::trim_zones(flecs::entity zone_entered);
```

- **`create_zone(generator)`** makes an empty zone, has the generator fill it, and populates it.
- **`load_zone(filepath)`** makes an empty zone, has the loader fill it from the file, and populates it unless the file
  is populated already: a saved zone, or a generated one with its denizens placed, which a flag in the file indicates.
- **Doors are the generator's and the loader's,** which build them into the zone with its layout and notation. `World`
  handles none.
- **Neither unloads anything.** Creating and loading add a zone to the world.
- **`populate(zone)`** places the zone's denizens. Until placement is built (`placement_plan.md`), it places one
  pursuer at the last open cell.
- **The population strategy comes from the zone's source:** the template it was loaded from, or the guidance it was
  generated with. The placeholder stays in `World` until one of them supplies a strategy.
- **`trim_zones(zone_entered)`** unloads the zones entered longest ago while the world has more than its maximum, never
  `zone_entered`. The criteria are the world's `ZoneEntryIndex`, with the data they use, which records when each zone was
  entered; `World` supplies its zones, read live, and does the unloading. The player is in one zone at a time, so the
  order zones were entered in is the order they were left in.
- **The maximum is a `World` constructor argument,** from `StratumGame` and ultimately the game's config file.
- **Zones are identified by persistent id, not by name.** A template can be loaded more than once, so no name is
  unique; `create_zone`'s name parameter and `World::zone(name)` go.

## Session

```cpp
void Session::create(const Generator& generator, flecs::entity access_type);
void Session::load(const std::filesystem::path& filepath, flecs::entity access_type);
void Session::enter(flecs::entity zone, flecs::entity access_type);
void Session::set_driver(Driver driver, double units_per_second = Clock::units_per_second_default);
```

- **`create` and `load`** call `World::create_zone` and `World::load_zone`, then `enter` the zone at the access point
  type.
- **`enter(zone, access_type)`** moves the player into the zone, then has the world `trim_zones`, so no unload can take
  the player with it. It clears the pending command and rebuilds the turn order from the zone's actors and the player.
- **The driver is set on its own,** not passed to every load.
- **`populate` leaves `Session`,** and so do the door walls and the notation passed through it. The notation is the
  zone's, read by the host through the zone.

## Entry point

- **No zone file names an entry point.** It is chosen among the access points.
- **An access point is an entity with a relocator sited on a surface,** a wall, the floor or the ceiling. Its type is
  a registered entity --- up, down, waypoint, portal, or one a mod adds --- and its entry, the cell and orientation an
  arriving actor is placed at, is explicit in the file. The JSON is the source of truth; the stair glyphs are visual
  aids, and a discrepancy between them is a warning.
- **Each access point has an `order`,** given when it is created, counting from 0 within its type, so each type has its
  own 0.
- **The entry point is the access point of the desired type with the lowest `order` available,** any type allowed. Available means not already bound to a relocator elsewhere: with two staircases
  leading up, up staircase 0 bound to the level above by an earlier arrival, arriving from a different staircase
  leading down above enters at up staircase 1.
- **The library has no default type.** The caller passes one; for now each host passes `up` from a temporary constant
  of its own, so the player enters at up staircase 0. A zone with no access point of the type is an error.
- **A dungeon level has at least one staircase leading up,** its way out, so a generator's minimum count of staircases
  leading up is 1. A staircase leading down is optional: its probability is inversely proportional to the zone's
  `depth` and proportional to its `difficulty`, and 1 for now. `depth` is set only on a zone of the dungeon level type.
- **`type`, `order` and `entry` are in the zone file format** within the current version (`discrete_layout.md`,
  `## layout`).

## Entering an access point

- **An access point on a wall is entered by moving into the wall.** Its orientation is the wall's normal.
- **An access point on the floor does not impede movement.** Its orientation, its site's, serves drawing alone, and a
  keystroke by an actor on its cell enters it.
- **Entering an access point relocates the actor through its relocator,** to the zone its target resolves to, at that
  zone's entry point of the type the relocator's type arrives at (`addressing_plan.md` step 8).
- **In the wall notation every staircase is sited on a wall,** as a door is; a staircase on the floor is the block
  notation's.
- **A staircase in a flat length of wall in the wall notation is the equivalent of one in a 1x1 alcove in the block
  notation,** which guides generating zones in either without precluding a block staircase in the open space of a room
  (`discrete_layout.md`, `### Staircases and access points`).

## Actors

- **A pursuer with no target pursues the controlled actor,** which `think` is already given, so populating a zone
  needs no player. `Pursue::target_id` becomes optional.
- **An actor has a speed,** a component, and entering a zone schedules every entity in it with one. The actor structure
  is not designed; this is the least that lets the turn order be rebuilt from the zone.

## Steps

- [x] **1. Zones made by `World`.** `create_zone(generator)` and `load_zone(filepath)` with their doors, zones
      identified by persistent id, `populate`, the speed component and the optional pursuit target. `Session::populate`
      has the world populate the zone and schedules what has a speed.
- [x] **2. `Session` on `World`.** `enter`, `create` and `load` calling into `World`, `set_driver`, the player's speed,
      and `Session::populate` deleted. The hosts and suites follow.
- [x] **3. Residency.** `World::trim_zones`, the world's `ZoneEntryIndex` recording when each zone was entered against
      a zone count maximum, and an unload that completes when flecs runs the deletion, deferred or not.
- [ ] **4. Pre-populated zones.** The file's flag, and `load_zone` skipping `populate` where it is set; with the
      loader building every entity in the file (`discrete_layout_rework.md`), since the denizens a file places are
      entities in it.
- [x] **5. The entry point.** A `type`, an `order` counted within each type, and an explicit `entry` on each access
      point, in the format, read by `stratum_format`, written by both generators and in every zone file, access points
      on any surface, and glyph discrepancies warnings. Access point types registered as entities with their arrivals.
      The generators' minimum count of staircases leading up raised to 1, a staircase leading down placed with
      probability 1. Access points built into the zone by the loader, and `Session::enter` placing the player at the
      entry point of the type passed, a zone with no access point of the type an error. Availability
      is `addressing_plan.md` step 8, where relocators bind, and the probability inversely proportional to `depth`
      waits on `## Regions` in `work_plan.md` for its maximum depth.
- [ ] **6. Entering an access point.** The reader requiring the site's `orientation` on an access point on the floor or
      the ceiling. Moving into a wall with an access point on it entering the access point; an access point on the
      floor not impeding movement, and a keystroke on its cell, a command the hosts map, entering it. Entering relocates
      the actor through the relocator, so it waits on `addressing_plan.md` step 8 for where the relocator leads. The
      views drawing an access point on the floor by its orientation.
- [ ] **7. Staircases placed in every form.** The generators able to place a staircase in a flat length of wall or a
      1x1 alcove, the two being equivalent across the notations, as well as a block staircase on floor cells in the
      open space of a room. In the wall notation every staircase is sited on a wall.
