# World Structure --- Plan

When a game is created, `World` creates the world's structure whole: every worldspace, its top-level region, and an empty
zone at every position of the worldspace's extent. An empty zone has no layout and no entities until it is first
entered or loaded. The steps below are commit-sized units, each passing every test, and are the work of **The world's
structure created whole** in `work_plan.md`.

## Design

- **For now, every zone is created from one default `ZoneSpec`,** with the zone's position and seed added. How a zone's
  `ZoneSpec` is made from its region spec and the spec files, with generation parameters or the name of a pregenerated
  zone, is designed later.
- **An empty zone is a zone with no layout,** built in memory and saved, unloaded and read back like any other. Its file
  has no `cells` (`discrete_layout.md`), and from the time it is generated its file has the layout.
- **Zones are created in worldspace position order:** `z`, then `y`, then `x`, each from lowest to highest
  (`region_spec_plan.md`, `## Zones of a worldspace`).
- **Seeds derive from the worldspace.** A random engine seeded from the world seed draws each worldspace's seed, in the
  order the worldspaces are created. The worldspace's random engine draws each zone's seed in worldspace position order,
  since the extent fixes the number of zones, and then the seeds of its region specs. A region spec has a seed, and a
  region does not. A spec's `seed` takes precedence over the drawn one. `world.json` saves the worldspace's seed, and each zone's file
  saves the zone's. A spec's seed is used only for the draws made while the spec is resolved, so nothing saves it.
- **The world definition file lists the game's region spec trees.** It is `assets/data/world_definition.json`, and
  more of the world's content is added to it later. Each tree is a list of region specs linked by the name of each
  spec's parent region, and a root spec has none. Each region's list of subregions is built when the file is loaded, for
  traversing the tree. A tree is identified by its root region's name, and the file's `starting_region` is the root
  region of the tree a new game starts in, `surface` in the MVP's file. The file lists the trees under `region_specs`.
- **A worldspace is created from the root region spec of a region spec tree.** The surface zone's staircases leading
  down each have a target `dungeon_<k>/-1` and the `region_spec` `dungeon`, and generating the zone creates each of
  those worldspaces from that spec.
- **A region spec has a `name` and a `display_name`, which its region takes.** The name is internal, part of the
  region's extended name, and the display name is the player's. Root spec names are distinct, and so are the names of
  the specs with the same parent, so extended names are distinct. A spec's name in `region_specs.json` refers to a
  designed spec.
- **A region spec is composed as it is read,** as `merge(parent, merge(designed, own))`. The designed spec that its
  `region_spec` key refers to is merged with the spec's own keys first, and the result is merged over the parent's
  resolved spec. The parent's layer has only its `store`, since a subregion does not inherit the typed members. A root
  spec has no parent's layer.
- **A region spec has typed members and a `store`.** The typed members, each optional, are `name`, `display_name`,
  `parent`, `region_spec`, `seed`, `extent`, a top-level spec's `zone_size`, and the listed `zones`. The other
  keys, such as `zone.*`, `parameters.*`, and the metadata, are in a `KeyValueStore`.
- **A layer is a merge patch** (JSON Merge Patch, RFC 7386). A key adds or replaces a value, and the value `&null`
  removes the key, in place of JSON's `null`. `&null` is a pattern whose value is null, and resolving the composed spec
  removes the key (`pattern_resolution_plan.md`, `## Design`). A
  list is replaced whole. A subregion's overrides and a mod's patches to types and styles (`modding_plan.md`)
  are one mechanism.
- **A spec's patterns are resolved once it is composed,** with draws from the spec's seed, so each spec is as
  instantiated as it can be. A parent is resolved before its subregions, so `parent.<key>` in a pattern refers to the
  parent's resolved value. The `zone.*` patterns and `zone_name_pattern` are resolved for each zone, since they use the
  zone's position and index.
- **A `Region` contains its resolved spec,** and the region spec trees are discarded once the structure is created. The
  `region_spec` and `parent` keys, the spec ids, and the lists of subregion ids are only for construction, and the
  `RegionTree`'s links between persistent ids replace them. A region has no seed. `world.json` saves each region's
  resolved spec, so opening a game reads neither `region_specs.json` nor the world definition file.
- **A region spec in memory has an id,** its slot in a vector, assigned when it is created.
- **A zone has the region id of the most specific region whose extent contains its position.** That is what it means
  for a zone to be in a region.
- **An extent is an object whose `arrangement` is `ranges` or `positions`:** a union of boxes, each with a range on
  each axis, or a list of zone positions (`world_generation_plan.md`, `### Extents`). A root region's extent is its
  worldspace's, and is `ranges` with a single box.
- **The start is a placeholder setting.** `world.start_zone` and `world.start_cell` default to `center`: the zone at
  the centroid of the starting worldspace's extent, and the cell nearest the center of that zone's layout that an actor
  can occupy. Any other value is a zone position or a cell position.
- **For now, a zone is named from its root region spec's `zone_name_pattern`,** or else `<root region name>_<n>`, with n
  counting the worldspace's zones from 0 in worldspace position order. Zone names are revisited with the region spec
  tree.

## Steps

- [x] 1. **The structure created whole.** `World::create_worldspace` creates a worldspace, its top-level region, and an
  empty zone at every position of its extent, in worldspace position order, each named from the spec's
  `zone_name_pattern`.
- [x] 2. **The world definition file.** `stratum_format::WorldDefinitionDocument` reads and writes the file: its
  `starting_region` and its `region_specs`, a list of region spec trees. `assets/data/world_definition.json` has the
  `surface` and `dungeon` trees, and `load_world_definition` reads it into a `WorldDefinition` of trees by root region
  name, each root spec first. The starting region replaces `Generation::m_initial_region_spec_name`.
- [x] 3. **Structured region specs.** `RegionSpec`, one type in `stratum_format`, has the typed members and the `store`
  of `## Design`, read from `region_specs.json`, the world definition file, and `world.json`. An extent is an object
  with its `arrangement`, and a top-level spec's extent is its worldspace's, checked to be `ranges` with a single box.
- [ ] 4. **Region spec trees.** A spec's id is its slot in its tree's `region_spec_tree_type`. Each spec's `parent` is
  resolved to its parent's id, and each spec's list of subregion ids is built from those. The names of specs with the
  same parent are checked to be distinct. Each spec is composed and its patterns resolved as `## Design` describes,
  parents first (`pattern_resolution_plan.md`, step 2).
- [ ] 5. **The region spec tree function.** It calls `World::create_worldspace` for a tree's root region spec, then adds
  a subregion to the worldspace's `RegionTree` for each spec below the root, with the spec's `name` and `display_name`.
  Each zone has the region id of the most specific region whose extent contains its position (`zone_plan.md`, `###
  Regions`). `World::create` calls it for each tree. `world.json` saves every region of a worldspace with its parent's
  id and its resolved spec, and `World::open` rebuilds the tree.
- [ ] 6. **Seeds from the worldspace.** The draws in `## Design`. `RegionPosition`, `new_region_position`, and the
  coordinate derivation in `region_seed` and `zone_seed` go.
- [ ] 7. **Access point trees.** Building a zone draws its seed pool, then calls the region spec tree function for each
  of its access points' trees. Region targets and `discard_made_zone` go.

## Open questions

- **Do a registered spec's `subregion.<name>` entries (`region_spec_plan.md`, `### subregion.`), with their counts and
  weighted choices, remain beside a tree's parent links?**

## Documents revised on approval

- `random_engine.md`, `## Generation`, and `region_spec_plan.md`, `## Seeds`: seeds drawn from the worldspace, and
  region specs, not regions, have seeds.
- `work_plan.md`: the item refers to this plan.

## Background and proposal for original items 1 and 2

### World creation
assets has a file of region specs (predefined and designed)
world definition file has a list of region spec trees (region specs may or may not be  based on either of the above)
load fhe file ...
(for now, instead of all of the above, we're pretending the file has a single tree with a single default region spec with only an extent provided, and it is stubbed in.)
for each region spec tree
    create a worldspace for the root region spec
        create an empty zone for each position in the worldspace's extent (including its seed, all worldspace zone seeds produced here, no other random usage)
    traverse the tree
        create a region for each region spec in the tree, filling in the parent and child relationships (region/zone)
when a zone is created (on entry or load as we decide)
    I'm thinking we'll need a a pool of seeds to be produced up front for any subregion specs/subworldspaces so it preceeds the more detailed randomization
    (Even if access points are generated first for early seeds, there may be some randomization even in that)
    Once access points are generated, each of them may have a region (typically vertical) and the "for each region spec" loop above is applied to that set of access point region specs

### Region spec trees

A world definition file is a list of region spec trees (more world content added later).
Root region spec names must be distinct.
Subregions with the same parent region must have distinct names.
This ensures that extended names are distinct.
Each tree itself is another list of region specs linked by parent region name (root regions have no parent).
This can be used to construct the list of region spec trees in memory.
Region specs in memory can be assigned ids (likely the slot in a vector) when they are created.
Zones have a region id for the most specific region they are in rather than relying on extended names.
This is all it means for a zone to be "in a region" or "belong to a region" (don't use the word belong for this)
Region specs define their extent one of two ways: A union of boxes with ranges or a list of zone positions.
The current extent in doc\world_generation_plan.md should be an object with an "arrangement" (or something) which can have values: "ranges" or "positions".
