# Worldspaces, Zones, and Regions --- Plan

Date: 2026-11-01

This plan defines the structure of the world: what a worldspace, a zone, and a region are, the specs they are created
from, how each is identified, the metadata they have, and the access points through which an actor moves between
zones. `## Design` is the structure, and `## Implementation` how stratum builds it. A zone's layout is in
`sim_zone_model.md`, how regions and zones are generated in `world_generation_plan.md`, templates and pregenerated
content in `template_plan.md`, and the access point component in `access_point_plan.md`. The open items are in `work_plan.md` (`## Zones and regions`).

## Design

### The world

- **The world is a set of worldspaces, which contain the zones, and a hierarchy of regions, which groups them.**
  Worldspaces are the spatial structure, and regions the logical one.
- **The world's structure is created whole before play starts.** Every worldspace, region, and zone is created with its
  spec and its name, and every zone with its position and its seed, and with no content. A zone's content is generated
  from its spec and its seed when it is needed, so a world created again from the same seed is the same world.
- **Metadata is available to scripts and generators and is not part of the world's mechanics.** A content designer
  chooses what metadata to include, such as a level or a depth, and what it means.

### Worldspaces

- **A worldspace defines the coordinate system of the zones it contains.** A zone's position is in its worldspace's
  coordinate system, and positions in different worldspaces have no relationship.
- **A worldspace has one top-level region,** whose region spec sets up the worldspace's coordinate system: its extent
  and, for a grid worldspace, its zone size.
- **The surface is one worldspace, a grid of zones.** A dungeon, a cave, a building, or a tower is a worldspace of its
  own.
- **A grid worldspace's adjacency is its coordinate space.** Two of its zones are adjacent edge to edge when their
  positions are adjacent in it. An actor walks from a zone into an adjacent one and sees across the boundary, so an
  adjacent zone is loaded along with the zone the character is in.
- **Zones that are not adjacent are connected only through access points,** within a worldspace and between
  worldspaces.

### Zones

- **A zone is the primary area unit of gameplay.** It is loaded, simulated, saved, and unloaded on its own, and it is
  its own coordinate space. This is the equivalent of a cell in a Bethesda game.
- **A zone is in one worldspace, at one position, and in one region.**
- **A zone has a size and layout, a list of its access points, and its contents.**
- **A zone is created from a zone spec** derived from its region's region spec. The region spec provides a template and
  the generation criteria, and the zone spec the details of the one zone created.
- **A zone's content is generated from its zone spec and its seed,** when the zone is first entered or loaded.
- **A zone has metadata,** such as its name, its level if the designer assigns one, and its difficulty.
- **A zone keeps its zone spec** until it is decided whether one is needed after its content is generated.

### Regions

- **A region is a subdivision of a worldspace's zones,** largely arbitrary, whose zones share properties, such as a
  wilderness, a large town, a dungeon, or a tower. Its zones might be the levels of a dungeon, the floors of a tower, or
  the areas of a town or a wilderness.
- **A region is data.** It acts on nothing directly, apart from what a generator reads from it.
- **A region has no part in movement, adjacency, or the layout of zones.** All movement is between zones, whether their
  regions are the same or not, and a region is a name and a list of zones, with whatever significance the design gives
  it.
- **Regions are hierarchical.** A worldspace's top-level region has subregions, which may have subregions of their own,
  such as a province of the surface, a town in the province, and a district of the town.
- **A region has an extent,** the set of positions in its worldspace that it covers. A top-level region's extent is its
  worldspace's. A subregion's extent is a part of its parent's: a set of grid zones on the surface, or a range of
  level zones in a vertical layout such as a dungeon.
- **Every position is in the most specific region whose extent covers it.** Zones that none of a region's
  subregions cover belong to the region itself, and are created using its region/zone spec. The set of subregions does
  not need to cover the entire parent region. It is an error for two sibling subregions to overlap.
- **A region has a list of the zones it covers,**
- **A region is created from a region spec,** which defines the region's properties, primarily to generate its zones,
  and has a region spec for each of its subregions. Several regions can be created from one region spec, and a region
  spec's name is separate from the names of the regions created from it.
- **A region has metadata,** such as its name, terrain type, level count, and base difficulty.
- **A region keeps its region spec** until it is decided whether one is needed after creation.

### Identity

Worldspaces, zones, and regions are identified in two contexts, by a name in design and by a persistent id in play.

- **A name identifies an item on its own, and an extended name is the chain of names down its hierarchy,** separated by
  `/`, as a brep surface has a name and an extended name.
- **Every worldspace, zone, and region has a name and an extended name.** A worldspace's extended name is its name. A
  zone's is its worldspace's name and its own, `worldspace_name/zone_name`. A region's is the names of the regions
  above it and its own, such as `surface/town/district`. A top-level region's name is its worldspace's, so every
  region's extended name begins with its worldspace's name.
- **Every extended name is unique across the game,** at every level of the hierarchy. A content designer
  refers to a zone or a region by its extended name, as in a target in a pregenerated zone.
- **A zone's display name,** shown to players, may differ from its extended name and from its name.
- **A region spec's naming patterns are the designer's,** one for the regions created from it and one for their zones,
  so generated zones and regions have unique extended names too. Region specs are design assets, or derive their
  values from design assets.
- **Every worldspace, zone, and region has a persistent id,** given when it is created. A bound target, a region's list
  of its zones, and a saved game refer to it by its persistent id.
- **A region is rarely referred to by name,** apart from the world map, or through one of its zones.
- **A zone refers to its region** by the region's persistent id or its extended name.

### Positions and levels

- **A zone's position is an `x`, `y`, and `z` in its worldspace,** and no two zones of a worldspace have the same
  position. A region's position, if it has one, is as defined by the designer.
- **A zone's level is metadata.** It may be the `z` of the zone's position or a value of its own, and any number of a
  region's zones may share one level, as the designer chooses. A designer might use 0 for the surface and -1, -2, ...
  going down, shown to players as an absolute value.
- **Depth is metadata,** which may be either or both a physical distance and/or integral count.

### Access points

- **Access points are the means of moving between zones discontinuously, and of fast travel.** An access point connects to another in
  the same zone or in a different one, in any worldspace and any region.
- **An access point is part of something in the zone,** such as a staircase, one side of a door, a portal, or a
  waypoint, and its position is that object's position.
- **An access point is just the transport information of the object it is attached to.** And has no type, etc. of its own
- **The order counts from 0 among a zone's access points of one type,** A zone's list of access points is ordered, a zone's staircases
  sharing one sequence for example.
- **An access point has an optional target.** The target is typically attached to an object of the same type as the original access point's, but this is not enforced.
  One with no target is arrival only, such as the far end of a one-way portal.

### Targets

- **A target refers to another access point directly or to a zone.** A direct target is bound, and refers to the
  access point by its persistent id. A target that refers to a zone is unbound.
- **A zone reference has a zone's extended name or a position,** and optionally the name of an access point in that
  zone to arrive at. A position is relative to the access point's zone, in its worldspace, or absolute in a worldspace.
- **Every zone a target refers to exists before play starts,** since the world's structure is created whole, so an
  unbound target always resolves to a zone. An extended name resolves to the zone of that extended name. A relative position *k* from
  a zone at *P* resolves to the zone at *P* + *k* in the same worldspace, and an absolute position to the zone at that
  position in its worldspace.
- **A target is bound by having been gone through.** Going through an unbound target generates the zone's content if it
  has none, places the arriving actor at the named access point or at an unbound access point of the arrival type
  whose target leads back, and binds both ends, so the binding is the connection from then on. A zone template's
  targets are all unbound.
- **The target's notation is not designed.**

### Doors between zones

- **A door that leads to another zone or another part of the current zone has an access point on one side.** Whether an actor gets through is still the
  door's, open, closed or locked, and the access point determines where it arrives, instead of the space beyond. A
  staircase is the same case, always open.
- **A crypt door is two doors, one in each zone,** each with an access point bound to the other's. One object cannot
  span two zones, since each zone unloads and saves on its own.
- **The two doors are kept open, closed, or locked alike by a rule,** since each is in its own zone.
- **A normal door has no access point,** and simply leads to the other side of its doorway.

### Saved for later

- **Region files.** A region's metadata and generation settings, and possibly then its zones.

## Implementation

- **`World` manages the worldspaces, zones, and regions,** with one flecs world containing their entities.
- **A zone's flecs parent is its worldspace.** `zone_of` stops at the first ancestor that is a zone, and a zone's
  contents are its subtree (`sim_zone_model.md`, `### Entity ownership and migration`).
- **A region's list of its zones** is its `zone_ids`, by `PersistentId`, and a loaded zone is found by its
  `PersistentId`.
- **A zone with no content** is an entity with its name, `ZonePosition`, `ZoneSpec`, and seed, and its layout and
  entities are added when its content is generated.
- **A zone's list of its access points** is `layout.components.access_points`, and a pregenerated zone's name is its
  `layout.components.name`. A zone's display name is its `DisplayName`, a component separate from its name.
- **The specs are `RegionSpec` and `ZoneSpec`.** A region spec's name is its key in the game's region specs.
- **A zone's position in its worldspace is its `ZonePosition`,** from the zone file's `layout.components.position` or
  the `ZoneSpec`. A region has no `RegionPosition`.
- **An access point is the `AccessPoint` component** of a staircase, a door facing, a portal, or a waypoint, and its
  position is the site of that entity, on a wall or a flat cell (`access_point_plan.md`). The type each type arrives at
  is its `Arrival`.
- **A door between zones has its access point on one door facing.** Its `Access` determines whether an actor gets
  through, and a staircase's `Access` is always open, from its type. A crypt door's two doors each have one door facing
  on a wall of its own zone, since a wall address means something only in its own zone. Keeping their `Access` equal is
  an open rule.
- **A door within one zone keeps its two door facings** on the walls of its doorway, with one `Access` for both sides.

The first pass puts the structures in place with these simplified forms, each to be refined later:

- **A name is the one its spec states, with a numeric suffix, `<name>_<counter>`, only if that name is taken.** A
  worldspace and its top-level region have the region spec's name. The game has a counter for top-level regions, saved
  with the game, and each region has a counter for its subregions and its zones, saved with the region. A worldspace's
  name is unique in the game, and a subregion's or a zone's in its region, so every extended name is unique. Random
  names replace the suffix: a name that is taken is then generated again.
- **Each worldspace has only its top-level region,** whose extent is the worldspace's. Subregions and their extents
  come later.
- **A target is `{zone, position}`.** The zone is an extended name or a persistent id, and the position is a string,
  either coordinates in a worldspace or an offset from the access point's zone, as `RelocatorTarget`'s level offset is
  now. It replaces the deprecated `RelocatorTarget`, and the target's notation replaces it in turn.

## Open questions

- **What is the target's notation?**
- **What forms does an extent take besides boxes?** A diagonal boundary takes one box for each row, so a list of
  positions or a predicate, such as a half-plane, may be needed too (`world_generation_plan.md`, `### Extents`).

## Checks

- **Every structure that refers to a worldspace, a zone, or a region by name** uses its extended name.
