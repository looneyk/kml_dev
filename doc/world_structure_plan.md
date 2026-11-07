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
  region does not. A spec's `seed` takes precedence over the drawn one. `world.json` saves the worldspace's seed, the region's copy of its spec saves the
  spec's seed, and each zone's file saves the zone's.
- **A worldspace is created from the root region spec of a region spec tree.** The `surface` spec in
  `assets/data/region_specs.json`, which `Generation::m_initial_region_spec_name` refers to, is the MVP's one tree until
  the world definition file replaces it. The surface zone's staircases leading down each have a target `dungeon_<k>/-1`
  and the `region_spec` `dungeon`, and generating the zone creates each of those worldspaces from that spec.
- **The start is a placeholder setting.** `world.start_zone` and `world.start_cell` default to `center`: the zone at
  the centroid of the starting worldspace's extent, and the cell nearest the center of that zone's layout that an actor
  can occupy. Any other value is a zone position or a cell position.
- **For now, a zone is named from its root region spec's `zone_name_pattern`,** or else `<root region name>_<n>`, with n
  counting the worldspace's zones from 0 in worldspace position order. Zone names are revisited with the region spec
  tree.

## Steps

2. **The region spec tree function.** It creates a worldspace for a tree's root region spec, then a region for each spec
   in the tree. `World::create` calls it for each tree; for the MVP, one tree of the `surface` spec. `world.json` saves
   and opens region trees.
3. **Seeds from the worldspace.** The draws in `## Design`. `RegionPosition`, `new_region_position`, and the
   coordinate derivation in `region_seed` and `zone_seed` go.
4. **Access point trees.** Building a zone draws its seed pool, then calls the region spec tree function for each of its
   access points' trees. Region targets and `discard_made_zone` go.

## Documents revised on approval

- `random_engine.md`, `## Generation`, and `region_spec_plan.md`, `## Seeds`: seeds drawn from the worldspace, and
  region specs, not regions, have seeds.
- `region_spec_plan.md`, `## The game's region specs`: region spec trees in the world definition file.
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
