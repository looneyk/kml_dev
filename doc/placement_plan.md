# Placement --- Plan

Status: not started
Date: 2026-09-04, revised 2026-09-26

This plan states intent only. `game_system_principles.md` has a `procgen` library for the code that the generators
have in common. Placement is the first code that would go there which neither generator shares, and its form decides
whether `procgen` is one library or several.

**`site` in this document is to be renamed.** `site_plan.md` uses `site` for where an entity is, and a site here is a
candidate location that a run chooses among: a wall id, a cell, or a room. The replacement term is tracked in
`work_plan.md`.

## Where a zone comes from

A zone is constructed whole, geometry and contents, from parameters and a seed when it is encountered; a zone file is a
snapshot of part of that construction. `World::create_zone` and `load_zone` are the entries, and `World::populate`
places a zone's denizens (`game_system_reference.md`, World).

**Placement is part of generation, not a step after it.** A generator builds geometry and places contents in it, and a
file, if one is written, preserves the result. Anything a file leaves out is filled in as though it were generated.

## What placement is

Placement puts contents in a zone whose geometry is built: doors on the walls a loader reported, items in rooms,
denizens on floor cells, a torch on a wall, or a trap on a doorway. One algorithm handles all of them; only the type of
the site and of the object placed there varies.

## The zone does not appear in it

`procgen` is not designed against `stratum_sim` types. A caller with a zone extracts the sites from it and supplies a
callback that builds into it; placement sees a range of sites, a table of candidates, a random engine and counts. The
site range is the interface between them, so the same placement can run against a `catagen::Level` before a layout
exists, or against something that is not a zone.

## What one run takes

A run has five roles, each independent of the rest:

- **The sites** --- a range of candidate locations: wall ids for an attachment, cells for an item, or rooms for a group
  of denizens.
- **Selection** --- which candidate is drawn from a weighted table, with an item's random properties. This is the part
  closest to `content`.
- **Distribution over the sites** --- how many sites are used and which: every one, a count, one per room, a density
  per area, or a cluster about a chosen site.
- **Acceptance** --- whether an item may go at a site. A torch needs a wall, a chest needs floor with space beside it,
  and a denizen needs neither.
- **Emission** --- what is built. It is the only part that touches a world, and the only part a caller writes.

Selection and distribution are kept apart, so "rare items, one per room" is the two separate choices it is.

## Reproducibility

Placement is seeded, so the sites must arrive in a stable order. flecs iteration is deterministic but not ordered
(`sim_zone_model.md`), so the caller that gathers a site range from a query sorts it, as the interface states.

## What it does not cover

**Dependent placement.** A key reachable before the door it opens, a lever before the gate, and a boss behind both need
a constraint solver over reachability, which is beyond this form.

**Weather.** It has no sites and no assignment, and is generated over time rather than once. It shares only a random
engine with placement.

## Steps

- [ ] **1. The interface, and a zone's doors built from its file through it.** The five roles as parameters, with only
      exhaustive distribution and constant selection. Nothing new is placed and no zone changes, so the reference
      frames check it exactly.
- [ ] **2. Weighted selection over a table**: an entry drawn from a weighted list, with its properties. The table
      itself belongs to `content`.
- [ ] **3. The distributions**: a count, one per room, and a density per area.
- [ ] **4. The first use that is not a door**, items or denizens, which settles acceptance and emission.

## Blockers

- **Step 2 needs `content`**, for weighted selection over prototypes.
- **Where it lives is unsettled,** which determines what it may depend on, and so what a site can be.

## Open

The open items are tracked in `work_plan.md`.
