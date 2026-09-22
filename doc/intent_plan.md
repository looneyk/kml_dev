# Intent --- Plan

Status: complete, as stages 2 and 3 of `work_plan.md`
Date: 2026-08-28, revised 2026-09-22

Replace the `Clock`'s callback, which pulled a one-shot decision when an actor became ready, with durable intent on the
actor. What exists is in `game_system_reference.md`, under Intent, ai and Clock; the reasoning is in
`decision_rationale.md`, "An intent persists on the actor; a command is one-shot".

## Shape

`Intent` is a component on the actor for the durable part: the movement direction and the facing direction asked for.
The host writes it for the player and an AI system for every other actor, and the clock reads it.

One-shot actions stay `Command`s: a direction is held, `Interact` is not.

Both parts of an intent are absolute direction vectors in world space, which the layout snaps where it is discrete. A
discrete zone quantizes the facing direction to one of its own; a continuous one rotates toward it at the actor's own
rate, a property of the actor rather than of the intent.

`Rotation` remains the argument of `Layout::turn` and is not in `Intent`. A first-person view turning left asks the
layout for the direction to its left and writes the result as the facing direction, so a hex zone turns sixty degrees.

The turn-based and real-time difference is one question: **what does a driver do when a ready actor has no intent?**
Turn-based stops, returns control, and does not advance time. Real time treats it as standing still and continues.

## Steps

- [x] **1. `Intent` component; the clock reads it; hosts write it.** Turn-based only, behavior unchanged, reflection
      registered member by member, checked against the reference frames.
- [x] **2. Retire the empty-command path.** `Clock::run` stops returning the actor it is stuck on, and the player is no
      longer identified by declining a callback.
- [x] **3. Cost from the result.** `apply` derives the cost from what the command achieved.
- [x] **4. Time driver in `Clock`.** Turn-based and real-time drivers behind one call, the fractional accumulator in
      `Clock`, and the driver chosen at load rather than by the host.
- [x] **5. Real-time movement.** Movement intent integrated per tick; movement within a cell is the continuous layout,
      in Phase 7.
- [x] **6. AI writes intent**, with a second actor: `Pursue` and `think`.

Steps 1 to 3 were stage 2 of `work_plan.md`, and steps 4 to 6 stage 3.

## Open

Tracked in `work_plan.md`.
