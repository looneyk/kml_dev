# Work Plan

Date: 2026-08-28, revised 2026-10-01

This is the stratum work queue: every item not yet built, and the open questions. An item is referred to by its bold
name. `game_system_principles.md` has
the phases and the principles, `game_system_reference.md` describes what is built, and `modding_plan.md` is the design
goal that every item is held to: everything a game adds is data or script.

The design behind the items is in these documents, which the items refer to by section:

- `addressing.md` specifies ids and addressing, and `addressing_plan.md` is the design of saved zones, the zone actor
  index, and the links and migration they build on.
- `difficulty_plan.md` covers the difficulty modes: the settings each is a preset of, permadeath among them.
- `discrete_layout.md` is the zone file format, and `discrete_layout_plan.md` the structures a zone file loads into.
- `game_file_spec.md` is how a game is saved: a directory with a file for the game's state and settings, one for the
  character, one for the world, and a zone file for each zone.
- `log_plan.md` is the reusable, thread-safe log library in `quetzal/log`, and the process-wide log the game writes to.
- `modding_plan.md` covers mods and data-driven design.
- `placement_plan.md` is the form placement takes: what fills a zone.
- `random_engine.md` covers the random engines, their seeds, and saving their state.
- `region_plan.md` covers regions, relocator targets, and pregenerated content.
- `session_plan.md` covers how `Game`, `Session`, `World` and `Character` divide the work of making and playing zones, and
  the game's lifecycle and the menu operations behind it: `Game` is the game in its entirety, lasting as long as the
  host, and owns the `World`, the `Character` and the `Session` by value, clearing them in place for each game created
  or opened.
- `settings_plan.md` covers the settings: one keyed store or typed structures for each layer, and game and program
  settings.
- `sim_zone_model.md` has the model beneath the code.
- `world_generation_plan.md` covers how a new game's regions and zones are generated: `RegionSpec` and `ZoneSpec`, the
  registries of generators, generator selectors and region specs, and the relocators that lead to new zones and
  regions.

`archived/` has the plans whose work is built.

## Commit points

Each box is one commit point. An item too large for one commit is split into boxes of its own before work on it
starts. When a box's work lands and every suite passes, the box is ticked and that is the moment to commit. The ticked
boxes are removed at the start of the next piece of work, since the reference documents then describe what they built.

## Next

These are the game's lifecycle and the menu operations behind it (`session_plan.md`, `## Games`), built in this order,
each on the ones before it. `Game`'s menu and lifecycle functions, and those they call on `World`, `Character` and
`Session`, are skeletons; each item designs what its functions still need and fills them in.

- [x] **`CharacterSpec`** (`session_plan.md`, `## Character`): the name, the stats, skills and aspects, the inventory as
      a list of `InventoryItem`, and `properties` for the game's other choices.
- [x] **Spec keys:** `find_unknown_keys` moved from `KeyValueStore` into `stratum_sim`'s `specs.hpp`, with
      `select_prefixed`, which returns a spec's entries under a prefix.
- [x] **The game's config.** `Config`, with the saves directory and the zone count maximum, passed to `Game`'s
      constructor, with each host passing a temporary one.
- [x] **A game cleared.** `World::clear`, `Character::clear` and `Session::clear`, with `Clock::clear` and
      `ZoneEntryIndex::clear`; a suite case clears a game played in two regions and checks that only the registrations
      remain.
- [x] **One game after another.** `Character::create` making
      the entity with a new id, and `World::create` making the starting region and its first zone and returning the
      entry point its region spec's `entry_access` states, in place of the constructors of `Character` and `Game`;
      `Game::create(name, ...)` creating the game's directory, resetting the id allocator and reseeding the random engine; and
      `Session::start(access_point)`. The hosts create a game from temporary specs, with `staircase,1` as the entry,
      and load no zone file. A suite case plays a game,
      clears it, creates another, and checks that only the new game's regions, zones, entities and ids remain.
- [x] **The menu in the hosts.** Each host draws a menu, opened by a key, with a new game, continuing, and quitting,
      through `create`, `menu_open`, `menu_close` and `quit`; and `Session::pause` and `resume` stopping and restarting
      play. The menu offers saving and opening with **A game saved** and **A game loaded**.
- [x] **The game's end.** The `Dead` tag, `Session::over`, and `advance` running no actor once the character is dead
      and reporting the death in its events; the hosts showing it until a key is pressed, then calling `quit` and
      returning to the menu.
- [x] **A zone as a document:** `make_document`, the reverse of `load_layout`, with each zone keeping its `ZoneDocument`'s
      geometry and `rebuild_document` a skeleton; an entity in a cell of the zone in the format; `Access`, `Detection`,
      `Speed`, `Pursue`, `Blocking` and `Dead` as file components, with tags written as empty objects; `Pursue::target` a
      reference and `Detection`'s actor states a list.
- [x] **Saved zones** (`addressing_plan.md`, `## Saved zones`; `game_file_spec.md`, `## A saved game's files`):
      `Config::working_directory`, `Game::create` making the game directory in it, `World` given that, and
      `World::save_zone` writing `zones/<persistent id>.zone` there, called when a zone unloads, which then frees its
      resources. Leaving a zone does not save it; every save of the game, explicit, quick or auto, saves each loaded zone.
- [x] **Saved zones read back:** entering a zone that is not loaded reads its file into its region, which lists its zone
      ids in place of its zone count, and a saved zone is not populated again.
- [x] **A game saved** (`game_file_spec.md`), in three commit points:
  - [x] **The game files' documents:** `GameDocument`, `CharacterDocument` and `WorldDocument` in `stratum_format`,
        each with a `write` producing its written form.
  - [x] **Saving:** `Game::save` writing `game.json`, `Character::save` writing `character.json`, and `World::save`
        writing `world.json` and every loaded zone, all into the game directory, which is then copied to a new save
        directory, `<character name>_<save time>`.
  - [x] **The hosts' menu offering it.**
- [x] **A game loaded**, in three commit points:
  - [x] **The game files read:** `read_game`, `read_character` and `read_world` in `stratum_format`, validating each
        file, and the world index in `world.json`, written by `World::save`.
  - [x] **Opening:** `Game::open` emptying the working directory and copying a save into it as the game directory, named
        by the character, then reading `game.json`, restoring the id allocator and, by a `Settings` field, either
        restoring the random engine's state or reseeding it; `World::open` reading `world.json`, each region with its zone ids,
        and the world index; `Character::open` reading `character.json`; and the session starting in the character's
        zone at its placement. `Game::create` loses its name, and names the game directory by the character too.
        `saved_games` lists each save's character name, from its `character.json`, and the time of the save.
  - [x] **The hosts' menu offering it:** the nine newest saves, each shown by its character's name and the time of the
        save, a digit opening one, and Escape going back.
- [x] **A game removed:** `Game::remove` deleting a save's directory from the saves directory, after checking that it
      is a save there; and the hosts' saves page selecting a save with the arrows and deleting it with Delete, once
      confirmed.

## Log

- [x] **The log library** (`log_plan.md`): `library/quetzal/log/`, the target `quetzal_log` in the namespace
      `quetzal::log`, with the reusable, thread-safe `Log` class, its sinks, an optional level for each sink and each
      category, and the process-wide instance, with `log_test`; `quetzal/common`'s `Log.hpp`, `logger.hpp`,
      `logger.cpp` and `LogLevel` removed.
- [x] **The log in use** (`log_plan.md`, `## The game's use`): the terminal installs a file sink writing to a `logs`
      directory in the same place as the saves and working directories; Godot installs a sink writing to Godot's
      output, and a file sink; the tools install a `std::clog` sink and take `--log-level`. dungen and
      `stratum_format::read_zone` write their warnings to the process-wide log in place of taking a warning function,
      and `stratum_gen` and the hosts stop passing one.

## Generators

- [x] **`stratum_gen` independent of `stratum_sim`.** A `stratum_gen` function takes a `ZoneRequest`, the name, the
      level, the seed and the generator's own parameters, and not `Settings` or a `ZoneSpec`. `stratum_gen::generators()`
      returns a map from each generator's name to its function and registers nothing; each host registers them in
      `Generation`, making the `ZoneRequest` from the `ZoneSpec`. `quetzal_stratum_gen` no longer links
      `quetzal_stratum_sim`.
- [x] **The settings process-wide** (`session_plan.md`, `## Settings, the log, and the random engines`): `Settings`
      reached through `settings()` from the sim, the views and the hosts, in place of `Game`'s member and accessors.
- [x] **The config file** (`game_file_spec.md`, `## The config file`): the one location in code, the program's data
      directory, `%LOCALAPPDATA%/KML/Stratum` on Windows, has `config.json`, which gives the saves, working, logs and
      cache directories, the user settings file (`settings.json`), and the zone count maximum, and is written with the defaults if there is
      none. The hosts read it into their `Config` with `load_config(data_directory())`.
- [ ] **Keyed settings** (`settings_plan.md`, `## Terms`): each setting's value kept in one store under its key, such
      as `gameplay.random_seeding_on_open`, read and written with `settings::store().value(declaration)` and
      `set_value`. It replaces a getter and a setter on `Settings` for each of what may become a hundred or more
      settings. In four commit points:
  - [x] **The settings library:** `library/quetzal/settings/`, the target `quetzal_settings` in the namespace
        `quetzal::settings`, with `Setting<T>` for any type that formats and parses through `string_util`,
        `SettingScope` `User` and `Game`, nested sections, types registered by name for declarations in data, and the
        store, a `Settings`, that `settings::store()` returns, with `settings_test`.
  - [ ] **`random_seeding_on_open` on the store:** `RandomSeedingOnOpen` given a `std::formatter` and a `from_chars`;
        `stratum_sim` registers it and declares `gameplay.random_seeding_on_open`, `Game` applies it, and
        `stratum_sim::Settings` and its `settings()` are removed.
  - [ ] **The settings saved:** `settings.json` in its own format, `quetzal.stratum_settings`, read and written by
        `stratum_format` with a `settings` object of each value as its declared JSON type; each host loads it when it
        starts and writes it when a setting changes;
        `Game` writes the game's values into `game.json`'s `settings` and sets them when the game is opened, and
        `Game::clear` returns them to their defaults.
  - [ ] **Settings declared in data,** so a mod can add its own.
  - **One store, below the sim:** a `quetzal/settings` library beside `quetzal/log`, which `stratum_sim::Settings` and
    its `settings()` move into, since a keyed store is not stratum's own.
  - **Each layer declares its own settings, in its own sections:** the sim `gameplay`, the view `video`, and the hosts
    `audio` and `controls`. Each reads only its own, so the sim depends on none of the view's or the hosts', as each
    section of Godot's `ProjectSettings` and Unreal's config is read by the module it belongs to.
  - **Each setting has one declaration,** with its key, type, default and scope: a `Setting<T>` in C++ for a setting
    C++ reads, as an Unreal `TAutoConsoleVariable` or a Source `ConVar` is declared, and an entry in data for one only
    data and scripts read, so a mod can add its own.
  - **Scope:** a game setting, such as the difficulty mode or the notation, is saved in `game.json` and set by `Game`
    when the game is opened. A user setting, such as video, audio or controls, is the player's, the same in every
    game, and saved in the user settings file, where the config file puts it.
  - **Each subsystem applies its settings** into structures of its own, as Unreal's `ApplySettings` does, when it starts,
    when a game is created or opened, and on a change notification for its section; it never reads the store
    otherwise, and never writes its structures back.
- [ ] **The generators integrated.** catagen's generator in `quetzal/stratum_gen` beside dungen's. The game's
      notation, a setting chosen when the game is created, saved in `game.json`'s `settings` and set again by `Game`
      when the game is opened, decides which of them generates every zone: dungen for the
      block notation and catagen for the wall notation, with the zone's one seed. Each reads its parameters, or the
      subset that applies, from the `ZoneRequest`'s `parameters` tokens, rather than running with a default-constructed
      set.
- [ ] **The generators on `geometry::DiscreteDirectedPoint`.** dungen's `Staircase` has a `directed_point_type` and
      still has the separate `position` and `direction` accessors. catagen stores a staircase as a cell type and a map
      from cell position to facing direction, which a list of directed points would replace.
- [ ] **Staircases in every form** (`session_plan.md`, `## Entering an access point`). The generators can place a
      staircase in a flat length of wall or in a 1x1 alcove, the two being equivalent across the notations, as well as
      a block staircase on floor cells in the open space of a room. In the wall notation every staircase is sited on a
      wall.
- [ ] **The common generator selector** (`world_generation_plan.md`, `## Generator selection`): a region's zone count;
      `GeneratorSelection` choosing a generator, a pre-built zone, or a new region; the selector reading its opening
      sequence, primary and rare weighted tables, zone limit, and weighted table of next regions from the `RegionSpec`,
      with the probability of a new region rising to 1 at the zone limit; and the starting
      region's spec, with a field or forest, then a town, then fields, hills, forests and towns. It needs field, forest,
      hills and town generators, and pre-built zones need **The pregenerated content index**.
- [ ] **Pre-built zones selected** (`world_generation_plan.md`, `## Generator selection`): a selection's
      `zone_template` read from **The pregenerated content index** into a `Document`, and the zone built from it as a
      generated zone is.
- [ ] **A second catagen algorithm,** and with it what the two generators have in common becoming `quetzal::procgen`.

## Random engine

- [ ] **The gameplay random engine process-wide** (`random_engine.md`, `## Ownership`): the gameplay random engine
      reached through a function in place of `Game`'s member and `World`'s reference, and thread-safe, locking its
      mutex for each draw; a host or a suite passing the seed, and the host logging it.
- [ ] **The random engine chosen** (`random_engine.md`, `## The random engine type`).
- [ ] **The generation random engine and zone seeds** (`random_engine.md`, `## Streams`, `## Generation`): the
      process-wide, thread-safe generation random engine, and `create_zone(region, zone_spec)` adding the zone's seed,
      drawn from it, to the `ZoneSpec` under `seed_key`, from which the host's `make_zone_request` passes it to the
      generator; the hosts' fallback to `std::random_device` for a `ZoneSpec` without a seed is removed.
- [ ] **Generation runs as objects** (`random_engine.md`, `## Ownership`): dungen's and catagen's runs, each owning its
      random engine, seeded from the zone's seed, with the reference files regenerated. It comes with **the generators
      integrated**.
- [x] **The random engines' state restored** (`random_engine.md`, `## Saving`), with **A game loaded**, or reseeded, as
      `Settings::random_seeding_on_open` chooses.

## Regions

- [ ] **A region's properties** as components: a name and, for a dungeon, the overall or starting difficulty and the
      other properties a whole dungeon has.
- [ ] **Transparent zone and region boundaries.** Contiguous zones joined edge to edge, so an actor walks from one zone
      into the next, across region boundaries too. Until then, zones connect only through relocators: waypoints, and
      dungeons with more than one entrance. Where zones can be traversed across adjacent edges, or through doorway access points,
      the zone on the other side is visible and must be rendered.
      First pass: Zones should be created with natural or constructed barriers that surround them so this problem can be deferred.

- [ ] **Unloading by region.** Criteria for `World::trim_zones` that choose the zones of other regions first, those
      left longest ago first, as `ZoneEntryIndex` does, and then those of the character's region, farthest first.
- [ ] **Region files.**
- [ ] **The pregenerated content index** (`region_plan.md`, `## Pregenerated content`): built at game start by scanning
      `assets/zones`, or read from an index file built in the background, listing each zone or region tag, with
      `layout.components.name` as a zone's tag, and its zone data file.
- [ ] **Relocator names:** a `name` in the `relocator` component, read, written, and listed in the index.
- [ ] **Named relocator targets resolved** through the index: a named zone, a named region, and a position.

## Attachments

- [ ] **A destroyed door transformed or unlinked, not deleted** (`archived/attachment_plan.md`, `## Lifetime`).
      Deleting the assembly would seal the doorway, since a wall with nothing on it is solid.
- [ ] **Crossing that depends on the actor, kept affordable** (`archived/attachment_plan.md`, `## The seam`): only
      attachments marked overridable consult the actor's properties, so the common case never looks.
- [ ] **A key that opens a lock.** `Access` has `Locked`, and nothing unlocks it.

## Views

- [ ] **Floor access points drawn by their orientation.** The views draw one with the stair glyph and ignore its
      orientation.

## Modding

- [ ] **Place types from data** (`modding_plan.md`, `## What blocks it now`). A reduction maps an entity's type to a
      place type that the presentation table names, instead of to an enum value, so a modded type has a glyph of its
      own.

These are Phase 4 (`modding_plan.md`):

- [ ] **Types as prefabs from data**, doors among them, with inheritance and patches.
- [ ] **Component schemas from data**, as flecs runtime components.
- [ ] **Styles:** the asset, the parameter map, inheritance by name, and a view that resolves them.
- [ ] **The scripting language chosen and embedded,** with hooks on types and bindings through reflection.
- [ ] **Mods:** the directory, load order, patches, and skipping what a missing mod declares.

## Placement

These are blocked: where placement lives is unsettled, which determines what it may depend on, and weighted selection
needs `content`. The population strategy comes with them. `placement_plan.md` (`## Order`) has the design.

- [ ] **The placement interface,** with a zone's doors built from its file through it.
- [ ] **Weighted selection over a table.**
- [ ] **The placement distributions:** a count, one per room, and a density per area.
- [ ] **Placement's first use that is not a door,** items or denizens.

## Lifecycle

Saving, and the difficulty modes that determine how a game may be saved and what ends it. The difficulty modes wait on
research into what each mode sets; `difficulty_plan.md` has the draft.

- [ ] **Quick saves and autosaves** (`session_plan.md`, `### Quick saves and autosaves`): `read_key` reporting
      function keys, F5 quick saving and a quickload key in both hosts, and an autosave on entering a zone, through a
      function that `Game` gives `Session`.
- [ ] **Difficulty modes** (`difficulty_plan.md`): Story, Novice, Adventure, Challenging and Hardcore, and Custom, as
      presets of individual settings in data, chosen when a game is created and saved with it, and changeable during
      play.
- [ ] **Permadeath,** Hardcore's death and save settings (`session_plan.md`, `### Permadeath`): one save, written on
      quit, and `quit` deleting it once the character is dead.

## Phase order

A minimal first-person 3D view comes just after Phase 3, ahead of content and procgen. It is the second genuinely
different view, and it validates the wall model before procgen generates against it. `game_system_principles.md`
places it at Phase 3b, with the rest of the Godot 3D view in Phase 6.

## Against the continuous layout

Not gating anything before Phase 7.

**`value_type`, `float` or `double`.** Float resolves to roughly a millimetre at ten thousand units,
which no zone comes near, and `Layout::move` regenerates position from the cell it reached rather
than accumulating, so a discrete layout does not drift however often it is called --- real time
included. What argues for double is neither of those: every vertex today is grid-derived and
therefore exactly representable, which arbitrary angled walls end, turning every tolerance the
geometry tests need into a tuning exercise. One line in `types.hpp`, the helpers
being templated on the value type and the tolerance retightening on its epsilon. The renderer using
float is not an argument against it: the view narrows at the boundary, where magnitudes are small.

## Open

These are the open questions.

### Attachments and doors

- **The result of smashing a door is not the door's to decide.** Transform is the mechanism, but the outcome depends on
  what hit the door and what it is made of: an axe and a charge differ as much on one door as an oak door and a
  portcullis differ under one axe. `sim_zone_model.md` settles that a script determines it, over an engine default, so
  the structure only has to let the script reach both the door's material and the damage.
- **Where damage accrues.** An axe is applied to a door facing, and a blast is centered in space, reaching many walls
  and everything on them. That argues that damage accrues per door facing --- the side that was chopped is the side
  that shows it --- and that the shared state changes only when the door as a whole gives way.
- **Whether a wall side without a door is worth an entity.** A wall's surface treatment is per side. If every wall side
  becomes an entity, the count argument that kept walls out of the entity store applies again, so surface treatment
  probably stays in layout data and only sides with something on them become entities.
- **Where a trap belongs:** on the door, or per door facing, since the side it is triggered from and the side it fires
  into need not be the same.
- **Whether concealment blocks sight as well as movement.** A secret door is drawn as wall, and `visible` binds the same
  `crossable` as `move`, so a closed or concealed way through stops a sight line. Whether a sight line should follow a
  predicate of its own, with a window open to sight and closed to movement, is open.
- **A way through's visual state,** such as off its hinges or a pile of rubble, is set by script and interpreted by the
  view. The component that has it is not designed.
- **What a swinging door needs, if anything.** A door facing has no direction of its own, since the wall determines its
  orientation, so the space a door opens into is either not modeled or one more field on the door.
- **Whether a door can shut behind an actor.** With one door on a doorway block, an actor inside the block is never
  between two doors. A trap that shuts an actor in needs a door that closes while occupied, and a block whose sides
  stop being free to pass. This is far future, and nothing depends on it.
- **How an actor selects the adjacent door it means.** In a corridor past an open door, a move turns toward the way it
  goes, so turning toward the door is moving into the doorway, and from inside a block there is nothing to shut.
  Acting on any adjacent door fails in a corridor with an open door on each side, since both are adjacent. The
  interaction needs a way to select a side that is neither the facing direction nor adjacency. The problem follows
  from the terminal host having no turn of its own, and the door is where it shows.

### Ids and addressing

- **Names.** `PersistentId`, `EntityReference`, `make_reference`, `AttachedEntities`, `AddressIndex`, `Access`,
  `Detection` and `InterzonePursuitCapability` are working names.

### Modding

- **The scripting language.** Lua is what the genre uses; Luau adds sandboxing and gradual typing. It is chosen before
  Phase 4.
- **Load order and patch rules:** how mods are ordered, what a patch may add, replace or remove, and how two mods that
  patch one type combine.
- **What the sandbox exposes** to a mod script: which world queries and which assets, and nothing of the file system.
- **How a style names its meshes,** and which surfaces a kit piece may replace: a wall side, a flat cell, or a corner.
- **Which components inherit** along the chain, and therefore which have fields that may be left unset.
- **How a place type is defined in data** --- as a name the presentation table maps, or as a field on the type --- and
  how `cells` and `lattice` reduce an entity of a modded type.

### Sites

- **The word that replaces `site` where it means a candidate location.** `stratum_sim`'s `Site` takes the word for
  where an entity is. Two places use it for a location that a generator or a placement pass chooses among, and one term
  replaces both, since the sense is the same: `placement_plan.md`, where the sites are one of the five roles of a run,
  and `catagen`'s `site_type`, `sites_type` and `door_sites`.

### Placement

- **Where it lives.** `procgen` is a slot rather than a library, and placement may not belong beside the zone generators
  at all, since its dependencies are tables and prototypes, not grids.
- **What the roles are called.** `distribution` in particular collides with `std::uniform_int_distribution` and the
  rest, in code that uses them.
- **Whether a site is a value or an id.** A wall is an `id_type`, a cell a cell position, and a room a record.
  Templating over the site type handles all three; whether a run needs them mixed is not known.
- **Where the boundary with `content` falls.** Weighted draws over prototypes may belong to `content`, leaving
  placement only the spatial half.

### Layout

- **Whether a continuous layout has `Flat` at all.** Its zone is ordinary 3D models, where a
  floor is geometry like any other rather than a bounding surface with a role. `Flat` may be a
  discrete convenience that does not cross, or the record both use with the sampled fields filled
  in. Nothing forces the choice until the skeleton has something behind it.
- **`cardinal_directions`, `discrete_point_type`, `grid_step` and `discrete_vector_type` are in `types.hpp`,** which
  every layout includes, so the discrete vocabulary is in the neutral header. Moving it under `DiscreteLayout.hpp`
  touches every file in the library, and is worth doing alongside other work.
- **Whether rooms rise to `Layout`.** A room is `room_id` on a `DiscreteLayout`'s cells and an entity with its
  components. The entity is neutral and the cells are not, so rooms stay on `DiscreteLayout` until something
  continuous needs one.
- **`DiscreteLayout::field_of_view` is declared and not defined.** A shadowcasting sweep computes every cell around one
  origin at once, where `visible` walks the ray between one pair, and `sim_zone_model.md` notes that the two give
  different results.

### Discrete layout format

- **Perimeter validation passes some invalid perimeters.** Each cell edge is checked on its own, and a wall toward a
  floor/door cell on either side satisfies it. A perimeter whose walls all face outward, around a pillar or solid
  ground, passes while enclosing no floor/door cell. The reader already determines which side of each edge is inside;
  a check of the enclosed area is needed to reject these.
- **Where access point types are registered from data,** including a mod's, instead of in `World`'s constructor
  (`modding_plan.md`).
- **The scale of the probability of a staircase leading down,** which is inversely proportional to how far below the
  surface the zone's level is and proportional to `difficulty`, and is 1 for now; and what bounds it, since a region
  has no lowest level.
- **Whether the layout interface returns a wall id.** An address encodes into one, so it can. Whether a handle to a
  wall belongs on an interface that mentions neither cells nor walls is an older question.

### Game file

- **The format's identifier,** `quetzal.stratum_game`, and the file names, `game.json` and the rest, for now.
- **The character's components:** its description, attributes, and skills.
- **What triggers an autosave.** Entering any zone, for now. Entering a town may be the trigger as well or instead,
  which depends on the generator selector's town zones (`## Generators`).
- **The quickload key,** F8 or F9, and which save it opens.

### Difficulty

- **What each mode sets,** and the values: the draft table in `difficulty_plan.md` (`## The modes`) is a placeholder.
- **What death does in the easy modes:** revival at the zone's entry point in Story, and reloading from Novice up, or
  something else.
- **How luck is applied** as a hidden stat of the character: what it changes, and by how much.

### Intent

- **Does intent cover actions or only movement?** Held attacks and channeled spells would need more than movement and
  facing direction, which is a design question.
- **Do actions get durations** --- wind-up, active and recovery? If so, `Intent` is where they go.

### Regions

- **The properties each type of region has.** A surface area's and a dungeon's overlap but differ, and difficulty goes
  from the starting value across a dungeon's zones in a way not yet decided.
- **Distance or level difference** as the measure within the character's region, and whether it differs by type of
  region.
- **The contents of a region file:** only the region's metadata and generation settings, or those followed by its
  zones; and how it relates to the zone file format.
- **How two bound doors keep their `Access` equal** (`region_plan.md`, `## Doors between zones`). With both zones
  loaded, changing one door sets its partner too. With the partner's zone unloaded, only this side changes, and the
  two are reconciled when an actor crosses or when the partner's zone loads; which change is newer needs deciding,
  such as by the simulation time of the last change stored with `Access`.
- **How a relocator between surface zones states where it leads.** It leads neither up nor down, so a level offset of 0
  may be enough, or it may need a horizontal distance or direction instead.
- **What the pregenerated content index file contains** (`region_plan.md`, `## Pregenerated content`).

### Generation

- **A second entrance into a region that exists.** A target with a region always creates a new region, and an existing
  region is reached only through relocators bound to it. A dungeon with two entrances in different surface zones needs
  the second entrance to refer to the region the first created, such as by persistent id once the region exists. A
  region nearing its zone limit has the same need: each exit that leads out of it creates a new region, where they could
  lead into a region that already follows it, which the selector would then refer to.
- **How the probability of a new region rises** with a region's zone count, from 0 to 1 at its limit: the curve, where
  it starts, and whether it is a parameter of the `RegionSpec`.
- **The region in a relocator target:** the name of a region spec, from which a new region is created, or a tag in the
  pregenerated content index (`region_plan.md`, `## Pregenerated content`); how the two are distinguished.
- **Which `RegionSpec` metadata becomes components** on the region, and which only the selector reads.

### Log

- **Whether entries are written asynchronously,** from a queue by a worker thread as spdlog's asynchronous mode does, or
  on the thread that logs.
- **The log file's name and how many are kept:** a file for each run, named by the program and its start time, with
  the oldest removed past a count, as Godot keeps them, or one file appended to.
- **Whether entries written before the first sink is installed are kept** and written to it, rather than dropped.

### Random engine

- **Which random engine** (`random_engine.md`, `## The random engine type`): `std::mt19937_64`, xoshiro256**, PCG, or jsf64.
  xoshiro256** is recommended.
- **Whether players get reproducible games from a shared seed.** It needs each zone's seed derived from the world seed
  and the zone's place instead of drawn, so that a world seed builds the same world however it is explored.
- **Whether checked-in reference files must match across standard libraries.** The standard distributions are
  implementation-defined, so matching needs integer and real distributions of the project's own.

### Session

- **The population strategy from the zone's source,** the template loaded or the generation guidance, which replaces
  `World::populate`'s placeholder (`session_plan.md`, `## World`).
- **The time a zone was entered:** simulation time or real time.
- **The saves directory's default on Windows:** `%USERPROFILE%\Saved Games\KML\Stratum`, the known folder
  `FOLDERID_SavedGames`, which Microsoft's GDK guidance recommends for a game that manages its own saves, and which
  Cyberpunk 2077 and The Outer Worlds 2 use, in place of the data directory's `saves`.
- **Whether the driver belongs to the game or to a zone.**
- **Actors.** The character and the denizens are both actors, and differ in what controls them; the actor structure is
  not designed, and neither is what kills an actor and marks it `Dead`.
- **Resident zones the character has left:** whether they are simulated or frozen, which thresholds apply --- memory
  usage among them --- and their values.
- **Zones made and never entered.** They are not candidates for unloading.
- **Where the unload order's data is kept** --- `ZoneEntryIndex`'s map from entry count to zone id, or a component on
  each zone with a flecs query ordered by it --- once regions set the criteria: other regions' zones by the time they
  were entered, and the character's region's zones by distance or level difference (`## Regions`).

### Settings

- **The declaration format:** the file or files the settings are declared in, what a declaration has --- type, default,
  scope, range or allowed values, display name, and whether a restart is required --- and how a mod adds a setting or
  changes another's default.
- **When a setting may change:** a user setting while a game is open, and a game setting during play, as the
  difficulty mode may (`difficulty_plan.md`). A game setting applies between advances, never partway through a turn.
- **The thread a change notification is called on** once a thread other than the host's main thread can change a
  setting. It is now called on the thread that changed it.
- **Whether a new game's settings start from the player's,** such as the difficulty mode last chosen, or from the
  declared defaults.

### View

- **How a glyph set or tile set marks `concealed`,** a way through that the viewer has found and actors in general
  have not, whether outlined or otherwise; the built-in sets draw it like any door.
- **Whether the Godot tile view draws the lattice.** A wall between two open cells is as invisible in the tile view,
  which draws `cells`, as in the terminal's cell view.
- **Whether `cells` needs state.** Its consumers draw one glyph per cell and have no edges, so it may need only the
  type, though having the two reductions produce different forms has its own cost.
- **What a tile set produces,** which decides whether the glyph table is templated on what it maps to, or characters
  and tile indices are two tables.
- **Loading a glyph set from a file in a host.** The hosts use the built-in sets; a file is how a set that is not built
  in arrives.
- **What a corner is made into, per view.** The reduction marks a joint there; the terminal draws a box-drawing
  character and a 3D view a column. Neither is decided, and the 3D view is why the corner is in the reduction.

## Revisit later

These are set aside on purpose.

- **Pursuers following the character through a relocator** (`session_plan.md`, `### Following through a relocator`).
  Only the controlled actor enters a relocator.
- **A doorway into a dead end,** which has a wall on one side only: what a door glyph there requires, and what the
  generators and the reader do with one.
- **maze1's unreachable corridor.** Excavation can leave corridor that no door reaches, which dead-end pruning removes
  only by chance, and a staircase placed at its end stops the pruning, so the staircase is unreachable too (`up_only`
  seeds 1 and 40 in `dungen_test`). The suite's check that every floor cell is reachable skips maze1 until this is
  done.
- **Naming a wall address's `normal` `facing`,** once the format settles, to match door facings; the value also being
  the surface normal a texture uses argues against it.
- **`passable_at` and `block_allowed` in the generators.** The sim uses `crossable` for getting through a wall and
  `blocking` for something in the way; dungen and catagen use `passable_at` for a cell an actor can be in and
  `block_allowed` for where a staircase may go. Both are cell questions rather than edge ones, so it is not the same
  rename, but the word is the one the sim dropped.

## Loose

These are not ordered, but worth a pass: `Scheduler::advance` integer overflow, and its `operator[]` on `m_speeds`.
