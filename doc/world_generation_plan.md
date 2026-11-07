# World Generation --- Plan

Date: 2026-10-01, revised 2026-11-01

This plan describes how a new game's world is generated: the world's structure of worldspaces, regions, and zones,
created whole from specs before play starts, the registries of zone generators and region specs, and how each zone's
content is generated when it is needed. `zone_plan.md` defines the structure, and `template_plan.md` the templates
that zones are made from. Saving and opening a game are in
`session_plan.md` (`## Games`) and `game_file_spec.md`, and the open items are in `work_plan.md` (`### Generation`).

## Overview

- **The world's structure is created whole before play starts.** `World::create` creates every worldspace, region,
  and zone from the game's region specs, each zone with its `ZoneSpec`, its name, its position, and its seed, and no
  content. A world created again from the same world seed is the same world.
- **A zone's content is generated when it is needed,** the first time the zone is entered or loaded, from its
  `ZoneSpec` and its seed. A zone's content is created whole: the generator returns a `stratum_format::Document`, which
  `World` builds into the zone in one pass.
- **A region spec's zone pattern defines each zone of the region.** The `zone.` keys, resolved with the zone's
  position and index (`region_spec_plan.md`, `## Patterns`), are the zone's `ZoneSpec`, its template and the zone
  generator's parameters among them; weighted tables in the pattern are drawn from as it is resolved. The specs are
  resolved when the structure is created, so every zone's `ZoneSpec` is complete before play starts.
- **The zone generator applies the `ZoneSpec` to its template** (`template_plan.md`).
- **A game has one zone generator,** chosen by the game's notation, since a zone generator writes one zone format. It
  creates one zone's content, as a function from a `ZoneSpec` to a `Document`, and reads nothing from the world.
- **The host only passes specs.** The host delegates game, world, and character creation to `Game`. It calls
  `Game::create` with the specs for each of these.

## Data structures

```cpp
// A request to create a region and its subregions, kept by the region it created
struct RegionSpec
{
    RegionPosition position() const;
    void set_position(const RegionPosition& position);

    KeyValueStore keys;
};
// A key with no prefix is the region's: its naming patterns, and its metadata: name, type (surface, dungeon, crypt, ...), competency level, difficulty, ...
// worldspace.*: for a top-level region, its worldspace's coordinate system: its extent and, for a grid worldspace, its
//     zone size
// extent: for a subregion, the positions of its parent's extent that it covers, as boxes (### Extents)
// zone.*: the pattern of the ZoneSpec of each of the region's zones, with the prefix removed, resolved for each zone
// subregion.*: the names of the region specs of its subregions

// A request to create a zone. The structure sets the position, and the region spec's zone pattern the other keys
struct ZoneSpec
{
    ZonePosition position() const;
    void set_position(const ZonePosition& position);

    KeyValueStore keys;
};
// name, position, template, ...
// parameters.*: the zone's generation parameters, which its generator reads, the seed among them

using zone_generator_type = std::function<stratum_format::Document(const ZoneSpec& zone_spec)>;

class Game
{
    std::map<stratum_format::Notation, zone_generator_type> m_zone_generators;
    zone_generator_type m_zone_generator; // The one for the game's notation

    // <region spec name, region spec>
    std::map<std::string, RegionSpec> m_region_specs;

    World m_world; // Takes the registries by reference
    ...
};
```

The entities:

```cpp
// Worldspace entity
//     Worldspace tag
//     PersistentId
//     its name, and its coordinate system from its top-level region's spec

// Region entity
//     Region tag
//     PersistentId
//     its parent region, if it has one, and its extended name
//     RegionSpec           its own copy, which its zones were created from
//     zone ids             the zones in it, loaded or not
//     metadata components  built from the spec's metadata: name, type, difficulty, ...

// Zone entity, a child of its worldspace
//     Zone tag
//     PersistentId
//     its name and its region
//     ZonePosition         its position in its worldspace
//     ZoneSpec             complete, with its seed and its generator's parameters
//     ZoneLayout and its layout entities and entities, built from the Document once its content is
//     generated
```

- **A region keeps its own copy of its spec.** A spec in `m_region_specs` is a definition that many regions are created
  from, such as every crypt; a region is unaffected if the definition changes later.
- **The ids are `flecs::entity` in memory and `PersistentId` when stored.** A zone refers to its region by the
  region's persistent id or its extended name (`zone_plan.md`, `### Identity`).
- **A zone keeps its `ZoneSpec`** until it is decided whether one is needed after its content is generated. The
  generator writes the parameters it ran with into the `Document`'s `generator` section, so a zone's file has every
  parameter of its generation.

## Registries

- **`Game` owns the registries,** which last as long as the program. `World` takes them by
  reference in its constructor, as it takes the id allocator, and `World::clear` keeps them.
- **The generators are in `quetzal/stratum_gen`,** which returns a map from each generator's name to its function, each
  taking a `ZoneRequest`: the name, the position, the seed, and the generator's parameters. Each host registers the one for each notation as
  a `zone_generator_type`, making the `ZoneRequest` from the settings and the `ZoneSpec`, since the hosts are what link both
  libraries. `stratum_sim` depends on no generator, and `stratum_gen` on no part of `stratum_sim`.
- **The region specs and the templates are the game's content.** Both are design assets, or derive their values from
  design assets (`modding_plan.md`); until then each host registers a temporary region spec.
- **A name that is not registered is an error** that reaches the host.

## Creating a game

```cpp
void Game::create(const std::vector<std::string>& region_spec_names, const CharacterSpec& character_spec)
{
    clear();
    // reset the id allocator, reseed the random engines, empty the working directory and create the game directory there
    m_character.create(character_spec);
    flecs::entity zone = m_world.create(region_spec_names, the world.start_zone setting, game_directory);
    m_session.start(zone, start_placement(zone, the world.start_cell setting));
}

flecs::entity World::create(const std::vector<std::string>& region_spec_names, start_zone, const std::filesystem::path& game_directory)
{
    // keep game_directory, which zones are saved into
    // create the structure: a worldspace for each top-level region spec, its regions, and their zones
    flecs::entity zone = the starting worldspace's zone at start_zone;
    generate(zone);
    return zone;
}

void Session::start(flecs::entity zone, const placement_type& placement)
{
    // enters the zone, placing the character at the placement
}
```

- **A spec's parts are separated by key prefixes.** A key with no prefix is read by the spec's consumer, and
  `select_prefixed(spec, prefix)` returns the entries under a prefix with it removed, so `zone.` yields an ordinary
  `ZoneSpec`. Each consumer checks the keys it reads with `find_unknown_keys(spec, known_keys)`. Both are stratum's
  (`specs.hpp`), not `KeyValueStore`'s: `World::create` reads the region spec's keys with no prefix, and the
  zone generator the `parameters.` keys of the resolved `ZoneSpec`.
- **The game's top-level region specs are its worldspaces.** Each creates a worldspace with the coordinate system its
  `worldspace.` keys state, and the region and its subregions, from the region specs its `subregion.` keys refer to.
- **The start settings place the character** in a new game: `world.start_zone`, the zone of the starting worldspace,
  and `world.start_cell`, the cell in it, each `center` or a position. `World::create` and `start_placement` throw,
  before the session starts, for a zone or a cell that does not exist or that an actor cannot occupy.
- **`Session::start` enters the access point's zone.** `Game::open` starts the session in the zone the
  character is in instead.

## Creating the structure

```cpp
flecs::entity World::create_region(flecs::entity worldspace, flecs::entity parent_region, const RegionSpec& region_spec)
{
    // Region tag, a new PersistentId, its parent region, its name from its parent's naming pattern, the RegionSpec,
    // and the metadata components built from it
    // create_zone for each of its zones, then create_region for each of its subregions
}

flecs::entity World::create_zone(PersistentId region_id, const ZoneSpec& zone_spec)
{
    const Region& region = this->region(region_id);
    const RegionSpec& region_spec = region.region_spec;

    // the region spec's zone pattern resolved for the zone, under the given keys, with the ZoneSpec's seed or the next
    // draw from the region's random engine under seed_key
    ZoneSpec resolved_zone_spec = resolve_zone_spec(region_spec, zone_spec);

    flecs::entity zone = create the zone in the worldspace, with a new PersistentId, its name from the resolved
        naming pattern, its region, the ZonePosition of zone_spec, and resolved_zone_spec;
    add the zone's id to the region's zone ids;
    return zone;
}
```

| Value | From |
| --- | --- |
| The position | The structure: the region spec and the worldspace's coordinate system |
| The template and the generator's parameters | The region spec's zone pattern, resolved with the zone's position and index |
| The seed | `World`, drawn from the region's random engine, unless the `ZoneSpec` has one (`random_engine.md`, `## Generation`) |
| The name | The region spec's naming pattern |

- **The seed is added first,** so the pattern's draws and the generator both have it. The draws use a random engine
  seeded from the zone's seed.
- **The generator does not read the world.** Anything from the region that a generator needs, such as difficulty, is
  in the zone pattern's parameters.
- **The designer decides the positions and names of a region's zones,** through the region spec's extent and naming
  patterns.

### Extents

An extent is a list of boxes in the worldspace's coordinates, and its positions are the union of the boxes. Each axis
of a box is an inclusive range `[a, b]`, a single integer for one position, as `"x": 3` is `[3, 3]`, or left out, which
spans the parent's whole range on that axis. A box can be any size, so an extent can have any form made of boxes, such
as a ring.

- **A top-level region's extent is the worldspace's extent,** one box with every axis given.
- **Boxes should not overlap.** An overlap, or a position listed twice, is logged as a warning, and the extent is the
  union of the boxes.
- **A region has a zone at each position of its extent that is in no subregion's extent** (`zone_plan.md`, `###
  Regions`).

The key names below are not designed.

A surface of 4x3 zones with no subregions is one box, and the top-level region has a zone at each of its twelve
positions, created in worldspace position order from `(0, 0, 0)` to `(3, 2, 0)`:

```json
"plains": {
    "worldspace": {"extent": [{"x": [0, 3], "y": [0, 2], "z": 0}], "zone_size": [64, 64]},
    "zone": {"name": "plains_{x}_{y}"}
}
```

A dungeon with ten levels, one zone to each, at `(0, 0, -1)` through `(0, 0, -10)`, and a crypt at levels -4 to -6. The
barrow's own levels are -1 to -3 and -7 to -10:

```json
"barrow": {
    "worldspace": {"extent": [{"x": 0, "y": 0, "z": [-10, -1]}]},
    "subregion": {"barrow_crypt": "crypt"}
},
"crypt": {
    "extent": [{"z": [-6, -4]}]
}
```

A target of `-1` from `(0, 0, -6)` resolves to the zone at `(0, 0, -7)`, which is in the barrow: regions play no part
in it. A second zone on a level, such as an east wing, is at another position, `(1, 0, -5)`, and the worldspace's
extent spans `"x": [0, 1]`.

An 8x8 surface at `z = 0`, split along its anti-diagonal: the moor is the triangle `x + y <= 7`, and the surface region
itself the rest.

The moor's staircase edge needs a box for each step:

```json
"surface": {
    "worldspace": {"extent": [{"x": [0, 7], "y": [0, 7], "z": 0}], "zone_size": [64, 64]},
    "subregion": {"west_moor": "moor"}
},
"moor": {
    "extent": [
        {"x": [0, 7], "y": 0},
        {"x": [0, 6], "y": 1},
        {"x": [0, 5], "y": 2},
        {"x": [0, 4], "y": 3},
        {"x": [0, 3], "y": 4},
        {"x": [0, 2], "y": 5},
        {"x": [0, 1], "y": 6},
        {"x": 0, "y": 7}
    ]
}
```

A shore around a lake in the middle of another 8x8 surface is a ring of four boxes, two rows and two columns, each larger
than one zone in both dimensions. A single position is a box with an integer on each axis it includes:

```json
"shore": {
    "extent": [
        {"x": [1, 6], "y": [1, 2]},
        {"x": [1, 6], "y": [5, 6]},
        {"x": [1, 2], "y": [3, 4]},
        {"x": [5, 6], "y": [3, 4]}
    ]
},
"islands": {
    "extent": [{"x": 3, "y": 3}, {"x": 4, "y": 4}]
}
```

## Generating a zone's content

```cpp
void World::generate(flecs::entity zone)
{
    const ZoneSpec& zone_spec = zone.get<ZoneSpec>();

    // the zone generator applies the spec to the spec's template, if it has one
    stratum_format::Document document = m_zone_generator(zone_spec);

    // check the document with check_buildable before building anything
    load_layout(zone, document);
    populate(zone);
}
```

- **A zone's content is generated once,** the first time it is entered or loaded, so every zone is built in the same
  way.
- **The generator writes the zone's position into the `Document`.**

## Targets

- **Every zone a target refers to exists before play starts,** so an unbound target resolves to a zone of the
  structure, by its extended name or by its position (`zone_plan.md`, `### Targets`).
- **Going through an unbound target generates the zone's content if it has none,** places the arriving actor at the
  named access point or at an unbound access point of the arrival type whose target leads back, and binds both ends.
- **An entrance into another worldspace,** such as a dungeon entrance in a fields zone, has the extended name of a zone
  in that worldspace as its target: the zone pattern puts it into the zone's parameters, and the generator writes it into
  the access point's target.

A new game, from the host's call through the structure, the first zone, and a descent into a dungeon:

```
host           Game::create({surface, barrow_dungeon}, character_spec)
Game           m_world.create({surface, barrow_dungeon})
World            worldspace surface, its region starting_area and its subregions, every zone with its ZoneSpec
World            worldspace barrow_dungeon, its region, every zone with its ZoneSpec
World            generate(the starting region's first zone)
World              the zone generator, template forest               -> Document, a staircase to barrow_dungeon/level_1
World              load_layout, populate
World          entry_point(the first zone, waypoint, 0)              -> access point
Game           m_session.start(access point)

character walks east into the adjacent zone
World          generate(the adjacent zone)                           -> template town, as the opening has it

character goes down the staircase in the first zone
World          resolve the target barrow_dungeon/level_1             -> the zone of that extended name
World            generate(barrow_dungeon/level_1)                    -> the zone generator, with a staircase +1 and one -1
World          the two staircases bound to each other
```

## Replacements

- **`Generator` and the `std::shared_ptr<const Generator>` component** are replaced by the registries, and
  `create_zone(region, const Generator&)` and `create_zone(region, level)` by `create_zone(worldspace, region,
  zone_spec)` and `generate(zone)`.
- **`Session::create` and `Session::load`** go: `World` creates every zone, and the session enters them.
- **The hosts' `DungenGenerator`** becomes dungen's generator in `stratum_gen`.
- **The hosts' zone file** goes: each host creates a new game with `Game::create`, whose first zone is generated.

## Open

The open items are tracked in `work_plan.md`, under `### Generation`.
