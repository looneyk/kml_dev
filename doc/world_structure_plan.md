# World Structure --- Plan

When a game is created, `World` creates the starting worldspace whole from the root region spec of the world
definition's starting region: the worldspace, its region tree, and an empty zone at every position of its extent.
Generating a zone creates, in the same way, the worldspace that each of its access points leads to. An empty zone has no
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
  world definition's `starting_region` and a seed from the world seed. An access point that leads to a new
  worldspace has a root region spec, designed, constructed, or generated, and a seed from its zone's seed pool, and
  generating the zone creates the worldspace from them. The surface zone's access points each have a target
  `dungeon_<k>/-1` and the `region_spec` `dungeon`, for example.
- **Seeds derive from the worldspace.** The worldspace's random engine, seeded from the worldspace's seed, draws each
  zone's seed in worldspace position order, since the extent fixes the number of zones, and then the seeds of its region
  specs. A region spec has a seed, and a region does not. A spec's `seed` takes precedence over the drawn one.
  `world.json` saves the worldspace's seed, and each zone's file saves the zone's. A spec's seed is used only for the
  draws made while the spec is resolved, so nothing saves it.
- **A zone's seed pool is drawn before the rest of its generation,** from the zone's seed, so the seeds of the
  worldspaces its access points lead to do not depend on the rest of the generation.
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
- **An extent is an object whose `arrangement` is `ranges` or `positions`:** a union of boxes, each with a range on
  each axis, or a list of zone positions (`world_generation_plan.md`, `### Extents`). A root region's extent is its
  worldspace's, and is `ranges` with a single box.
- **The start is a placeholder setting.** `world.start_zone` and `world.start_cell` default to `center`: the zone at
  the centroid of the starting worldspace's extent, and the cell nearest the center of that zone's layout that an actor
  can occupy. Any other value is a zone position or a cell position.
- **A zone is named from its region spec's `zone.name`,** or else `&region_name .. '_' .. zone_counter`, with
  `zone_counter` counting the zones in the worldspace from 0 in worldspace position order. Its placeholder name is
  replaced when its `ZoneSpec` is resolved.

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
  with its `arrangement`, and a top-level spec's extent is its worldspace's, checked to be `ranges` with a single box.
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
- [ ] 6. **Seeds from the worldspace.** The draws in `## Design`. `RegionPosition`, `new_region_position`, and the
  coordinate derivation in `region_seed` and `zone_seed` go.
- [ ] 7. **Access point trees.** Building a zone draws its seed pool, then calls the region spec tree function for each
  of its access points' trees, each with a seed from the pool. Region targets and `discard_made_zone` go.

## Open questions

- **Do a registered spec's `subregion.<name>` entries (`region_spec_plan.md`, `### subregion.`), with their counts and
  weighted choices, remain beside a tree's parent links?**

## Documents revised on approval

- `random_engine.md`, `## Generation`, and `region_spec_plan.md`, `## Seeds`: seeds drawn from the worldspace, and
  region specs, not regions, have seeds.
- `world_generation_plan.md`, `## Overview` and `## Creating a game`, `zone_plan.md`, `### The world`, and
  `region_spec_plan.md`, `## The structure World::create builds`: the starting worldspace created with the game, and
  each other worldspace when the zone of an access point that leads to it is generated.

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
