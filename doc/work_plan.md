# Work Plan

Date: 2026-08-28, revised 2026-10-01

This is the stratum work queue: every item not yet built, and the open questions. An item is referred to by its bold
name. `game_system_principles.md` has
the phases and the principles, `game_system_reference.md` describes what is built, and `modding_plan.md` is the design
goal that every item is held to: everything a game adds is data or script.

The design behind the items is in these documents, which the items refer to by section:

- `access_point_plan.md` merges `Relocator` into one `AccessPoint` component with an optional target, and derives the
  arrival placement from the site of the access point's entity.
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
- `session_plan.md` covers how `Game`, `Session`, `World` and `Character` divide the work of making and playing zones, and
  the game's lifecycle and the menu operations behind it: `Game` is the game in its entirety, lasting as long as the
  host, and owns the `World`, the `Character` and the `Session` by value, clearing them in place for each game created
  or opened.
- `settings_plan.md` covers the settings: one keyed store or typed structures for each layer, and game and program
  settings.
- `region_spec_plan.md` defines the form of a `RegionSpec` and a `ZoneSpec`, patterns included, so that a designer can
  write one by hand and the game can generate one.
- `sim_zone_model.md` has the model beneath the code.
- `template_plan.md` covers templates, the data a zone's content is made from, and pregenerated content.
- `testing_plan.md` covers how the tests are written, registered and run: doctest, CTest, and reference files that one
  option updates.
- `world_generation_plan.md` covers how a new game's world is generated: its structure of worldspaces, regions and
  zones created whole from `RegionSpec`s before play, the registries of zone generators and region
  specs, and each zone's content generated when it is needed.
- `zone_plan.md` defines the world's structure: worldspaces, zones, and hierarchical regions, their names, extended
  names and persistent ids, positions and levels, and access points and their targets.

`archived/` has the plans whose work is built.

## Commit points

Each box is one commit point. An item too large for one commit is split into boxes of its own before work on it
starts. When a box's work is done and every test passes, the box is ticked and that is the moment to commit. The ticked
boxes are removed at the start of the next piece of work, since the reference documents then describe what they built.

## Generators

- [ ] **Staircases in every form** (`session_plan.md`, `## Entering an access point`). The generators can place a
      staircase in a flat length of wall or in a 1x1 alcove, the two being equivalent across the notations, as well as
      a block staircase on floor cells in the open space of a room. In the wall notation every staircase is sited on a
      wall.
- [ ] **Zone template tables** (`template_plan.md`, `## A zone's template`): the `zone.template` pattern drawing each
      zone's template from the opening sequence and the primary and rare weighted tables in the `RegionSpec`, and the
      starting region's spec, with terrain templates, which need **Templates**.
- [ ] **Templates** (`template_plan.md`, `## Templates`): a `ZoneSpec`'s `template` read from **The pregenerated
      content index**, and the zone generator applying the `ZoneSpec` to it.
- [ ] **A second catagen algorithm,** and with it what the two generators have in common becoming `quetzal::procgen`.

## Random engine

- [ ] **Generation runs as objects** (`random_engine.md`, `## Ownership`): dungen's and catagen's runs, each owning its
      random engine, seeded from the zone's seed, with the reference files regenerated. It comes with **the generators
      integrated**.

## Zones and regions

`zone_plan.md` has the design.

- [x] **Worldspaces** (`zone_plan.md`, `### Worldspaces`): a worldspace entity with its name and `PersistentId`, the
      flecs parent of its zones, with its coordinate system from its top-level region's spec, and each zone's
      `ZonePosition` in its worldspace, unique there.
- [x] **Top-level regions** (`zone_plan.md`, `### Regions`): one region for each worldspace, created from its region
      spec, with the worldspace's extent, and a zone referring to its region by persistent id.
- [x] **Extended names** (`zone_plan.md`, `### Identity`): a name and an extended name for every worldspace, zone and
      region, unique across the game, a name that is taken given the suffix `_<counter>`, from the game's counter for
      top-level regions or a region's counter for its subregions and zones (`zone_plan.md`, `## Implementation`), a
      worldspace and its top-level region named for the region spec, and a zone's `DisplayName` separate from both.
- [ ] **Region specs defined** (`region_spec_plan.md`): the keys of a `RegionSpec` and a `ZoneSpec`, the form of their
      patterns, and the open questions there answered, before the structure is created from them.
- [ ] **Pattern expressions** (`pattern_resolution_plan.md`): a `&` value evaluated as Lua with the symbols of its
      spec.
- [ ] **Listed zones** (`region_spec_plan.md`, `### Listed zones`): a region spec's `zones` member, each zone's position
      and the keys that override the `zone.` keys for it, read with the other typed members.
- [ ] **The world's structure created whole** (`world_generation_plan.md`, `## Creating the structure`;
      `region_spec_plan.md`, `## Zones of a worldspace`): every worldspace, with its single region, created from the
      game's JSON object of region specs, and a zone at every position of its extent, in worldspace position order, each created by `World::create` with its spec, its name, its position and its seed, the
      seeds drawn hierarchically (`random_engine.md`, `## Generation`), and a zone's content generated the first time it
      is entered or loaded. `RegionPosition`, which a region's seed derives from, goes.
- [ ] **Targets resolved by extended name and by position** (`zone_plan.md`, `### Targets`): the `{zone, position}`
      target in place of `RelocatorTarget` (`zone_plan.md`, `## Implementation`), and an unbound target resolved to the
      zone of an extended name or a persistent id, or at a position relative to the access point's zone or absolute in a
      worldspace, with the zone's content generated if it has none.
- [ ] **A reference's zone in the reference:** an `EntityReference` to an entity that never changes zone, such as an
      access point, stores that zone's persistent id, and the world index has no entry for it.
- [ ] **The world index limited to references across zones:** an entry for an entity only while something outside its
      zone refers to it --- another zone, or the character's inventory --- added when a relocation or a new reference
      separates the two, and removed when the last such reference is gone or the two are in one zone again. An entry
      no longer outlives its references.
- [ ] **A region's properties** as components: a name and, for a dungeon, the overall or starting difficulty and the
      other properties a whole dungeon has.
- [ ] **Adjacent zones** (`zone_plan.md`, `### Worldspaces`): the zones of a grid worldspace adjacent edge to edge by
      their positions, so an actor walks from one zone into the next. Where zones can be traversed across adjacent edges,
      or through doorway access points, the zone on the other side is visible and must be rendered, and an adjacent zone
      is loaded with the zone the character is in.
      First pass: Zones should be created with natural or constructed barriers that surround them so this problem can be deferred.
- [ ] **Region files.**
- [ ] **The pregenerated content index** (`template_plan.md`, `## Pregenerated content`): built at game start by scanning
      `assets/zones`, or read from an index file built in the background, listing each zone's and region's extended
      name with its zone data file and the names of the access points in it.
- [ ] **Access point names:** a `name` on an access point, read, written, and listed in the index, for a target that
      arrives at a named access point.
- [ ] **Subregions** (`zone_plan.md`, `### Regions`): a region's parent region, a `RegionSpec`'s subregion specs, and
      each subregion's extent, in the syntax that the open question on the extent's key names decides
      (`world_generation_plan.md`, `### Extents`).
- [ ] **Naming patterns** (`zone_plan.md`, `### Identity`): a region spec's naming patterns, one for its regions and one
      for their zones, in place of `<basename>_<counter>`, with extended names unique at every level of the region
      hierarchy.
- [ ] **The target's notation** in place of `{zone, position}`, once the open question on it is answered.

## Access points

`access_point_plan.md` has the design.

- [x] **The arrival placement from the site.** stratum_sim's `AccessPoint` has no placement, and an arriving actor is
      placed from the site of the access point's entity. The zone file is unchanged.
- [x] **One `AccessPoint` component in stratum_sim,** with an optional target, `Relocator` merged into it, and the
      identifiers that refer to the component renamed (`access_point_plan.md`, `## Renames`).
- [x] **The `access_point` file component,** format 0.6, with no placement, every zone file in the tree rewritten,
      and the reference files regenerated.
- [x] **One-way access points resolved:** an unbound target arriving at an access point type whose access points have
      no target.

## Attachments

- [ ] **A destroyed door transformed or unlinked, not deleted** (`archived/attachment_plan.md`, `## Lifetime`).
      Deleting the assembly would seal the doorway, since a wall with nothing on it is solid.
- [ ] **Crossing that depends on the actor, kept affordable** (`archived/attachment_plan.md`, `## The seam`): only
      attachments marked overridable consult the actor's properties, so the common case never looks.
- [ ] **A key that opens a lock.** `Access` has `Locked`, and nothing unlocks it.

## Views

- [ ] **Floor access points drawn by their orientation.** The views draw one with the stair glyph and ignore its
      orientation.
- [ ] **A lattice view in the Godot host,** drawn from `lattice` with the walls and doors on cell edges, so that it
      draws either notation, and listed in the Godot manifests as `lattice`. The tile view stays drawn from `cells`.
- [ ] **Dimetric, isometric, and first-person views,** each offered by a game whose assets support it. The Godot host
      is likely to become the primary viewer. The views are top-down for now, so these are undecided. Dimetric might
      need little more than another tile set, and isometric not much more, but neither has been examined.

A wall notation zone in a view that draws one place per cell, the cell view or the tile view, remains allowed, but only
a wall notation zone that a block notation zone could have been converted to is supported there. A wall or door on a
cell edge, such as two rooms back to back sharing a wall, cannot be drawn reliably in such a view, and no time is spent
on those cases. Even the supported cases are the lowest priority configuration.

## Assets

- [ ] **The asset types defined:** floor, wall, attachment or decoration, item, denizen, and player.
- [ ] **A couple of simple asset sets.** The views a game offers then follow from the assets available, in place of
      the game manifest's list.

## Modding

- [ ] **Place types from data** (`modding_plan.md`, `## Current blockers`). A reduction maps an entity's type to a
      place type that the presentation table names, instead of to an enum value, so a modded type has a glyph of its
      own.

These are Phase 4 (`modding_plan.md`):

- [ ] **Types as prefabs from data**, doors among them, with inheritance and patches.
- [ ] **Component schemas from data**, as flecs runtime components.
- [ ] **Styles:** the asset, the parameter map, inheritance by name, and a view that resolves them.
- [ ] **Lua embedded** (`scripting_language_plan.md`), with hooks on types and bindings through reflection.
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
- [ ] **A save that writes only what changed:** world.json's zone lists and world index in storage that is read and
      updated by entry, such as SQLite or files split into fixed-size parts, in place of one JSON file rewritten whole on every save.

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

### Zones and regions

- **Can a `RegionSpec`'s `extent` and `zone_size` be patterns?** Their values are not text
  (`pattern_resolution_plan.md`).
- **What type does a store entry have as a pattern's symbol?** The stub makes it a number when its text parses as
  one, and a string otherwise.
- **Which symbols does a region spec's own resolution have?**
- **What is the target's notation?** `RelocatorTarget` is deprecated until it is designed (`zone_plan.md`,
  `### Targets`).
- **What properties does each type of region have?** A surface area's and a dungeon's overlap but differ, and
  difficulty goes from the starting value across a dungeon's zones in a way not yet decided.
- **What does a region file contain, and how does it relate to the zone file format?** It may have only the region's
  metadata and generation settings, or those followed by its zones.
- **How do two bound doors keep their `Access` equal?** `zone_plan.md` (`### Doors between zones`) has the design.
  With both zones loaded, changing one door sets its partner too. With the partner's zone unloaded, only this side changes, and the
  two are reconciled when an actor crosses or when the partner's zone loads; which change is newer needs deciding,
  such as by the simulation time of the last change stored with `Access`.
- **What does the pregenerated content index file contain?** `template_plan.md` (`## Pregenerated content`) describes
  the index.

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

- **Can a script suspend?** A coroutine has state between calls, which saving does not cover
  (`scripting_language_plan.md`).
- **Is one Lua state shared by the game,** with an environment for each script and spec, or is there one for each mod?
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
  instead, which depends on the town zones that the `zone.template` patterns draw (`## Generators`).
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

### Generation

- **What are the extent's key names and syntax,** and what forms does it take besides boxes? A diagonal boundary takes
  one box for each row (`world_generation_plan.md`, `### Extents`).
- **Which `RegionSpec` metadata becomes components on the region, and which do only the patterns of the `zone.` keys read?**

### Log

- **Are entries written asynchronously, from a queue by a worker thread as spdlog's asynchronous mode does, or on the
  thread that logs?**
- **Are entries written before the first sink is installed kept and written to it, rather than dropped?**

### Random engine

- **Do players get reproducible games from a shared seed?** Each zone's seed is drawn from the world seed before play,
  so a world seed builds the same zones however the world is explored.

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
  seed.
- **Scripted new game settings,** such as an `on_new_game` script computing game settings when a game is created,
  beyond the values in the game manifest's `settings`.

- **Pursuers following the character through an access point** (`session_plan.md`, `### Following through an access
  point`). Only the controlled actor enters an access point.
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
