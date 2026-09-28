# Session --- Plan

Date: 2026-09-22, revised 2026-09-26

This plan describes how the parts of `StratumGame` divide the work of creating, loading, populating and playing zones.
`World` does the zone management, and `Session` keeps the state of play and calls into `World`. `sim_zone_model.md`
(`### Residency, simulation, and persistence`) describes when zones unload. `game_system_reference.md` describes what
is built, loading a zone from its document among it, and the open items are in `work_plan.md` (`### Session`).

## Responsibilities

| Part | Responsible for |
| --- | --- |
| `World` | Zones: creating, loading, populating and unloading them, knowing which are loaded, and choosing which unload as zones are entered; entities and persistent ids |
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
  is already populated: a saved zone, or a generated one with its denizens placed, which a flag in the file indicates.
- **The generator and the loader build the doors** into the zone with its layout and notation. `World` handles none.
- **Neither unloads anything.** Creating and loading add a zone to the world.
- **`populate(zone)`** places the zone's denizens. Until placement is built (`placement_plan.md`), it places one
  pursuer at the last open cell.
- **The population strategy comes from the zone's source:** the template it was loaded from, or the guidance it was
  generated with. The placeholder stays in `World` until one of them supplies a strategy.
- **`trim_zones(zone_entered)`** unloads the zones entered longest ago until the world has no more than its maximum,
  and never unloads `zone_entered`. The world's `ZoneEntryIndex` applies the criteria and keeps the data they use,
  which is when each zone was entered; `World` supplies its zones, queried each time, and does the unloading. The
  player is in one zone at a time, so the order in which zones were entered is the order in which they were left.
- **The maximum is a `World` constructor argument,** from `StratumGame` and ultimately from the game's config file.
- **Zones are identified by persistent id, not by name.** A template can be loaded more than once, so no name is
  unique; `create_zone`'s name parameter and `World::zone(name)` are removed.

## Session

```cpp
void Session::create(flecs::entity region, const Generator& generator, flecs::entity access_type, level_type offset);
void Session::load(flecs::entity region, const std::filesystem::path& filepath, flecs::entity access_type, level_type offset);
void Session::enter(flecs::entity zone, flecs::entity access_type, level_type offset);
void Session::set_driver(Driver driver, double units_per_second = Clock::units_per_second_default);
```

- **`create` and `load`** call `World::create_zone` and `World::load_zone`, then `enter` the zone.
- **`enter(zone, access_type, offset)`** moves the player into the zone, at its entry point, then has the world `trim_zones`, so no unload can take
  the player with it. It clears the pending command and rebuilds the turn order from the zone's actors, the player
  included.
- **The driver is set separately,** not passed to every load.
- **`populate` moves out of `Session`,** and so do the door walls and the notation passed through it. The notation is
  the zone's, and the host reads it through the zone.

## Entry point

- **No zone file names an entry point.** It is chosen from among the access points.
- **An access point is an entity with a relocator, sited on a surface:** a wall, the floor or the ceiling. Its type is
  a registered entity --- staircase, door, portal, waypoint, or a type that a mod adds --- and its placement, the cell
  and orientation at which an arriving actor is placed, is explicit in the file. Whether it goes up, down, or across
  is in its relocator's target. The JSON is the source of truth; the stair glyphs are visual aids, and a discrepancy between them is a warning.
- **Each access point has an `order`,** given when it is created, counting from 0 within its type, so each type has its
  own 0.
- **The entry point is the available access point with the lowest `order` among those of the desired type whose
  relocators target the desired level offset,** and any type is allowed. An actor arriving from another level wants the
  offset that leads back. Available means not already bound to a relocator elsewhere. With
  two staircases leading up, where the lower-ordered one was bound to the level above by an earlier arrival, an actor
  arriving from a different staircase leading down above enters at the other.
- **The library has no default type or offset.** The caller passes them; for now each host passes `staircase` and 1
  from temporary constants of its own, so the player enters at the lowest-ordered staircase leading up. A zone with no
  such access point is an error.
- **A dungeon level has at least one staircase leading up,** its way out, so a generator's minimum count of staircases
  leading up is 1. A staircase leading down is optional: its probability is inversely proportional to how far below
  the surface the zone's level is and proportional to its `difficulty`, and is 1 for now (`region_plan.md`,
  `## Level`).
- **`type`, `order` and `placement` are in the zone file format** within the current version (`discrete_layout.md`,
  `## layout`).

## Entering an access point

- **An access point on a wall is entered by interacting with the wall,** as a door is opened. Moving into the wall is
  blocked, so a held movement key never takes the stairs. Its orientation is the wall's normal.
- **An access point on the floor does not impede movement.** Its orientation, which is its site's, is used only for
  drawing, and a keystroke by an actor on its cell enters it.
- **Entering an access point relocates the actor through its relocator,** to the zone that its target resolves to, at
  that zone's entry point of the type at which the relocator's type arrives that leads back.
- **In the wall notation every staircase is sited on a wall,** as a door is; a staircase on the floor belongs to the
  block notation.
- **A staircase in a flat length of wall in the wall notation is the equivalent of one in a 1x1 alcove in the block
  notation.** This guides generating zones in either notation, without ruling out a block staircase in the open space
  of a room (`discrete_layout.md`, `### Staircases and access points`).

## Actors

- **A pursuer with no target pursues the controlled actor,** which `think` is already given, so populating a zone
  needs no player. `Pursue::target_id` becomes optional.
- **An actor has a speed,** which is a component, and entering a zone schedules every entity in it that has one. The
  actor structure is not designed; this is the least that lets the turn order be rebuilt from the zone.
