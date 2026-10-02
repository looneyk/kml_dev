# Game File Format Specification

Status: draft --- nothing reads or writes it yet
Date: 2026-09-28, revised 2026-10-01

This document specifies how a game is saved. A game has four parts: the game's own state, its settings, the player's
character, and the world, which is a set of regions, each containing zones. A saved game is a
directory with one file for the game's state and settings, one for the character, one for the world, and one for each
zone. The world file organizes the regions and refers to the zone files. A zone file is the file that `discrete_layout.md` specifies. `session_plan.md` (`## Games`) describes
the `Game` that writes and reads these files, and `sim_zone_model.md` the model that the world serializes.

## A saved game's files

The directory structure is KML/Stratum (probably qualified by the view type) and lives in the AppData or temp directories.
Subdirectories config/ and saves/ are in the AppData/Roaming path and cache/ is in the AppData/Local path.
A game in progress is in working/, which is always in temp. Every save of the game, explicit, quick or auto, copies its
directory from working/ to saves/, so a saved game is one consistent point.
While development is in its early stages as it is now, all four directories will live in the temp directory, same directory structure.
The path structure, production and development respectively:

AppData/
  Roaming/
    KML/
      Stratum/
        config/
        saves/
  Local/
    KML/
      Stratum/
        cache/
temp/
  KML/
    Stratum/
      working/

temp/
  KML/
    Stratum/
      config/
      saves/
      cache/
      working/

**The working directory and the saves directory.** A game's directory is in both, under the same name. The game
directory is the one in the working directory.

- **Play reads and writes the game directory.** A zone that unloads is written to it, so a zone file there may be
  newer than the game's last save.
- **Saving writes and then copies.** `game.json`, `character.json`, `world.json` and every loaded zone are written to the
  game directory, which is then copied to the saves directory, replacing the game's directory there. Every file in a
  saved game is therefore from the same moment: the id allocator's count covers every id in its zones, the character's
  inventory and the zones agree, and `world.json` lists every zone file.
- **Opening copies back.** Opening a game replaces its game directory with a copy of its saved directory and plays
  from that. A zone that unloaded after the last save, and was not saved since, is discarded with the old game
  directory.
- **A new game** has a game directory from its creation and a saved directory from its first save.

| File | |
| --- | --- |
| `game.json` | The game's own state --- the engines' state, the seeds, and the next persistent id --- and its settings |
| `character.json` | The player's character: its description, its skills and other properties, its inventory, and where it is |
| `world.json` | The regions, and a reference to the file of each of their zones |
| `zones/<persistent id>.zone` | One zone, in the zone file format |

Every id in a saved game is persistent: the id the entity, zone or region has in the game, unique across all of its
files. The whole world is in the directory, so every reference in it refers to an entity in it.

`game.json`, `character.json` and `world.json` are each one JSON object with `format`, `version`, and its parts under
their own keys: `game` and `settings`, `character`, or `world`.

## An example

A specific game in the saves directory with a surface region of one zone and a dungeon region of one zone reached from it, and a character in it
carrying a torch would have the following structure:

    20260928T141500/
      game.json
      character.json
      world.json
      zones/
        3.zone
        5.zone

(With a zones directory, we can use .json for the zone file extension as well.)

`game.json`, with the engines' state elided:

```json
{
  "format": "quetzal.stratum_game",
  "version": "0.1",
  "game": { "next_id": 21, "seed": "20260928", "engines": { "gameplay": "...", "generation": "..." } },
  "settings": { "permadeath": false }
}
```

`character.json`:

```json
{
  "format": "quetzal.stratum_game",
  "version": "0.1",
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
  "version": "0.1",
  "world": {
    "regions": [
      { "id": 2, "components": { "name": "The Barrow Downs", "type": "surface" },
        "spec": { "type": "surface", "generator_selector": "fields_and_hills", "service_level": "basic" },
        "zones": [
          { "id": 5, "file": "zones/5.zone" }
        ] },
      { "id": 4, "components": { "name": "The Flooded Cistern", "type": "dungeon", "difficulty": 3 },
        "spec": { "type": "dungeon", "generator_selector": "dungeon", "difficulty": "3", "extent": "40,30" },
        "zones": [
          { "id": 3, "file": "zones/3.zone" }
        ] }
    ]
  }
}
```

`zones/3.zone` is a saved zone, with `"zone": { "id": 3 }`; `discrete_layout.md` (`## An example`) has a whole zone. A
staircase in zone 5 and one in zone 3 are bound to each other, and are the only connection between the two regions.

## format and version

`format` is `"quetzal.stratum_game"`. The version is a label that a reader matches, not a number it compares. It is
`0.1`, and the leading zero marks it as pre-release: no older file is kept readable. A reader given a version it does
not know rejects the file. A zone file has the zone file format's own `format` and `version`.

## game

The state the game needs to continue from where it was saved, which belongs to neither the world nor the character:

| Key | |
| --- | --- |
| `next_id` | The persistent id the game's id allocator returns next |
| `seed` | The world seed the engines were seeded from |
| `engines` | The state of each engine, by the engine's stream: `gameplay` and `generation` (`random_engine.md`, `## Streams`) |

An engine's state is written as the text that the engine writes and reads (`random_engine.md`, `## Saving`).

## settings

The game's settings, chosen when the game begins, as a component object:

| Key | |
| --- | --- |
| `permadeath` | True if the game ends for good when the character dies |

## character

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

The world's regions:

| Key | |
| --- | --- |
| `regions` | Every region, in no hierarchy |

### Regions

A region groups contiguous zones: a surface area, a dungeon, a crypt, a tower, or a house (`region_plan.md`).

| Key | |
| --- | --- |
| `id` | The region's persistent id |
| `components` | Its name, its type, and, for a dungeon, its difficulty and the other properties a whole dungeon has |
| `spec` | The `RegionSpec` it was created from, from which its later zones are created (`world_generation_plan.md`) |
| `zones` | A reference to each of its zones |

A region's zones are every zone the world has made in it. Regions refer to no other region; the bound relocators in
their zones connect them.

A region's list of zone ids, whose number is the zone count its generator selector reads (`world_generation_plan.md`), is
not written separately: it is its `zones`, and reading the world sets it from them.

### Zones

A zone reference is the zone's persistent id and its file, relative to the directory of the world file:

```json
{ "id": 3, "file": "zones/3.zone" }
```

A zone's file is a saved zone in the zone file format, with its `zone` section. The zone's region is the region whose
`zones` refers to it, and its level is its `layout.components.level`. Unloading a zone writes its file into the game
directory, which frees its resources, so a zone that is not loaded is in the game only as its file has it.

## Templates

A template of a region is a world file with the parts specific to a game omitted: each region's `id`, and each zone
reference's `id`. Its zone references refer to zone templates, whose entity ids are local (`discrete_layout.md`,
`## zone`) and are given persistent ids when the template is made into a zone. A template of a zone is a zone file.

## The written form

A writer emits two-space indentation. In the world file, a region's `id` and `components` are on the line that opens
it, its `spec` is on the next line, its `zones` follow, and each zone reference is on a line of its own. A zone file has the
form that `discrete_layout.md` (`### The written form`) specifies.

## Open

The open items are tracked in `work_plan.md`, under `### Game file`.
