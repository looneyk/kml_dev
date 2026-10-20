# Difficulty --- Plan

Date: 2026-10-04, revised 2026-10-07

This plan describes the game's difficulty modes: what each sets, and how a mode relates to the game's settings and to a
region's difficulty. It is a first draft, and the values in it are placeholders until the research behind them is
done. The open items are in `work_plan.md` (`### Difficulty`).

## What a mode is

- **A difficulty mode is a named preset of individual settings,** chosen when a game is created and saved in
  `game.json`'s `settings` (`game_file_spec.md`, `## settings`). Permadeath, the save rules, and each multiplier are a
  setting of their own, and a mode fills them in. Permadeath is not a mode; it is a setting that the hardest mode has.
- **The modes are Story, Novice, Adventure, Challenging and Hardcore,** from easiest to hardest, with Adventure the
  normal game.
- **Custom sets each setting,** starting from any of the five.
- **The modes are data,** the game's own table of them in `assets/data`, which a mod can extend or rebalance, and not
  an enum (`modding_plan.md`).
- **A game's mode can be changed during play,** in either direction, with no restriction.
- **A mode is not a region's difficulty.** A region's `difficulty` grows with the depth and progress of the game
  (`world_generation_plan.md`). A mode scales the whole game: generation reads the region's difficulty, and the mode's
  settings scale what it generates.
- **Luck is a hidden stat of the character,** which the mode sets and the player does not see. It affects drops,
  critical hits, and rare finds.

## The modes

| | Story | Novice | Adventure | Challenging | Hardcore |
| --- | --- | --- | --- | --- | --- |
| **On death** | Revived at the zone's entry point, with no loss | Reload any save | Reload any save | Reload any save | Permadeath: the save is deleted |
| **Saving** | Any time; autosave and quick save | Any time; autosave and quick save | Any time; autosave and quick save | Explicit save and autosave; no quick save or quickload | One save, written on quit; no explicit save, autosave, or quick save |
| **Opening a save** | Reseeds the random engines | Reseeds the random engines | Reseeds the random engines | Restores the random engines | Restores the random engines |
| **Damage taken** | x0.5 | x0.75 | x1 | x1.25 | x1.5 |
| **Denizen level** | -2 | -1 | 0 | +1 | +2 |
| **Denizen count** | x0.5 | x0.75 | x1 | x1.25 | x1.5 |
| **Luck** | High | Raised | Neutral | Lowered | Low |
| **Pursuit:** how long a pursuer hunts after losing sight | Short | Short | Normal | Long | Long, and pursuers follow through relocators |
| **Denizen tactics:** groups, flanking, items | None | Basic | Normal | Full | Full, with reinforcements |
| **Aids:** concealed doors hinted, items identified | All | Most | Some | None | None |
| **Rest and healing** | Free | Easy | Normal | Limited | Scarce |

Hardcore is the pure roguelike game: one life, and one save that exists only to continue a game after quitting it.

## The sample modes

Until the research behind the five modes is done, the game has three sample modes and Custom, enough to build the
mechanism on. Each sets the settings that the sim has something to act on now: `World::populate` places the denizens,
and `Speed` and `Detection` are components it gives them. Damage, luck and the rest wait for what they act on.

| Key | Type | `easy` | `normal` | `hard` |
| --- | --- | --- | --- | --- |
| `gameplay.difficulty.denizen_count_multiplier` | real | 0.5 | 1 | 1.5 |
| `gameplay.difficulty.denizen_speed_multiplier` | real | 0.75 | 1 | 1.25 |
| `gameplay.difficulty.detection_range_multiplier` | real | 0.75 | 1 | 1.25 |

- **Every difficulty setting is a game setting,** in the section `gameplay.difficulty`, saved in `game.json`.
- **A new game is Normal by default.**
- **The mode is the setting group `gameplay.difficulty`,** and each mode is one of its presets (`settings_plan.md`,
  `### Setting groups`). Selecting a mode sets the group's value, and the multipliers are then the mode's.
- **Normal is the default preset,** the one a new game has. Each multiplier's own default is what resetting it in
  Custom's editor sets.
- **Custom is an editable preset.** Its values are undefined in a new game; the first time it is selected, they are
  the mode selected before it, and from then on they are the game's own, kept when another mode is selected and
  restored when Custom is selected again. The menu opens an editor of the multipliers while Custom is selected, and
  edits a multiplier only there. A script or the console setting one under another mode switches the mode to Custom
  with that mode's values, replacing Custom's own, and then sets the multiplier. A game built on the same framework that
  does not allow customization defines no editable preset.
- **The menu may show each preset's values** beside it, from the preset itself.

## The modes' presets

The game's setting definitions in `assets/data` define the group, its members and its presets:

```json
"groups": [
  {
    "key": "gameplay.difficulty", "scope": "game", "default_preset": "normal",
    "members": ["gameplay.difficulty.denizen_count_multiplier", "gameplay.difficulty.denizen_speed_multiplier", "gameplay.difficulty.detection_range_multiplier"],
    "presets": [
      { "name": "easy", "display_name": "Easy", "values": { "gameplay.difficulty.denizen_count_multiplier": "0.5", "gameplay.difficulty.denizen_speed_multiplier": "0.75", "gameplay.difficulty.detection_range_multiplier": "0.75" } },
      { "name": "normal", "display_name": "Normal", "values": { "gameplay.difficulty.denizen_count_multiplier": "1", "gameplay.difficulty.denizen_speed_multiplier": "1", "gameplay.difficulty.detection_range_multiplier": "1" } },
      { "name": "hard", "display_name": "Hard", "values": { "gameplay.difficulty.denizen_count_multiplier": "1.5", "gameplay.difficulty.denizen_speed_multiplier": "1.25", "gameplay.difficulty.detection_range_multiplier": "1.25" } },
      { "name": "custom", "display_name": "Custom", "editable": true }
    ]
  }
]
```

- **The sim applies the multipliers into a `Difficulty` struct,** which `World` keeps, applied by a change listener
  when any of the group's members changes. `populate`, `Speed` and `Detection` read the struct, never the store.

## What the settings replace

- **`gameplay.reseed_on_open`** becomes the mode's setting for opening a save.
- **Permadeath** (`session_plan.md`, `### Permadeath`) becomes Hardcore's death and save settings: `quit` saves the
  game while the character lives, and deletes the save once it is dead.
- **The quick save and autosave counts** (`session_plan.md`, `### Quick saves and autosaves`), and whether quick saves,
  quickloads and autosaves exist at all, are the mode's save settings.
- **`InterzonePursuitCapability`** (`session_plan.md`, `### Following through a relocator`) is given to pursuers by
  the mode's pursuit setting, as well as by a denizen's type or by script.

## Open

The open items are tracked in `work_plan.md`, under `### Difficulty`.
