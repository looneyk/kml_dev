# Zone and Region Identity --- Plan

Date: 2026-10-30

How a zone and a region are identified is defined piecemeal across `addressing.md`, `discrete_layout.md`,
`region_plan.md`, `sim_zone_model.md`, `world_generation_plan.md`, and the code, and the definitions disagree. A
relocator's target refers to zones and regions through these identifiers, so its notation cannot be settled before them.
This document describes the structures, the intent stated so far, and the discrepancies, as the basis for a definition
and possibly a redesign. The open questions are also listed in `work_plan.md` (`### Zone and region identity`).

## The structures

- **A zone's persistent id** is the `PersistentId` of its entity, given when the zone is created. A saved zone's file
  writes it as `zone`, and a zone template has none. It is the only identifier a loaded zone is found by.
- **A zone's name** is `layout.components.name`, read into the zone's `DisplayName`. `game_system_reference.md` states
  that it identifies nothing. `region_plan.md` (`## Pregenerated content`) makes it the zone's tag in the pregenerated
  content index, by which a relocator target finds a pregenerated zone.
- **A region's persistent id** is the `PersistentId` of its entity, given when the region is created.
- **A region's name** is planned as a component (`work_plan.md`, **A region's properties**) and is not built. The region
  part of a target refers either to a region spec or to a pregenerated region's tag (`## Discrepancies`).
- **A region spec's name** is its key in `Generation::m_region_specs`. A region has its spec, and several regions can be
  created from one spec.
- **A zone's position** is its `ZonePosition`, `x`, `y`, and `z`, in its region, from the file's `layout.components.position`
  or the `ZoneSpec`. `new_zone_position` is a placeholder that returns `(0, 0, level)`.
- **A zone's level** is the `z` of its `ZonePosition`.
- **A region's position** is its `RegionPosition`, `x` and `y`, in the world.
- **A relocator target** is `[region/]zone[@position]` (`discrete_layout.md`, `### A relocator's target`). Its zone is a
  relative level, an absolute level, or a zone name, and its position is the name of a relocator in that zone.
  `RelocatorTarget` is deprecated (`access_point_plan.md`, `## Renames`).

## The intent

- **A level is metadata,** a component of the zone and nothing more.
- **A zone name is unique within its region.** It is probably unique across the game too, possibly as the region's
  name and the zone's name joined, `regionname_zonename`.
- **Every structure that refers to a zone or a region by name adheres to the same uniqueness.** Whether each one does
  now has not been checked.
- **Separate region and zone parts of a target might be unnecessary,** if zone names are unique across the game. This
  has not been examined.

## Discrepancies

- **A level is a lookup key in the code,** against `sim_zone_model.md` (`### Level`), which states that nothing is keyed
  or looked up by it. `World::resolve_relocator_target` finds the zone to arrive in by its level, and an unbound target
  states nothing but a level.
- **The target notation calls a level a zone.** Two of the three forms of a target's `zone`, `-1` and `=-3`, are levels.
- **One zone per level, or several, is unsettled.** `sim_zone_model.md` states that the levels of a dungeon are
  separate zones, and that a region may have many zones at one level; the target `+0` is another zone at the same level.
- **A zone's name identifies nothing,** in `game_system_reference.md`, and identifies a pregenerated zone, in
  `region_plan.md`.
- **The region part of a target has three meanings.** In `region_plan.md` it is a region spec from which a new region
  is created (`sewers/=-1`), and a pregenerated region's tag (`town/market@fountain`). In `discrete_layout.md` it is an
  existing region, the only one searched, and the code creates a new region from the region spec of that name.
- **A region spec and a region are not distinguished** in a target, though one spec can create several regions.
- **`discrete_layout.md` and `region_plan.md` each define the target grammar.** `region_plan.md` omits the `region` and
  `position` rules.
- **The code resolves only a level and a region spec.** A target with a named zone or a position throws, and
  `discrete_layout.md` does not state that.

## Open questions

- **Is a zone identified by its persistent id, its name, or both?** Does every zone have a name, generated zones
  included?
- **Is a region identified by its persistent id, its name, or its region spec's name?** Is a region's name unique?
- **Is a zone name unique across the game,** and if so, is the region's name part of it?
- **How is a level offset such as `-1` resolved** once a level is metadata only?
- **Does a region have one zone per level, or several?**
- **Is a level the `z` of the zone's position,** or a component of its own?
- **Which structures refer to a zone or a region by name,** and do they all adhere to the uniqueness above?
