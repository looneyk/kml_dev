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
flecs::entity World::create(const RegionSpec& region_spec, const std::filesystem::path& game_directory);
void World::open(const std::filesystem::path& game_directory);
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
- **`create(region_spec, game_directory)`** creates a new game's starting region and its first zone from a
  `RegionSpec`, and returns the zone's entry point for the spec's `entry_access`. Each game has its own starting region.
  The game directory is the game's directory in the working directory, which zones are saved into; `open` takes it too,
  and `clear` removes it.
- **A `RegionSpec` is a request to create a region,** not the region, and a `ZoneSpec` a request to create a zone.
  Both are a `KeyValueStore` (`archived/key_value_store_plan.md`). `world_generation_plan.md` describes what they
  contain and how a region and its zones are created from them. A key that is not read, or a value that does not
  parse, is an error that reaches the host.
- **`open(game_directory)`** reads the game's `world.json`: its regions, with their specs, components and zone ids,
  and the world index. It loads no zone; each loads as it is entered.
- **`save()`** writes `world.json`, with the world index, and the file of every loaded zone into the game directory.
- **`save_zone(zone)`** writes one zone's file into the game directory. The zone's unloading calls it, and so
  does every save of the game, explicit, quick or auto, for each loaded zone. Leaving a zone does not save it.
- **`create_region_zone(region_spec, zone_spec)`** creates a region and its first zone, and returns the zone; it is how
  every region is created. **`create_zone(region, zone_spec)`** creates a zone in the region: the region's generator
  selector chooses a generator and its parameters, the generator returns the zone's `Document`, and `World` builds the
  zone from it and populates it (`world_generation_plan.md`).
- **The registries** of generators, generator selectors and region specs are `Game`'s, and `World` takes them by
  reference in its constructor, as it takes the id allocator and the engine.
- **`load_zone(region, filepath)`** makes an empty zone, has the loader fill it from the file, and populates it if the
  file's `unpopulated` flag is set. A file whose denizens are already placed is not unpopulated, and a saved zone never
  is.
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
void Session::start(flecs::entity access_point);
void Session::pause();
void Session::resume();
void Session::enter(flecs::entity zone, flecs::entity access_type, level_type offset);
void Session::set_driver(Driver driver, double units_per_second = Clock::units_per_second_default);
bool Session::loaded() const;
bool Session::over() const;
```

- **`clear()`** clears the play state --- the zone being played, the turn order, the clock and the pending command.
  `loaded()` is then false. The driver remains. `Game` clears the character and the world.
- **`start(access_point)`** begins play of the game that `Game` has created or opened, entering the access point's zone
  and placing the character there. For a new game, that is the entry point `World::create` returns; for an opened game,
  the zone the character is in. The session makes no zone.
- **`pause()` and `resume()`** stop and restart play while the menu is open, and `paused()` returns whether play is
  stopped. Calling `advance` while play is paused is an error. Real time earns nothing while paused, so closing the
  menu does not credit the time it was open. Both do nothing while no game is loaded.
- **A host starts at its menu,** and opens it with Escape during play. Quitting is chosen from the menu; no key quits
  during play.
- **The session makes no zone.** `World` creates every zone, and the session enters them.
- **`enter(zone, access_type, offset)`** moves the character into the zone, at its entry point, then has the world `trim_zones`, so no unload can take
  the character with it. It clears the pending command and rebuilds the turn order from the zone's actors, the character
  included.
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
  entity is in no zone until one is entered. The constructor makes none, so a game's character has an id from that
  game's allocator.
- **A `CharacterSpec` is a request to create a character,** not the character: the choices a new game makes.

  ```cpp
  struct CharacterSpec
  {
      struct InventoryLocation
      {
          std::string relation; // "in", "worn", ...
          std::string place;    // "backpack", "legs", ...
      };

      struct InventoryItem
      {
          std::string type;         // The item's type, defined in data
          InventoryLocation location;
          KeyValueStore properties; // name, quantity, and values that differ from the type's, such as size or weight
      };

      std::string name;
      std::map<std::string, int> stats; // <stat name, value>
      std::map<std::string, KeyValueStore> skills; // <name, properties>, include skills, feats, abilities, ...; properties might include level, proficiency, usage_count, usage_count_max, range, aoe, ...
      std::map<std::string, KeyValueStore> aspects; // <name, properties>, where a skill might be spellcasting, this would be the list of spells; properties might include parent skill in addition to the same set as skills might have
      std::vector<InventoryItem> inventory;
      KeyValueStore properties; // race, class, background, appearance, level, ...
  };
  ```

- **Only what the engine reads whatever the game is has a field of its own:** the name, which becomes the character's
  `DisplayName`. Race, class and background are one game's choices, so they are `properties` that data defines, and a
  game or a mod adds others without changing the structure. The stats, skills and spells are maps of names that data
  defines, typed because their values are numbers, and the inventory is a list of items.
- **`create` resolves the names to types,** from which it builds the character's components, its skills and spells, and
  its character level, which starts at 1. Each `InventoryItem` becomes the entity of its `type`, at the location it
  states, with its `properties` set over the type's values. A name that does not resolve, a property that data does not
  define, or a stat that is not defined, is an error that reaches the host. The spec is not saved: `character.json` has
  the character it made.
- **`open(game_directory)`** reads the game's `character.json`: the character's components, its inventory, and the
  zone it is in and its placement there.
- **`save(game_directory)`** writes `character.json`.
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
    std::filesystem::path working_directory;
    std::size_t zone_count_max;
};

Game::Game(const Config& config);
void Game::menu_open();
void Game::menu_close();
void Game::clear();
void Game::create(const RegionSpec& region_spec, const CharacterSpec& character_spec);
void Game::open(std::string_view save_name);
std::string Game::save(SaveType save_type);
void Game::quit();
std::vector<SavedGameDetails> Game::saved_games() const;
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
- **The working directory has only the game in progress.** `create` and `open` empty it before making the game
  directory, which the character names.
- **`create`** clears, resets the id allocator and reseeds the engines, so no state passes from one game to the next.
  It creates the game directory, then has the character created from the `CharacterSpec`, and the world's starting
  region and its first zone from the `RegionSpec`, and starts the session at the access point `World::create` returns.
  The host passes only the specs. Until a new game chooses its region and creates its character, each host passes
  temporary ones.
- **`open(save_name)`** clears, copies the save into the working directory as the game directory, reads `game.json`
  from it, restoring the id allocator's count and either restoring the engines' state or reseeding them, as the
  settings choose, has the world and then the character open their files, and starts the session in the zone the
  character is in, which loads from its file, at the character's placement.
- **`save(save_type)`** is the menu's save of the whole game, and a quick save or an auto save is the same, with its
  `SaveType`. It writes `game.json`, with the game's state, into the game directory, has the character and the world
  save theirs there, the world saving every loaded zone, and then copies the game directory to a new save directory in
  the saves directory, `<character name>_<save time>`, the name the character's own, with `_autosave` or `_quicksave`
  appended for those types. A game whose character is dead cannot be saved.
- **`quit`** is called by the host when the player chooses to quit. It clears the game and returns, and the host
  continues with its termination.
- **A suite plays a game, clears it, and creates another,** and checks that the world then contains only the new game's
  regions, zones, entities and ids, so that nothing a clear misses survives into the next game.
- **The config is the program's:** the saves directory, the working directory and the zone count maximum, from the
  game's config file. Until that file is read, each host passes a temporary config, with the two directories in the temp
  directory's `KML/Stratum/saves` and `KML/Stratum/working` (`game_file_spec.md`, `## A saved game's files`). A suite
  passes directories of its own.
- **The settings are the game's,** chosen when it begins and saved with it. They come with permadeath, the first of
  them (`### Permadeath`).

### Saving and loading

- **A game is a directory in the working directory while it is played, and each save of it a directory in the saves
  directory,** named by the character's name and the time of the save (`game_file_spec.md`, `## A saved game's files`).
  The game directory is named by the character.
- **`World::save_zone` writes a zone's file into the game directory when the zone unloads,** which then frees its
  resources, and for every loaded zone when the game is saved, explicitly, by a quick save, or by an auto save. Leaving
  a zone does not write it. Entering a zone that is not loaded reads its file from the game directory.
- **A saved game is one consistent point.** Saving copies the whole game directory, so every file in the saved game
  is from the moment of the save. Opening starts from that copy, so a zone that unloaded after the last save, and was
  not saved since, is discarded, and quitting without saving loses everything since the last save.
- **The character's intent and pending command are not saved.** A loaded character starts with neither, so it never
  continues an action the player may not remember having started.
- **`saved_games`** returns each save in the saves directory, newest first, for a menu to list: the save's name, which
  `open` takes, the character's name as the player entered it, the time of the save, and its save type, read from its
  name. The menu shows the character's name, the time, and the save type, "Autosave", "Quicksave" or nothing for an
  explicit save, and not the save's name.
- **Whether opening a save repeats its outcomes is the player's setting,** `Settings::random_seeding_on_open`: restoring the
  engine's saved state repeats them, and reseeding lets a reload have outcomes of its own. The engine is the same
  either way.

### Quick saves and autosaves

- **A quick save and an autosave are ordinary saves,** differing from an explicit save only in the `_quicksave` or
  `_autosave` that ends the save's name. Depending on the game's settings, an autosave or quicksave may replace the previous one in its track.
  There should be a quicksave_count and autosave_count, and only the most recent of that sequence should be retained. Generally those counts will be 1.
- **F5 quick saves during play** in both hosts, through `save(SaveType::Quick)`, and the status line reports it as the
  menu's save is reported.
- **A quickload key opens a quick save during play,** F8 or F9, through `open`.
- **The game autosaves when the character enters a zone,** through `save(SaveType::Auto)`, once, at the end of the
  advance in which it entered, after the session has made the new zone the one being played, so the save is of a
  finished advance.
- **`Game` gives `Session` the function that autosaves** when it constructs it, as it gives it the `World` and the
  `Character`. `Session::advance` calls it when the character went through a relocator into another zone during that
  advance. The hosts call `Session::advance` as they do now, and only `Game`'s function decides what triggers an
  autosave. `start`, from `create` or `open`, does not call it.
- **A failed autosave does not end the game.** `Game`'s function catches the error and reports it in an event of the
  advance, which the hosts show on the status line; an exception leaving `advance` would make the hosts quit the game.
- **`read_key` reports the function keys** the hosts use: on Windows, `_getch`'s prefix of 0 or 0xE0 and its code, and
  elsewhere the terminal's escape sequence, as it does for the arrows.

### Ending

- **`Session::over()` returns true once the character is dead,** and `advance` then runs no actor. The character is dead when
  its entity has the `Dead` tag.
- **The character's death forces a quit.** `advance` reports the death to the host in its events, the host shows it
  with the game's last state and waits for a key, then calls `quit` and returns to its menu. The game remains as it
  was last saved, to be opened again.

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
- **The library has no default type or offset.** A new game's region spec states them in its `entry_access` key
  (`world_generation_plan.md`, `## Creating a game`); the hosts' temporary spec states `staircase`
  and 1, so the character enters at the lowest-ordered staircase leading up. A zone with no such access point is an
  error.
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

### Following through a relocator

This is a future feature. Only the controlled actor enters a relocator.

- **An active pursuer can follow the character through a relocator** if it has the `InterzonePursuitCapability`
  component. The component is set when the pursuer is created, from its type, or given to it by script.
- **When the character leaves through a relocator,** such a pursuer is removed from its zone and queued for placement in
  the zone the character arrived in.
- **A queued pursuer is placed when the conditions allow,** for timing and for space. At least one turn passes first,
  and it is placed only at the placement of the access point the character arrived at, so the character has to move off
  that cell before the pursuer can arrive.
- **The character's use of a relocator may be restricted while hostiles are nearby,** which, where it applies, leaves
  no pursuer close enough to follow.

Open:

- **Which pursuers follow:** any active pursuer of the character, or only those within some distance or in sight of it
  when it leaves.
- **Where the queue is kept,** and what becomes of a queued pursuer whose destination zone unloads before it is placed.
- **A queued pursuer is in no zone,** so it is unscheduled and in no zone's save; how it is saved with the game.
- **How the delay is measured:** turns of the character, or the scheduler's units.
