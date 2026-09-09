# Layout Hierarchy --- Plan

Status: complete. Stage 4 of `work_plan.md` is done
Date: 2026-09-01, revised 2026-09-02

Detail for stage 4 of `work_plan.md`, as `intent_plan.md` covers stage 2. `sim_zone_model.md` owns
the two axes and what each name means; what is built goes to `game_system_reference.md`.

## What and why

One layout class exists and it is called `Layout`, so it doubles as every layout there will be.
That is how it accumulated comments, asserts and machinery for a continuous case it does not
implement --- nothing in the name said the case was absent, so the case was written into the class
that was there. Naming the classes is what stops it.

The skeletons are as much of the deliverable as the working class. A declared
`ContinuousLayout` with nothing behind it marks continuous movement as not written; an
absent one leaves that to be inferred from a class whose name does not mention it.

Nothing about behaviour changes. Every case in `stratum_sim_test` passes as it did, and the
reference frames in `stratum_view_test` stay byte for byte what they were --- this is a refactor with
a test rather than a build-ahead.

## The classes

Three, splitting on how movement is resolved and on nothing else:

- **`Layout`** is the interface every layout satisfies: where a mover ends up, what it faces, what
  is adjacent, what is visible, and how far one action of movement travels. No cells, no walls, and
  nothing suggesting either exists.
- **`DiscreteLayout`** is what exists today --- walls on cell edges, a floor and a ceiling, and
  the rooms defined over them --- and it is the whole of the discrete side, directly under `Layout`.
- **`ContinuousLayout`** is movement that integrates, through geometry that is ordinary 3D models.
  A skeleton: every member pure, nothing behind it.

A class is earned where two cases differ in behaviour rather than in data, and geometry does not.
A level with no height data loads a constant floor, which is a `Flat` with fewer of its features
used, so a level with height and one with none load the same class from different data ---
which is why neither name mentions `Flat`. Continuous movement is not like that: a swept path collides
partway, partners cannot be found by exact edge match, and topology and occupancy solve together.
`decision_rationale.md` makes the argument.

## Where each member of today's `Layout` goes

| Goes on | Members |
|---|---|
| `Layout` | `move`, `turn`, `face`, `same_location`, `neighbors`, `visible`, `step_distance`, and the vocabulary they need: `MoveResult`, `Rotation`, `obstructed_type`, `placements_type` |
| `DiscreteLayout` | everything else: `Wall`, `Flat`, `Grid`, the three stores and their accessors, `create_walls`, `create_room`, `room_at`, `opposed`, `connect`, `disconnect`, `connect_walls`, `connect_folds`, `set_passable`, `set_flats`, `floor_id`, `ceiling_id`, `grid`, `set_grid`, `open`, `set_cell_open`, `cell_at`, `cell_position`, `wall_toward`, `blocked`, `clear`, and the two cell tables |

`step_distance` is on the interface rather than below it: how far one action of movement travels is
what a move is charged against, and a continuous layout owes that as much as a discrete one
does. It is the grid's spacing here and something else there.

`blocked` stays below the interface because it is asked of a cell and a direction. What a
continuous layout is asked instead is `move` itself, the result reporting what stopped it.

## What the interface does not get

**`position(placement)`.** The sketch in `sim_zone_model.md` has it, from when a placement was a
handle a layout resolved. It is a `DirectedPoint` and has its own point, so the call has nothing
to do. The sketch there needs correcting rather than implementing.

**Anything that writes.** `set_grid`, `set_cell_open` and the rest are how a loader builds one
particular kind of layout, and a loader picks which kind it is building. The interface is what a
rule, a view or an AI asks.

## How a layout is made and held

`World` holds `std::map<flecs::entity_t, std::unique_ptr<Layout>>`, which is what an interface
costs. Move stays defaulted, so `Session::load` still builds a `World` alongside and moves it in
once the whole load has succeeded.

**`create_zone` takes the layout.** `create_zone(name, std::unique_ptr<Layout>)`, with no
default --- a default would put one concrete class back inside `World`, which is the coupling being
undone. Tests pass `std::make_unique<DiscreteLayout>()`.

**`load_layout` returns one.** `load_layout(filepath, height_ceiling)` returns the layout rather
than filling one handed to it, so what class a zone's layout is comes from what was loaded.

It returns `std::unique_ptr<DiscreteLayout>` rather than the interface. Both formats it reads
are grids of cells, so both build that class and declaring it is the accurate signature --- a format
describing something else is a reader of its own rather than another return from this one. It also
leaves every test that loads a level and then asks it about cells reading as it did.

**Reaching the kind of layout it is.** `World::layout(zone)` returns `Layout&`. A caller that
genuinely needs cells asks `layout_as<DiscreteLayout>(world.layout(zone))`, a checked cast that
asserts --- a view asking for the cells of a zone that has none is a mistake rather than a question
with an answer. `Object::cast_to` in Godot and `cast`/`dyn_cast` in LLVM are the same arrangement.

A free function over a `Layout&` rather than a member of `World`, so that one definition suffices
`World::layout`, `Session::layout` and any other way a layout is reached.

`Layout` gets a virtual destructor and is not copyable. The concrete classes keep value semantics:
`stratum_sim_test` builds them on the stack by the dozen.

## The callers, and what each ends up with

| Caller | Asks | Holds |
|---|---|---|
| `ai.cpp` | `visible`, `same_location`, `neighbors`, `step_distance` | `Layout` |
| `commands.cpp`, `Step` and `TurnTo` | `same_location`, `face`, `step_distance` | `Layout` |
| `World::move` | `same_location` | `Layout` |
| `World::entities_at(cell)` | `cell_at` | `DiscreteLayout` |
| `Session::load` | `grid`, `open`, `cell_position` | `DiscreteLayout` |
| `stratum_view::cells` | `grid`, `open`, `cell_at` | `DiscreteLayout` |
| Terminal host status line | `cell_at` | `DiscreteLayout` |
| Godot host `extent` and `cell` | `grid`, `cell_at` | `DiscreteLayout` |
| `load_layout` | everything that writes | `DiscreteLayout` |

`World::move` currently finds what is in the way by reducing a candidate placement to a cell and
asking `entities_at`. `same_location` decides the same question without referring to a cell, so `move`
takes an occupancy query over placements and `World` stops being discrete. The cell form of
`entities_at` stays for what genuinely counts cells, the tests included.

**`Interact` was the one leak stage 4 did not close,** stepping the actor's facing into a cell
coordinate and asking which wall stood that way, which is cell arithmetic in the rules layer. Stage
5 owned the fix and has made it: `Layout::ahead` returns what is in front, so no rule depends on a
concrete class. That the leak was visible at all was the hierarchy doing its job.

## Steps

- [x] **1. Rename in place.** `Layout` becomes `DiscreteLayout`, its files with it, and every
      caller follows. No interface yet, nothing else moves, and the whole diff is one name --- which
      is what makes it checkable by eye before anything is restructured.
- [x] **2. `Layout` becomes the interface.** The neutral queries go pure virtual on it,
      `DiscreteLayout` overrides them, and `MoveResult`, `Rotation` and `obstructed_type` move
      in `Layout.hpp` with it. `World`'s layout map moves to `unique_ptr`, `create_zone` takes one,
      `load_layout` returns one, and `layout_as` appears. Callers that need nothing more take
      `Layout&`.
- [x] **3. Nothing between `Layout` and `DiscreteLayout`.** The discrete side is one class:
      movement, walls, blocking and the grid do not differ across the flat line, so an
      intermediate would be the same code twice. Which data a level has is what a game style
      chooses. `decision_rationale.md` makes the argument.
- [x] **4. `ContinuousLayout`.** Declared and not defined, each member commented with what it
      would compute and why the discrete class cannot substitute for it. Every member is pure, so nothing
      constructs one and nothing has to be defined. One skeleton rather than a pair, for the same
      reason step 3 states: geometry is not a second axis of classes, so there is nothing for an
      intermediate to declare. `stratum_sim_test` includes the header, a header nothing includes
      never being compiled, and checks that it is a `Layout` and that it is abstract.
- [x] **5. Flesh out `DiscreteLayout`.** It is the one with an implementation and the one the
      others are read against, so what is stubbed in it reads as a gap in the model rather than a
      gap in one class. `Flat` became the height field the model asks for, and `cell_position`
      reads it. What is left in the list below is owned by stage 6.

Steps 1 and 2 are one change split for review; 4 and 5 are independent of them and of each other.

## What step 5 reached, and what it left

Found in passing rather than surveyed. What another stage owns is marked.

- [x] **`Flat` is a height field.** A constant until something is sampled into it, over a `Grid` of
      the surface's own so a height map may be finer than the cells. A sample is
      `std::optional<value_type>`, absent meaning the surface is not there rather than low. That is
      what makes a level with height data and one without the same class with more or less of one
      record used, which `decision_rationale.md` rests on.
- [x] **Something reads a flat.** `cell_position` takes x and y from the grid and z from the floor,
      so a cell sits on the ground. No floor yet, or the floor absent under a cell, falls back to
      the grid's plane.
- [x] **The cell arithmetic moved to `Grid`.** `cell_at` and `cell_position` on the grid, with the
      layout delegating, since a `Flat` samples over a grid of its own and wanted the same
      arithmetic.

## Open

Tracked in `work_plan.md`, which collects the open items of every plan.
