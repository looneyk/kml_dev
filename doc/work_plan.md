# Work Plan

Status: stages 1 to 5 complete; stage 6 all but one item; stages 7 and 8 in progress
Date: 2026-08-28, revised 2026-09-16

The ordered work queue from the session 2 evaluation. `game_system_principles.md` owns the phased
plan and the principles behind it; this is the near-term ordering within Phase 3 and the run up to
real-time. What is built goes to `game_system_reference.md`.

A stage large enough to need its own detail gets a plan of its own: `intent_plan.md` covers stage 2,
`layout_plan.md` stage 4, `view_plan.md` stage 5, and `attachment_plan.md` and `glyph_plan.md` two
items of stage 6, all complete. `dungen_plan.md` and `catagen_plan.md` cover the two generators, also
complete. `placement_plan.md` is not started, and it is not a stage here --- it is the shape
placement takes whenever it is built. `site_plan.md` is likewise not a stage: it is where every entity
is and what it belongs to, and it replaces the shape `attachment_plan.md` built. It is complete.
`discrete_layout_plan.md` covers stage 8, with `discrete_layout.md` the format it builds to and
`discrete_layout_rework.md` the reading of the current code against it.
`addressing.md` specifies ids and addressing --- geometry, entities, attachments, references --- and
`addressing_plan.md` is the design of the part of stage 8 not built.

**Open items live here**, under `Open` below, rather than in the plan that raised them. A plan marked
complete is not somewhere anyone looks, and its own list going quiet is what made the residue
invisible.

## Plans

One box per plan that keeps steps of its own, checked when every step in that plan is done.

- [ ] [`addressing_plan.md`](addressing_plan.md) --- ids and addressing: format 0.4, persistent ids,
      links, loading a zone against them, and a zone actor index. 9 steps, steps 1, 2 and 3 done.
- [x] [`site_plan.md`](site_plan.md) --- where every entity is and what it belongs to: the parent and
      site links, `Relation`, and the file component that replaces `attached`.
- [ ] [`placement_plan.md`](placement_plan.md) --- what fills a zone: sites, selection, distribution,
      acceptance and emission. 4 steps, none started.
- [x] [`intent_plan.md`](intent_plan.md) --- stage 2, intent and the schedule.
- [x] [`layout_plan.md`](layout_plan.md) --- stage 4, the layout interface and its structures.
- [x] [`view_plan.md`](view_plan.md) --- stage 5, the views and the reference frames.
- [x] [`attachment_plan.md`](attachment_plan.md) --- stage 6, doors and what hangs on walls.
- [x] [`glyph_plan.md`](glyph_plan.md) --- stage 6, glyph sets that load from a file.
- [x] [`dungen_plan.md`](dungen_plan.md) --- the dungen generator.
- [x] [`catagen_plan.md`](catagen_plan.md) --- the catagen generator.

`discrete_layout_plan.md`, `discrete_layout_rework.md`, `addressing.md`, `discrete_layout.md` and
`sim_zone_model.md` carry no steps of their own; what they specify is tracked by the stage items below.

## Stage 1 --- one place for it

- [x] **Extract `Session`** out of `Application` and `StratumWorld` into `stratum_sim`: world,
      scheduler, clock, player, zone loading. A host becomes input mapping and drawing.
      Prerequisite for everything below that touches the clock.

## Stage 2 --- Intent

Detail in `intent_plan.md`.

- [x] **`Intent` component; clock reads it; hosts write it.** Turn-based, behavior unchanged, checked
      against the existing reference frames.
- [x] **Retire the empty-command path** and the stuck-actor return from `Clock::run`. Lands with the
      step above.
- [x] **Cost from the result** rather than a flat `Scheduler::action_cost`.

## Stage 3 --- real-time

- [x] **Compose topology with occupancy on `World`** --- see Decisions below.
- [x] **Time driver in `Clock`**, fractional accumulator inside it, driver chosen from world data at
      load rather than by the host. Passed to `Session::load` until a zone supplies its own.
- [x] **Real-time movement** --- a held intent goes on producing steps as they are earned. Timing is
      continuous, space is not: sub-cell movement is `ContinuousLayout`, in Phase 7.
- [x] **AI writes intent**, and a second actor exists. `Pursue` and `think` in `ai.cpp`, one
      pursuer placed by `Session::load`.

## Stage 4 --- the layout hierarchy

Sooner rather than later, so that what is being built is stated rather than inferred. Naming the
classes is what stops the one that exists from quietly serving as all of them.
 `sim_zone_model.md` sets out the two axes and what each name means, and which of them earn a class.

Detail in `layout_plan.md`.

- [x] **`Layout` becomes the interface**, and what exists today became `DiscreteLayout` behind
      it. The rules layer, the AI and `World` go through the interface; what genuinely counts cells ---
      the loader, the cell reduction both views consume, the two host status lines, `Session`'s
      opening placement and `Interact` --- goes around it with `layout_as`, and declares it.
- [x] **`ContinuousLayout`**, declared and not written --- a skeleton with nothing behind it is
      the point: it marks what is absent rather than leaving it to be guessed. Two classes under
      `Layout` and nothing else: geometry is not a second axis of classes, since a zone with no
      height data loads a constant floor rather than a different class, so which data a game style
      loads is what differs. Neither name mentions `Flat` for that reason.
      `decision_rationale.md` argues why that is data and continuous movement is behaviour.
- [x] **Flesh out `DiscreteLayout`** as far as it can go, since it is the one with an
      implementation and the one the others are read against. `Flat` became the height field the
      model asks for --- a constant until sampled, over a grid of the surface's own, a sample absent
      where the surface is not there --- and `cell_position` reads it, so a cell sits on the ground
      rather than on the grid's datum. What is left in it is doors and the wall index, both stage
      6's.

## Stage 5 --- the view axis

Independent of stages 2 and 3. `catagen` moved the first of these from validation to necessity: it
emits walls and doors between open cells, and `cells` reduces both sides of one to `Floor`, so a
catagen zone renders as open ground a mover cannot cross. The reason is on screen nowhere.

Detail in `view_plan.md`.

- [x] **Wall-aware ASCII view**, on a 2N+1 grid --- the same shape the wall format already writes.
      `lattice` is the reduction and `render_lattice` the mapping, the terminal host draws it, and
      a catagen zone is playable: every wall that stops a mover is on screen. Checked twice over, by
      a round trip against the file and by a fourth reference frame.
- [x] **`Interact` asks the layout what is in front** instead of doing cell arithmetic.
      `Layout::ahead` is the query; the rules layer refers to no cell and depends on no concrete
      class, and the snapping of a facing to a direction the layout owns went with it.
- [x] **Derive the `View` interface** from three consumers rather than two --- and there is none to
      derive. What the three share is a convention already explicit in the code: a reduction to a
      `Buffer<Place>`, then a mapping to a `Buffer<T>` a device emits. Nothing dispatches, so an
      interface would be a vtable with no call site. The consumer that would settle it is the
      first-person 3D view, which produces mesh instances rather than a buffer.
      `view_plan.md` makes the argument.

## Stage 6 --- what the wall format surfaced

None of it gates stage 5. Several of these were done ahead of their place, as the work they fell
out of reached them.

- [x] **Doors.** A door is an entity with a facing on each of two walls, and whether a mover may
      cross is what it decides. Both formats load theirs, they start shut, and space opens one.
      `Wall::passable` is gone: nothing on the geometry marks a wall crossable, the answer coming
      from what is attached to it. `attachment_plan.md` gives the shape --- the mechanism is
      attachments generally, a door being the first of them, and what is left there is the view
      showing a door's state.
- [x] **State beside the type, and a glyph set that loads.** `PlaceType` gained `DoorClosed` and
      `DoorOpen` so a shut door draws apart from an open one, which multiplies the enum for every
      state every attachment can be in. A position reports what it is and what state it is in
      separately, the state named after the component data it comes from; the four-bit neighbour
      mask joins the key so walls and corners resolve alike; and the mapping to characters becomes
      a table a file can supply rather than a struct and a chain of conditions.
      `glyph_plan.md` gives the shape.
- [x] **Staircases state where they lead.** Both generators write each staircase as a `staircase` entity
      attached to the wall on its open side, with a `relocator` whose `target` is `down` or `up` as its glyph says,
      and `stratum_format` reads and writes it as content. Loading a zone (stage 8) builds the relocator
      from it.
- **Relocating through a relocator** --- an entity with a relocator moving an actor to its target, and
      both ends binding to each other --- is [`addressing_plan.md`](addressing_plan.md) step 8, boxed
      under Plans. `addressing.md` (`### Relocators and access points`) specifies the two kinds of target.
- [x] **Suites for both generators, and the drivers out of `application/`.** The drivers were named
      as tests and were not tests. They are `tool/dungen` and `tool/catagen` now, taking the plain
      names --- the `quetzal_` prefix on the library targets is the distinction, as it already is
      everywhere else --- and `test/dungen_test` and `test/catagen_test` are the suites.

      Each compares checked-in reference zone files byte for byte, in the style of the stratum reference
      frames, and reads each one's own header back into parameters to regenerate it. The format
      rests on a written header reproducing its zone, so a parameter a writer emits and the reader
      does not recognise fails there rather than passing silently.

      A checked-in zone file is only as good as the check over it: a generator change can leave one
      unproducible by the generator it names, and nothing but a byte comparison notices.

- [ ] **The loader builds one zone.** A file of either format is one `Zone` whatever is in it --- a
      zone spans rooms, and a room is a label with nothing built for it. Its walls are continuous
      outlines wound locally counterclockwise about the open space they face, all of them alike,
      with none distinguished from the rest. See `sim_zone_model.md`. The two `Flat`s bounding a
      zone hang off this, so it needs settling before floors and ceilings have anything.
- [x] **`region` is gone from `stratum_sim`.** The per-contiguous-area aggregate was not room
      metadata and was deleted rather than renamed. `Room` in `Room.hpp` defines what a room is ---
      an outline, the cells it covers, a zone id and an entity id --- and `Layout` keeps a
      `room_store` beside its walls with `create_room`, `room_at` and `set_cell_room`.
      `Wall` refers to no area at all: `region_id` went with it, room membership being the room's
      to state.
- [x] **The loader builds rooms.** dungen writes room floor and corridor floor as different
      symbols, which the reader had been discarding; one `Room` per contiguous run of room cells
      comes from that. catagen has one open-cell symbol and so builds none until its format
      defines rooms --- a zone that defines none loads with none rather than guessing.
- [x] **Index the walls by cell edge.** A wall is a bit on the cell it faces and its id encodes the
      cell and the side, so `wall_toward` is a bit test rather than a scan of every wall.
- [x] **Every outline alike.** Walls are created an outline set at a time with none
      distinguished, and containment counts how many outlines enclose a point.
- [x] **Dropped the loop enumeration, kept the connectivity.** `region_walls`, `ring_walls` and
      `ring_outline` are gone --- a `Room` owns its outline outright, so nothing walks a loop.
      `next_id` and `prev_id` stay as they are, a wall taking its far end from `next_id`.

Background for these is in `decision_rationale.md`.

## Stage 7 --- the session lifecycle

**In progress.** The entry points are built; the lifecycle around them is not. A session today
constructs one zone and the player together and has no way to reach a second.

- [x] **Three entries, one of them the general case.** `create` takes a `Generator` and builds a
      zone whole, which is how one normally arrives; `load` reads a file, which is a debug and test
      route rather than how a game works; `populate` takes geometry already built and is what the
      other two are written in terms of, so no route can drift from another.
      `placement_plan.md` gives the shape of what fills a zone.
- [ ] **The player belongs to the session, not to a zone.** A session has a world and a player
      ready to go before it has any zone. Today `populate` places the controlled entity and
      `create_player` runs as part of building a zone's contents, so the player cannot outlive the
      zone it was made in --- which is what a transition requires.
- [ ] **A zone is added to the world when it is encountered.** At startup or on a transition, the
      zone has its layout created, detailed and populated, and is added to the world; then it is
      playable. `World` already holds many zones, but `Session` has a single-zone `loaded()` shape
      and no transition path.
- [ ] **There is never a new session.** One session for the whole game, cleared on a new-game
      selection or on death under a permadeath option. There is no `clear` today.

The three are one change rather than three: the player cannot move to the session until a zone can
be added to it, and neither matters until a session outlives the zone it started with.

## Stage 8 --- the discrete layout format

**In progress.** One JSON zone file for both generators, with rooms, flats, styles and entities
the two grid formats could not express. Detail in `discrete_layout_plan.md`.

- [x] **Settle the structures and read the current code against them.** `discrete_layout_rework.md`
      is the reading: the interface, the view, the entity side and the session are untouched, and
      the change is concentrated in `DiscreteLayout` and `load_layout`, most of it deletion.
- [x] **The format, with nothing behind it.** `quetzal_stratum_format` reads a file into a
      `Document`, validating its geometry and keeping its content as written, and writes one back in
      the written form. `stratum_format_test` checks every rejection and that the round trip is equivalent.
- [x] **The generators write 0.3.** dungen and catagen write through `stratum_format`, their suites
      check reference zone files are equivalent and every generated zone is one the reader accepts.
- [x] **Staircases have an open side.** Both generators draw it from the sides with floor beyond, a side
      with a door only where there is none, and a staircase placed blocking a doorway first moves one cell
      in a random available direction. `discrete_layout_rework.md` (`### Staircase placement and facing`)
      has how it is chosen.
- [x] **Every zone has its access points.** `layout.components.access_points` is required: at least one
      reference to an entity with a relocator, attached to the open side of a cell written `<`, the
      entrance glyph of any non-local transporter, or `>`, as a tower entered from below has, whatever the
      entity's type, and a file missing one is rejected. The caller arriving chooses among them by what it
      arrives through. Both generators list every staircase they place; `validate` rejects staircase counts
      whose minimums are both zero and `generate` throws where none could be placed. Once a layout is
      read, `down` into it is a real target.
- [x] **Doors have entities.** Both generators write each door glyph as a `door` entity with a
      `door_facing` child on each wall `stratum_format::door_walls` finds for that glyph, and every
      hand-drawn zone, the assets included, has one per door. The generator suites check it over their
      seed sweeps and the sim and view suites over their zone files. The loader reads none of it yet; loading
      a zone is what builds from it.
- [x] **Counts of staircases each way.** dungen's `StairPlacements` is gone, and in both generators
      `staircase_down_count` and `staircase_up_count` are closed ranges, `--stairs-down` and
      `--stairs-up` on the command line. dungen writes a staircase leading up as `<`, as catagen did.
- [x] **The structures, then the reader filling them.** `Cell` has the wall mask, `Wall` and `Room`
      are gone, and `load_layout` builds a `DiscreteLayout` from a `Document` through the same
      `stratum_format` functions that validate it. Every zone file a suite or a host loads is 0.3.
- [x] **A `DiscreteLayout` takes its grid at construction.** An optional means absent, never "set
      later". The constructor takes the origin, the scale and the extent and sizes the cell table;
      `set_grid` is gone, `m_grid` is a plain `GridMapping` and `grid()` asserts nothing. `clear` went
      with it, a layout being destroyed with its zone or moved over, and the suite checks a move
      instead.

A stair cell is a block with a wall on each side (`addressing_plan.md`, `## Decided`). These four items
are that change to the geometry, done on 0.3 ahead of format 0.4, which then moved a staircase from its
cell's floor to the wall on its open side.

- [x] **Stair cells are blocks in `stratum_format`.** `floor_stair_cell` and `floor_stair_door_cell` are
      `floor_cell` and `floor_door_cell`, and `has_wall` gives floor and doorway cells in the block
      notation a wall toward a stair cell. The wall notation requires a wall between a floor cell and a
      stair cell, as between floor and a blank cell, and none between a stair cell and anything else. A
      room's location is a floor cell, the room fill and a perimeter's cells leave stair cells out, a
      perimeter edge needs floor or a doorway on one side, a wall address's cell is a floor or door
      cell, and `door_walls` finds walls on floor alone. `stratum_format_test` has a case for each, and
      its zone files route their perimeters around their stair cells and wall them in.
- [x] **`load_layout` reads a stair cell as a block,** closed, with the floor beside it walled toward it,
      so no actor moves onto it. Until a staircase is attached to its open side and lets an actor cross
      (`addressing_plan.md` step 5), none can enter one. The hand-drawn sim and view zone files have their
      entrance moved into the solid ring beside the cell it stood on, which leaves their floor as it
      was; the catagen zone files among them have their stair cells walled in. The view suite's round trip
      reads `<` and `>` as solid, its lattice case for a bare corner uses the vault, and the reference
      frames change only at stair cells.
- [x] **The generators face a staircase toward a floor neighbor and wall it in.** The facing, the open
      side, is drawn from the sides with floor beyond, a stair cell not counting, with a door only where
      there is none. catagen's `Level` keeps its walls as generated, and its rows write a wall on every
      edge between a stair cell and floor and none on the stair cell's other edges; dungen's block
      notation needs nothing written, and its rooms and barriers leave stair cells out. Both suites'
      open side checks follow, taking what the written zone no longer shows about a stair cell from the
      zone generated again.
- [x] **Staircase placement keeps every passage open.** Each generator passes over a candidate whose
      block would cut a passage, its passable neighbors no longer reaching one another around it, or
      would leave it or a staircase beside it nothing passable to face, a door counting as passable, and
      moving a staircase off a doorway holds its destination to the same rule. cave1, whose candidates
      are its whole open region, is held to it like the others. Both generator suites check over their seed sweeps that every floor cell
      is reachable from every other with the stair blocks in place, maze1 excepted until its unreachable
      corridor, under Revisit later, is done. Only the catagen references changed, by the walls around
      their stair cells.
- [x] **Format 0.4 in `stratum_format`** ([`addressing_plan.md`](addressing_plan.md) step 1). One
      `entities` list replaces `attachments`, each entity with an id, a type and file components, of which
      the format defines `attached` and `parent` and passes the rest through. A wall address is a
      `cell_position` and a `normal` written apart, every cell position in the format is `cell_position`,
      a barrier is an object, and a saved zone's `zone` section makes every id in the file persistent.
      `references.hpp` finds and translates references, `{ "entity": id }`, wherever they are written.
      The reader checks entity ids, placement, cycles, references, and that each door glyph has one entity
      with a child on each wall of its doorway and each stair glyph one entity with a relocator on a wall
      around it. `layout.components.access_points` replaces `entry`, as references to those relocators.
- [x] **The generators and every zone file in 0.4** ([`addressing_plan.md`](addressing_plan.md) step 2).
      Both generators write a `door` entity with a `door_facing` child per wall, and each staircase
      attached to the wall on its open side with its relocator, ids in the order written so the same
      parameters give the same ids; `facing` is gone. The generator, sim and view suites read the new
      list, and the reference zone files, the sim and view data, the assets and the tool samples are all 0.4.
      No frame changed.

The rest of stage 8 is [`addressing_plan.md`](addressing_plan.md), boxed under Plans above: persistent
ids, loading a zone, migration, saved zones and the zone actor index.

## Done ahead of the plan

Work the stages above did not call for, reached by what they turned up.

- [x] **One wall question in the layout, and the predicate passed rather than held.**
      `Layout::set_crossable`, `m_crossable` and the protected `Layout::crossable` are gone;
      `move`, `neighbors` and `visible` take a `crossable_type`, and `World` binds it to the zone at
      each of them. `DiscreteLayout::blocked` is `crossable(cell_position, direction, crossable_wall)`:
      no wall is a way through, a wall is the predicate's to answer for, and the grid boundary is no
      longer a case of its own, floor being enclosed and absence of a wall asserting a cell beyond.
      `World::neighbors` and `World::visible` join `World::move`, so a rule asks the world rather than
      reaching for the layout, and nothing outside `World` binds a predicate. Ahead of
      `addressing_plan.md` step 5, which adds the actor the answer depends on.
- [x] **Two words for the two questions.** Getting through a wall is `crossable` everywhere: the
      `Crossable` tag, `crossable(zone, wall_id)` asked of a wall, `crossable(cell_position, direction,
      crossable_wall)` asked of a step, and the view's `State::Crossable` with `crossable` as its glyph
      name. Something in the way is `blocking`: the `Blocking` tag, `Layout::blocking_type`, and
      `blocking_wall_id` and `blocking_entity_id` on `MoveResult` and on the `Blocked` event.
      `Passable`, `passable` and `obstructed` are gone from the sim and the view.
- [x] **A `GridPosition` is a position and a `discrete_vector_type` is a displacement.** `Cell` was
      serving as a coordinate, a step and a count at once. The three are separate now:
      `GridPosition` is `long i, j` and moves only by adding a displacement, subtracting two
      produces one, and `Grid::extent` is one. Rationale in `decision_rationale.md`.
- [x] **`math::DiscreteVector<T, N>`**, templated on a signed integral type and a dimension, beside
      `math::Vector`. `dungen::Vector` was the same class under another name and is gone; dungen's
      `position_type`, `dimensions_type` and `vector_type` alias the shared one, and dungen takes
      `id_type` from `common/id.hpp` rather than defining its own.
- [x] **`direction` in `math`.** A member reducing a discrete vector to its unit step, a free form
      over one, and a free form over a `math::Vector` whose dimension must match.
- [x] **`Direction`**, the four steps of a flat grid as named constants.
- [x] **`Rotation`** replaced `Turn`, its values `CCW`, `CW` and `Half` naming rotations rather
      than directions.
- [x] **`geometry::MinProjection` in place of hand-rolled projection.** Templated on the cartesian
      type, so it takes a point or a vector and returns the same. `Room`'s containment test and
      `grid_step` both use it and take their types from it; the `footprint` aliases and helper are
      gone.
- [x] **The queries take a direction.** `wall_toward(position, direction)` and
      `blocked(position, direction)` replaced the two-cell forms, which only ever used the
      difference. Nothing refers to a cell that need not exist.
- [x] **`Grid::contains`**, the one place a position is tested against the grid, and asked in
      assertions only. The loader walls every boundary cell, so a step off the grid cannot be
      taken and there is nothing to branch on. `cell_at` stays total.
- [x] **Openness is layout data.** `open` and `set_cell_open` over a table of their own, filled by
      the loader from what each format encodes per cell. Nothing reads room membership to decide
      whether a mover may enter somewhere.
- [x] **No reserved value for an absent cell.** `Session`'s scans return `std::optional<Cell>`.

## Decisions

**Topology and occupancy compose on `World`.** Nothing merges them today, so entities do not block
each other. `sim_zone_model.md` had `Layout::move` consulting both and marked it settled;
`game_system_reference.md` has a `Layout` independent of entities, which is what the code does.
Neither is where it belongs. `World` already owns both the layouts and the entities, so it is the one
place that can resolve it without giving `Layout` an entity view or making the rules layer re-walk a
path the layout just walked. `Layout` stays pure geometry.

Continuous movement is what forces a single answer rather than a sequence: colliding with a wall and
colliding with an entity are one solve, since a blocker partway along a swept path changes where the
slide against the wall happens. It also settles that a pushed boulder blocks the way a monster does,
which is what would otherwise diverge between the two layout models.

`sim_zone_model.md` has been corrected to match.

## Phase order

A minimal first-person 3D view comes just after Phase 3, ahead of content and procgen: it is the
second genuinely different view, and it validates the wall model before procgen generates against
it. Stage 5 is what makes it cheap. `game_system_principles.md` places it at Phase 3b, with the
rest of the Godot 3D view left in Phase 6.

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

Questions a stage left behind rather than work it owes. They are here rather than in the plan that
raised them, because a plan marked complete is not somewhere anyone looks.

### Attachments and doors

- **What a smashed door leaves behind is not the door's to decide.** Transform is the mechanism, but the
  outcome resolves what hit it against what it is made of --- an axe and a charge differ as much on
  one door as an oak door and a portcullis differ under one axe. `sim_zone_model.md` already settles
  that as script-determined over an engine default, so the structure owes only that the script can
  reach both ends: what the door is built of, and what the damage was.
- **Where damage accrues.** An axe is applied to a facing and a blast is centred in space, reaching
  many walls and everything on them. That argues damage accrues per facing --- the side chopped is the
  side that shows it --- with the shared state changing only when the door as a whole gives way.
- **A breach is the same shape as a smashed door.** A wall blown through has an attachment with
  `Passable`, so no wall record is removed, ids stay stable and `crossable` needs no new case. `Wall`
  does have a `deleted` flag that keeps its slot, but that erases geometry a blown wall needs to
  keep, being something a player should see.
- **Whether a facing without a door is worth having.** A wall's surface treatment is per side, and
  that is a facing with nothing behind it. If every wall face becomes an entity the count argument
  that kept walls out of the entity store applies again, so surface probably stays on the record and
  only *interesting* facings become entities.
- **Where a trap belongs.** On the door, or per facing, since the side you trigger it from and the side
  it fires into need not be the same.
- **Whether `Concealed` blocks sight as well as movement.** A secret door reads as wall, and a wall
  stops a sight line --- so `visible` needs the same query, which is an argument for the predicate
  being about the wall rather than about crossing it.
- **What a swinging door needs, if anything.** A facing has no direction and the wall determines its
  orientation, so which space a door opens into is either not modelled or is one more field on the
  common part describing a facing. Nothing needs it until something is drawn or blocked by a leaf
  held open.
- **Whether a door can shut behind a mover.** One door on a doorway block means being inside it is
  never being between two doors, so the trap that shuts a mover in needs something else --- a door
  that closes while occupied, and a block whose backs stop being free to pass. Far future; nothing
  is shaped against it.
- **How a mover selects which adjacent door it means.** Walking a corridor past an open door, there is
  no way to come to face it: a step is taken facing the way it goes, so turning toward the door is
  stepping into the doorway, and from inside a block the faces present their backs and there is
  nothing to shut. Closing it means backing off and approaching from the angle that faces it.

  Acting on whatever is adjacent, in any direction, is what some games do and it does not survive a
  corridor with an open door on each side: both are adjacent, and nothing chooses between them
  without entering one of the rooms. So the interaction needs a way to select a side that is neither
  the facing nor mere adjacency, and that is unsettled. It is the cost of the terminal host having
  no turn of its own rather than anything the door model decides, but it is the door that shows it.

### Sites

- **What replaces `site` where it means a candidate place.** `site_plan.md` takes the word for where an
  entity is. Two places use it for a place a generator or a placement pass chooses among, and one term
  replaces both, the sense being the same: `placement_plan.md`, where the sites are one of the five
  roles of a run and are defined only as "a range of whatever a place is", and `catagen`'s `site_type`,
  `sites_type` and `door_sites`, which `placement_plan.md` cites as the precedent for its own use.

### Layout

- **Whether a continuous layout has `Flat` at all.** Its zone is ordinary 3D models, where a
  floor is geometry like any other rather than a bounding surface with a role. `Flat` may be a
  discrete convenience that does not cross, or the record both use with the sampled fields filled
  in. Nothing forces the choice until the skeleton has something behind it.
- **`Direction`, `GridPosition`, `grid_step` and `discrete_vector_type` are in `types.hpp`,** which
  every layout includes, so the discrete vocabulary is in the neutral header. Moving it under
  `DiscreteLayout.hpp` is the honest arrangement and touches every file in the library; it is not
  worth doing on its own and is worth doing alongside something else.
- **Whether rooms rise to `Layout`.** A room is `room_id` on a `DiscreteLayout`'s cells and an entity
  with its components. The entity is neutral and the cells are not, so it stays on `DiscreteLayout`
  until something continuous needs one.
- **`Flat` has no attachments yet.** An attachment is one mechanism over walls and flat cells alike;
  loading a zone (`discrete_layout_rework.md`) builds both, and until then only walls have them.
- **`DiscreteLayout::field_of_view` is declared and nothing defines it.** A shadowcasting sweep answers
  for every cell around one origin at once, where `visible` walks the ray between one pair, and
  `sim_zone_model.md` names the two as producing different results. The declaration fixes the signature
  the sweep would have; a caller of it fails to link until one is written.

### Discrete layout format

- **Perimeter validation passes some invalid perimeters.** Each cell edge is checked on its own, and a
  wall facing a floor/stair/door cell on either side satisfies one. A perimeter whose walls all face
  outward, around a pillar or solid ground, passes and encloses no floor/stair/door cell. The reader
  already determines which side of each edge is inside; a check of the enclosed region is needed to
  reject these.
- **Whether the layout interface hands back a wall id.** An address encodes into one, so it can.
  Whether a handle to a wall belongs on an interface that mentions neither cells nor walls is an
  older question.

### Intent

None of the six steps needed an answer and nothing since has forced one.

- **Does intent cover actions or only movement?** Movement and facing under the level/edge split.
  Held attacks and channeled spells would need otherwise, and that is a design question rather than
  an obvious call.
- **Do actions get durations** --- wind-up, active, recovery? If they ever do, `Intent` is the
  attachment point.
- **Does a partial intent survive save and reload?** Plausible for movement, likely wrong for
  anything else.

### View

- **Whether the Godot tile view follows.** A wall between two open cells is exactly as invisible in
  the tile view as in the terminal, so the same argument applies; what stops it being part of stage
  5 is that the tile view is Phase 3b and 6 work and nothing about the reduction blocks it.
- **What a stair renders as.** The wall format writes `>` and `<`, the loader reads both as floor,
  and stage 6 owns giving them somewhere to reach. Every generated zone has a pair, so the round
  trip compares against the file with them taken as open --- the one case it cannot check, marked in
  `catagen_reference.md` as a known limit. It goes when the loader reads them.
- **What a corner is made into, per view.** The reduction marks a joint there; the terminal picks
  a box-drawing character and a 3D view puts a column. Neither is decided, and the 3D one is the
  reason the corner is in the reduction at all rather than being invented by the mapping.

## Consider

Not planned. Revisited only if the need arises.

- **A value type of `stratum_format`'s own in place of `nlohmann::ordered_json`.** `Document` content is
  `values_type`, the JSON library's value tree, and linking it PUBLIC hands that dependency to every
  consumer. Its costs grow with the data: a linear scan of string keys per lookup, a heap allocation
  per string, array and object, deep copies, and a parser slow for its class. Its own variant of
  null, bool, integer, real, string, list and ordered map, with nlohmann private to reading and
  writing, would remove both.

## Revisit later

Set aside on purpose. No effort goes into these until one is picked up again.

- **A doorway leading into a dead end,** which has a wall on one side only: what a door glyph there
  requires, and what the generators and the reader do with one.
- **maze1's unreachable corridor.** Excavation can leave corridor that no door reaches, which dead-end pruning
  removes only by chance, and a staircase placed at its end stops the pruning, so the staircase is unreachable too
  (`up_only` seeds 1 and 40 in `dungen_test`). The suite's check that every floor cell is reachable skips maze1
  until this is done.
- **Naming a wall address's `normal` `facing`,** once the format settles. Only the name would change, to
  match door facings; that the value is also the surface normal a texture uses argues against it.
- **`passable_at` and `block_allowed` in the generators.** The sim says `crossable` for getting through a
  wall and `blocking` for something in the way; dungen and catagen still say `passable_at` for a cell a
  mover can be in and `block_allowed` for where a staircase may go. Both are cell questions rather than
  edge ones, so it is not the same rename, but the word is the one the sim just dropped.

## Loose

Not ordered, worth a pass: `Scheduler::advance` integer overflow and its `operator[]` on `m_speeds`.
