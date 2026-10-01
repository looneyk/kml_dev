# Session --- Plan

Date: 2026-09-22, revised 2026-10-01

This plan describes how `Game`, `Session`, `World` and `Character` divide the work of creating, loading, populating and
playing zones, and how a game begins, is saved and opened, and ends, through the menu operations a host calls.

**`Game` is the game in its entirety.** It is the program from its loading, through the splash screen and the menu, to
the play sessions it runs, and it lasts as long as the host does. It owns, by value, everything that survives outside
play: the config, the settings, the id allocator, the engines, the `World`, the `Character` and the `Session`. None of them
is replaced; creating or opening a game clears them in place. The hosts draw the splash screen and the menu; `Game` has
the state and the operations behind them. A new game resets the persistent id counter to zero.

**`Session` is the game being played,** from its creation or opening until it is quit, across every zone the character
enters. It plays the zones of the game's `World` through references, and owns none of the structures it plays.

`sim_zone_model.md` (`### Residency, simulation, and persistence`) describes when zones unload.
`game_system_reference.md` describes what is built, loading a zone from its document among it, and the open items are
in `work_plan.md` (`### Session`).

## Responsibilities

| Part | Responsible for |
| --- | --- |
| `Game` | The game in its entirety, lasting as long as the host: the config, the settings, the id allocator, the engines, the `World`, the `Character` and the `Session`, all by value; the registries of generators, generator selectors and region specs; the menu operations --- opening and closing the menu, creating, opening, saving and quitting games, and listing the saved games --- and the game's own file |
| `Session` | The game being played: starting, pausing, resuming and clearing play; the zone the character is in, entering a zone, the turn order, the clock and its driver, the character's intent and pending command, advancing, and whether the game is over |
| `World` | Regions and zones: creating regions and zones from their specs; creating, loading, populating, saving, unloading and clearing zones, knowing which are loaded, and choosing which unload as zones are entered; the world's file; entities and persistent ids; the registrations that last as long as the program |
| `Character` | The entity the host controls: creating it, opening and saving its file, and clearing it |

## World

```cpp
void World::clear();
flecs::entity World::create(const RegionSpec& region_spec);
void World::open(std::string_view directory);
void World::save(std::string_view directory);
void World::save_zone(flecs::entity zone);
flecs::entity World::create_region_zone(const RegionSpec& region_spec, const ZoneSpec& zone_spec);
flecs::entity World::create_zone(flecs::entity region, const ZoneSpec& zone_spec);
flecs::entity World::load_zone(flecs::entity region, const std::filesystem::path& filepath);
void World::populate(flecs::entity zone);
void World::trim_zones(flecs::entity zone_entered);
```

- **`clear()`** unloads every region, which deletes its zones, and empties the world index and the `ZoneEntryIndex`.
  The registrations remain: the file components, the entity types, the access point types, and the registries that
  `World` takes from `Game`, which are the program's and, later, its mods'.
- **`create(region_spec)`** creates a new game's starting region and its first zone from a `RegionSpec`, and returns
  the zone. Each game has its own starting region, where `Game`'s constructor makes one now.
- **A `RegionSpec` is a request to create a region,** not the region, and a `ZoneSpec` a request to create a zone.
  Both are a `KeyValueStore` (`archived/key_value_store_plan.md`). `world_generation_plan.md` describes what they
  contain and how a region and its zones are created from them. A key that is not read, or a value that does not
  parse, is an error that reaches the host.
- **`open(directory)`** reads the game's `world.json`: its regions, with their specs and components, and the reference
  to each zone's file. It loads no zone; each loads as it is entered.
- **`save(directory)`** writes `world.json` and the file of every loaded zone into the directory.
- **`save_zone(zone)`** writes one zone's file into the game's directory. The zone's unloading calls it, and so does the
  session when the character exits the zone, so a zone's file is the zone as it was last exited.
- **`create_region_zone(region_spec, zone_spec)`** creates a region and its first zone, and returns the zone; it is how
  every region is created. **`create_zone(region, zone_spec)`** creates a zone in the region: the region's generator
  selector chooses a generator and its parameters, the generator returns the zone's `Document`, and `World` builds the
  zone from it and populates it (`world_generation_plan.md`).
- **The registries** of generators, generator selectors and region specs are `Game`'s, and `World` takes them by
  reference in its constructor, as it takes the id allocator and the engine.
- **`load_zone(region, filepath)`** makes an empty zone, has the loader fill it from the file, and populates it unless the file
  is already populated: a saved zone, or a generated one with its denizens placed, which a flag in the file indicates.
- **A zone's doors are in its `Document`,** generated or read, and `load_layout` builds them. `World` handles none.
- **Neither unloads anything.** Creating and loading add a zone to the world.
- **`populate(zone)`** places the zone's denizens. Until placement is built (`placement_plan.md`), it places one
  pursuer at the last open cell.
- **The population strategy comes from the zone's source:** the template it was loaded from, or the guidance it was
  generated with. The placeholder stays in `World` until one of them supplies a strategy.
- **`trim_zones(zone_entered)`** unloads the zones entered longest ago until the world has no more than its maximum,
  and never unloads `zone_entered`. The world's `ZoneEntryIndex` applies the criteria and keeps the data they use,
  which is when each zone was entered; `World` supplies its zones, queried each time, and does the unloading. The
  character is in one zone at a time, so the order in which zones were entered is the order in which they were left.
- **The maximum is a `World` constructor argument,** which `Game` passes from its config.
- **Zones are identified by persistent id, not by name.** A template can be loaded more than once, so no name is
  unique; `create_zone`'s name parameter and `World::zone(name)` are removed.

## Session

```cpp
void Session::clear();
void Session::start(flecs::entity zone);
void Session::pause();
void Session::resume();
void Session::enter(flecs::entity zone, flecs::entity access_type, level_type offset);
void Session::set_driver(Driver driver, double units_per_second = Clock::units_per_second_default);
bool Session::loaded() const;
bool Session::over() const;
```

- **`clear()`** clears the play state --- the zone being played, the turn order, the clock and the pending command.
  `loaded()` is then false. The driver remains. `Game` clears the character and the world.
- **`start(zone)`** begins play of the game that `Game` has created or opened, in the zone. For a new game, that is the
  first zone, which `World::create` returns; for an opened game, the zone the character is in. The session makes no
  zone.
- **`pause()` and `resume()`** stop and restart play while the menu is open. `advance` runs no actor while play is
  paused, and real time earns nothing, so closing the menu does not credit the time it was open. Both do nothing while
  no game is loaded.
- **The session makes no zone.** `World` creates every zone, and the session enters them. The hosts load a zone file
  with `World::load_zone` only until generation creates a playable first zone.
- **`enter(zone, access_type, offset)`** moves the character into the zone, at its entry point, then has the world `trim_zones`, so no unload can take
  the character with it. It clears the pending command and rebuilds the turn order from the zone's actors, the character
  included.
- **Changing the zone being played** --- by `enter`, or by the character going through a relocator during `advance` ---
  saves the zone exited with `World::save_zone`.
- **The driver is set separately,** not passed to every load.
- **`populate` moves out of `Session`,** and so do the door walls and the notation passed through it. The notation is
  the zone's, and the host reads it through the zone.

## Character

```cpp
void Character::clear();
void Character::create(const CharacterSpec& character_spec);
void Character::open(std::string_view directory);
void Character::save(std::string_view directory);
flecs::entity Character::entity() const;
```

- **`clear()`** deletes the character's entity and its children, the inventory. The world's clear does not reach them,
  since the character has no parent.
- **`create(character_spec)`** makes the character's entity, with a new persistent id, from a `CharacterSpec`. The
  entity is in no zone until one is entered. The constructor makes the entity now, so its id is allocated before a new
  game resets the allocator.
- **A `CharacterSpec` is a request to create a character,** not the character: a `KeyValueStore` of the choices a new
  game makes, such as its name, class, race, history and appearance, and its stats. Most choices are names that
  `create` resolves to types, from which it builds the character's components, its skills and spells, and its
  character level, which starts at 1. A list, such as a starting inventory, is named rather than written out: the
  request names a kit that is defined in data. A key `create` does not read, or a value that does not parse, is an
  error that reaches the host.
- **`open(directory)`** reads the game's `character.json`: the character's components, its inventory, and the zone it
  is in and its placement there.
- **`save(directory)`** writes `character.json`.
## Games

A game runs from its creation or opening until the player quits, opens another game, or the character dies, and it is
played in one `Session`. A game is the player's character and the world's regions, which group its zones, and it is
saved as a directory of files: one for the game's state and settings, one for the character, one for the world, and
one for each zone (`game_file_spec.md`). The menu that creates, opens, saves and quits games is drawn by a host,
which calls `Game` for each of them.

```cpp
struct Config
{
    std::filesystem::path saves_directory;
    std::size_t zone_count_max;
};

Game::Game(const Config& config);
void Game::menu_open();
void Game::menu_close();
void Game::clear();
void Game::create(const RegionSpec& region_spec, const CharacterSpec& character_spec);
void Game::open(std::string_view directory);
void Game::save(std::string_view directory);
void Game::quit();
std::vector<std::string> Game::saved_games();
Session& Game::session();
World& Game::world();
Character& Game::character();
```

### Lifetime

- **Each host owns one `Game` for as long as it runs,** and `Game` owns its members by value, none of them replaced. A
  host keeps a reference to the session, as it does now, and `Session::loaded()` returns false while no game is being
  played, as while the menu is shown before the first game.
- **The host's menu calls `Game` for each operation.** `menu_open` pauses play and `menu_close` resumes it; `create`,
  `open`, `save` and `quit` act on the whole game; and `saved_games` lists the games that `open` can open.
- **`clear` clears the session, the character and the world, in that order, in place.** `create` and `open` clear
  first, so the game in progress ends once the player has chosen, after a confirmation if the host asks for one. A
  failure leaves no game in progress; the host reports it and returns to the menu.
- **`create`** clears, resets the id allocator and reseeds the engines, so no state passes from one game to the next.
  It then has the character created from the `CharacterSpec`, and the world's starting region and its first zone from
  the `RegionSpec`, and starts the session in that zone. The host passes only the specs. Until a new game chooses its
  region, each host passes temporary ones.
- **`open(directory)`** clears, reads the game's `game.json`, restoring the id allocator's count and the engines'
  state, has the world and then the character open their files, and starts the session in the zone the character is
  in, which loads from its file.
- **`save(directory)`** is the menu's save of the whole game. It writes `game.json`, with the game's state, then has
  the character and the world save theirs.
- **`quit`** is called by the host when the player chooses to quit. It clears the game and returns, and the host
  continues with its termination.
- **A suite plays a game, clears it, and creates another,** and checks that the world then contains only the new game's
  regions, zones, entities and ids, so that nothing a clear misses survives into the next game.
- **The config is the program's:** the saves directory and the zone count maximum, from the game's config file. Until
  that file is read, each host passes a temporary config, with the saves directory in the platform's per-user data
  directory: `user://saves` in Godot, and `stratum/saves` under `%APPDATA%` or `$XDG_DATA_HOME` in the terminal host. A
  suite passes a directory of its own.
- **The settings are the game's,** chosen when it begins and saved with it. They come with permadeath, the first of
  them (`### Permadeath`).

### Saving and loading

- **A game is a directory in the saves directory,** named by the game. A host names a new game by the time it began, in
  ISO 8601 basic format, until a player names a game.
- **`World::save_zone` writes a zone's file when the character exits the zone and when the zone unloads,** which then
  frees its resources. Entering a zone that is not loaded reads its file.
- **A load restores the zones from their files as they are,** including those written on unloading after the game was
  last saved, so quitting without saving leaves each such zone in the state at which it was last exited. Auto saves and
  quick saves override this (`work_plan.md`, `### Game file`).
- **The character's intent and pending command are not saved.** A loaded character starts with neither, so it never
  continues an action the player may not remember having started.
- **`saved_games`** returns the name of each game's directory in the saves directory, for a menu to list.

### Ending

- **`Session::over()` returns true once the character is dead,** and `advance` then runs no actor. The character is dead when
  its entity has the `Dead` tag.
- **The character's death forces a quit.** `advance` reports the death to the host in its events, the host shows it
  with the game's last state, and then calls `quit`. The game remains as it was last saved, to be opened again.

### Permadeath

- **Permadeath comes later,** as a setting of the game, the first of its `Settings`. It changes what `create`,
  `save`, `open` and `quit` do, without changing their signatures: `quit` saves the game if the character is alive and
  deletes the game's directory if the character is dead.

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
  from temporary constants of its own, so the character enters at the lowest-ordered staircase leading up. A zone with no
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
  needs no character. `Pursue::target_id` becomes optional.
- **An actor has a speed,** which is a component, and entering a zone schedules every entity in it that has one. The
  actor structure is not designed; this is the least that lets the turn order be rebuilt from the zone.
