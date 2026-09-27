# Work Plan

Status: stages 1 to 6 complete; stages 7 and 8 in progress
Date: 2026-08-28, revised 2026-09-26

This is the ordered work queue within Phase 3 and toward real time. `game_system_principles.md` has the phased plan and
its principles, and `game_system_reference.md` describes what is built.

A stage large enough to need its own detail has a plan of its own: `intent_plan.md` for stage 2, `layout_plan.md` for
stage 4, `view_plan.md` for stage 5, and `attachment_plan.md` and `glyph_plan.md` for two items of stage 6.
`dungen_plan.md` and `catagen_plan.md` cover the two generators. `placement_plan.md` is the form placement takes when
it is built, and `site_plan.md` describes where every entity is and what it belongs to; neither is a stage.
`modding_plan.md` is the design goal that every stage is held to: everything a game adds is data or script.
`session_plan.md` describes how the parts of `StratumGame` divide stage 7's work. `discrete_layout_plan.md` covers
stage 8, `discrete_layout.md` is the format it builds to, and `discrete_layout_rework.md` is the design of loading a
zone from it. `addressing.md` specifies ids and addressing --- geometry, entities, attachments and references --- and
`addressing_plan.md` is the design of the rest of stage 8. `region_plan.md` is the design of regions, which relocating
between zones needs.

**Open items are listed here**, under `Open` below, rather than in the plan that raised them.

## Next

These are the remaining steps in the order they are built, each building on the ones before it. A step leaves this
list when it is done.

1. **`region_plan.md` steps 1 to 4,** regions and levels, which an unbound relocator's level offset is resolved against.
2. **`addressing_plan.md` step 8,** relocating through a relocator.
3. **`session_plan.md` step 6,** entering an access point.
4. **Stage 7, a zone added to the world when it is encountered.**
5. **`addressing_plan.md` step 7,** saved zones and the reset to a save point, with the rest of stage 7.

Outside that order:

- **At any time:** `session_plan.md` step 4, pre-populated zones, where the loader builds a file's denizens.
- **Blocked:** `placement_plan.md`, since where it lives is unsettled and its step 2 needs `content`; the population
  strategy comes with it.
- **At any time:** `addressing_plan.md` step 9, the zone actor index; `session_plan.md` step 7, staircases in every
  form; and `modding_plan.md` step 1, place types from data.
- **At any time:** the generators on `geometry::DiscreteDirectedPoint`. dungen's `Staircase` has a
  `directed_point_type` and still has the separate `position` and `direction` accessors. catagen stores a staircase as a cell type and a map from cell position to facing direction, which a list of
  directed points would replace.
- **Later:** unloading by region and region files (`## Regions`), and `modding_plan.md` steps 3 to 7, which are
  Phase 4.

## Plans

There is one box per plan that has steps of its own, checked when every step is done.

- [ ] [`addressing_plan.md`](addressing_plan.md) --- ids and addressing: format 0.4, persistent ids, links, loading a
      zone against them, and a zone actor index. 9 steps, steps 1 to 6 done.
- [x] [`site_plan.md`](site_plan.md) --- where every entity is and what it belongs to: the parent and site links,
      `Relation`, and the file component that replaces `attached`.
- [ ] [`modding_plan.md`](modding_plan.md) --- mods and data-driven design: place types from data, components decoded
      at load, types as prefabs, component schemas from data, styles, scripting and mods. 7 steps, step 2 done, with
      `addressing_plan.md` step 5; steps 3 to 7 are Phase 4.
- [ ] [`session_plan.md`](session_plan.md) --- the division of work among `StratumGame`'s parts: `World` creating,
      loading, populating and unloading zones; `Session` entering them and keeping the state of play; each zone's
      entry point, the lowest-ordered staircase leading up; and entering an access point on a wall or the floor. 7
      steps, steps 1 to 3 and 5 done.
- [ ] [`region_plan.md`](region_plan.md) --- regions: the region entity with its parent region, a zone's level, an
      unbound relocator's level offset, and finding or making the zone at an offset. 4 steps, none started.
- [ ] [`placement_plan.md`](placement_plan.md) --- what fills a zone: sites, selection, distribution, acceptance and
      emission. 4 steps, none started.
- [x] [`intent_plan.md`](intent_plan.md) --- stage 2, intent and the schedule.
- [x] [`layout_plan.md`](layout_plan.md) --- stage 4, the layout interface and its structures.
- [x] [`view_plan.md`](view_plan.md) --- stage 5, the views and the reference frames.
- [x] [`attachment_plan.md`](attachment_plan.md) --- stage 6, doors and what is attached to walls.
- [x] [`glyph_plan.md`](glyph_plan.md) --- stage 6, glyph sets that load from a file.
- [x] [`dungen_plan.md`](dungen_plan.md) --- the dungen generator.
- [x] [`catagen_plan.md`](catagen_plan.md) --- the catagen generator.

`discrete_layout_plan.md`, `discrete_layout_rework.md`, `addressing.md`, `discrete_layout.md` and `sim_zone_model.md`
have no steps of their own; the stage items below track what they specify.

## Stage 1 --- one place for it

- [x] **Extract `Session`** from `Application` and `StratumWorld` into `stratum_sim`: the world, scheduler, clock,
      player and zone loading. A host is input mapping and drawing.

## Stage 2 --- Intent

Detail is in `intent_plan.md`.

- [x] **An `Intent` component, which the clock reads and hosts write.**
- [x] **Retire the empty-command path** and the stuck-actor return from `Clock::run`.
- [x] **Cost from the result** rather than a flat `Scheduler::action_cost`.

## Stage 3 --- real time

- [x] **Compose topology with occupancy on `World`.**
- [x] **A time driver in `Clock`**, with a fractional accumulator, and the driver chosen at load rather than by the
      host.
- [x] **Real-time movement**, where a held intent produces moves as they are earned.
- [x] **AI writes intent**, with a second actor: `Pursue` and `think`.

## Stage 4 --- the layout hierarchy

Detail is in `layout_plan.md`.

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

Detail is in `view_plan.md`.

- [x] **A wall-aware ASCII view**, on a 2N+1 grid: `lattice` and `render_lattice`, drawn by the terminal host.
- [x] **`Interact` asks the layout which wall is ahead**, through `Layout::ahead`.
- [x] **The `View` interface**, found unneeded: a reduction to a `Buffer<Place>` and a mapping to a `Buffer<T>` are
      enough (`decision_rationale.md`).

## Stage 6 --- what the wall notation surfaced

- [x] **Doors.** A door is an entity with a door facing on each of its walls, and the entities attached to a wall
      determine whether it may be crossed (`attachment_plan.md`).
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
- [x] **Walls are indexed by cell edge**: a wall is a bit on the cell it faces, with an id that encodes the cell and
      the side.

## Stage 7 --- the session lifecycle

**In progress.** The entries are built; the lifecycle around them is not.

- [x] **Two entries, one of them the general case.** `World::create_zone` makes a zone from a `Generator` and
      `World::load_zone` from a file, each making the zone whole, and `Session::enter` takes the player into it
      (`session_plan.md`).
- [x] **The player belongs to the game, not to a zone.** `StratumGame` owns the id allocator, the world, the player and
      the session; the player exists before any zone and is moved into each zone loaded.
- [ ] **A zone is added to the world when it is encountered.** At startup or on a transition, the zone's layout is
      created, detailed and populated, and the zone is added to the world, where it is playable.
- [x] **Zones unload past a threshold, oldest first.** Leaving a zone leaves it resident. On entering one,
      `World::trim_zones` unloads the zones entered longest ago, as the world's `ZoneEntryIndex` chooses, once the zone
      count maximum is exceeded (`sim_zone_model.md`, `### Residency, simulation, and persistence`).
- [ ] **There is never a new session.** One session lasts the whole game, and is cleared when a new game is selected,
      or on death under a permadeath option.

A session's responsibilities are open (`Open`, `### Session`).

## Stage 8 --- the discrete layout format

**In progress.** Both generators write one JSON zone file format, with rooms, flats, styles and entities. Detail is in
`discrete_layout_plan.md`.

- [x] **Settle the structures and read the code against them** (`discrete_layout_rework.md`).
- [x] **The format**: `quetzal_stratum_format` reads a file into a validated `Document` and writes it back, with
      `stratum_format_test`.
- [x] **The generators write the format** through `stratum_format`.
- [x] **Staircases have an open side**, chosen from the sides with floor beyond, or a side with a door only if there is
      none. A staircase placed blocking a doorway first moves one cell (`dungen_reference.md`,
      `catagen_reference.md`).
- [x] **Every zone has its access points**: `layout.components.access_points`, at least one reference to an entity with
      a relocator on the open side of a cell written `<` or `>`.
- [x] **Doors have entities**: each door glyph has a `door` entity with a `door_facing` child on each wall that
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

The rest of stage 8 is [`addressing_plan.md`](addressing_plan.md), boxed under Plans: relocating through a relocator,
saved zones, and the zone actor index.

## Regions

**Next.** A `Region` groups contiguous zones: a surface area, a dungeon, a crypt, a tower or a house. It is data, an
entity with a parent region that is its zones' parent, and it determines which zones unload first (`sim_zone_model.md`,
`## World, Zones, and Levels`). `region_plan.md` has the design of the region entity, a zone's level, and resolving
an unbound relocator's level offset, and the open questions are under `Open`, `### Regions`.

- [x] **"Region" out of catagen,** first. Its pipeline is rectangles, the rooms kept among them, and areas, each a room
      or a run of corridor, which walls separate and doors join. dungen's cave1 keeps "the largest connected region",
      which is harmless; `roguelike_survey.md` uses the word in its general sense, and `texture_region_size` is Godot's
      own name.
- [ ] **`Region`,** an entity with its properties as components and a parent region, and the parent of its zones in
      place of the root. The properties are a name and, for a dungeon, the overall or starting difficulty and the other
      properties a whole dungeon has. Boxed under Plans as [`region_plan.md`](region_plan.md).
- [ ] **Transparent zone and region boundaries.** Contiguous zones joined edge to edge, so an actor walks from one zone
      into the next, across region boundaries too. Until then, zones connect only through relocators: waypoints, and
      dungeons with more than one entrance.
- [ ] **A new region made on resolving,** when an unbound relocator finds nothing, instead of a zone in its own or its
      parent region (`region_plan.md`).
- [ ] **Unloading by region.** Criteria for `World::trim_zones` that choose the zones of other regions first, those
      left longest ago first, as `ZoneEntryIndex` does, and then those of the player's region, farthest first.
- [ ] **Region files.**

## Done ahead of the plan

- [x] **One wall question in the layout, with the predicate passed rather than stored.** `move`, `neighbors` and
      `visible` take a `crossable_function_type`, which `World` binds to the zone.
- [x] **`crossable` for getting through a wall and `blocking` for something in the way**, throughout the sim and the
      view.
- [x] **A cell position is a position and a `discrete_vector_type` is a displacement** (`decision_rationale.md`).
- [x] **`math::DiscreteVector<T, N>`**, beside `math::Vector`, replacing `dungen::Vector`.
- [x] **`direction` in `math`**, which reduces a vector to the unit vector along its longest axis.
- [x] **Cardinal directions** as named constants.
- [x] **`Rotation`**, with values `CCW`, `CW` and `Half`.
- [x] **`geometry::MinProjection`** in place of hand-written projection.
- [x] **The queries take a direction**: `wall_toward(cell_position, direction)` and `crossable(cell_position,
      direction, ...)`.
- [x] **`GridMapping::contains`**, called in assertions only.
- [x] **Openness is layout data**: `open` and `set_cell_open`.
- [x] **No reserved value for an absent cell**: `Session`'s scans return `std::optional`.

## Phase order

A minimal first-person 3D view comes just after Phase 3, ahead of content and procgen. It is the second genuinely
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

These are the questions that stages left behind.

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

- **The word that replaces `site` where it means a candidate location.** `site_plan.md` takes the word for where an
  entity is. Two places use it for a location that a generator or a placement pass chooses among, and one term
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
- **The access point type at which a new game enters a zone that is not a dungeon level,** such as a tower entered from
  below; `up` is the hosts' temporary `entry_access_type` (`session_plan.md`, `## Entry point`).
- **Where access point types are registered from data,** including a mod's, instead of in `World`'s constructor
  (`modding_plan.md`).
- **The scale of the probability of a staircase leading down,** which is inversely proportional to how far below the
  surface the zone's level is and proportional to `difficulty`, and is 1 for now; and what bounds it, since a region
  has no lowest level.
- **Whether the layout interface returns a wall id.** An address encodes into one, so it can. Whether a handle to a
  wall belongs on an interface that mentions neither cells nor walls is an older question.

### Intent

- **Does intent cover actions or only movement?** Held attacks and channeled spells would need more than movement and
  facing direction, which is a design question.
- **Do actions get durations** --- wind-up, active and recovery? If so, `Intent` is where they go.
- **Does a partial intent survive save and reload?** Plausibly for movement, and likely not for anything else.

### Regions

- **The properties each type of region has.** A surface area's and a dungeon's overlap but differ, and difficulty goes
  from the starting value across a dungeon's zones in a way not yet decided.
- **Distance or level difference** as the measure within the player's region, and whether it differs by type of
  region.
- **The contents of a region file:** only the region's metadata and generation settings, or those followed by its
  zones; and how it relates to the zone file format.
- **Which region the first zone of a new game is made in,** and whether the session asks the region to make it.
- **How two bound doors keep their `Access` equal** (`region_plan.md`, `## Doors between zones`). With both zones
  loaded, changing one door sets its partner too. With the partner's zone unloaded, only this side changes, and the
  two are reconciled when an actor crosses or when the partner's zone loads; which change is newer needs deciding,
  such as by the simulation time of the last change stored with `Access`.
- **Where moving down looks, and where it makes a zone.** `region_plan.md` has moving down look for an unbound
  relocator and make a zone only in its own region; whether a hole from a surface zone at level 0 finds or makes its
  zone in a child region, and which one, is not settled.

### Session

- **The pre-populated flag's name and place in the format** (`session_plan.md`), and whether it is a format version
  change.
- **The population strategy from the zone's source,** the template loaded or the generation guidance, which replaces
  `World::populate`'s placeholder (`session_plan.md`, `## World`).
- **The time a zone was entered:** simulation time or real time.
- **The game's config file,** which supplies the zone count maximum, now `StratumGame`'s temporary 8, and the game's
  other settings.
- **Whether the driver belongs to the game or to a zone.**
- **Actors.** The player and the denizens are both actors, and differ in what controls them; the actor structure is
  not designed.
- **Resident zones the player has left:** whether they are simulated or frozen, which thresholds apply --- memory
  usage among them --- and their values.
- **Zones made and never entered.** They are not candidates for unloading.
- **Where the unload order's data is kept** --- `ZoneEntryIndex`'s map from entry count to zone id, or a component on
  each zone with a flecs query ordered by it --- once regions set the criteria: other regions' zones by the time they
  were entered, and the player's region's zones by distance or level difference (`## Regions`).

### View

- **How a glyph set or tile set marks `concealed`,** a way through that the viewer has found and actors in general
  have not, whether outlined or otherwise; the built-in sets draw it like any door.
- **Whether the Godot tile view draws the lattice.** A wall between two open cells is as invisible in the tile view,
  which draws `cells`, as in the terminal's cell view.
- **What a staircase renders as.** A stair cell is a block that the views draw as solid, and the wall notation round
  trip compares `<` and `>` as solid.
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
