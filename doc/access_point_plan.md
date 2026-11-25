# Access Points --- Plan

Date: 2026-10-30

An access point is a component of an entity at which a zone is entered or left: a staircase, a portal, a waypoint, a
door facing, or a type that a mod adds. That entity is sited on a wall or a flat cell, and the access point has no site
or placement of its own. Today stratum_sim splits the concept between two components, `AccessPoint` and `Relocator`,
and the zone file stores both in one `relocator` component with an arrival placement that repeats the entity's site.
This plan makes one `AccessPoint` component in the sim and one `access_point` component in the file, with the arrival
placement derived from the site. Its open work is `world_structure_plan.md`, step 7.

## The component

- **An access point has a type, an order, an egress, and an optional destination.** The type is the entity's prefab, and the order
  counts from 0 without gaps among the zone's access points of that type.
  The destination is an `AccessPoint::destination_type`: the persistent id of the access point it is bound to, or, unbound, a
  region spec name or a zone's extended name, such as a relative zone level. An entity has one access point at most.
- **Most access points have a destination, and most destinations are bound in pairs.** Going through an access point that has a
  destination moves the actor to the access point the destination refers to, as `World::relocate_through` does now.
- **An access point with no destination is arrival only,** such as a starting spot or the far end of a one-way portal.
  Nothing can be entered through it.
- **An actor arrives at an access point of the type it went through,**
- **A magic doorway has an access point on one door facing.** An ordinary door has none, and the passage from the other
  facing is unspecified.

## The arrival placement

The placement given to an arriving actor is computed from the position or the destination object plus the access point's egress vector

## The file

The `relocator` file component becomes `access_point`, with `order` and an optional `destination`, and no `type` or
`placement`, since the access point's type is its entity's `type`. `stratum_format::Relocator` becomes `stratum_format::AccessPoint`. An entity with an `access_point`
component is listed in `layout.components.access_points`, the zone's list of its access points (`zone_plan.md`), and the format version becomes 0.6, with the changes in an
appendix to `discrete_layout.md`. dungen and catagen write the component without a placement; a `TransitEndpoint`'s
`cell_position` and `egress` remain the stair cell and the offset from it to the cell it faces, from which each generator computes the site.

## Renames

The verb *relocate* remains, since relocation is what going through an access point does: `World::relocate`,
`World::relocate_through`, and the `Relocated` event. The identifiers that refer to the component change:

| Now | Becomes |
| --- | --- |
| `Relocator` (sim and file) | `AccessPoint` |
| `find_relocator(zone, address)` | `find_access_point(zone, address)` |
| `enter_relocator` (`commands.cpp`) | `enter_access_point` |
| `World::resolve_relocator_target(relocator)` | `World::resolve_destination(access_point)` |
| `targets_relocator_level(e, offset)` | `targets_level(e, offset)` |

`can_enter` also requires a destination. The reference documents that use *relocator* for the component are swept in the
same commit as the code: `archived/addressing.md`, `discrete_layout.md`, `game_system_reference.md`, `game_file_spec.md`,
`testing_plan.md`, `catagen_reference.md`, and `dungen_reference.md`.

## Order

1. **The arrival placement from the site.** stratum_sim's `AccessPoint` loses its placement, and `relocate_through`
   and `Session::start` use the placement computed from the site. The file is unchanged; the loader no longer reads its
   placement, and `make_document` writes the computed one, so every reference file stays identical.
2. **One `AccessPoint` component in stratum_sim,** with the optional destination, `Relocator` merged into it, and the renames
   above. The reader rejects a `relocator` on an entity that `access_points` does not list.
3. **The `access_point` file component,** format 0.6: the reader, the writers, dungen's and catagen's `write_zone`,
   every zone file in the tree, and the reference files regenerated.
4. **One-way access points resolved.** An unbound destination whose type's access points in the arrival zone have no destination resolves to
   one of them by order, without requiring that it leads back, and binding sets only the departure's destination.

The work that follows is `world_structure_plan.md`, step 7.


## New access point proposal

**The classes**

AccessPoint: one end of a translocation pair, a component of the object it is on
    discrete_vector2_type egress // the offset from the object position upon arriving
    AccessPointDestination

The object, a staircase, ladder, portal, ..., has the placement (position, orientation) and optional directionality.

AccessPointDestination: a specifier for the destination of an access point translocation, one of:
    access_point_id // an existing access point
    region_name/region_spec_name // an access point in the first zone created in a new worldspace
    zone extended name // absolute or relative, see notes about tracking unused access points below, not used if access point pairs are generated up front

**Specific usages**

Block notation

Interact with any access point type from the block cell position itself or an adjacent cell. Landing cells are this fixed set of position and adjacent cells.

Wall notation

|type|rise|pos|ori|egress|lpos|ingress|
|---|---|---|---|---|---|---|
|stair|down|0|1|-1|-1|1|
|stair|up|0|1|-1|-1|1|
|door|null|0|1|-1|-1|1|
|ladder|down|0|1|-1|-1|1|
|ladder|up|0|1|0|0|0|
|rope|down|0|1|-1|-1|1|
|rope|up|0|1|0|0|0|
|portal|null|0|null|0|0|0|
|shaft|down|0|null|0|0|0|
|shaft|up|0|null|0|0|0|

pos - position is the cell position of the access point, the block cell for stair/door, the shaft cell for ladder/rope, and the flat cell for portal.
ori - orientation is the horizontal component of the direction that one would walk into a staircase/door or enter a shaft from (the normal of the wall that a ladder/rope is attached to). It can be calculated using the egress.
egress is the direction of travel emerging from a staircase/door or shaft

landing position lpos = pos + egress
ingress = -egress

ori = -egress

Stairs in both notations are a full cell. (Changed from previous where wall notation stairs were attached to a wall)
Stair orientation is the horizontal component of the direction that one would walk into the staircase.
Door here only applies to doors with access points, normal doors do not use the access point procedure.
ladder/rope is the actual access point attached to the top and bottom of a shaft.
A shaft access point attached to the top is one way down and the bottom access point is invisible.
Numbers are x coordinates where the access point and landing are aligned along the x-axis.
Example shows values where the access point is on the right and the landing is to the left of it.
With the exception of ladder/rope where the bottom landing and the access point positions coincide.
ladder/rope are shafts with an attached access point.
Entering a shaft from the bottom is not possible without a lift/aid and/or climbing gear, or levitation.

**Notes**

Landing really only applies to wall notation, for block notation, the landing spaces are fixed, the placement position and open adjacent positions

enter from placement position
arrive at landing position, in opposite landing direction

destination zone coding
+|-x real value, a relative distance
+|-[n] [] integer value, a relative count of floors, levels, cells, ...
= prefix indicates an absolute distance or count

destination is not checked against directionality; agreement is the designer's responsibility, which allows non-Euclidean layouts

destination may be invisible, so it looks like just a regular location
destination may not have a destination, one-way

stair, ladder, rope cells can be interactive or automatic
shaft/rope top cells walked on, if automatic, the rope will be used if there, otherwise fall
notification state controls whether it is interactive or automatic

transporting to a portal, they are not active until stepped off

When the enpty zones of a worldspace are created, along with the zone seed, a seed pool will be created for the access points that will eventually be created in it.

The static way of doing access points
It may be the case that upon creation of a worldspace, each zone is created and all the inter-zone translocation access point pairs are also created,
and placed at a position in their zones when the zone is filled.
This means that AccessPointDestination will be either one of these access point ids generated up front,
or a region spec that will define the creation of a new worldspace/region.

The dynamic way of doing access points
When a zone/level is created, a number of down staircases will be created. they will typically have a relative zone level offset of -1, but can be any lower value as well.
When each new zone/level is created, a number of up staircases will be created, one for each down staircase above it, and similarly for down staircases.
Entering the first staircase leading to each new zone/level will trigger its creation
So each empty zone in a dungeon will store a list of up and down staircases that it will have to link to on creation.
That would become the minimum number of staircases for the generator parameters. Any additional staircases betond that minimum would need to link to other new zones/levels.
This is written in terms of staircases, but it applies to other type of access points as well.
If unconstrained, this can create an unmanageable web of access points.
