# Layout Hierarchy --- Plan

Status: complete, as stage 4 of `work_plan.md`
Date: 2026-09-01, revised 2026-09-22

Name the layout classes: the one layout class, `Layout`, becomes `DiscreteLayout` under a `Layout` interface, with a
declared `ContinuousLayout` beside it. `sim_zone_model.md` has what each name means; what is built is in
`game_system_reference.md`, and the reasoning in `decision_rationale.md`, "Layouts split on discrete versus
continuous, and on nothing else".

Behavior does not change: every case in `stratum_sim_test` passes, and the reference frames in `stratum_view_test` stay
byte for byte the same.

## The classes

There are three, split on how movement is resolved and on nothing else:

- **`Layout`** is the interface every layout satisfies: where an actor ends up, which wall is ahead, which placements
  are adjacent, which are visible, and how far one action of movement travels. It declares no cells and no walls.
- **`DiscreteLayout`** is walls on cell edges, a floor and a ceiling, and the rooms over them, and the whole of the
  discrete side, directly under `Layout`.
- **`ContinuousLayout`** is movement that integrates, through geometry that is ordinary 3D models.
  A skeleton: every member pure, nothing behind it.

## Members

`Layout` has `move`, `turn`, `face`, `same_location`, `neighbors`, `visible`, `step_distance`, and the types they need:
`MoveResult`, `Rotation` and `placements_type`. Everything else is `DiscreteLayout`'s, including every member that
writes, since a loader builds one particular kind of layout.

`step_distance` is on the interface: one action of movement is what a move is charged against, in any layout.

## Ownership

`World` stores each zone's layout as a `std::unique_ptr<Layout>`, and `create_zone(layout)` takes it with no
default. `load_layout` returns a `std::unique_ptr<DiscreteLayout>`, both notations being grids of cells.

`World::layout(zone)` returns `Layout&`. A caller that needs cells uses `layout_as<DiscreteLayout>(layout)`, a free
function and a checked cast that asserts, as `Object::cast_to` in Godot and `cast`/`dyn_cast` in LLVM are.

`Layout` has a virtual destructor and is not copyable through the base; the concrete classes keep value semantics.

`World::move` finds an occupant with `same_location` over placements, so `World` refers to no cell; the cell form of
`entities_at` remains for callers that count cells.

## Steps

- [x] **1. Rename in place.** `Layout` becomes `DiscreteLayout`, its files with it, and every caller follows.
- [x] **2. `Layout` becomes the interface**, with `MoveResult` and `Rotation` in `Layout.hpp`, the layout map of `World`
      holding `unique_ptr`, `create_zone` taking one, `load_layout` returning one, and `layout_as`.
- [x] **3. Nothing between `Layout` and `DiscreteLayout`.**
- [x] **4. `ContinuousLayout`.** Declared and not defined, each member commented with what it
      would compute and why the discrete class cannot substitute for it. Every member is pure, so nothing
      constructs one and nothing has to be defined. One skeleton rather than a pair, for the same
      reason step 3 states: geometry is not a second axis of classes, so there is nothing for an
      intermediate to declare. `stratum_sim_test` includes the header, a header nothing includes
      never being compiled, and checks that it is a `Layout` and that it is abstract.
- [x] **5. `DiscreteLayout` completed**: `Flat` as a height field, read by the cell position, with the grid arithmetic
      on the grid mapping.

## Open

The open items are tracked in `work_plan.md`.
