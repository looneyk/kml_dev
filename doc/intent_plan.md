# Intent --- Plan

Status: complete. Landed as stages 2 and 3 of `work_plan.md`
Date: 2026-08-28, revised 2026-09-02

Progress tracking for replacing the `Clock`'s pull-a-decision callback with durable intent on the
actor. Companion to `game_system_principles.md`, which owns the phased plan; what actually exists
is in `game_system_reference.md`, under Intent, Deciding for itself and Clock.

## What and why

A `Command` is an edge and there is no representation of a level. "I pressed north" exists; "I am
trying to go north" has nowhere to live. `Clock::decide_type` pulls a one-shot decision at the
moment an actor becomes ready, applies it instantly, and moves on --- the turn-based shape, and
correct for turn-based.

Two cases need the level form:

- **Actor state has to serialize.** A monster's goal --- last known player position, leash target,
  patrol index --- spans many turns. A callback re-invoked per action must cache that outside the
  ECS, where it does not unload with the zone, does not migrate through a portal, and is not in a
  save. This bites in turn-based, ahead of any real-time work.
- **Real-time is the direction.** Holding a direction is a level, and re-sending it as one-shot
  commands per tick makes the stable state transient and leaves no field for a rule to read.

This is a departure from principle 7 only in appearance: real-time is the intended direction rather
than a hypothetical second case, and the serialization argument holds for the turn-based game alone.

## Shape

`Intent` is a component on the actor for the durable part --- desired movement direction and
desired facing. The host writes it for the player; an AI system writes it for everything else. The
clock reads it rather than calling out.

One-shot actions stay `Command`s. A direction is held, `Interact` is not, so `Command` narrows
rather than disappears.

Both parts of an intent are direction vectors in world space, absolute rather than relative, and the
layout snaps them where it is discrete --- the arrangement `Layout::move` already has, rounding being
the layout's business and not the caller's. One representation covers both models: a discrete zone
quantizes the desired facing to one of its own directions, a continuous one rotates toward it at the
actor's own rate. That rate is a property of the actor rather than part of the intent, which is what
lets a snap turn and a free look share a representation --- mouse look retargets every frame, a snap
turn targets once.

`Rotation` survives as the argument to `Layout::turn` and appears nowhere in `Intent`. A first person
view pressing left genuinely has a relative turn rather than an absolute one, so it asks the layout
what lies to its left and writes the result as facing intent, the layout still owning the
quantization so that a hex zone turns sixty degrees. `TurnTo` folds into intent along with the
rest of facing, which also retires a command whose name reads absolute and whose field is relative.

The whole turn-based/real-time difference reduces to one question:

> **What does a driver do when a ready actor has no intent?**
> Turn-based: stop, hand control out, do not advance time.
> Real-time: treat it as standing still and carry on.

## Steps

- [x] **1. `Intent` component, clock reads it, hosts write it.** Turn-based only, behavior
      unchanged. Component reflection registered member by member, per the flecs obligation in
      `sim_zone_model.md`. Compared byte for byte against the existing reference frames --- this step is a
      refactor with a test, not a build-ahead.
- [x] **2. Retire the "empty command" path.** `Clock::run` stops returning the actor it is stuck
      on, and the player stops being identified by declining a callback. Closes the schedule stall
      that appears as soon as a second actor exists.
- [x] **3. Cost from the result.** `apply` derives energy from what the command achieved rather
      than returning a flat `Scheduler::action_cost`. Independent of the steps around it, and the
      mechanical blocker to a cheap per-tick step.
- [x] **4. Time driver in `Clock`.** Turn-based and real-time drivers behind one host-facing call,
      the fractional accumulator owned by `Clock` rather than by each host, and the driver chosen
      from world data at load rather than by the host. Passed to `Session::load` until a level
      supplies its own.
- [x] **5. Real-time movement.** Per-tick integration of movement intent, `Layout::move` already
      being model-agnostic in the right way. Timing is continuous and space is not: sub-cell
      movement is the continuous layout, in Phase 7.
- [x] **6. AI writes intent.** A second actor in the world, which is what proves step 1's
      serialization argument rather than asserting it. `Pursue` and `think` in `ai.cpp`, with one
      pursuer placed by `Session::load`.

Steps 1 and 2 were done together as stage 2 of `work_plan.md`, with step 3; steps 4 to 6 as stage 3.

## Open

Tracked in `work_plan.md`, which collects the open items of every plan.

## Not covered

Session extraction out of the two hosts, which gave steps 1 and 4 one place instead of two.
It was done first, as stage 1 of `work_plan.md`.

The view-side findings from the same review --- the grid-shaped view contract, the grid arithmetic in
`Interact`, the entity-blocking contradiction between the two design documents --- are separate work
and are not tracked here. Two are settled: topology and occupancy compose on `World`, which
`decision_rationale.md` makes the argument for, and the grid arithmetic is out of `Interact`, which
now asks `Layout::ahead`. The grid-shaped view contract is stage 5 of `work_plan.md`.
