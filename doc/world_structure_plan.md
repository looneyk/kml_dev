# World Structure --- Plan

When a game is created, `World` creates the starting worldspace whole from the root region spec of the world
definition's starting region: the worldspace, its region tree, and an empty zone at every position of its extent.
Entering an access point whose destination is in a worldspace that does not exist creates that worldspace in the same way. An empty zone has no
layout and no entities until it is first entered or loaded. The steps below are commit-sized units, each passing every
test, and are the work of **The world's structure created whole** in `work_plan.md`.

## Design

- **A worldspace contains a grid of zones and a region tree.** Play takes place in zones. Regions group them for the
  map and for the player, and have little more than metadata and display data. A zone is in its worldspace at its
  position, and in its worldspace's region tree through the most specific region whose extent contains that position.
- **A zone is created in two steps.** Creating its worldspace creates it empty, with its `ZoneSpec` and its seed, and its
  layout and content are generated the first time it is entered or loaded. Creating a worldspace and its empty zones
  from a root region spec and a seed is deterministic, though making the region spec need not be.
- **A worldspace is created in this order:**
  1. The worldspace, from the root spec composed with its template, whose `extent` and `zone_size` it reads.
  2. An empty zone at each position, in worldspace position order, with only its seed and the placeholder name
     `<root region name>_<zone_counter>`, in the root region.
  3. The region specs, resolved down the tree, a region for each, and each zone in the most specific region whose
     extent contains its position.
  4. Each zone's `ZoneSpec`, resolved from its region's spec, with the zone's name, and saved in the zone for
     generating its content.
- **A zone's `ZoneSpec` is its region spec's `zone.` keys,** with the zone's position and seed added and its patterns
  resolved with the zone's symbols (`archived/pattern_resolution_plan.md`, `### Symbols`).
- **An empty zone is a zone with no layout,** built in memory and saved, unloaded and read back like any other. Its file
  has no `cells` (`discrete_layout.md`), and from the time it is generated its file has the layout.
- **Zones are created in worldspace position order:** `z`, then `y`, then `x`, each from lowest to highest
  (`region_spec_plan.md`, `## Zones of a worldspace`).
- **A worldspace is created from a root region spec and a seed.** The starting worldspace's are the root spec of the
  world definition's `starting_region` and the first draw of a random engine seeded from the world seed. An access point that leads to a
  worldspace that does not exist has a region spec name or an absolute extended name as its destination, and entering it
  creates the worldspace from the region spec tree that the destination refers to and a seed from its zone's seed pool.
- **Seeds derive from the worldspace.** The worldspace's random engine, seeded from the worldspace's seed, draws each
  zone's seed in worldspace position order, since the extent fixes the number of zones, and then the seeds of its region
  specs. A region spec has a seed, and a region does not. A spec's `seed` takes precedence over the drawn one.
  `world.json` saves the worldspace's seed, and each zone's file saves the zone's. A spec's seed is used only for the
  draws made while the spec is resolved, so nothing saves it.
- **A zone's seed pool is drawn before the rest of its generation,** from the zone's seed, so the seeds of the
  worldspaces its access points lead to do not depend on the rest of the generation.
- **An access point's destination is an `AccessPoint::destination_type`:** the persistent id of the access point it is bound to, a
  region spec name, or a zone's extended name with an optional position (`## Extended names`).
- **A zone's staircase destinations are in its `ZoneSpec`,** resolved from its region spec's `zone.` keys with the rest
  of the spec: `parameters.stairs_up_destinations` and `parameters.stairs_down_destinations`, each a comma-separated
  list of destinations. The generators write each staircase's destination from the list for its direction, in order with the
  last repeated, and create no staircase in a direction whose list is empty.
- **Every zone id is known when the `ZoneSpec`s are resolved,** since a worldspace's empty zones are created first. The
  symbols `zone_above` and `zone_below` are `#<zone id>` of the zones above and below the zone, at its x and y, or nil
  where the worldspace has none, and the default lists are made from them. The bottom zone of a dungeon has no
  staircase leading down.
- **A worldspace created through an access point has a listed zone for its entry zone** (`region_spec_plan.md`,
  `### Listed zones`), whose destinations in the direction leading back, opposite the access point's
  `Directionality`, are the departing zone.
- **A destination in another region or worldspace is the designer's,** written in the region spec's staircase
  destinations, such as the surface spec's `dungeon`. `[`*k*`]` and `[=`*n*`]` are for the designer's destinations, and
  the generators write none.
- **A destination in a worldspace that does not exist is a region spec name or an absolute extended name,** which its
  designer knows from the designed worldspace in the assets. An absolute extended name that does not fully specify its
  zone is an error.
- **The entity an access point is on, such as a staircase, has an optional `Directionality`,** up or down, which the
  generators write and the views draw.
- **The world definition file lists the game's region spec trees.** It is `assets/data/world_definition.json`, and
  more of the world's content is added to it later. Each tree is a list of region specs linked by the name of each
  spec's parent region, and a root spec has none. Each region's list of subregions is built when the file is loaded, for
  traversing the tree. A tree is identified by its root region's name, and the file's `starting_region` is the root
  region of the tree a new game starts in, `surface` in the MVP's file. The file lists the trees under `region_specs`.
- **A region spec has a `name` and a `display_name`, which its region takes.** The name is internal, part of the
  region's extended name, and the display name is the player's. Root spec names are distinct, and so are the names of
  the specs with the same parent, so extended names are distinct. A spec's name in `region_specs.json` refers to a
  designed spec.
- **A region spec is composed when its tree is resolved,** as `merge(parent, resolve(merge(designed, own)))`. The
  designed spec that its `region_spec` key refers to is merged with the spec's own keys first, the result's patterns are
  resolved, and the resolved spec is merged over the parent's resolved spec. The parent's layer has only its `store`,
  since a subregion does not inherit the typed members. A root spec has no parent's layer.
- **A region spec has typed members and a `store`.** The typed members, each optional, are `name`, `display_name`,
  `parent`, `region_spec`, `seed`, `extent`, a top-level spec's `zone_size`, and the listed `zones`. The other
  keys, such as `zone.*`, `parameters.*`, and the metadata, are in a `KeyValueStore`.
- **A layer is a merge patch** (JSON Merge Patch, RFC 7386). A key adds or replaces a value, and the value `&nil`
  removes the key, in place of JSON's `null`. `&nil` is a pattern whose value is nil, and a key whose pattern resolves to
  nil is removed from the spec and from the parent's layer (`archived/pattern_resolution_plan.md`, `## Design`). A list
  is replaced whole. A subregion's
  overrides and a mod's patches to types and styles (`modding_plan.md`) are one mechanism.
- **A spec's patterns are resolved before it is merged over its parent,** with draws from the spec's seed, so each spec
  is as instantiated as it can be and a parent's values are resolved once. A parent is resolved before its subregions, so `parent.<key>` in a pattern refers to the
  parent's resolved value. The `zone.*` patterns are resolved for each zone, since they use the zone's position and
  zone counter.
- **A region spec has no use once its region and its zones' `ZoneSpec`s are made,** and the specs are expected to go
  then. For now, a `Region` contains its resolved spec and `world.json` saves it, which may help debugging. The
  resolved tree is discarded once its worldspace is created. The `region_spec` and `parent` keys, the spec ids, and the
  lists of subregion ids are only for construction, and the `RegionTree`'s links between persistent ids replace them. A
  region has no seed.
- **A region spec in memory has an id,** its slot in its tree's vector, assigned when the tree is loaded.
- **A zone has the region id of the most specific region whose extent contains its position.** That is what it means
  for a zone to be in a region.
- **An extent is an object with either `boxes` or `positions`:** a union of boxes, each with a range on
  each axis, or a list of zone positions (`world_generation_plan.md`, `### Extents`). A root region's extent is its
  worldspace's, and is `boxes` with a single box.
- **The start is a placeholder setting.** `world.start_zone` and `world.start_cell` default to `center`: the zone at
  the centroid of the starting worldspace's extent, and the cell nearest the center of that zone's layout that an actor
  can occupy. Any other value is a zone position or a cell position.
- **A zone is named from its region spec's `zone.name`,** or else `&region_name .. '_' .. zone_counter`, with
  `zone_counter` counting the zones in the worldspace from 0 in worldspace position order. Its placeholder name is
  replaced when its `ZoneSpec` is resolved.

## Extended names

An extended name refers to a worldspace, a region, or a zone by the chain of names down the hierarchy, as a path does in
a Linux file system. It is absolute, relative, or begins with an id, and it may end with a position in the zone. A zone
level is the zone's signed index from the surface, not a depth in the region tree.

```text
extended_name := [ "/" ] segment { "/" segment } [ "@" position ]
segment       := name | ".." | "#" id | "[" [ "=" ] zone_level "]"
position      := "(" coordinate "," coordinate ")"
coordinate    := signed_numeric_value
```

| Symbol | Meaning | Details |
| --- | --- | --- |
| *name* | Name | A letter or underscore followed by letters, digits, and underscores |
| `/` | Separator | Separates the segments |
| Initial `/` | Root | The first name is a root region's name, which is its worldspace's name: `/<root region name>/.../<zone name>` |
| No initial `/` | Relative | Starts at the current zone, the zone that contains the reference, such as the zone of an access point whose destination it is |
| `..` | Parent | The region that contains a zone, or the parent of a region |
| `#`*id* | Persistent id | The first segment. The rest of the name continues from the zone or region with that id: `#<zone id>`, or `#<region id>/<zone name>` |
| `[`*k*`]` | Relative zone level | Replaced by the name of the zone whose zone level is *k* more than the current zone's, in the region that the preceding segments refer to. *k* is a signed integer, and the segment is the last |
| `[=`*n*`]` | Absolute zone level | Replaced by the name of the zone whose zone level is *n*, in the region that the preceding segments refer to. *n* is a signed integer, and the segment is the last |
| `@`*position* | Optional position | The coordinate position in the zone. Must be the last element if given. |

| Extended name | Refers to |
| --- | --- |
| `/surface/surface_3_4` | The zone `surface_3_4` in the root region `surface` |
| `/town/market@(12,7)` | The zone `market` in the root region `town`, at the cell position [12, 7] |
| `../[-2]` | The zone whose zone level is two less than the current zone's, in the region that contains the current zone |
| `../[=-3]` | The zone whose zone level is -3, in the region that contains the current zone |
| `../../crypt/[-1]` | The zone in the region `crypt`, a sibling of the current zone's region, whose zone level is one less than the current zone's |
| `#812` | The zone with the persistent id 812 |
| `#40/market` | The zone `market` in the region with the persistent id 40 |

## Steps

- [x] 1. **The structure created whole.** `World::create_worldspace` creates a worldspace, its top-level region, and an
  empty zone at every position of its extent, in worldspace position order, each named from the spec's
  `zone.name`.
- [x] 2. **The world definition file.** `stratum_format::WorldDefinitionDocument` reads and writes the file: its
  `starting_region` and its `region_specs`, a list of region spec trees. `assets/data/world_definition.json` has the
  `surface` and `dungeon` trees, and `load_world_definition` reads it into a `WorldDefinition` of trees by root region
  name, each root spec first. The starting region replaces `Generation::m_initial_region_spec_name`.
- [x] 3. **Structured region specs.** `RegionSpec`, one type in `stratum_format`, has the typed members and the `store`
  of `## Design`, read from `region_specs.json`, the world definition file, and `world.json`. An extent is an object
  with either `boxes` or `positions`, and a top-level spec's extent is its worldspace's, checked to be `boxes` with a single box.
- [x] 4. **Region spec trees.** A spec's id is its slot in its tree's `region_spec_tree_type`. Each spec's `parent` is
  resolved to its parent's id, and each spec's list of subregion ids is built from those. The names of specs with the
  same parent are checked to be distinct. Each spec is composed and its patterns resolved as `## Design` describes,
  parents first (`archived/pattern_resolution_plan.md`, step 2).
- [x] 5. **The region spec tree function.** It creates a worldspace from a tree in the order of `## Design`. Zone
  creation is split: the empty zone with its seed and placeholder name, then its region and its `ZoneSpec`. Each spec
  below the root adds a subregion to the worldspace's `RegionTree`, with the spec's `name` and `display_name`, and each
  zone has the region id of the most specific region whose extent contains its position (`zone_plan.md`, `###
  Regions`). `World::create` calls it for the starting region's tree. `world.json` saves every region of a worldspace
  with its parent's id and its resolved spec, a single root region being the case of one, and `World::open` rebuilds
  the tree.
- [x] 6. **Seeds from the worldspace.** The draws in `## Design`, and each worldspace's seed saved in `world.json`.
  `RegionPosition`, `new_region_position`, and the coordinate derivation in `region_seed` and `zone_seed` go.
- [ ] 7. **Access points and their destinations.** The design is `access_point_plan.md`'s, with the destinations of `## Design`.
  - [x] a. **`Arrival` removed.** An actor arrives at an access point of the type it went through.
  - [x] b. **The access point type from the prefab.** An access point's type is its entity's prefab, and `AccessType`,
    the registered access point types, and the `access_point` file component's `type` go. `register_game_types`
    registers the `staircase`, `portal`, and `waypoint` prefabs beside `door`. Every zone file in the tree is rewritten
    and the reference files regenerated.
  - [x] c. **`Directionality`.** The entity an access point is on, such as a staircase, has an optional
    `Directionality` component, the entity's `directionality` file component, which the generators write from each
    `TransitEndpoint`. The views draw a staircase leading up or down from it, in place of its destination's level.
  - [x] d. **The arrival placement from the egress.** An `AccessPoint` has an egress, the `access_point` file
    component's `egress`, which the generators write from each `TransitEndpoint`'s `egress_direction`. An arriving actor
    is placed in the cell at the position of the access point's entity plus the egress, facing along the egress, or in
    the entity's orientation if the egress is zero. In the wall notation a staircase is sited on its stair block's cell
    instead of on the wall that the floor presents to the block, and the reference files are regenerated.
  - [x] e. **The destination variant.** `AccessPoint::destination_type` is a variant of an access point's persistent id, a
    region spec name, and a zone's extended name, and an `AccessPoint`'s destination is an optional `destination_type`.
    A bound destination is the persistent id of the access point it is bound to, found through the world index, and the
    stored id of that access point in a file. The `EntityReference` destination goes. A region spec name destination arrives at
    the zone whose name is the region spec's `entry_zone`, a pattern such as `&region_name .. '_1'`, and every region
    spec's resolution has the symbol `region_name`.
  - [x] f. **The extended name grammar.** `stratum_format` reads and writes a zone extended name in the whole grammar of
    `## Extended names`, and the reader rejects a destination that does not parse. A destination that is a name is a
    region spec name.
  - [ ] g. **Listed zones** (`region_spec_plan.md`, `### Listed zones`). A region spec's `zones` list is read with the
    other typed members, and each listed zone's keys override the `zone.` keys when its `ZoneSpec` is resolved.
  - [ ] h. **Staircase destinations from the region spec.** Each zone's `ZoneSpec` has its staircase destinations, from
    the symbols `zone_above` and `zone_below` by default, and a worldspace created through an access point has the
    departing zone in its entry zone's listed zone, as `## Design` describes. The surface spec's
    `zone.parameters.stairs_down_destinations` is `dungeon`. dungen and catagen write the destinations from the
    lists. `entry_point` takes the lowest-ordered access point of the departure's type that has no destination or an
    unbound destination to the zone the actor came from, in place of `targets_level` and `accepts_arrival`, and going
    through a `#<zone id>` destination arrives in that zone. `stairs_down_region_spec`, `relative_zone_extended_name`,
    `relative_zone_offset`, `load_relative_zone`, and `new_zone_position` go.
  - [ ] i. **Destinations resolved when gone through.** Going through an unbound destination resolves it to a zone, by
    its zone extended name or its region spec name, and creates the zone's content, and its worldspace as needed, then.
    `create_destination_worldspaces` goes.
  - [ ] j. **Interaction through attachments.** `Interact` and a move blocked by a wall act on the entities attached to
    the wall and to the floor of the cell beyond it, and `Enter` and a move onto a cell act on those attached to the
    cell's floor, each by the components the entity has: an `AccessPoint` is entered, and an `Access` is opened or
    closed. In the wall notation, an access point on the floor of the cell beyond is entered only from the cell its
    egress leads to. A move onto a cell enters an access point whose `automatic_entry` is true, the `access_point` file
    component's `automatic_entry`, false when absent, and any other is entered by `Interact` or `Enter`. The hosts'
    messages come from the same attachments. `find_access_point`, `find_entered_access_point`, and
    `enter_access_point_by_moving` go.
- [ ] 8. **Seed pools.** Building a zone draws its seed pool, and a worldspace created through one of its access points
  takes its seed from the pool. `World::destination_seed` goes.

## Open questions

- **Do a registered spec's `subregion.<name>` entries (`region_spec_plan.md`, `### subregion.`), with their counts and
  weighted choices, remain beside a tree's parent links?**
- **What criteria choose a zone's destinations from those its region spec offers?**
- **How are a region spec name destination and a zone extended name destination used?**

## Documents revised on approval

- `world_generation_plan.md`, `## Overview` and `## Creating a game`, `zone_plan.md`, `### The world`, and
  `region_spec_plan.md`, `## The structure World::create builds`: the starting worldspace created with the game, and
  each other worldspace when an access point that leads to it is first entered.

## Worldspace, region, and zone design

Intro
A worldspace is the primary grouping object. A worldspace contains a grid of zones, and a region tree.
Zones are where all gameplay takes place.
Note that "root" and "top-level" are synonymous in the following sections.
Note also the difference between regions and region specs in the text.

Background
Regions are hierarchical groupings of map areas. They are organized in a tree structure with a single game world spanning region at the root
and the smallest, most detailed regions (i.e. towns, landmarks, groves, etc.) at the leaves.
Root regions correspond to worldspaces, and leaf regions correspond to subsets of the zones.
They are each created from the details in a region spec.
Region specs have two purposes, however. The first is to provide parameters for the creation of a worldspace and its empty zones.
The second is to create the matching hierarchy of Regions. Regions are primarily for drawing the map and identifying areas for the player.
Regions themselves are not much more than metadata and display data.
Region specs have no use beyond this point.

Game start
At the start of the game, there is a single primary world-spanning root region spec and a primary seed.
Root region specs will have a rectangular extent that is used when creating a similarly rectangular worldspace.

Generation
Zone generation is a two step process. The first step is coordinated by a top-level root region spec.
A worldspace is created with the extent as defined by the rectangular/grid extent of its region spec.
An empty zone is created at each of the worldspace's grid positions. All zones are created immediately, all at once, with the worldspace for seed integrity.
Worldspace and zone creation is a deterministic process once you have a region spec. (Region spec creation itself may not be, however.)
As each zone is encountered by the player, only then is its layout and content generated. This is the second step of the zone creation process.
Each new zone will have some number of access points: staircases, shafts, portals, waypoints, etc.
Each access point will have a new top-level root region spec, designed, constructed, or generated. It will also get a seed derived directly from the zone's seed.
These seeds, like the zones, are created all at once, and are derived directly from the zone's seed.
This region spec and seed become the "primary" region spec and seed for the new worldspace spawning at that access point.

Region specs and relationships
There is a direct connection between worldspaces and the zones they contain.
In between them, there is a hierarchy of region specs. Child region specs subdivide the extent of their parent.
Each region spec in the path from worldspace to zone can provide additional detail about the makeup of the zones its region ultimately contains.
And of course, a region spec contains the parameters to create its corresponding region.
The top-level region spec defines the worldspace, and the bottom-level region specs define the layout and content of its zones.
So between the worldspace and a zone, there are two paths, one direct, and the other through the hierarchy of region specs.

Seeds
A seed is generated for each worldspace, and each of its zones' seeds is immediately derived from it.
As each zone is encountered and before it is filled in, each of its access points gets a new top-level root region spec and a seed derived from the zone's seed.
This region spec and seed are directly analogous to the primary region spec and seed.
The exact same process is used for the primary worldspace and all access point worldspaces.

Region spec generation, resolution and composition
There is a file of region spec templates that are predefined or that can be filled in, automatically or copied and edited by hand.
Each region spec in a tree can have a template. The values for a region spec are those of the template overlaid with its own values.
Pattern resolution is done before composition. Some patterns are not resolved right away, &nil and && for example, and some not until a zone spec is needed (the zone. fields). So maybe the region spec composition with its template should be done first. In any case, when composed with a parent, templates should have already been composed.
Composition is completed by overlaying the region spec's values onto those of its parent, if any.

## Accesss points

When a zone is built and its access points generated, each will get a target.
The target will have the components in order: worldspace, zone, position (exact format to be designed)
and the worldspace alone, or worldspace and zone together, can be omitted. See the Summary below for target interpretation.
If there is a worldspace in the target, a region_spec and seed will be generated for that access point,
and worldspace creation will be deferred until the access point is entered.
If not, then there is already an empty zone, with its seed, ready to be filled.
So access points to another worldspace work similarly to access points or travel to another zone.
Each has a spec (region, zone respectively) and the target is generated upon entry, only as needed.

Summary
An access point can have a target of... and when entered...
another location in the current zone
    relocate to that position
a location in another zone in the current worldspace
    create a new zone if needed (from the empty zone spec)
    relocate to that position
a location in a zone of another worldspace
    create a worldspace if needed (from the empty region spec)
    create a new zone if needed (from the empty zone spec)
    relocate to that position
