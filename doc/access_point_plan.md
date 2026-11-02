# Access Points --- Plan

Date: 2026-10-30

An access point is a component of an entity at which a zone is entered or left: a staircase, a portal, a waypoint, a
door facing, or a type that a mod adds. That entity is sited on a wall or a flat cell, and the access point has no site
or placement of its own. Today stratum_sim splits the concept between two components, `AccessPoint` and `Relocator`,
and the zone file stores both in one `relocator` component with an arrival placement that repeats the entity's site.
This plan makes one `AccessPoint` component in the sim and one `access_point` component in the file, with the arrival
placement derived from the site. The open items are in `work_plan.md` (`## Access points`).

## The component

- **An access point has a type, an order, and an optional target.** The type is the `(AccessType, type)` pair to a
  registered access point type, and the order counts from 0 without gaps among the zone's access points of that type.
  The target is unbound, a `RelocatorTarget` such as a level offset, or bound to another access point.
- **Most access points have a target, and most targets are bound in pairs.** Going through an access point that has a
  target moves the actor to the access point it refers to, as `World::relocate_through` does now.
- **An access point with no target is arrival only,** such as a starting spot or the far end of a one-way portal.
  Nothing can be entered through it.
- **An exit-only access point,** such as a pit, has a target, and its type is no type's `Arrival`, so nothing arrives at
  it. This needs no field of its own.
- **A door that is a magic doorway has the access point on one door facing.** An ordinary door has none, and the other
  facing of a magic doorway remains an ordinary way through.

## The arrival placement

The placement given to an arriving actor is computed from the site of the access point's entity, so it cannot disagree
with the site:

- **An access point on a wall** puts the actor in the cell the wall belongs to, facing along the wall's normal, away
  from the wall.
- **An access point on a floor flat cell** puts the actor in that cell, facing in the site's orientation.
- **An access point on a ceiling flat cell** puts the actor on the floor of that cell, facing in the site's orientation.

A floor or ceiling site already requires an orientation. The reader's check that an actor can stand in the placement's
cell becomes a check on the cell the site determines. `DiscreteLayout::arrival_placement` computes the placement,
reached through `layout_as`, so `ContinuousLayout` is unchanged.

## The file

The `relocator` file component becomes `access_point`, with `type`, `order`, and an optional `target`, and no
`placement`. `stratum_format::Relocator` becomes `stratum_format::AccessPoint`. An entity with an `access_point`
component is listed in `layout.components.access_points`, the zone's list of its access points (`zone_plan.md`), and the format version becomes 0.6, with the changes in an
appendix to `discrete_layout.md`. dungen and catagen write the component without a placement; `Staircase::placement()`
remains the stair block's cell and open direction, from which each generator computes the site.

## Renames

The verb *relocate* remains, since relocation is what going through an access point does: `World::relocate`,
`World::relocate_through`, and the `Relocated` event. The identifiers that refer to the component change:

| Now | Becomes |
| --- | --- |
| `Relocator` (sim and file) | `AccessPoint` |
| `find_relocator(zone, address)` | `find_access_point(zone, address)` |
| `enter_relocator` (`commands.cpp`) | `enter_access_point` |
| `World::resolve_relocator_target(relocator)` | `World::resolve_target(access_point)` |
| `targets_relocator_level(e, offset)` | `targets_level(e, offset)` |

`RelocatorTarget` keeps its name. It is deprecated, and the first pass's `{zone, position}` target is to replace it
(`zone_plan.md`, `## Implementation`).

`can_enter` also requires a target. The reference documents that use *relocator* for the component are swept in the
same commit as the code: `addressing.md`, `discrete_layout.md`, `game_system_reference.md`, `game_file_spec.md`,
`testing_plan.md`, `catagen_reference.md`, and `dungen_reference.md`.

## Order

1. **The arrival placement from the site.** stratum_sim's `AccessPoint` loses its placement, and `relocate_through`
   and `Session::start` use the placement computed from the site. The file is unchanged; the loader no longer reads its
   placement, and `make_document` writes the computed one, so every reference file stays identical.
2. **One `AccessPoint` component in stratum_sim,** with the optional target, `Relocator` merged into it, and the renames
   above. The reader rejects a `relocator` on an entity that `access_points` does not list.
3. **The `access_point` file component,** format 0.6: the reader, the writers, dungen's and catagen's `write_zone`,
   every zone file in the tree, and the reference files regenerated.
4. **One-way access points resolved.** An unbound target whose arrival type's access points have no target resolves to
   one of them by order, without requiring that it leads back, and binding sets only the departure's target.
