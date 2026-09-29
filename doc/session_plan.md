# Session --- Plan

Date: 2026-09-22, revised 2026-09-28

This plan describes how `Game`, `Session`, `World` and `Character` divide the work of creating, loading, populating and
playing zones, and how a game begins, is saved and loaded, and ends.

**`Game` is the game in its entirety.** It is the program from its loading, through the splash screen and the menu, to
the play sessions it runs, and it lasts as long as the host does. It owns, by value, everything that survives outside
play: the config, the settings, the id allocator, the engines, the `World`, the `Character` and the `Session`. None of them
is replaced; a new game or a load clears them in place. The hosts draw the splash screen and the menu; `Game` has the
state and the operations behind them. A new game resets the persistent id counter to zero.

**`Session` is the game being played,** from a new game or a load until the game ends, across every zone the character
enters. It plays the zones of the game's `World` through references, and owns none of the structures it plays.

`sim_zone_model.md` (`### Residency, simulation, and persistence`) describes when zones unload.
`game_system_reference.md` describes what is built, loading a zone from its document among it, and the open items are
in `work_plan.md` (`### Session`).

## Responsibilities

| Part | Responsible for |
| --- | --- |
| `Game` | The game in its entirety, lasting as long as the host: the config, the settings, the id allocator, the engines, the `World`, the `Character` and the `Session`, all by value; creating, loading, saving and ending games, and the game's own file |
| `Session` | The game being played: creating, loading, saving and clearing its character and world; the zone the character is in, entering a zone, the turn order, the clock and its driver, the character's intent and pending command, advancing, and whether the game is over |
| `World` | Regions and zones: creating, loading, populating, unloading and clearing them, knowing which are loaded, and choosing which unload as zones are entered; entities and persistent ids; the registrations that last as long as the program |
| `Character` | The entity the host controls |

## World

```cpp
flecs::entity World::create_zone(flecs::entity region, level_type level);
flecs::entity World::load_zone(flecs::entity region, const std::filesystem::path& filepath);
void World::populate(flecs::entity zone);
void World::trim_zones(flecs::entity zone_entered);
void World::clear();
```

- **`clear()`** unloads every region with no parent region, which deletes its child regions and their zones, and
  empties the world index and the `ZoneEntryIndex`. The registrations remain: the file components, the entity types
  and the access point types, which are the program's and, later, its mods'.
- **A generator registry** would make a region's generator data: the generator's name and its parameters, as the world
  file writes them (`game_file_spec.md`, `### Regions`), with the generators registered by name for as long as the
  program runs, in place of the `std::shared_ptr<const Generator>` component (`work_plan.md`, `### Regions`).

- **`create_zone(region, level)`** makes an empty zone at the level, has the region's generator fill it, and populates
  it.
- **`load_zone(region, filepath)`** makes an empty zone, has the loader fill it from the file, and populates it unless the file
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
  character is in one zone at a time, so the order in which zones were entered is the order in which they were left.
- **The maximum is a `World` constructor argument,** which `Game` passes from its config.
- **Zones are identified by persistent id, not by name.** A template can be loaded more than once, so no name is
  unique; `create_zone`'s name parameter and `World::zone(name)` are removed.

## Session

```cpp
void Session::clear();
void Session::create(const Generator& generator, level_type level, flecs::entity access_type, level_type offset);
void Session::create(const Generator& generator, const std::filesystem::path& zone_filepath, level_type level,
    flecs::entity access_type, level_type offset);
void Session::load(const std::filesystem::path& directory);
void Session::save(const std::filesystem::path& directory) const;
void Session::enter(flecs::entity zone, flecs::entity access_type, level_type offset);
void Session::set_driver(Driver driver, double units_per_second = Clock::units_per_second_default);
bool Session::loaded() const;
bool Session::over() const;
```

- **`clear()`** clears the play state --- the zone being played, the turn order, the clock and the pending command ---
  then the world, with `World::clear`, and the character, whose entity and inventory are deleted. `loaded()` is then false.
- **`create`** creates the character's entity, a starting region with the generator, and its first zone at the level, with
  `World::create_zone(region, level)`, then `enter`s it. The second form is a test convenience: the first zone is the
  zone file's, at the level if the file states none, and the region's generator makes the zones after it.
- **`load(directory)`** reads the game's `character.json` and `world.json`, builds the regions and the character, loads the
  zone the character is in from its file, and plays it, with the character at its saved placement. The other zones load as
  they are entered.
- **`save(directory)`** writes `character.json`, `world.json`, and the file of every loaded zone.
- **`enter(zone, access_type, offset)`** moves the character into the zone, at its entry point, then has the world `trim_zones`, so no unload can take
  the character with it. It clears the pending command and rebuilds the turn order from the zone's actors, the character
  included.
- **The driver is set separately,** not passed to every load.
- **`populate` moves out of `Session`,** and so do the door walls and the notation passed through it. The notation is
  the zone's, and the host reads it through the zone.

## Games

A game runs from creating a new game or loading a game until the player quits, loads another game, or dies, and it is
played in one `Session`. A game is the player's character and the world's regions, which group its zones, and it is
saved as a directory of files: one for the game's state and settings, one for the character, one for the world, and
one for each zone (`game_file_spec.md`). The menu that creates, saves, loads and quits games is drawn by a host,
which calls `Game` for each of them.

```cpp
struct GameConfig
{
    std::filesystem::path saves_directory;
    std::size_t zone_count_max;
};

struct GameSettings
{
    bool permadeath;
};

struct SavedGame
{
    std::filesystem::path directory;
    std::string time_saved; // ISO 8601
    level_type level;       // Of the zone the character is in
};

Game::Game(const GameConfig& config);
void Game::new_game(const std::string& name, const GameSettings& settings);
void Game::load_game(const std::filesystem::path& directory);
void Game::save_game();
void Game::end_game();
Session& Game::session();
World& Game::world();
Character& Game::character();
const GameSettings& Game::settings() const;
std::vector<SavedGame> Game::find_saved_games() const;
```

### Lifetime

- **Each host owns one `Game` for as long as it runs,** and `Game` owns its members by value, none of them replaced. A
  host keeps a reference to the session, as it does now, and `Session::loaded()` returns false while no game is being
  played, as while the menu is shown before the first game.
- **`new_game` and `load_game` clear the game in progress in place,** once the player has chosen, after a confirmation
  if the host asks for one. `Session::clear` clears the play state, the world and the character, and `Game` resets the id
  allocator and reseeds the engines, so no state passes from one game to the next. A failure leaves no game in
  progress; the host reports it and returns to the menu.
- **`new_game` sets the settings and the game's directory,** and the host then has the session `create` the character, a
  starting region and its first zone. The host passes the generator, the level, the access point type and the level
  offset, from temporary constants of its own until a new game chooses its region.
- **`load_game` reads the game's `game.json`,** restoring the settings, the id allocator's count and the engines' state,
  then has the session `load` the character and the world.
- **`end_game` clears nothing.** The world and the character remain until the next new game or load clears them, so a host
  can draw the game's last state behind the menu or a death screen.
- **A suite plays a game, clears it, and creates another,** and checks that the world then contains only the new game's
  regions, zones, entities and ids, so that nothing a clear misses survives into the next game.
- **The config is the program's:** the saves directory and the zone count maximum, from the game's config file. Until
  that file is read, each host passes a temporary config, with the saves directory in the platform's per-user data
  directory: `user://saves` in Godot, and `stratum/saves` under `%APPDATA%` or `$XDG_DATA_HOME` in the terminal host. A
  suite passes a directory of its own.
- **The settings are the game's,** chosen when it begins and saved with it; `permadeath` is the first. The library has
  no default settings: each host passes temporary settings of its own until a menu chooses them.
- **A host's zone file is a test convenience:** it creates a new game whose first zone is the file's.

### Saving and loading

- **A game is a directory in the saves directory,** named by the game. A host names a new game by the time it began, in
  ISO 8601 basic format, until a player names a game.
- **Unloading a zone writes its zone file, then frees its resources,** and entering a zone that is not loaded reads its
  file.
- **`save_game` writes the game's files:** `game.json`, with the game's state and the settings, then has the session
  `save` the character, the world, and the file of every loaded zone.
- **Under permadeath, `Game` saves each time the character enters a zone,** so no earlier state can be loaded.
- **`find_saved_games`** returns a `SavedGame` for each directory in the saves directory that has a game's files, for a
  menu to list.

### Ending

- **`Session::over()` returns true once the character is dead,** and `advance` then runs no actor. The character is dead when
  its entity has the `Dead` tag.
- **`end_game` is called when the game stops,** by quitting or by the character's death. Under permadeath it deletes the
  game's directory if the character is dead, and saves the game otherwise. Without permadeath the game remains to be loaded,
  and a host that offers saving on quitting calls `save_game` first.

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
