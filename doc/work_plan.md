# Work Plan

Status: stages 1 to 6 complete; stages 7 and 8 in progress
Date: 2026-08-28, revised 2026-09-24

The ordered work queue within Phase 3 and toward real time. `game_system_principles.md` has the phased plan and its
principles; what is built is described in `game_system_reference.md`.

A stage large enough to need its own detail has a plan of its own: `intent_plan.md` for stage 2, `layout_plan.md` for
stage 4, `view_plan.md` for stage 5, and `attachment_plan.md` and `glyph_plan.md` for two items of stage 6.
`dungen_plan.md` and `catagen_plan.md` cover the two generators. `placement_plan.md` is the form placement takes when
it is built, and `site_plan.md` is where every entity is and what it belongs to; neither is a stage. `modding_plan.md`
is the design goal every stage is held to: everything a game adds is data or script. `session_plan.md` is how the
parts of `StratumGame` divide stage 7's work.
`discrete_layout_plan.md` covers stage 8, with `discrete_layout.md` the format it builds to and
`discrete_layout_rework.md` the design of loading a zone from it. `addressing.md` specifies ids and addressing ---
geometry, entities, attachments, references --- and `addressing_plan.md` is the design of the rest of stage 8.

**Open items are listed here**, under `Open` below, rather than in the plan that raised them.

## Next

The order the remaining steps are built in, each on the ones before it. A step leaves this list when it is done.

1. **`addressing_plan.md` step 5, loading a zone,** with `modding_plan.md` step 2, in `discrete_layout_rework.md`'s
   order: types in `World` and the property query; places as entities and `load_layout` building every entity with
   its links, persistent ids translated and components decoded; `crossable` through `Access` and `Detection`, the door
   functions deleted; the generators and suites on the new path, `create_doors` deleted.
2. **`addressing_plan.md` step 6,** migration, and the player an entity of the game's rather than in a zone.
3. **`addressing_plan.md` step 8,** relocating through a relocator.
4. **`session_plan.md` step 6,** entering an access point.
5. **Stage 7, a zone added to the world when it is encountered.**
6. **`addressing_plan.md` step 7,** saved zones and the reset to a save point, with the rest of stage 7.

Beside that order:

- **After step 1:** `session_plan.md` step 4, pre-populated zones, the loader building a file's denizens.
- **Blocked:** `placement_plan.md`, its step 1 changing once `create_doors` goes and its step 2 needing `content`; the
  population strategy comes with it.
- **At any time:** `addressing_plan.md` step 9, the zone actor index; `session_plan.md` step 7, staircases in every
  form; `modding_plan.md` step 1, place types from data.
- **Later:** regions, and `modding_plan.md` steps 3 to 7, which are Phase 4.

## Plans

One box per plan with steps of its own, checked when every step is done.

- [ ] [`addressing_plan.md`](addressing_plan.md) --- ids and addressing: format 0.4, persistent ids, links, loading a
      zone against them, and a zone actor index. 9 steps, steps 1 to 4 done.
- [x] [`site_plan.md`](site_plan.md) --- where every entity is and what it belongs to: the parent and site links,
      `Relation`, and the file component that replaces `attached`.
- [ ] [`modding_plan.md`](modding_plan.md) --- mods and data-driven design: place types from data, components decoded
      at load, types as prefabs, component schemas from data, styles, scripting and mods. 7 steps, none started; step 2
      is part of `addressing_plan.md` step 5, and steps 3 to 7 are Phase 4.
- [ ] [`session_plan.md`](session_plan.md) --- the division of work among `StratumGame`'s parts: `World` creating,
      loading, populating and unloading zones, `Session` entering them and keeping the state of play, and each zone's
      entry point, the lowest-ordered staircase leading up, and entering an access point on a wall or the floor. 7 steps,
      steps 1 to 3 and 5 done.
- [ ] [`placement_plan.md`](placement_plan.md) --- what fills a zone: sites, selection, distribution, acceptance and
      emission. 4 steps, none started.
- [x] [`intent_plan.md`](intent_plan.md) --- stage 2, intent and the schedule.
- [x] [`layout_plan.md`](layout_plan.md) --- stage 4, the layout interface and its structures.
- [x] [`view_plan.md`](view_plan.md) --- stage 5, the views and the reference frames.
- [x] [`attachment_plan.md`](attachment_plan.md) --- stage 6, doors and what is attached to walls.
- [x] [`glyph_plan.md`](glyph_plan.md) --- stage 6, glyph sets that load from a file.
- [x] [`dungen_plan.md`](dungen_plan.md) --- the dungen generator.
- [x] [`catagen_plan.md`](catagen_plan.md) --- the catagen generator.

`discrete_layout_plan.md`, `discrete_layout_rework.md`, `addressing.md`, `discrete_layout.md` and
`sim_zone_model.md` have no steps of their own; the stage items below track what they specify.

## Stage 1 --- one place for it

- [x] **Extract `Session`** from `Application` and `StratumWorld` into `stratum_sim`: world, scheduler, clock, player,
      zone loading. A host is input mapping and drawing.

## Stage 2 --- Intent

Detail in `intent_plan.md`.

- [x] **`Intent` component; the clock reads it; hosts write it.**
- [x] **Retire the empty-command path** and the stuck-actor return from `Clock::run`.
- [x] **Cost from the result** rather than a flat `Scheduler::action_cost`.

## Stage 3 --- real time

- [x] **Compose topology with occupancy on `World`.**
- [x] **Time driver in `Clock`**, with a fractional accumulator, the driver chosen at load rather than by the host.
- [x] **Real-time movement**, a held intent producing moves as they are earned.
- [x] **AI writes intent**, with a second actor: `Pursue` and `think`.

## Stage 4 --- the layout hierarchy

Detail in `layout_plan.md`.

- [x] **`Layout` becomes the interface**, with `DiscreteLayout` behind it. The rules, the AI and `World` go through the
      interface; code that counts cells uses `layout_as`.
- [x] **`ContinuousLayout`**, declared and not written --- a skeleton with nothing behind it is
      the point: it marks what is absent rather than leaving it to be guessed. Two classes under
      `Layout` and nothing else: geometry is not a second axis of classes, since a zone with no
      height data loads a constant floor rather than a different class, so which data a game style
      loads is what differs. Neither name mentions `Flat` for that reason.
      `decision_rationale.md` argues why that is data and continuous movement is behaviour.
- [x] **`DiscreteLayout` completed**, including `Flat` as the height field the model asks for.

## Stage 5 --- the view axis

Detail in `view_plan.md`.

- [x] **Wall-aware ASCII view**, on a 2N+1 grid: `lattice` and `render_lattice`, drawn by the terminal host.
- [x] **`Interact` asks the layout what is ahead**, through `Layout::ahead`.
- [x] **The `View` interface**, found unneeded: a reduction to a `Buffer<Place>` and a mapping to a `Buffer<T>` suffice
      (`decision_rationale.md`).

## Stage 6 --- what the wall notation surfaced

- [x] **Doors.** A door is an entity with a door facing on each of its walls, and what is attached to a wall decides
      whether it may be crossed (`attachment_plan.md`).
- [x] **State beside the type, and a glyph set that loads** (`glyph_plan.md`).
- [x] **Staircases state where they lead.** Both generators write each staircase as a `staircase` entity attached to
      the wall on its open side, with a `relocator` whose `target` is `down` or `up`.
- **Relocating through a relocator** --- an entity with a relocator moving an actor to its target, and both ends
      binding to each other --- is [`addressing_plan.md`](addressing_plan.md) step 8, boxed under Plans.
      `addressing.md` (`### Relocators and access points`) specifies the two kinds of target.
- [x] **Suites for both generators, and the drivers moved out of `application/`** to `tool/dungen` and `tool/catagen`,
      with `test/dungen_test` and `test/catagen_test` comparing reference zone files and regenerating each from its own
      parameters.
- [x] **The loader builds one zone.** A file of either notation is one zone, with its rooms as labels over cells and
      its two flats.
- [x] **Rooms are labels over cells**: `room_id` on each cell, set by the loader from the file's rooms.
- [x] **Walls indexed by cell edge**: a bit on the cell they face, with an id encoding the cell and the side.

## Stage 7 --- the session lifecycle

**In progress.** The entries are built; the lifecycle around them is not.

- [x] **Two entries, one of them the general case.** `World::create_zone` from a `Generator` and `World::load_zone`
      from a file, each making the zone whole, and `Session::enter` taking the player into it (`session_plan.md`).
- [x] **The player belongs to the game, not to a zone.** `StratumGame` owns the id allocator, the world, the player and
      the session; the player exists before any zone and is moved into each zone loaded.
- [ ] **A zone is added to the world when it is encountered.** At startup or on a transition, the zone's layout is
      created, detailed and populated, and the zone is added to the world, where it is playable.
- [x] **Zones unload past a threshold, oldest first.** Leaving a zone leaves it resident; entering one, `World::trim_zones`
      unloads the zones entered longest ago, as the world's `ZoneEntryIndex` chooses, once the zone count maximum is
      exceeded (`sim_zone_model.md`, `### Residency, simulation, and persistence`).
- [ ] **There is never a new session.** One session for the whole game, cleared on a new-game selection or on death
      under a permadeath option.

What a session is responsible for is open (`Open`, `### Session`).

## Stage 8 --- the discrete layout format

**In progress.** One JSON zone file for both generators, with rooms, flats, styles and entities. Detail in
`discrete_layout_plan.md`.

- [x] **Settle the structures and read the code against them** (`discrete_layout_rework.md`).
- [x] **The format**: `quetzal_stratum_format` reads a file into a validated `Document` and writes it back, with
      `stratum_format_test`.
- [x] **The generators write the format** through `stratum_format`.
- [x] **Staircases have an open side**, drawn from the sides with floor beyond, a side with a door only where there is
      none, a staircase placed blocking a doorway first moving one cell (`dungen_reference.md`,
      `catagen_reference.md`).
- [x] **Every zone has its access points**: `layout.components.access_points`, at least one reference to an entity with
      a relocator on the open side of a cell written `<` or `>`.
- [x] **Doors have entities**: each door glyph a `door` entity with a `door_facing` child on each wall
      `stratum_format::door_walls` finds for it.
- [x] **Counts of staircases each way**: `staircase_down_count` and `staircase_up_count` as closed ranges.
- [x] **The structures, then the reader filling them**: `Cell` with the wall mask, and `load_layout` building a
      `DiscreteLayout` from a `Document`.
- [x] **A `DiscreteLayout` takes its grid at construction.**
- [x] **Stair cells are blocks** with a wall on each side (`addressing_plan.md`, `## Decided`): in `stratum_format`, in
      `load_layout`, and in both generators, which face a staircase toward a floor neighbor and wall it in.
- [x] **Staircase placement keeps every passage open**, in every generator, checked over the seed sweeps.
- [x] **Format 0.4 in `stratum_format`** ([`addressing_plan.md`](addressing_plan.md) step 1).
- [x] **The generators and every zone file in 0.4** ([`addressing_plan.md`](addressing_plan.md) step 2).

The rest of stage 8 is [`addressing_plan.md`](addressing_plan.md), boxed under Plans: persistent ids, loading a zone,
migration, saved zones and the zone actor index.

## Regions

**Future.** A `Region` groups zones: a surface area, or a dungeon whose levels are its zones. It is data, an entity
that is its zones' parent, and it decides which zones unload first (`sim_zone_model.md`, `## World, Zones, and
Levels`). Open questions under `Open`, `### Regions`.

- [x] **"Region" out of catagen,** first: its pipeline is rectangles, the rooms kept among them, and areas, a room or a
      run of corridor, which walls separate and doors join. dungen's cave1 keeps "the largest connected region", which
      is harmless; `roguelike_survey.md` keeps the word in its general sense, and Godot's `texture_region_size` is
      Godot's own name.
- [ ] **`Region`,** an entity with its properties as components --- a name, and for a dungeon the maximum depth the
      probability of a staircase leading down needs, the overall or starting difficulty, and the other properties a
      whole dungeon has --- and the parent of its zones in place of the root.
- [ ] **Unloading by region.** Criteria for `World::trim_zones` that choose the zones of other regions first, left
      longest ago first, as `ZoneEntryIndex` does, then those of the player's region, farthest first.
- [ ] **Region files.**

## Done ahead of the plan

- [x] **One wall question in the layout, the predicate passed rather than stored.** `move`, `neighbors` and `visible`
      take a `crossable_function_type`, which `World` binds to the zone.
- [x] **`crossable` for getting through a wall and `blocking` for something in the way**, throughout the sim and the
      view.
- [x] **A cell position is a position and a `discrete_vector_type` is a displacement** (`decision_rationale.md`).
- [x] **`math::DiscreteVector<T, N>`**, beside `math::Vector`, replacing `dungen::Vector`.
- [x] **`direction` in `math`**, reducing a vector to the unit vector along its longest axis.
- [x] **Cardinal directions** as named constants.
- [x] **`Rotation`**, with values `CCW`, `CW` and `Half`.
- [x] **`geometry::MinProjection`** in place of hand-written projection.
- [x] **The queries take a direction**: `wall_toward(cell_position, direction)` and `crossable(cell_position,
      direction, ...)`.
- [x] **`GridMapping::contains`**, asked in assertions only.
- [x] **Openness is layout data**: `open` and `set_cell_open`.
- [x] **No reserved value for an absent cell**: `Session`'s scans return `std::optional`.

## Phase order

A minimal first-person 3D view comes just after Phase 3, ahead of content and procgen: it is the second genuinely
different view, and it validates the wall model before procgen generates against it. `game_system_principles.md`
places it at Phase 3b, with the rest of the Godot 3D view in Phase 6.

## Against the continuous layout

Not stage 3, and not gating anything before Phase 7.

**`value_type`, `float` or `double`.** Float resolves to roughly a millimetre at ten thousand units,
which no zone comes near, and `Layout::move` regenerates position from the cell it reached rather
than accumulating, so a discrete layout does not drift however often it is called --- real time
included. What argues for double is neither of those: every vertex today is grid-derived and
therefore exactly representable, which arbitrary angled walls end, turning every tolerance the
geometry tests need into a tuning exercise. One line in `types.hpp`, the helpers
being templated on the value type and the tolerance retightening on its epsilon. The renderer using
float is not an argument against it: the view narrows at the boundary, where magnitudes are small.

## Open

Questions a stage left behind.

### Attachments and doors

- **What a smashed door leaves is not the door's to decide.** Transform is the mechanism, but the outcome resolves what
  hit it against what it is made of --- an axe and a charge differ as much on one door as an oak door and a portcullis
  differ under one axe. `sim_zone_model.md` settles that as script-determined over an engine default, so the structure
  owes only that the script can reach both: what the door is made of, and what the damage was.
- **Where damage accrues.** An axe is applied to a door facing and a blast is centered in space, reaching many walls and
  everything on them. That argues damage accrues per door facing --- the side chopped is the side that shows it --- with
  the shared state changing only when the door as a whole gives way.
- **A breach is the form of a smashed door.** A wall blown through has an attachment with `Crossable`, so no wall is
  removed, ids stay stable and `crossable` needs no new case.
- **Whether a wall side without a door is worth an entity.** A wall's surface treatment is per side. If every wall side
  becomes an entity, the count argument that kept walls out of the entity store applies again, so surface probably
  stays in layout data and only sides with something on them become entities.
- **Where a trap belongs.** On the door, or per door facing, since the side it is triggered from and the side it fires
  into need not be the same.
- **Whether `Concealed` blocks sight as well as movement.** A secret door reads as wall, and a wall stops a sight line,
  so `visible` needs the same query, which argues for the predicate being about the wall rather than about crossing it.
- **What a swinging door needs, if anything.** A door facing has no direction of its own, the wall determining its
  orientation, so which space a door opens into is either not modeled or one more field on the door.
- **Whether a door can shut behind an actor.** One door on a doorway block means being inside it is never being between
  two doors, so a trap that shuts an actor in needs a door that closes while occupied, and a block whose sides stop
  being free to pass. Far future; nothing depends on it.
- **How an actor selects which adjacent door it means.** In a corridor past an open door, a move turns toward the way it
  goes, so turning toward the door is moving into the doorway, and from inside a block there is nothing to shut.
  Acting on whatever is adjacent fails in a corridor with an open door on each side, as both are adjacent. The
  interaction needs a way to select a side that is neither the facing direction nor adjacency. It follows from the
  terminal host having no turn of its own, and the door is where it shows.

### Modding

- **The scripting language.** Lua is what the genre uses; Luau adds sandboxing and gradual typing. Chosen before
  Phase 4.
- **Load order and patch rules.** How mods are ordered, what a patch may add, replace or remove, and how two mods
  patching one type combine.
- **What the sandbox exposes** to a mod script: which world queries, which assets, and nothing of the file system.
- **How a style names its meshes,** and which surfaces a kit piece may replace: a wall side, a flat cell, a corner.
- **Which components inherit** along the chain, and so which have fields that may be left unset.
- **How a place type is defined in data** --- a name the presentation table maps, or a field on the type --- and how
  `cells` and `lattice` reduce an entity of a modded type.

### Sites

- **What replaces `site` where it means a candidate place.** `site_plan.md` takes the word for where an entity is. Two
  places use it for a place a generator or a placement pass chooses among, and one term replaces both, the sense being
  the same: `placement_plan.md`, where the sites are one of the five roles of a run, and `catagen`'s `site_type`,
  `sites_type` and `door_sites`.

### Placement

- **Where it lives.** `procgen` is a slot rather than a library, and placement may not belong beside the zone generators
  at all --- its dependencies are tables and prototypes, not grids.
- **What the roles are called.** `distribution` in particular collides with `std::uniform_int_distribution` and the
  rest, in code that uses them.
- **Whether a site is a value or an id.** A wall is an `id_type`, a cell a cell position, a room a record. Templating
  over it serves all three; whether a run needs them mixed is not known.
- **Where the boundary with `content` falls.** Weighted draws over prototypes may be `content`'s, leaving placement only
  the spatial half.

### Layout

- **Whether a continuous layout has `Flat` at all.** Its zone is ordinary 3D models, where a
  floor is geometry like any other rather than a bounding surface with a role. `Flat` may be a
  discrete convenience that does not cross, or the record both use with the sampled fields filled
  in. Nothing forces the choice until the skeleton has something behind it.
- **`cardinal_directions`, `discrete_point_type`, `grid_step` and `discrete_vector_type` are in `types.hpp`,** which
  every layout includes, so the discrete vocabulary is in the neutral header. Moving it under `DiscreteLayout.hpp`
  touches every file in the library, and is worth doing alongside something else.
- **Whether rooms rise to `Layout`.** A room is `room_id` on a `DiscreteLayout`'s cells and an entity with its
  components. The entity is neutral and the cells are not, so it stays on `DiscreteLayout` until something continuous
  needs one.
- **Entities sited on flat cells.** A site is one mechanism over walls and flat cells alike; loading a zone
  (`discrete_layout_rework.md`) builds both.
- **`DiscreteLayout::field_of_view` is declared and not defined.** A shadowcasting sweep computes every cell around one
  origin at once, where `visible` walks the ray between one pair, and `sim_zone_model.md` notes the two give different
  results.

### Discrete layout format

- **Perimeter validation passes some invalid perimeters.** Each cell edge is checked on its own, and a wall toward a
  floor/door cell on either side satisfies one. A perimeter whose walls all face outward, around a pillar or solid
  ground, passes while enclosing no floor/door cell. The reader already determines which side of each edge is inside;
  a check of the enclosed area is needed to reject these.
- **The access point type a new game enters a zone at when it is not a dungeon level,** such as a tower entered from
  below, `up` being the hosts' temporary `entry_access_type` (`session_plan.md`, `## Entry point`).
- **Where access point types are registered from data,** a mod's included, rather than in `World`'s constructor
  (`modding_plan.md`).
- **The scale of the probability of a staircase leading down,** inversely proportional to `depth` and proportional to
  `difficulty`, 1 until regions supply the maximum depth, and what it is for a zone with no `depth`.
- **Whether the layout interface returns a wall id.** An address encodes into one, so it can. Whether a handle to a
  wall belongs on an interface that mentions neither cells nor walls is an older question.

### Intent

- **Does intent cover actions or only movement?** Held attacks and channeled spells would need more than movement and
  facing direction, which is a design question.
- **Do actions get durations** --- wind-up, active, recovery? If so, `Intent` is where they go.
- **Does a partial intent survive save and reload?** Plausible for movement, likely wrong for anything else.

### Regions

- **Which properties each kind of region has,** a surface area's and a dungeon's overlapping but differing, and how
  difficulty goes from the starting value across a dungeon's zones.
- **Distance or depth difference** as the measure within the player's region, and whether it differs by kind of
  region.
- **What a region file holds:** only the region's metadata and generation settings, or those followed by its zones;
  and how it relates to the zone file format.

### Session

- **The pre-populated flag's name and place in the format** (`session_plan.md`), and whether it is a format version
  change.
- **The population strategy from the zone's source,** the template loaded or the generation guidance, replacing
  `World::populate`'s placeholder (`session_plan.md`, `## World`).
- **The time a zone was entered:** simulation time or real time.
- **The game's config file,** supplying the zone count maximum, now `StratumGame`'s temporary 8, and the game's other
  settings.
- **Whether the driver is the game's or a zone's.**
- **Actors.** The player and the denizens are both actors, differing in what controls them; the actor structure is not
  designed.
- **What components a zone has** beyond `ZoneLayout` and `ZoneNotation`, and which operations change from layouts to
  zones (`discrete_layout_rework.md`, `### Zones`).
- **Resident zones the player has left.** Whether they are simulated or frozen, and which thresholds apply --- memory
  usage among them --- and their values.
- **Zones made and never entered.** They are no candidates for unloading.
- **Where the unload order's data is kept** --- `ZoneEntryIndex`'s map from entry count to zone id, or a component on
  each zone with a flecs query ordered by it --- once regions set the criteria: other regions' zones by the time they
  were entered, the player's region's by distance or depth difference (`## Regions`).

### View

- **Whether the Godot tile view draws the lattice.** A wall between two open cells is as invisible in the tile view,
  which draws `cells`, as in the terminal's cell view.
- **What a staircase renders as.** A stair cell is a block the views draw as solid, and the wall notation round trip
  compares `<` and `>` as solid.
- **Whether `cells` needs state.** Its consumers draw one glyph per cell and have no edges, so it may need the type
  alone, though the two reductions producing different forms is its own cost.
- **What a tile set produces,** which decides whether the glyph table is templated on what it maps to or characters and
  tile indices are two tables.
- **Loading a glyph set from a file in a host.** The hosts use the built-in sets; a file is how a set that is not built
  in arrives.
- **What a corner is made into, per view.** The reduction marks a joint there; the terminal draws a box-drawing
  character and a 3D view a column. Neither is decided, and the 3D view is why the corner is in the reduction.

## Revisit later

Set aside on purpose.

- **A doorway into a dead end,** which has a wall on one side only: what a door glyph there requires, and what the
  generators and the reader do with one.
- **maze1's unreachable corridor.** Excavation can leave corridor no door reaches, which dead-end pruning removes only
  by chance, and a staircase placed at its end stops the pruning, so the staircase is unreachable too (`up_only` seeds 1
  and 40 in `dungen_test`). The suite's check that every floor cell is reachable skips maze1 until this is done.
- **Naming a wall address's `normal` `facing`,** once the format settles, to match door facings; the value also being
  the surface normal a texture uses argues against it.
- **`passable_at` and `block_allowed` in the generators.** The sim uses `crossable` for getting through a wall and
  `blocking` for something in the way; dungen and catagen use `passable_at` for a cell an actor can be in and
  `block_allowed` for where a staircase may go. Both are cell questions rather than edge ones, so it is not the same
  rename, but the word is the one the sim dropped.

## Loose

Not ordered, worth a pass: `Scheduler::advance` integer overflow and its `operator[]` on `m_speeds`.
