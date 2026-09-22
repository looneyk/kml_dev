# Placement --- Plan

Status: not started
Date: 2026-09-04, revised 2026-09-22

Intent only. `game_system_principles.md` has a `procgen` library for what the generators have in common. Placement is
the first code that would go there which neither generator shares, and its form decides whether `procgen` is one library
or several.

**`site` in this document is to be renamed.** `site_plan.md` takes `site` for where an entity is, and a site here is a
candidate place a run chooses among --- a wall id, a cell, a room. The replacement term is tracked in `work_plan.md`.

## Where a zone comes from

A zone is constructed whole when it is encountered, geometry and contents, from parameters and a seed; a zone file is a
snapshot of part of that construction. `Session::create`, `load` and `populate` are the entries
(`game_system_reference.md`, Session).

**Placement is part of generation, not a step after it.** A generator builds geometry and places contents in it, and a
file, where one is written, preserves the result. What a file leaves out is filled in as though generated.

## What placement is

Putting contents in a zone whose geometry is built: doors on the walls a loader reported, items in rooms, monsters on
floor cells, a torch on a wall, a trap on a doorway. One algorithm serves all of them; what varies is the type of the
site and of what is placed there.

## The zone does not appear in it

`procgen` is not designed against `stratum_sim` types. A caller with a zone extracts the sites from it and supplies a
callback that builds into it; placement sees a range of sites, a table of candidates, a random engine and counts. The
site range is the interface between them, so the same placement can run against a `catagen::Level` before a layout
exists, or against something that is not a zone.

## What one run takes

Five roles, each independent of the rest:

- **The sites** --- a range of places. Wall ids for an attachment, cells for an item, rooms for a monster group.
- **Selection** --- which candidate, drawn from a weighted table, with an item's random properties. The part closest to
  `content`.
- **Distribution over the sites** --- how many are used and which: every one, a count, one per room, a density per area,
  or clustered about a chosen site.
- **Acceptance** --- whether an item may go at a site. A torch needs a wall, a chest needs floor with space beside it, a
  monster needs neither.
- **Emission** --- what is built. The only part that touches a world, and the only part a caller writes.

Selection and distribution are kept apart, so "rare items, one per room" is the two choices it is.

## The case that already works

`create_doors` is an instance: the sites are the wall sets `load_layout` reports, the distribution is all of them,
selection is constant, acceptance is unconditional, and emission builds the door. Reimplementing it through the
interface proves the interface.

## Reproducibility

Placement is seeded, so the sites must arrive in a stable order. flecs iteration is deterministic but not ordered
(`sim_zone_model.md`), so a site range gathered from a query is sorted by the caller producing it, which the interface
states.

## What it does not cover

**Dependent placement.** A key reachable before the door it opens, a lever before the gate, a boss behind both: a
constraint solver over reachability, beyond this form.

**Weather.** No sites and no assignment, and generated over time rather than once. It shares only a random engine with
placement.

## Steps

- [ ] **1. The interface, and `create_doors` through it.** The five roles as parameters, exhaustive distribution and
      constant selection only. Nothing new is placed and no zone changes, so the reference frames check it exactly.
- [ ] **2. Weighted selection over a table**: an entry drawn from a weighted list, with its properties. The table
      itself is `content`'s.
- [ ] **3. The distributions**: a count, one per room, a density per area.
- [ ] **4. The first use that is not a door**, items or monsters, settling acceptance and emission.

## Blockers

- **Step 1's subject is being replaced.** `create_doors`, `door_walls_type` and `load_layout`'s door callback go when a
  zone is loaded from its entities rather than from its glyphs (`addressing_plan.md`, step 5). Step 1 then reimplements
  how a zone's doors are built from its file, with the reference frames unchanged.
- **Step 2 needs `content`**, for weighted selection over prototypes.
- **Where it lives is unsettled,** which decides what it may depend on and so what a site can be.

## Open

Tracked in `work_plan.md`.
