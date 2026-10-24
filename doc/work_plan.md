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
- `testing_plan.md` covers how the tests are written, registered and run: doctest, CTest, and reference files that one
  option updates.
- `world_generation_plan.md` covers how a new game's regions and zones are generated: `RegionSpec` and `ZoneSpec`, the
  registries of generators, generator selectors and region specs, and the relocators that lead to new zones and
  regions.

`archived/` has the plans whose work is built.

## Commit points

Each box is one commit point. An item too large for one commit is split into boxes of its own before work on it
starts. When a box's work is done and every test passes, the box is ticked and that is the moment to commit. The ticked
boxes are removed at the start of the next piece of work, since the reference documents then describe what they built.

## Generators

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

- [ ] **Generation runs as objects** (`random_engine.md`, `## Ownership`): dungen's and catagen's runs, each owning its
      random engine, seeded from the zone's seed, with the reference files regenerated. It comes with **the generators
      integrated**.

## Regions

- [ ] **A region's properties** as components: a name and, for a dungeon, the overall or starting difficulty and the
      other properties a whole dungeon has.
- [ ] **Transparent zone and region boundaries.** Contiguous zones joined edge to edge, so an actor walks from one zone
      into the next, across region boundaries too. Until then, zones connect only through relocators: waypoints, and
      dungeons with more than one entrance. Where zones can be traversed across adjacent edges, or through doorway access points,
      the zone on the other side is visible and must be rendered.
      First pass: Zones should be created with natural or constructed barriers that surround them so this problem can be deferred.

- [ ] **Unloading by region.** Criteria for `World::trim_zones` that choose the zones of other regions first, those
      left longest ago first, and then those of the character's region, farthest first.
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

- [ ] **Place types from data** (`modding_plan.md`, `## Current blockers`). A reduction maps an entity's type to a
      place type that the presentation table names, instead of to an enum value, so a modded type has a glyph of its
      own.

These are Phase 4 (`modding_plan.md`):

- [ ] **Types as prefabs from data**, doors among them, with inheritance and patches.
- [ ] **Component schemas from data**, as flecs runtime components.
- [ ] **Styles:** the asset, the parameter map, inheritance by name, and a view that resolves them.
- [ ] **The scripting language chosen and embedded,** with hooks on types and bindings through reflection.
- [ ] **Mods** (`modding_plan.md`): the directory, the manifest, the mod list, load order, patches, and a missing mod:
      skipping what it declares, telling deliberate removal from damage, and the user's choice when a game used it.
- [ ] **Mods' settings** (`settings_plan.md`, `### Mods' settings`), after **Mods**: each mod's definitions read after
      the game's, with the loader checking the `mod.<id>` section of the keys a mod defines; patches to another's
      settings by full key, skipped with a warning for a mod that is not loaded; a missing mod's saved values kept by
      the loader and written back.

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
- **Where does damage accrue?** An axe is applied to a door facing, and a blast is centered in space, reaching many
  walls and everything on them. That argues that damage accrues per door facing --- the side that was chopped is the
  side that shows it --- and that the shared state changes only when the door as a whole gives way.
- **Does a wall side without a door merit an entity?** A wall's surface treatment is per side. If every wall side
  becomes an entity, the count argument that kept walls out of the entity store applies again, so surface treatment
  probably stays in layout data and only sides with something on them become entities.
- **Does a trap belong on the door, or on each door facing?** The side it is triggered from and the side it fires into
  need not be the same.
- **Does concealment block sight as well as movement?** A secret door is drawn as wall, and `visible` binds the same
  `crossable` as `move`, so a closed or concealed way through stops a sight line. A sight line could instead follow a
  predicate of its own, with a window open to sight and closed to movement.
- **A way through's visual state,** such as off its hinges or a pile of rubble, is set by script and interpreted by the
  view. The component that has it is not designed.
- **What, if anything, does a swinging door need?** A door facing has no direction of its own, since the wall
  determines its orientation, so the space a door opens into is either not modeled or one more field on the door.
- **Can a door shut behind an actor?** With one door on a doorway block, an actor inside the block is never between two
  doors. A trap that shuts an actor in needs a door that closes while occupied, and a block whose sides stop being free
  to pass. This is far future, and nothing depends on it.
- **How does an actor select the adjacent door it means?** In a corridor past an open door, a move turns toward the way it
  goes, so turning toward the door is moving into the doorway, and from inside a block there is nothing to shut.
  Acting on any adjacent door fails in a corridor with an open door on each side, since both are adjacent. The
  interaction needs a way to select a side that is neither the facing direction nor adjacency. The problem arises
  because the terminal host has no turn of its own, and doors are where it appears.

### Ids and addressing

- **Names.** `PersistentId`, `EntityReference`, `make_reference`, `AttachedEntities`, `AddressIndex`, `Access`,
  `Detection` and `InterzonePursuitCapability` are working names.

### Modding

- **The scripting language.** Lua is what the genre uses; Luau adds sandboxing and gradual typing. It is chosen before
  Phase 4.
- **What are the load order and the patch rules?** They decide how mods are ordered, what a patch may add, replace or
  remove, and how two mods that patch one type combine.
- **Which world queries and assets does the sandbox expose to a mod script?** It exposes nothing of the file system.
- **How does a style refer to its meshes, and which surfaces may a kit piece replace?** The candidates are a wall side,
  a flat cell and a corner.
- **Which components inherit along the chain?** Those are the components with fields that may be left unset.
- **How is a place type defined in data, and how do `cells` and `lattice` reduce an entity of a modded type?** A place
  type could be a name that the presentation table maps, or a field on the type.
- **What does opening a game do when a mod's version differs from the version the game was saved with?**
- **Does a zone keep the entities of a missing mod's types, or lose them when it is saved?** Keeping them returns them
  with the mod, as its settings return.
- **Does a mod's manifest list the mods it requires?**

### Sites

- **The word that replaces `site` where it means a candidate location.** `stratum_sim`'s `Site` uses the word for
  where an entity is. Two places use it for a location that a generator or a placement pass chooses among, and one term
  replaces both, since the sense is the same: `placement_plan.md`, where the sites are one of the five roles of a run,
  and `catagen`'s `site_type`, `sites_type` and `door_sites`.

### Placement

- **Where does placement live?** `procgen` is a name reserved in the module layout rather than a library, and placement
  may not belong beside the zone generators at all, since its dependencies are tables and prototypes, not grids.
- **What are the roles called?** `distribution` in particular collides with `std::uniform_int_distribution` and the
  rest, in code that uses them.
- **Is a site a value or an id?** A wall is an `id_type`, a cell a cell position, and a room a record. Templating over
  the site type handles all three; it is not yet known whether a run needs them mixed.
- **Where does the boundary with `content` fall?** Weighted draws over prototypes may belong to `content`, leaving
  placement only the spatial half.

### Layout

- **Does a continuous layout have `Flat` at all?** Its zone is ordinary 3D models, where a
  floor is geometry rather than a bounding surface with a role. `Flat` may be a
  discrete convenience that does not cross, or the record both use with the sampled fields filled
  in. Nothing forces the choice until the skeleton has something behind it.
- **`cardinal_directions`, `discrete_point_type`, `grid_step` and `discrete_vector_type` are in `types.hpp`,** which
  every layout includes, so the discrete vocabulary is in the neutral header. Moving it under `DiscreteLayout.hpp`
  touches every file in the library, and is worth doing alongside other work.
- **Do rooms rise to `Layout`?** A room is `room_id` on a `DiscreteLayout`'s cells and an entity with its
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
- **Where are access point types registered from data, a mod's among them, instead of in `World`'s constructor?**
  `modding_plan.md` has the goal.
- **What scale does the probability of a staircase leading down take, and what bounds it?** It is inversely
  proportional to how far below the surface the zone's level is and proportional to `difficulty`, and is 1 for now. A
  region has no lowest level to bound it.
- **Does the layout interface return a wall id?** An address encodes into one, so it can. An older question is
  whether a handle to a wall belongs on an interface that mentions neither cells nor walls.

### Game file

- **Are the format's identifier and the file names final?** They are `quetzal.stratum_game`, and `game.json` and the
  rest, for now.
- **What are the character's components?** Its description, attributes, and skills are not designed.
- **What triggers an autosave?** Entering any zone does, for now. Entering a town may be the trigger as well or
  instead, which depends on the generator selector's town zones (`## Generators`).
- **Is the quickload key F8 or F9, and which save does it open?**

### Difficulty

- **What does each mode set, and to which values?** The draft table in `difficulty_plan.md` (`## The modes`) is a
  placeholder.
- **What does death do in the easy modes?** It might revive the character at the zone's entry point in Story and
  reload a save from Novice up, or do something else.
- **How is luck applied as a hidden stat of the character?** Neither what it changes nor by how much is decided.

### Intent

- **Does intent cover actions or only movement?** Held attacks and channeled spells would need more than movement and
  facing direction, which is a design question.
- **Do actions get durations** --- wind-up, active and recovery? If so, `Intent` is where they go.

### Regions

- **What properties does each type of region have?** A surface area's and a dungeon's overlap but differ, and
  difficulty goes from the starting value across a dungeon's zones in a way not yet decided.
- **Is the measure within the character's region distance or level difference, and does it differ by type of region?**
- **What does a region file contain, and how does it relate to the zone file format?** It may have only the region's
  metadata and generation settings, or those followed by its zones.
- **How do two bound doors keep their `Access` equal?** `region_plan.md` (`## Doors between zones`) has the design.
  With both zones loaded, changing one door sets its partner too. With the partner's zone unloaded, only this side changes, and the
  two are reconciled when an actor crosses or when the partner's zone loads; which change is newer needs deciding,
  such as by the simulation time of the last change stored with `Access`.
- **How does a relocator between surface zones state where it leads?** It leads neither up nor down, so a level offset
  of 0 may be enough, or it may need a horizontal distance or direction instead.
- **What does the pregenerated content index file contain?** `region_plan.md` (`## Pregenerated content`) describes
  the index.

### Generation

- **A second entrance into a region that exists.** A target with a region always creates a new region, and an existing
  region is reached only through relocators bound to it. A dungeon with two entrances in different surface zones needs
  the second entrance to refer to the region the first created, such as by persistent id once the region exists. A
  region nearing its zone limit has the same need: each exit that leads out of it creates a new region, where they could
  lead into a region that already follows it, which the selector would then refer to.
- **How does the probability of a new region rise with a region's zone count, from 0 to 1 at its limit?** The curve
  and where it starts are undecided, and so is whether it is a parameter of the `RegionSpec`.
- **Which of its two forms is the region in a relocator target, and how are they told apart?** It is the name of a
  region spec, from which a new region is created, or a tag in the pregenerated content index (`region_plan.md`,
  `## Pregenerated content`).
- **Which `RegionSpec` metadata becomes components on the region, and which does only the selector read?**

### Log

- **Are entries written asynchronously, from a queue by a worker thread as spdlog's asynchronous mode does, or on the
  thread that logs?**
- **Are entries written before the first sink is installed kept and written to it, rather than dropped?**

### Random engine

- **Do players get reproducible games from a shared seed?** Each zone's seed is derived from the world seed and the
  zone's position, so a world seed builds the same zones however the world is explored.

### Testing

- **What happens to a reference file whose test case no longer exists?** `--update-references` could remove it, which a
  filtered run must not do, or a run could report it, as Jest reports an obsolete snapshot.
- **Which record of each test case's result is kept?** The candidates are CTest's `LastTest.log`, a JUnit XML file from
  each run, and doctest's output with `--success`.

### Session

- **What population strategy does a zone's source supply?** The source is the template loaded or the generation
  guidance, and the strategy replaces `World::populate`'s placeholder (`session_plan.md`, `## World`).
- **Is the time a zone was entered simulation time or real time?**
- **Should the saves directory default to `%USERPROFILE%\Saved Games\KML\Stratum` on Windows, in place of the user
  directory's `saves`?** That is the known folder `FOLDERID_SavedGames`, which Microsoft's GDK guidance recommends for a
  game that manages its own saves, and which Cyberpunk 2077 and The Outer Worlds 2 use.
- **Does the driver belong to the game or to a zone?**
- **Actors.** The character and the denizens are both actors, and differ in what controls them; the actor structure is
  not designed, and neither is what kills an actor and marks it `Dead`.
- **Are resident zones the character has left simulated or frozen, and which thresholds apply, with what values?**
  Memory usage is one of the thresholds.
- **Zones made and never entered.** They are not candidates for unloading.
- **Where is the unload order's data kept, once regions set the criteria?** It may be `ZoneEntryIndex`'s map from entry
  count to zone id, or a component on each zone with a flecs query ordered by it. The criteria are other regions' zones
  by the time they were entered, and the character's region's zones by distance or level difference (`## Regions`).

### Settings

- **Which thread is a change notification called on, once a thread other than the host's main thread can change a
  setting?** It is now called on the thread that changed it.
- **Must a preset set every member?** It must, for now; a preset that leaves members out would need a fallback for
  them.
- **May a mod add a preset to another's group, named `mod.<id>.<name>` as `modding_plan.md` (`## A mod's identity`) has
  it, or change a preset's values?** A mod cannot change a group's members.
- **Can a preset select another group's preset, as an overall video quality would?**

### View

- **How does a glyph set or tile set mark `concealed`?** A way through is concealed when the viewer has found it and
  actors in general have not. It could be outlined or drawn otherwise; the built-in sets draw it as a door.
- **Does the Godot tile view draw the lattice?** A wall between two open cells is invisible in the tile view, which
  draws `cells`, and in the terminal's cell view.
- **Does `cells` need state?** Its consumers draw one glyph per cell and have no edges, so it may need only the type,
  though having the two reductions produce different forms has its own cost.
- **What does a tile set produce?** That decides whether the glyph table is templated on what it maps to, or characters
  and tile indices are two tables.
- **How does a host load a glyph set from a file?** The hosts use the built-in sets; a file is how a set that is not
  built in arrives.
- **What does each view make of a corner?** The reduction marks a joint there; the terminal draws a box-drawing
  character and a 3D view a column. Neither is decided, and the 3D view is why the corner is in the reduction.

## Revisit later

These are set aside on purpose.

- **Restarting a game with the last game's settings,** kept in memory for the program's run, possibly on the same world
  seed, and **templates** that a new game's settings are initialized from, which would let settings outlast the program.
- **Scripted initializers,** such as an `on_new_game` script computing game settings when a game is created, beyond an
  `initializer` that refers to a user setting. They may overlap with templates.

- **Pursuers following the character through a relocator** (`session_plan.md`, `### Following through a relocator`).
  Only the controlled actor enters a relocator.
- **A doorway into a dead end,** which has a wall on one side only: what a door glyph there requires, and what the
  generators and the reader do with one.
- **maze1's unreachable corridor.** Excavation can leave corridor that no door reaches, which dead-end pruning removes
  only by chance, and a staircase placed at its end stops the pruning, so the staircase is unreachable too (`up_only`
  seeds 1 and 40 in `dungen_test`). `dungen_test`'s check that every floor cell is reachable skips maze1 until this is
  done.
- **Naming a wall address's `normal` `facing`,** once the format settles, to match door facings; the value also being
  the surface normal a texture uses argues against it.
- **`passable_at` and `block_allowed` in the generators.** The sim uses `crossable` for getting through a wall and
  `blocking` for something in the way; dungen and catagen use `passable_at` for a cell an actor can be in and
  `block_allowed` for where a staircase may go. Both are cell questions rather than edge ones, so it is not the same
  rename, but the word is the one the sim dropped.

## Loose

These are not ordered, but worth a pass: `Scheduler::advance` integer overflow, and its `operator[]` on `m_speeds`.
