# Placement --- Plan

Status: not started
Date: 2026-09-04

Intent only; nothing here is built. `game_system_principles.md` gives `procgen` a slot in the library
layout as what the two generators come to have in common. Placement is the first code that would go
there which is not shared between them at all, and settling its shape is what decides whether
`procgen` is one library or several. What is built goes to `game_system_reference.md`.

**`site` in this document is owed a rename.** `site_plan.md` takes `site` for where an entity is, and what this
document calls a site is a candidate place a run chooses among --- a wall id, a cell, a room. The replacement term
is tracked in `work_plan.md`.

## Where a zone comes from

**A zone is constructed whole when it is encountered.** Geometry and every content of it, in
one pass, from parameters and a seed. That is how the game works: nothing but prototypes lives in
files, and a zone is not one of them.

The level files are skeletons of a finished zone --- a snapshot of part of that construction,
taken at the point where only geometry existed. They are useful for testing and for authoring, and
they are not how a zone normally arrives.

Three entries, in order of how general they are:

```cpp
// The general case: geometry and contents, from parameters
void create(const Generator& generator, Driver driver = Driver::TurnBased,
    double units_per_second = Clock::units_per_second_default);

// A zone read whole -- geometry, attachments, items, monsters. Today's files contain only the
// first, so what is read is a skeleton and the rest is placed as though generated
void load(const std::filesystem::path& filepath, Driver driver = Driver::TurnBased,
    double units_per_second = Clock::units_per_second_default);

// Geometry already built, populated as a generated zone is. Not the general case: it is the
// entry for a caller that has geometry from somewhere else
void populate(std::unique_ptr<Layout> layout, const door_walls_type& doors, Format format,
    Driver driver = Driver::TurnBased, double units_per_second = Clock::units_per_second_default);
```

**The generator is not `Session`'s to choose.** `stratum_sim` links `quetzal_common` and flecs and
nothing else; it reads both file formats with readers of its own rather than depending on either
generator. So `Generator` is an interface here and an implementation at application level, over
whichever generator was named --- and naming it is what settles which parameters apply, the concrete
one declaring its own. It returns the geometry, the walls each door is attached to, and the `Format`, which
a constructed zone has no file to supply and the terminal host asks for when it picks a view.

**Placement is part of generation, not a step after it.** A generator builds geometry and puts
contents in it, and the file --- when one is written at all --- preserves the result. So `load` reading a
zone whole and `create` constructing one are the same zone by two routes, and neither is a route
that skips placement. What today's files leave out is filled in as though generated, which is the
sense in which they are a snapshot rather than a lesser format.

## What placement is

Putting contents somewhere in a zone whose geometry is already built: doors on the walls a loader reported,
items in rooms, monsters on floor cells, a torch on a wall, a trap on a doorway. The geometry exists
and something is being assigned to parts of it.

One algorithm covers all of them. What varies is the type of the site and the type of what goes on it.

## The zone does not appear in it

Placement needs a common interface over a zone, and it gets one by having no zone in it.

`procgen` may not be designed against `stratum_sim` types. A caller with a zone extracts the sites
from it and supplies a callback that builds into it; placement itself sees a range of sites, a table
of candidates, a random engine and counts. The commonality is the algorithm, not an abstraction of a
zone, and the site range is the seam between them.

This is also what lets the same placement run against a `catagen::Level` before a layout exists, or
against something that is not a level at all.

## What one run takes

Five roles, each independent of the rest:

- **The sites** --- a range of whatever a place is. Wall ids for an attachment, cells for an item,
  rooms for a monster group. `catagen` already calls these sites.
- **Selection** --- which candidate, drawn from a weighted table. This is where an item's random
  properties come from, and it is the part closest to `content`.
- **Distribution over the sites** --- how many are used and which. Every one of them, a count, one per
  room, a density per area, or clustered about a chosen site.
- **Acceptance** --- whether a given item may go at a given site. A torch needs a wall, a chest
  needs floor with room beside it, a monster needs neither.
- **Emission** --- what is actually built. The only part that touches a world at all, and the only part
  a caller has to write.

Selection and distribution are two operations under one word and are kept apart deliberately. Conflated,
"rare items, one per room" needs a bespoke distribution for that one pairing rather than the two
choices it actually is.

## The case that already works

`create_doors` is an instance of this today: the sites are the wall sets `load_layout` reports, the
distribution is every one of them, selection is constant, acceptance is unconditional, and emission
builds the facing tree. A general form that cannot express what already works is the wrong form, so
reimplementing `create_doors` through the interface is what proves it rather than a new use does.

## What it must not lose

Placement is seeded, so it must reproduce. That puts one requirement on the sites: they must arrive
in a stable order. `sim_zone_model.md` states that flecs iteration is deterministic but **not
ordered** --- entities are visited grouped by archetype --- so a site range gathered from a query has to
be sorted before it is placed against, or the same seed produces a different zone. The requirement
belongs to the caller producing the range, and the interface should state it rather than sort behind
one's back.

## What it does not cover

**Dependent placement.** A key reachable before the door it opens, a lever before the gate, a boss
behind both. That is not assigning contents to sites; it is a constraint solver over reachability, and
it is what eventually breaks this shape rather than extending it. Naming it here so that the first
version is not quietly bent toward it.

**Weather.** No sites and no assignment, and generated over time rather than once. It shares nothing
with placement but a random engine, which is evidence that `procgen` is not naturally one library.

## Steps

- [ ] **1. The interface, and `create_doors` through it.** The five roles as parameters, exhaustive
      distribution and constant selection only. Nothing new is placed and no zone changes, which is
      what makes it a comparison against the reference frames that already exist.
- [ ] **2. Weighted selection over a table.** An entry drawn from a list with weights, and its
      properties drawn with it. Where the table comes from is `content`'s, not this.
- [ ] **3. The distributions.** A count, one per room, a density per area. Each is small on
      its own; they are together because they are what the interface exists to vary.
- [ ] **4. The first use that is not a door.** Whichever of items or monsters arrives first, which
      is what settles whether acceptance and emission were drawn correctly.

Step 1 changes no output, so it is the one that can be checked exactly.

## Blockers

- **Step 1's subject is being deleted.** `create_doors`, `door_walls_type` and `load_layout`'s door
  callback go when a zone is loaded from its entities rather than from its glyphs
  (`addressing_plan.md`, step 5). Step 1 waits for that, and what it then reimplements through the
  interface is how a zone's doors are built from its file, with the reference frames unchanged as
  before.
- **Step 2 has no table to draw from.** Weighted selection over prototypes needs `content`, which does
  not exist. Where the boundary with it falls is open below.
- **Where it lives is unsettled,** which decides what it may depend on and therefore what a site can be.

## Open

- **Where it lives.** `procgen` is a slot rather than a library, and placement may not belong beside
  the level generators at all --- its dependencies are tables and prototypes, not grids.
- **What the roles are called.** Named here by what they do rather than with terms of their own.
  `distribution` in particular collides with `std::uniform_int_distribution` and the rest, in a part
  of the codebase that uses them.
- **Whether a site is a value or an id.** A wall is an `id_type`, a cell is a `GridPosition`, a room
  is a record. Templating over it covers all three; whether anything needs them mixed in one run is
  not known.
- **Where the boundary with `content` falls.** Weighted draws over prototypes may be `content`'s
  own, leaving placement only the spatial half.
