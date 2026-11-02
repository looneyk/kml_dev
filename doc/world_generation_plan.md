# World Generation --- Plan

Date: 2026-10-01, revised 2026-11-01

This plan describes how a new game's world is generated: the world's structure of worldspaces, regions, and zones,
created whole from specs before play starts, the registries of generators and generator selectors, and how each zone's
content is generated when it is needed. `zone_plan.md` defines the structure. Saving and opening a game are in
`session_plan.md` (`## Games`) and `game_file_spec.md`, and the open items are in `work_plan.md` (`### Generation`).

## Overview

- **The world's structure is created whole before play starts.** `World::create` creates every worldspace, region,
  and zone from the game's region specs, each zone with its `ZoneSpec`, its name, its position, and its seed, and no
  content. A world created again from the same world seed is the same world.
- **A zone's content is generated when it is needed,** the first time the zone is entered or loaded, from its
  `ZoneSpec` and its seed. A zone's content is created whole: the generator returns a `stratum_format::Document`, which
  `World` builds into the zone in one pass.
- **A generator selector chooses how each zone of a region is generated.** It is the region's policy: it reads the
  region and its zones, and chooses a generator and that generator's parameters or a pre-built zone. It runs when the
  structure is created, so every zone's `ZoneSpec` is complete, and every pre-built zone's name is in the structure,
  before play starts.
- **Most regions share one data-driven selector,** configured by the parameters in their `RegionSpec`. A region with
  unusual rules has a selector of its own.
- **A generator creates one zone's content.** It is a function from a `ZoneSpec` to a `Document`, and reads nothing
  from the world.
- **The host only passes specs.** The host delegates game, world, and character creation to `Game`. It calls
  `Game::create` with the specs for each of these.

## Data structures

```cpp
// A request to create a region and its subregions, kept by the region it created
using RegionSpec = KeyValueStore;
// A key with no prefix is the region's: generator_selector, the name of the region's generator selector, its naming
//     patterns, and its metadata: name, type (surface, dungeon, crypt, ...), competency level, difficulty, ...
// worldspace.*: for a top-level region, its worldspace's coordinate system: its extent and, for a grid worldspace, its
//     zone size
// extent: for a subregion, the positions of its parent's extent that it covers, as boxes (### Extents)
// entry_access: the access point type and offset at which the character enters a new game's first zone, such as
//     "staircase,1"; read by World::create only, and by no region that is not a game's starting region
// parameters.*: the region's generation parameters, which its selector reads
// zone.*: the ZoneSpec that the region's zones are created from, with the prefix removed; the selector modifies it for
//     each zone, as for its position
// subregion.*: the names of the region specs of its subregions

// A request to create a zone. The structure states the position; the selector adds the generator's parameters
using ZoneSpec = KeyValueStore;
// name, position, ...
// parameters.*: the zone's generation parameters, which its generator reads, the seed among them

// Exactly one of generator_name and zone_template is set
struct GeneratorSelection
{
    std::optional<std::string> generator_name;   // A generator creates the zone's content
    std::optional<std::string> zone_template;    // A pre-built zone, by its extended name in the pregenerated content index
    ZoneSpec zone_spec;                          // The request, completed with the generator's parameters
};

using generator_selector_type = std::function<GeneratorSelection(const World& world, flecs::entity region, const ZoneSpec& zone_spec)>;
using generator_type = std::function<stratum_format::Document(const ZoneSpec& zone_spec)>;

class Game
{
    // <generator selector name, generator selector>
    std::map<std::string, generator_selector_type> m_generator_selectors;

    // <generator name, generator>
    std::map<std::string, generator_type> m_generators;

    // <region spec name, region spec>
    std::map<std::string, RegionSpec> m_region_specs;

    World m_world; // Takes the three maps by reference
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
//     ZoneLayout, ZoneNotation, and its layout entities and entities, built from the Document once its content is
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

- **`Game` owns the three maps,** which last as long as the program. `World` takes them by
  reference in its constructor, as it takes the id allocator, and `World::clear` keeps them.
- **The generators are in `quetzal/stratum_gen`,** which returns a map from each generator's name to its function, each
  taking a `ZoneRequest`: the name, the position, the seed, and the generator's parameters. Each host registers them as
  `generator_type`, making the `ZoneRequest` from the settings and the `ZoneSpec`, since the hosts are what link both
  libraries. `stratum_sim` depends on no generator, and `stratum_gen` on no part of `stratum_sim`.
- **The generator selectors and the region specs are the game's content.** Region specs are design assets, or derive
  their values from design assets, and the selectors become data or script (`modding_plan.md`); until then each host
  registers temporary ones.
- **A name that is not registered is an error** that reaches the host.

## Creating a game

```cpp
void Game::create(const std::vector<std::string>& region_spec_names, const CharacterSpec& character_spec)
{
    clear();
    // reset the id allocator, reseed the random engines, empty the working directory and create the game directory there
    m_character.create(character_spec);
    flecs::entity access_point = m_world.create(region_spec_names, game_directory);
    m_session.start(access_point);
}

flecs::entity World::create(const std::vector<std::string>& region_spec_names, const std::filesystem::path& game_directory)
{
    // keep game_directory, which zones are saved into
    // create the structure: a worldspace for each top-level region spec, its regions, and their zones
    flecs::entity zone = the starting region's first zone;
    generate(zone);
    auto [type_name, offset] = the type and offset parsed from the starting region spec's entry_access;
    return entry_point(zone, access_type(type_name), offset);
}

void Session::start(flecs::entity access_point)
{
    // enters zone_of(access_point), placing the character at the access point
}
```

- **A spec's parts are separated by key prefixes.** A key with no prefix is read by the spec's consumer, and
  `select_prefixed(spec, prefix)` returns the entries under a prefix with it removed, so `zone.` yields an ordinary
  `ZoneSpec`. Each consumer checks the keys it reads with `find_unknown_keys(spec, known_keys)`. Both are stratum's
  (`specs.hpp`), not `KeyValueStore`'s: `World::create` reads the region spec's keys with no prefix, and the
  generator selector and generator their prefixes.
- **The game's top-level region specs are its worldspaces.** Each creates a worldspace with the coordinate system its
  `worldspace.` keys state, and the region and its subregions, from the region specs its `subregion.` keys refer to.
- **The starting region's spec states the entry** in its `entry_access` key: the name of a registered access point
  type and the offset of the target at which the character enters, separated by a comma, in one string that its
  consumer parses. Only `World::create` reads it, so no other zone is affected. The library has no default, and the
  generator must place a matching access point. `World::create` returns `entry_point(zone, type, offset)` and throws,
  before the session starts, if the zone has none.
- **A region that is not a dungeon states its own entry.** A tower entered from below states `door,0`, and a dungeon
  level states `staircase,1`.
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

flecs::entity World::create_zone(flecs::entity worldspace, flecs::entity region, const ZoneSpec& zone_spec)
{
    const RegionSpec& region_spec = region.get<RegionSpec>();

    // the ZoneSpec's seed, or the next draw from the region's random engine
    ZoneSpec seeded_zone_spec = zone_spec with its seed under seed_key;
    GeneratorSelection selection = m_generator_selectors.at(region_spec.generator_selector)(*this, region, seeded_zone_spec);

    flecs::entity zone = create the zone in the worldspace, with a new PersistentId, its name from the region's
        naming pattern or the pre-built zone's, its region, the ZonePosition of zone_spec, and selection's ZoneSpec;
    add the zone's id to the region's zone ids;
    return zone;
}
```

| Value | From |
| --- | --- |
| The position | The structure: the region spec and the worldspace's coordinate system |
| The generator and its parameters, or a pre-built zone | The region's generator selector, from the region's spec and the zone's position |
| The seed | `World`, drawn from the region's random engine, unless the `ZoneSpec` has one (`random_engine.md`, `## Generation`) |
| The name | The region spec's naming pattern, or the pre-built zone's name |

- **The seed is added first.** `World` adds the zone's seed to the `ZoneSpec` under `seed_key` before calling the
  selector, so the selector and the generator both have it.
- **The selector reads the world; the generator does not.** The selector reads the region and its zones, and its
  random draws use a random engine seeded from the zone's seed. Anything from the region that a generator needs, such
  as difficulty, the selector copies into the zone's parameters.
- **The designer decides the positions and names of a region's zones,** through the region spec's extent and naming
  patterns.

### Extents

An extent is a list of boxes in the worldspace's coordinates, each with an inclusive range in `x`, `y`, and `z`. A
range `0..0` contains only 0, and an axis left out of a box spans the parent's whole range in it. A region's zones are
at the positions its extent covers and no subregion's does (`zone_plan.md`, `### Regions`). The key names below are
not designed.

A dungeon with ten levels, one zone to each, at `(0, 0, -1)` through `(0, 0, -10)`, and a crypt at levels -4 to -6:

```
barrow                                   // top-level region spec for the dungeon worldspace
    worldspace.extent: x 0..0, y 0..0, z -10..-1
    zone.*: ...                          // the barrow's own levels: -1 to -3 and -7 to -10
    subregion.barrow_crypt: crypt

crypt
    extent: z -6..-4
    zone.*: ...
```

A target of `-1` from `(0, 0, -6)` resolves to the zone at `(0, 0, -7)`, which is in the barrow: regions play no part
in it. A second zone on a level, such as an east wing, is at another position, `(1, 0, -5)`, and the worldspace's
extent spans `x 0..1`.

An 8x8 surface at `z = 0`, split along its anti-diagonal: the moor is the triangle `x + y <= 7`, and the surface region
itself the rest.

```
surface
    worldspace.extent: x 0..7, y 0..7, z 0..0
    worldspace.zone_size: 64, 64
    zone.*: ...                          // the rest of the surface
    subregion.west_moor: moor

moor                                     // one box for each row
    extent: x 0..7, y 0..0
            x 0..6, y 1..1
            x 0..5, y 2..2
            x 0..4, y 3..3
            x 0..3, y 4..4
            x 0..2, y 5..5
            x 0..1, y 6..6
            x 0..0, y 7..7
    zone.*: ...
```

## Generating a zone's content

```cpp
void World::generate(flecs::entity zone)
{
    const ZoneSpec& zone_spec = zone.get<ZoneSpec>();
    stratum_format::Document document;

    if (zone_spec has a zone template)
    {
        document = the pre-built zone's Document, from the pregenerated content index;
    }
    else
    {
        document = m_generators.at(generator name of zone_spec)(zone_spec);
    }

    // check the document with check_buildable before building anything
    load_layout(zone, document);
    populate(zone);
}
```

- **A zone's content is generated once,** the first time it is entered or loaded, whether a generator returned its
  `Document` or a pre-built zone supplied it, so every zone is built in the same way.
- **The generator writes the zone's position into the `Document`.**

## Generator selection

A region's selector decides, for each zone of the region, one of two outcomes:

- **A generator and its parameters,** which is the usual outcome.
- **A pre-built zone,** a zone template from the pregenerated content index, for a rare special zone. It is read into a
  `Document` when the zone's content is generated, and the zone is built from it; no generator reads a file.

### The starting region

The starting region is a surface region. Its first zone is a field or a forest with few denizens, if any, and a zone
next to it is a town. Every other zone of the region is drawn at random from fields, hills, forests and towns.

None of this is special code. The starting region is a `RegionSpec` whose parameters give the common selector an
opening sequence; any region may have one.

### The common selector

Most regions use one selector, configured by its `RegionSpec`'s parameters. Many have one primary generator and,
optionally, rare generators or pre-built zones that are drawn occasionally.

```cpp
// The starting region's spec, as the common selector reads it
starting_area
    type: surface
    generator_selector: common
    difficulty: 1
    service_level: basic
    opening: [{fields 1, forest 1}, {town 1}]        // weighted tables, one for each of the first zones, in order
    primary: {fields 4, forest 4, hills 2, town 1}   // the weighted table for every zone after the opening
    rare: {hermit_glade 0.02, ruined_shrine 0.01}    // generators or pre-built zones, each drawn with its probability
    entry_access: waypoint,0                         // the access point the first zone's generator places
    zone.parameters.denizens: few

GeneratorSelection common(const World& world, flecs::entity region, const ZoneSpec& zone_spec)
{
    const RegionSpec& region_spec = region.get<RegionSpec>();
    size_t zone_index = the zone's index among the region's zones;

    if (zone_index < region_spec.opening.size())
    {
        return {generator_name: draw from region_spec.opening[zone_index], zone_spec + the region's parameters};
    }

    if (a draw from region_spec.rare selects an entry)
    {
        return {generator_name or zone_template: that entry, zone_spec + the region's parameters};
    }

    return {generator_name: draw from region_spec.primary, zone_spec + the region's parameters};
}
```

- **A weighted table is the mechanism that placement draws from** (`placement_plan.md`, `## A run's roles`), so
  the selector and placement share it.
- **A selector of its own is script,** once scripting is chosen (`modding_plan.md`), for a region whose rules the
  common selector's parameters cannot express. Until then each is C++ that a host registers.
- **A zone's denizens are among its parameters,** such as the starting zone's few, for the population strategy that
  replaces `World::populate`'s placeholder.

## Targets

- **Every zone a target refers to exists before play starts,** so an unbound target resolves to a zone of the
  structure, by its extended name or by its position (`zone_plan.md`, `### Targets`).
- **Going through an unbound target generates the zone's content if it has none,** places the arriving actor at the
  named access point or at an unbound access point of the arrival type whose target leads back, and binds both ends.
- **An entrance into another worldspace,** such as a dungeon entrance in a fields zone, has the extended name of a zone
  in that worldspace as its target: the selector puts it into the zone's parameters, and the generator writes it into
  the access point's target.

A new game, from the host's call through the structure, the first zone, and a descent into a dungeon:

```
host           Game::create({surface, barrow_dungeon}, character_spec)
Game           m_world.create({surface, barrow_dungeon})
World            worldspace surface, its region starting_area and its subregions, every zone with its ZoneSpec
World            worldspace barrow_dungeon, its region, every zone with its ZoneSpec
World            generate(the starting region's first zone)
World              generator "forest"                                -> Document, a staircase to barrow_dungeon/level_1
World              load_layout, populate
World          entry_point(the first zone, waypoint, 0)              -> access point
Game           m_session.start(access point)

character walks east into the adjacent zone
World          generate(the adjacent zone)                           -> generator "town", as the opening states

character goes down the staircase in the first zone
World          resolve the target barrow_dungeon/level_1             -> the zone of that extended name
World            generate(barrow_dungeon/level_1)                    -> generator "dungen", with a staircase +1 and one -1
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
