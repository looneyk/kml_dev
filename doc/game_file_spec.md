# Game File Format Specification

Status: draft --- read and written by `stratum_format`
Date: 2026-09-28, revised 2026-10-07

This document specifies how a game is saved. A game has four parts: the game's own state, its settings, the player's
character, and the world, which is a set of worldspaces, each containing zones and a region that groups them. A saved
game is a directory with one file for the game's state and settings, one for the character, one for the world, and one
for each zone. The world file organizes the worldspaces and their regions and refers to the zone files. A zone file is the file that `discrete_layout.md` specifies. `session_plan.md` (`## Games`) describes
the `Game` that writes and reads these files, and `sim_zone_model.md` the model that the world serializes.

## A saved game's files

The program's files are in two directories. The program reads the program directory, and writes only in the user
directory; SDL calls the two the base path and the pref path.

```
<install>/                      the program directory, which the program never writes
  stratum_terminal.exe
  config.json                   optional, the config file
  assets/                       the program's own data, read only
    data/manifests/             the game manifest files

%LOCALAPPDATA%/KML/Stratum/     the user directory, everything the program creates while it runs
  settings.json                 the user settings file
  cache/
  logs/
  mods/
  saves/
  working/
```

- **The program directory is the one location in code:** the executable's directory, which `executable_directory()`
  finds, or, for the Godot host, whose executable is Godot's, its project's directory.
- **The user directory** is `KML/Stratum` in `%LOCALAPPDATA%` on Windows, `~/Library/Application Support` on macOS, and
  `$XDG_DATA_HOME` or `~/.local/share` elsewhere, unless the config file gives another.
- **The config file is optional.** Without one, these locations are the program's defaults; with one, each value it
  has overrides its default (`## The config file`). In development the executables are in `bin/<Config>/`, so the
  build copies a development `config.json` beside each, whose `assets_directory` is the source tree's `../../assets`,
  and the Godot project has its own `config.json`.

`settings.json` is the user settings file, which has the user's settings: video, audio, controls, and the rest that are
the same in every game (`settings_plan.md`). A game's settings are in its `game.json`. `cache/` has data the program
can make again, which may be deleted, and `mods/` the mods the user adds.

A game in progress is in `working/`. Every save of the game copies its directory from `working/` to a new directory in
`saves/`, so each save is one consistent point.

**The working directory and the saves directory.** The working directory has only the game in progress, in the game
directory, named by the character. Each save of it is a directory in the saves directory,
named by the character's name and the time of the save, `<character name>_<save time>`, the time in ISO 8601 basic
format, which is what a player looks for. An auto save has `_autosave` appended, and a quick save `_quicksave`. A
character of the name that cannot be in a directory name is replaced by `_`.

- **Play reads and writes the game directory.** A zone that unloads is written to it, so a zone file there may be
  newer than the game's last save.
- **Saving writes and then copies.** `game.json`, `character.json`, `world.json` and every loaded zone are written to the
  game directory, which is then copied to a new save directory. An explicit save never replaces an earlier one. Every
  file in a save is therefore from the same moment: the id allocator's count is above every id in its zones, the
  character's inventory and the zones agree, and `world.json` lists every zone file.
- **A save's name is unique.** If a save directory of that name exists, from a save within the same second, the save
  waits for the next second.
- **A game whose character is dead cannot be saved.**
- **Opening copies back.** Opening a save empties the working directory and copies the save into it as the game
  directory, and plays from that. A zone that unloaded after the last save, and was not saved since, is discarded with
  the old game directory.
- **A new game** empties the working directory too, and has a game directory from its creation and a save from its
  first save.

| File | |
| --- | --- |
| `game.json` | The game's own state --- the random engines' state, the seeds, and the next persistent id --- its settings' values, its setting groups' selected presets among them, and its modifiable presets' values |
| `character.json` | The player's character: its description, its skills and other properties, its inventory, and where it is |
| `world.json` | The worldspaces and their regions, and a reference to the file of each of their zones |
| `zones/<persistent id>.zone` | One zone, in the zone file format |

Every id in a saved game is persistent: the id the entity, zone, region or worldspace has in the game, unique across all of its
files. The whole world is in the directory, so every reference in it refers to an entity in it.

`game.json`, `character.json` and `world.json` are each one JSON object with `format`, `version`, and its parts under
their own keys: `game`, `settings` and `presets`, `character`, or `world`.

## An example

A save at 15:30 of a game with a surface worldspace of one zone and a dungeon worldspace of one zone reached from it, and a
character named Ailis in it carrying a torch, would have the following structure:

    Ailis_20260928T153000/
      game.json
      character.json
      world.json
      zones/
        3.zone
        5.zone

(With a zones directory, we can use .json for the zone file extension as well.)

`game.json`, with the random engines' state elided:

```json
{
  "format": "quetzal.stratum_game",
  "version": "0.3",
  "game": { "notation": "block", "next_id": 21, "seed": "20260928", "random_engines": { "gameplay": "..." } },
  "settings": { "gameplay.difficulty": "custom" },
  "presets": { "gameplay.difficulty": { "custom": { "gameplay.difficulty.denizen_count_multiplier": "1", "gameplay.difficulty.denizen_speed_multiplier": "1", "gameplay.difficulty.detection_range_multiplier": "2" } } }
}
```

`character.json`:

```json
{
  "format": "quetzal.stratum_game",
  "version": "0.3",
  "name": "Ailis",
  "character": {
    "id": 1,
    "components": { "name": "Ailis", "speed": 100 },
    "zone": 3,
    "placement": { "cell_position": [3, 1], "orientation": [0, 1] },
    "inventory": [
      { "id": 20, "type": "torch", "components": { "parent": { "entity": 1 }, "site": { "relation": "on" } } }
    ]
  }
}
```

`world.json`:

```json
{
  "format": "quetzal.stratum_game",
  "version": "0.3",
  "world": {
    "name_counter": 0,
    "worldspaces": [
      { "id": 6, "name": "surface",
        "region": { "id": 2, "name": "surface", "name_counter": 0, "components": { "type": "surface" },
          "spec": { "extent": { "arrangement": "ranges", "boxes": [ { "x": 0, "y": 0, "z": 0 } ] }, "zone_size": [40, 30], "service_level": "basic", "type": "surface" },
          "zones": [
            { "id": 5, "name": "surface_0_0", "file": "zones/5.zone", "position": [0, 0, 0] }
          ] } },
      { "id": 7, "name": "cistern",
        "region": { "id": 4, "name": "cistern", "name_counter": 0, "components": { "type": "dungeon", "difficulty": 3 },
          "spec": { "extent": { "arrangement": "ranges", "boxes": [ { "x": 0, "y": 0, "z": [-10, -1] } ] }, "difficulty": "3", "type": "dungeon", "zone.name": "Level {z}" },
          "zones": [
            { "id": 3, "name": "Level -1", "file": "zones/3.zone", "position": [0, 0, -1] }
          ] } }
    ],
    "entity_zones": [
      [1, 3],
      [8, 5],
      [11, 3]
    ]
  }
}
```

`zones/3.zone` is a saved zone, with `"zone": { "id": 3 }`; `discrete_layout.md` (`## An example`) has a whole zone. A
staircase in zone 5, entity 8, and one in zone 3, entity 11, are bound to each other, and are the only connection
between the two worldspaces. A denizen in zone 3 pursues the character, entity 1.

## format and version

A file's header is its leading keys, which a reader can take without parsing the rest: `format` and `version`, and in
the character file `name` after them.

`format` is `"quetzal.stratum_game"`. The version is a label that a reader matches, not a number it compares. It is
`0.3`, and the leading zero marks it as pre-release: no older file is kept readable. A reader given a version it does
not know rejects the file. A zone file has the zone file format's own `format` and `version`.

## game

The state the game needs to continue from where it was saved, which belongs to neither the world nor the character:

| Key | |
| --- | --- |
| `notation` | The game's notation, `block` or `wall`, the notation of every zone; opening the game restores it |
| `next_id` | The persistent id the game's id allocator returns next |
| `seed` | The world seed, which the gameplay random engine is seeded from and every region's and zone's seed derives from |
| `random_engines` | The state of each random engine, by the random engine's stream: `gameplay` and `generation` (`random_engine.md`, `## Streams`) |

A random engine's state is written as the text that the random engine writes and reads (`random_engine.md`, `## Saving`).

## settings and presets

`settings` has the value of each game setting that is set, by its key, as text. A setting group is a setting whose
value is its selected preset's name, so its selection is in `settings` too. A group's member has no value of its own,
so a writer never puts it in `settings`; a member's value that `settings` has is set as setting the member sets it, in
its group's modifiable preset. `presets` has the values of each game setting group's modifiable preset that are defined,
by the group's key and then the preset's name. Either is an empty object when it has none (`settings_plan.md`,
`### Setting groups`).

Opening a game first returns the game settings to their defaults, which selects each group's default preset and
leaves its modifiable preset undefined, then sets the values in `presets`, then those in `settings`. A key that is not a
game setting's or a group's, a value that is not one of its setting's, a value out of its
setting's range, a preset that is not one of its group's or is not modifiable, or a modifiable preset's values that do
not set each of its group's members, is an error.

## character

The character file has `name` third, after `format` and `version`: a copy of the character's `name` component, and
equal to it. A menu that lists saves reads the name from there and stops, without parsing the character, whatever it
carries. The two copies are written together, in one file, so they cannot disagree; a reader rejects a file in which
they do.

The player's character, owned by the game and in no zone's subtree:

| Key | |
| --- | --- |
| `id` | The character's persistent id |
| `components` | Its description, attributes, skills and other properties, as a component object |
| `zone` | The persistent id of the zone it is in |
| `placement` | Its cell position and orientation in that zone |
| `inventory` | The entities it carries, in the form of a zone's `entities` |

The character's intent and pending command are not saved (`session_plan.md`, `### Saving and loading`).

An entity in the inventory is the character's child or the child of another entity in the inventory, and is sited
`on` its parent. A zone's entities cannot refer to the inventory's by its site, since a site's address belongs to its
own zone; they can by reference.

## world

The world's worldspaces, and the world index:

| Key | |
| --- | --- |
| `name_counter` | The last suffix given to a worldspace's name that another worldspace had (`zone_plan.md`, `## Implementation`) |
| `worldspaces` | Every worldspace, with its top-level region |
| `entity_zones` | The world index: for each entity that something refers to, its persistent id and its zone's, as a pair |

The world index is how a reference finds the zone its entity is in when that zone is not loaded, such as a bound
staircase whose partner's zone has unloaded. An entity is in it from the first reference to it, wherever the reference
is, and its zone changes as the entity moves. Each zone in it is one that a region's `zones` refers to.

### Worldspaces

A worldspace is the coordinate system of its zones: the surface, a dungeon, a cave, or a tower.

| Key | |
| --- | --- |
| `id` | The worldspace's persistent id |
| `name` | Its name, unique in the game: the name of the region spec its region was created from, with a suffix if another worldspace had it |
| `region` | Its top-level region |

A worldspace's coordinate system is not written separately: its region's `spec` has it in its `extent` and
`zone_size`, and reading the world sets it from them.

### Regions

A region groups zones of one worldspace: a surface area, a dungeon, a crypt, a tower, or a house.

| Key | |
| --- | --- |
| `id` | The region's persistent id |
| `name` | Its name; a top-level region's is its worldspace's |
| `name_counter` | The last suffix given to a name of its zones that another of its zones had |
| `components` | Its type and, for a dungeon, its difficulty and the other properties a whole dungeon has |
| `spec` | The `RegionSpec` it was created from, in the form of a region spec file's specs, from which its later zones are created (`world_generation_plan.md`) |
| `zones` | A reference to each of its zones |

A region's zones are every zone the world has made in it. Regions refer to no other region; the bound access points in
their zones connect them.

A region's list of zone ids, whose number is the zone count its patterns read (`world_generation_plan.md`), is
not written separately: it is its `zones`, and reading the world sets it from them.

### Zones

A zone reference is the zone's persistent id, its name, its file, relative to the directory of the world file, and its
position in its worldspace if it has one:

```json
{ "id": 3, "name": "Level -1", "file": "zones/3.zone", "position": [0, 0, -1] }
```

A zone's file is a saved zone in the zone file format, with its `zone` section. The zone's region is the region whose
`zones` refers to it, and its worldspace that region's. No two zones of a region have one name, and no two zones of a
worldspace one position. The name and the position are the zone file's `layout.components.name` and
`layout.components.position`, written here so the zone of a name or a position is found without reading its file. Its level is the position's z. Unloading a zone writes its file into the game
directory, which frees its resources, so a zone that is not loaded is in the game only in its file.

## Templates

A template of a zone is a zone file, whose entity ids are local (`discrete_layout.md`, `## zone`) and are given
persistent ids when the template is made into a zone.

## The written form

A writer emits two-space indentation. In the game file, `game`, `settings` and `presets` are each on one line, and so
are `settings` and `presets` in the user settings file. In the character
file, `name` is on the line after `version`, each key of `character` is on a line of its own, and each entity of the inventory is on a line of its own, as a
zone's entities are. In the world file, `name_counter` is on the line before `worldspaces`; a worldspace's `id` and
`name` are on the line that opens it and its `region` follows; a region's `id`, `name`, `name_counter` and `components`
are on the line that opens it, its `spec` is on the next line, its `zones`
follow, and each zone reference is on a line of its own, as each pair of the world index is, in entity id order. A zone file has the
form that `discrete_layout.md` (`### The written form`) specifies. In the config file, each key that is set is on a line
of its own.

## The config file

`config.json`, in the program directory if it is there at all, overrides the program's `Config`: where its files are,
and the most zones the world keeps loaded. It is not a saved game's file, and has a format of its own,
`quetzal.stratum_config`, version `0.1`. The program never writes it. The development config of the terminal is:

```json
{
  "format": "quetzal.stratum_config",
  "version": "0.1",
  "assets_directory": "../../assets"
}
```

| Key | Default | |
| --- | --- | --- |
| `assets_directory` | `assets`, in the program directory | The program's own data |
| `user_directory` | `KML/Stratum` in the platform's directory for each user's application data | Everything the program creates |
| `saves_directory` | `saves` | The directory each save is a directory in |
| `working_directory` | `working` | The directory the game in progress is in |
| `logs_directory` | `logs` | The directory each host writes a log file in for each run |
| `cache_directory` | `cache` | The directory of data the program can make again |
| `mods_directory` | `mods` | The directory of the mods the user adds |
| `user_settings_file` | `settings.json` | The user settings file, with the user's settings (`settings_plan.md`) |
| `zone_count_max` | 0 | The most zones the world keeps loaded at once; 0 for no limit |
| `log_file_count_max` | 10 | The most log files of each host kept in the logs directory, the current run's included; 0 for no limit |

Every key is optional, and a key the format does not have is an error. `assets_directory` and `user_directory` are
relative to the program directory unless absolute, so a `user_directory` there makes a portable install; the other
locations are relative to the user directory unless absolute.

## The game manifest file

A game manifest file defines the game the program plays, and a host is given one as its `manifest=` argument, relative
to the assets directory. It is not a saved game's file, and has a format of its own, `quetzal.stratum_game_manifest`,
version `0.1`. The program never writes it. `assets/data/manifests/` has one for each host and notation; the
terminal's block notation manifest is:

```json
{
  "format": "quetzal.stratum_game_manifest",
  "version": "0.1",
  "notation": "block",
  "views": ["cell", "lattice"],
  "view_menu": true,
  "default_view": "cell",
  "settings": { "world.zone_size.x": "40", "world.zone_size.y": "20" }
}
```

| Key | |
| --- | --- |
| `notation` | The notation a new game is created in, `block` or `wall`, unless a host's menu changes it |
| `views` | The names of the views the host offers, in the order its menu lists them, each a view the host has |
| `view_menu` | True if the host's menu offers the view and the notation, false if it uses the default view and the manifest's notation, as a test does |
| `default_view` | One of the views: the one the menu starts on, or the one used without a menu |
| `settings` | Optional. Game settings by key, each value as text, that a new game uses instead of the settings' defaults |

Every key but `settings` is required, and a key the format does not have is an error, as are a notation that is not
one, an empty list of views, a view listed twice, a default view that is not one of the views, and a setting value that
is not text. Creating a game with a setting that is not a game setting, or a value outside its setting's range, is an
error.

## The user settings file

`settings.json`, in the user directory unless the config file puts it elsewhere, has the user's settings: those that
are the same in every game. It is not a saved game's file, and has a format of its own, `quetzal.stratum_settings`,
version `0.1`. It has `settings` and `presets` in the form `## settings and presets` specifies, for the user
settings and the user setting groups:

```json
{
  "format": "quetzal.stratum_settings",
  "version": "0.1",
  "settings": { "gameplay.reseed_on_open": "true" },
  "presets": { }
}
```

Each key is optional, and a key the format does not have is an error. In `settings` and `presets`, a key that is not a
user setting's or a user setting group's is an error, as is any key, value or preset that would be one in `game.json`.
A host reads the file when it starts, if it exists, and writes it when it closes if a user setting changed, creating
its directory if needed.

## Open

The open items are tracked in `work_plan.md`, under `### Game file`.
