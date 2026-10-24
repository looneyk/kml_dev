# World Generation --- Plan

Date: 2026-10-01

This plan describes how a new game's world is generated: how a region and a zone are each created whole from a spec, the
registries of generators and generator selectors that do it, and how relocators lead to the zones and regions that are
created as the character moves. It covers the world before anything is saved, when every region and zone is generated.
Saving and opening a game are in `session_plan.md` (`## Games`) and `game_file_spec.md`, and the open items are in
`work_plan.md` (`### Generation`).

## Overview

- **A region and a zone are each created whole.** A region is created from a `RegionSpec`, and a zone from the
  `stratum_format::Document` its generator returns, which `World` builds into the zone in one pass. Nothing is created
  empty and filled in afterward.
- **A region is an entity with no hierarchy.** It has no parent region and no child regions. Zones connect only through
  their access points, so the bound relocators between zones are the world's connections, within a region and between
  regions.
- **A zone is in one region,** as a child of the region's entity, and every zone is created in a region.
- **A generator selector chooses how each zone of a region is created.** It is the region's policy: it reads the
  region and the zones the region already has, and chooses a generator and that generator's parameters, a pre-built
  zone, or a new region for the zone to begin, which becomes certain as the region reaches its zone limit.
- **Most regions share one data-driven selector,** configured by the parameters in their `RegionSpec`. A region with
  unusual rules has a selector of its own.
- **A generator creates one zone.** It is a function from a `ZoneSpec` to a `Document`, and reads nothing from the world.
- **Zones are created on demand.** A new game creates its starting region and that region's first zone. Every other zone
  is created when an unbound relocator is first accessed.
- **The host only passes specs.** The host delegates game, region/zone, and character creation to `Game`.
  It calls `Game::create` with the specs for each of these.

## Data structures

```cpp
// A request to create a region, kept by the region it created, from which every later zone of the region is created
using RegionSpec = KeyValueStore;
// A key with no prefix is the region's: generator_selector, the name of the region's generator selector, and its
//     metadata: name, type (surface, dungeon, crypt, ...), competency level, difficulty, service level, ...
// entry_access: the access point type and level offset at which the character enters a new game's first zone, such as
//     "staircase,1"; read by World::create only, and by no region that is not a game's starting region
// parameters.*: the region's generation parameters, which its selector reads
// zone.*: the ZoneSpec that the region's zones are created from, with the prefix removed; the caller and the selector
//     modify it for each zone, as for its level

// A request to create a zone. The caller states the level; the selector adds the generator's parameters
using ZoneSpec = KeyValueStore;
// level, name, ...
// parameters.*: the zone's generation parameters, which its generator reads, the seed among them

// Exactly one of generator_name, zone_template and region_spec_name is set
struct GeneratorSelection
{
    std::optional<std::string> generator_name;   // A generator creates the zone
    std::optional<std::string> zone_template;    // A pre-built zone, by its tag in the pregenerated content index
    std::optional<std::string> region_spec_name; // The zone is the first of a new region, created from this region spec
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
// Region entity
//     Region tag
//     PersistentId
//     RegionSpec           its own copy, which every later zone of the region is created from
//     zone ids             the zones created in it, loaded or not, whose number is the zone count its selector reads;
//                          not saved apart from world.json's list of its zones
//     metadata components  built from the spec's metadata: name, type, difficulty, ...

// Zone entity, a child of its region
//     Zone tag
//     PersistentId
//     ZonePosition         from the ZoneSpec
//     ZoneLayout, ZoneNotation, and its layout entities and entities, built from the Document
```

- **A region keeps its own copy of its spec.** A spec in `m_region_specs` is a definition that many regions are created
  from, such as every crypt; a region is unaffected if the definition changes later.
- **A region counts the zones created in it.** Zones unload, so the loaded zones are not the zones the region has
  created, and the count is what a selector's opening sequence and zone limit read. A saved zone read back after
  unloading is not counted again. The count is not saved: it is the number of the region's zones in `world.json`.
- **The ids are `flecs::entity` in memory and `PersistentId` when stored.** A region needs no spec id, since its spec
  is a component on it, and a zone finds its region with `region_of(zone)`.
- **A zone keeps no `ZoneSpec`.** The generator writes the parameters it ran with into the `Document`'s `generator`
  section, so a zone's file has every parameter of its generation.

## Registries

- **`Game` owns the three maps,** which last as long as the program. `World` takes them by
  reference in its constructor, as it takes the id allocator, and `World::clear` keeps them.
- **The generators are in `quetzal/stratum_gen`,** which returns a map from each generator's name to its function, each
  taking a `ZoneRequest`: the name, the position, the seed, and the generator's parameters. Each host registers them as
  `generator_type`, making the `ZoneRequest` from the settings and the `ZoneSpec`, since the hosts are what link both
  libraries. `stratum_sim` depends on no generator, and `stratum_gen` on no part of `stratum_sim`.
- **The generator selectors and the region specs are the game's content.** They become data or script (`modding_plan.md`);
  until then each host registers temporary ones.
- **A name that is not registered is an error** that reaches the host.

## Creating a game

```cpp
void Game::create(const RegionSpec& region_spec, const CharacterSpec& character_spec)
{
    clear();
    // reset the id allocator, reseed the random engines, empty the working directory and create the game directory there
    m_character.create(character_spec);
    flecs::entity access_point = m_world.create(region_spec, game_directory);
    m_session.start(access_point);
}

flecs::entity World::create(const RegionSpec& region_spec, const std::filesystem::path& game_directory)
{
    // keep game_directory, which zones are saved into
    ZoneSpec zone_spec = select_prefixed(region_spec, "zone.");
    flecs::entity zone = create_region_zone(region_spec, zone_spec);
    auto [type_name, offset] = the type and level offset parsed from region_spec's entry_access;
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
- **The region's `zone.` keys are the zone spec of every zone in it,** which the caller and the selector modify, as
  with the level a relocator states. The first zone of the starting region is created from them as they are.
- **The starting region's spec states the entry** in its `entry_access` key: the name of a registered access point
  type and the level offset of the target at which the character enters, separated by a comma, in one string
  that its consumer parses. Only `World::create` reads it, so no later zone is affected. The library has no
  default, and the generator must place a matching access point. `World::create` returns `entry_point(zone, type,
  offset)` and throws, before the session starts, if the zone has none.
- **A region that is not a dungeon states its own entry.** A tower entered from below states `door,0`, and a dungeon
  level states `staircase,1`.
- **`Session::start` enters the access point's zone.** `Game::open` starts the session in the zone the
  character is in instead.

## Creating a region and its first zone

```cpp
flecs::entity World::create_region_zone(const RegionSpec& region_spec, const ZoneSpec& zone_spec)
{
    flecs::entity region = create_region(region_spec);
    return create_zone(region, zone_spec);
}

flecs::entity World::create_region(const RegionSpec& region_spec)
{
    // Region tag, a new PersistentId, the RegionSpec, and the metadata components built from it
}
```

A region is never created without a zone, so `create_region_zone` is how every region is created: the starting region
by `World::create`, and every other region by a relocator that leads into a new region.

## Creating a zone

```cpp
flecs::entity World::create_zone(flecs::entity region, const ZoneSpec& zone_spec)
{
    const RegionSpec& region_spec = region.get<RegionSpec>();

    // the ZoneSpec's seed, or one derived from the region's seed and the zone's coordinates
    ZoneSpec seeded_zone_spec = zone_spec with its seed under seed_key;
    GeneratorSelection selection = m_generator_selectors.at(region_spec.generator_selector)(*this, region, seeded_zone_spec);

    if (selection.region_spec_name)
    {
        return create_region_zone(m_region_specs.at(*selection.region_spec_name), zone_spec);
    }

    stratum_format::Document document;

    if (selection.zone_template)
    {
        document = the pre-built zone's Document, from the pregenerated content index;
    }
    else
    {
        document = m_generators.at(*selection.generator_name)(selection.zone_spec);
    }

    // check the document with check_buildable before creating anything

    flecs::entity zone = create the zone in the region, with a new PersistentId and the ZonePosition of zone_spec;
    add the zone's id to the region's zone ids;
    load_layout(zone, document);
    populate(zone);
    return zone;
}
```

The values a zone is created from come from three places:

| Value | From |
| --- | --- |
| The level, and anything else that depends on where the zone is reached from | The caller: the region's `zone.` keys, or the relocator gone through |
| The generator and its parameters, a pre-built zone, or a new region | The region's generator selector, from the region's spec, the region's zone count, and the level |
| The seed | `World`, derived from the region's seed and the zone's coordinates, unless the `ZoneSpec` has one (`random_engine.md`, `## Generation`) |

- **The seed is added first.** `World` adds the zone's seed to the `ZoneSpec` under `seed_key` before calling the
  selector, so the selector and the generator both have it.
- **The selector reads the world; the generator does not.** The selector reads the region and its zone count, and its
  random draws use a random engine seeded from the zone's seed. Anything from the region that a generator needs, such as difficulty, the
  selector copies into the zone's parameters.
- **The position comes from the request.** `World` sets `ZonePosition` from the `ZoneSpec`, and the generator writes
  the level of that position into the `Document`.
- **The zone is built only from its `Document`,** through `load_layout`, whether a generator returned it or a pre-built
  zone supplied it, so every zone is created whole in the same way.

## Generator selection

A region's selector decides, for each zone requested in the region, one of three outcomes:

- **A generator and its parameters,** which is the usual outcome.
- **A pre-built zone,** a zone template from the pregenerated content index, for a rare special zone. It is read into a
  `Document`, and the zone is built from it; no generator reads a file.
- **A new region,** with a probability that rises to 1 as the region's zone count approaches its limit. The zone
  requested becomes the first zone of a region created from the region spec the selector chooses, so the relocator that
  requested it leads into the new region.

### The starting region

The starting region is a surface area. Its first zone is a field or a forest with few denizens, if any, and its second
zone is a town. The second zone is reached through one of the first zone's exits, so the town is next to the first zone.
Every zone after that is drawn at random from fields, hills, forests and towns, or begins a new region, more difficult
than the starting region. The probability of a new region rises as the region's zone count approaches its limit, and
is 1 at the limit. The new region's spec is drawn from a list of the specs that may follow the starting region, and the
regions after it continue the same way, each spec listing the specs that may follow it.

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
    zone_count_max: 12
    next_region: {wild_hills 2, deep_forest 1}       // the weighted table of region specs that may follow this one
    zone.position.z: 0
    entry_access: waypoint,0                         // the access point the first zone's generator places
    zone.parameters.denizens: few

GeneratorSelection common(const World& world, flecs::entity region, const ZoneSpec& zone_spec)
{
    const RegionSpec& region_spec = region.get<RegionSpec>();
    size_t zone_count = the region's zone count;

    if (zone_count < region_spec.opening.size())
    {
        return {generator_name: draw from region_spec.opening[zone_count], zone_spec + the region's parameters};
    }

    // Rises with zone_count, and is 1 at zone_count_max
    double new_region_probability = new_region_probability(zone_count, region_spec.zone_count_max);

    if (a draw from the zone seed's random engine is below new_region_probability)
    {
        return {region_spec_name: draw from region_spec.next_region, zone_spec};
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

## Relocators

An unbound relocator's target either leads within the relocator's own region or refers to a region spec, which leads
into a new region. A relocator that leads within its own region still leads into a new region if the region's selector
chooses one.

```cpp
flecs::entity World::resolve_relocator_target(flecs::entity relocator)
{
    // target = [region/]zone, with the zone a level: +k, -k or =n; L is the relocator's zone's level

    if (target has a region)
    {
        const RegionSpec& region_spec = m_region_specs.at(target.region);
        flecs::entity zone = create_region_zone(region_spec, ZoneSpec {position: new_zone_position(world, region, L + k)});
        return entry_point(zone, arrival type, -k);
    }

    flecs::entity region = region_of(zone_of(relocator));

    if (a loaded zone of region at level L + k has an unbound access point of the arrival type that leads back)
    {
        return it;
    }

    flecs::entity zone = create_zone(region, ZoneSpec {position: new_zone_position(world, region, L + k)});
    return entry_point(zone, arrival type, -k);
}
```

- **A new region begins in one of two ways.** An entrance to a region of another type, such as a dungeon entrance in a
  fields zone, has the region spec in its target, `barrow_dungeon/-1`: the selector puts the spec's name into the zone's
  parameters, and the generator writes it into the relocator's target. A new region of the kind that follows this one
  is decided when the next zone is requested, so any exit the region's zones already have may lead into one. Each such
  exit creates a region of its own until a selection can refer to a region that exists (`work_plan.md`,
  `### Generation`).
- **Finding looks only in the relocator's own region.** A relocator that leads into another region always creates one,
  and going back is through the bound relocator.
- **Going through binds both ends,** as now (`game_system_reference.md`, `### World`), so a region, once created, is
  reached again through the relocators bound to it.

A new game, from the host's call through the region's first zones, a new region, and a descent into a dungeon:

```
host           Game::create(starting_area, character_spec)
Game           m_world.create(starting_area)
World            create_region_zone(starting_area, select_prefixed(starting_area, "zone."))
World              create_region(starting_area)                      -> region 1
World              create_zone(region 1, {level: 0, denizens: few})
World                selector "common", zone count 0, opening[0]     -> generator "forest"
World                generator "forest"                              -> Document, exits +0, a staircase to barrow_dungeon/-1
World                load_layout, populate                           -> zone 2
World          entry_point(zone 2, waypoint, 0)                      -> access point
Game           m_session.start(access point)

character takes an exit from zone 2
World          resolve_relocator_target                              target +0, in region 1
World            create_zone(region 1, {level: 0})
World              selector "common", zone count 1, opening[1]       -> generator "town"
World              generator "town", load_layout, populate           -> zone 3, its exit bound to zone 2's

character takes an exit once region 1 has created 9 zones
World          resolve_relocator_target                              target +0, in region 1
World            create_zone(region 1, {level: 0})
World              selector "common", zone count 9, draw below its   -> new region, "wild_hills" drawn from next_region
                   new region probability
World              create_region_zone(m_region_specs["wild_hills"], {level: 0})
World                create_region                                   -> region 20
World                create_zone(region 20, {level: 0})              -> its selector chooses the zone, as above

character goes down the staircase in zone 2
World          resolve_relocator_target                              target barrow_dungeon/-1
World            create_region_zone(m_region_specs["barrow_dungeon"], {level: -1})
World              create_region                                     -> region 30
World              create_zone(region 30, {level: -1})
World                selector "common", primary {dungen 1}           -> generator "dungen", {level: -1, difficulty, ...}
World                generator "dungen"                              -> Document, with a staircase +1 and one -1
World                load_layout, populate                           -> zone 31
World            entry_point(zone 31, staircase, +1)
World          the two staircases bound to each other
```

## Replacements

- **`Generator` and the `std::shared_ptr<const Generator>` component** are replaced by the registries, and
  `create_zone(region, const Generator&)` and `create_zone(region, level)` by `create_zone(region, zone_spec)`.
- **`Session::create` and `Session::load`** go: `World` creates every zone, and the session enters them.
- **The hosts' `DungenGenerator`** becomes dungen's generator in `stratum_gen`.
- **The hosts' zone file** goes: each host creates a new game with `Game::create`, whose first zone is generated.

## Open

The open items are tracked in `work_plan.md`, under `### Generation`.
