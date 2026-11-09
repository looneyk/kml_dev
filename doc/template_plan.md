# Templates --- Plan

Date: 2026-11-06

A template is the data that a zone's content is made from, and pregenerated content is the zones and regions that the
game has as files. `world_generation_plan.md` describes how a zone's content is generated, `region_spec_plan.md` the
keys of a region spec, and `discrete_layout.md` the zone file. The open items are in `work_plan.md` (`## Generators`
and `## Zones and regions`).

## Templates

- **A template is data,** such as fields, forest, cave1, grid1, or a landmark that is partly or wholly pre-built. The
  zone generator applies the `ZoneSpec` to it as far as it makes sense, which is to be defined.
- **The region specs and the templates are the game's content** (`world_generation_plan.md`, `## Registries`).
- **A zone template and a pregenerated zone have one form,** a zone file with optional embedded placeholders. A saved
  zone is a specialization of the same form, with persistent ids in place of local ones (`discrete_layout.md`,
  `## File structure`).
- **A template's entity ids are local,** and loading it assigns each entity a new persistent id, so one template can be
  made into several zones (`addressing.md`, `### Ids`; `addressing_plan.md`, `### Loading a template`;
  `decision_rationale.md`). Every target in a template is unbound (`discrete_layout.md`, `### An access point's target`).
- **A `ZoneSpec` may accompany a zone template file** for making a zone from it.
- **A zone's population strategy comes from the template it was loaded from,** or from the guidance it was generated
  with (`session_plan.md`, `## World`).

## A zone's template

- **A zone's `ZoneSpec` has its template,** resolved from the region spec's `zone.template` pattern with the zone's
  position and index (`world_generation_plan.md`, `## Overview`).
- **A listed zone's `template` replaces the pattern's draw,** so a designer can place a town or a pre-built landmark
  at a fixed position in a generated region (`region_spec_plan.md`, `### Listed zones`).

### The starting region

The starting region is a surface region. Its first zone is a field or a forest with few denizens, if any, and a zone
next to it is a town. Every other zone of the region is drawn at random from fields, hills, forests and towns.

None of this is special code. The starting region is a `RegionSpec` whose weighted tables have an opening sequence;
any region may have one.

### Weighted tables

A region spec's `zone.template` pattern draws each zone's template from weighted tables: an opening sequence for its first zones, a
primary table, and rare templates that are drawn occasionally. The tables are among the region spec's `parameters.`
keys (`region_spec_plan.md`, `### parameters.`).

```cpp
// The starting region's spec
starting_area
    type: surface
    difficulty: 1
    service_level: basic
    opening: [{fields 1, forest 1}, {town 1}]        // weighted tables, one for each of the first zones, in order
    primary: {fields 4, forest 4, hills 2, town 1}   // the weighted table for every zone after the opening
    rare: {hermit_glade 0.02, ruined_shrine 0.01}    // templates, each drawn with its probability
    zone.parameters.denizens: few

// The value of the zone.template pattern for the zone with a zone counter
template = zone_counter < opening.size() ? draw from opening[zone_counter]
         : a draw from rare selects an entry ? that entry
         : draw from primary;
```

- **A weighted table is the mechanism that placement draws from** (`placement_plan.md`, `## A run's roles`), so
  the `zone.template` pattern and placement share it.
- **A zone's denizens are among its parameters,** such as the starting zone's few, for the population strategy that
  replaces `World::populate`'s placeholder.

## Pregenerated content

- **Pregenerated zones are in `assets/zones`.**
- **The game builds an index of pregenerated content when it starts.** It scans the directory of pregenerated zones, or
  reads a pregenerated content index file instead, which is probably built in the background as the game loads.
- **The index lists each zone's and region's extended name** with its zone data file and the names of the access
  points in it.
- **A `ZoneSpec`'s template is read from the index.**
- **A target in pregenerated content refers to a zone by its extended name,** as in `town/market@fountain`, the zone
  `market` in the region `town` at the position `fountain` (`zone_plan.md`, `### Identity`; `discrete_layout.md`,
  `### An access point's target`).
- **A pregenerated zone's name is its `layout.components.name`** (`zone_plan.md`, `## Implementation`).

## Region templates

Region templates can be considered if needed.
A `RegionSpec` should be able to express a region's settings and layout, but zones have more detail that makes that impractical.
