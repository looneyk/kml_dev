# Regions --- Plan

Status: built
Date: 2026-09-26

A region groups contiguous zones: a surface area, a dungeon, a crypt, a tower or a house. Regions have parent regions,
so a dungeon under a surface area is that area's child. Moving between zones through a relocator changes the region,
the level, or both, and an unbound relocator states how many levels it moves; resolving it needs zones to know their
region and level. So regions come before relocating through a relocator (`addressing_plan.md` step 8).
`sim_zone_model.md` (`## World, Zones, and Levels`) states the model, and the open items are in `work_plan.md`
(`### Regions`).

## What a region is

- **A region is data.** It is an entity with its properties as components: a name, and for a dungeon the overall or
  starting difficulty and the other properties a whole dungeon has. It acts on nothing directly, apart from what a
  generator reads from it.
- **A region has a parent region,** except a region at the top, such as the first surface area. A crypt, a dungeon, a
  tower or a house is the child of the surface area it is in.
- **A region is its zones' parent.** It takes the place in the hierarchy that the root has now, so a zone's flecs
  parent is its region. `zone_of` still stops at the first ancestor that is a zone, and a zone's region is its parent.
  Unloading a zone leaves its region; deleting a region deletes its zones and its child regions.
- **A region has a persistent id,** given when it is created, as a zone's is.
- **A region has the generation settings for the zones it makes:** the generator and its parameters. The generator is
  given the level of the zone it makes.

## Level

- **A zone's level is its position relative to the surface,** the same scale in every region: 0 at the surface, -1,
  -2, ... going down, and 1, 2, ... going up. It is shown to players as an absolute value, since whether they are
  descending or ascending is obvious from context.
- **Level is a label on the zone,** a component, and never an object or entity. Nothing is keyed or looked up by it,
  and a region may have any number of zones at one level. It is used when making connections between zones.
- **Any region may have zones at level 0.** Entering a crypt on the surface is a region change at the same level, not
  a level change, and a tower or house always has a zone at level 0. A hole in a surface zone at level 0 can lead
  straight into a dungeon region's zone at level -1 or lower.
- **Moving from one surface region to another is nothing special:** a change of region at level 0.
- **Depth is metadata only,** which may or may not be a count, and is not used.

## Relocators with a level offset

- **An unbound relocator may state a level offset,** a relative target (`## Relocator targets`): `-1` for a staircase
  leading down, `+1` for one leading up, `-2` for a hole that drops two levels, and `+0` for a way into another region
  at the same level, such as a crypt's door (`## Doors between zones`).
- **Resolving an unbound relocator finds a zone or makes one,** then binds both ends, so the binding is the connection
  from then on (`addressing_plan.md` step 8).
- **Finding:** an unbound relocator with offset *k* in a zone at level *L* looks among the loaded zones at level
  *L* + *k* for an unbound relocator with the opposite offset, -*k*. Moving up from a region's zone looks in its own
  region and its parent region: an unbound `up` from level -1 looks for an unbound `down` at level 0 of the parent
  region. Moving across, with an offset of 0, follows the same rules as moving up. Moving down looks in its own region.
- **Making:** if nothing is found, a new zone at level *L* + *k* is made from a region's generation settings. Moving up
  to level 0 makes it at random in the relocator's own region, such as a crypt's zone at level 0, or in the region it
  originated from, its parent. Moving down makes it in the relocator's own region.
- **The arriving actor is placed** at the found relocator's access point, or at the new zone's entry point of the type
  at which the relocator's type arrives.

## Relocator targets

An unbound relocator's target is a string, `[region/]zone[@position]`: the region, which is optional, the zone, and a
position in the zone, which is optional. A bound target stays a reference to the relocator it is bound to.

```
target   = [region "/"] zone ["@" position]
zone     = relative | absolute | name
relative = ("+" | "-") digits
absolute = "=" ["-"] digits
name     = letter {letter | digit | "_"}
```

| Target | Destination |
| --- | --- |
| `-1` | One level down, from the relocator's region |
| `+2` | Two levels up |
| `+0` | Another zone at the same level |
| `=0` | Level 0 |
| `=-3` | Level -3 |
| `sewers/=-1` | Level -1 of the region `sewers` |
| `../=0` | Level 0 of the parent region |
| `town/market@fountain` | The zone `market` in the region `town`, at the position `fountain` |

- **A relative zone** is a level offset from the relocator's zone, and always has its sign, so `+0` is the offset 0.
- **An absolute zone** is a level, marked with `=`, so a level below the surface is written `=-3` and is never read as
  an offset.
- **A named zone** is a zone of pregenerated content, and the position is a place in it, for transporting to a
  specific place. A zone name or a region name is a tag in the pregenerated content index (`## Pregenerated content`).
- **The region** is `..` for the parent region and `.` for the relocator's own region. An omitted region is the
  relocator's own region, from which the rules of `## Relocators with a level offset` start. A named region confines
  finding and making to that region.
- **An omitted position** places the arriving actor at an access point, as `## Relocators with a level offset`
  describes.
- **Finding with an absolute zone:** a relocator whose zone is at level *L*, with the target `=`*n*, looks at level *n*
  for an unbound relocator that leads back to level *L*, with the target *L* - *n* or `=`*L*. The opposite offset of
  `## Relocators with a level offset` is the relative case of the same rule.
- **Reserved characters:** `/` and `@` separate the parts, and `=`, `+`, and `-` at the start of a zone mark a level.
  A name cannot contain `/` or `@`.
- **The reader parses the string** and rejects one that does not match the grammar. `stratum_format` and the sim store
  the parsed target, and the writer writes it back as the same string. The generators write `-1` and `+1`.

## Pregenerated content

- **The game builds an index of pregenerated content when it starts.** It scans the `assets/zones` directory, or reads
  a pregenerated content index file instead, which is probably built in the background as the game loads.
- **The index lists each tag with its zone data file and the names of the relocators in it.** A tag identifies a zone,
  a region, or both.
- **A zone's tag is its `layout.components.name`.**
- **The tags are the values a relocator target's zone and region may have** (`## Relocator targets`), so a named zone
  or region resolves through the index to its file.
- **A target's position is the name of a relocator in the zone,** a `name` in its `relocator` component:
  `town/market@fountain` arrives at the relocator named `fountain`.

## Access points

- **An access point's type is what it is,** not whether it goes up or down: `staircase`, `door`, `portal`, or
  `waypoint`, or a type a mod adds. The direction is its relocator's target, so `up` and `down` are no longer types.
- **An access point's relocator has `type`, `order`, and `placement`.** The placement is where an arriving actor is
  placed: a `cell_position` in the access point's own zone and an `orientation`.
- **The order counts from 0 among the zone's access points of one type,** so a zone's staircases share one sequence,
  up and down alike.
- **Each type arrives at a type,** as `Arrival` states: a staircase at a staircase, a door at a door, a portal at a
  portal, and a waypoint at a waypoint. Of the access points of that type, an actor arrives at the lowest-ordered one
  that leads back to the level it left: through a `-1`, at a staircase whose relocator's target is `+1`.

## Doors between zones

- **A door that leads to another zone is a door with a `Relocator`.** `Access` still determines whether an actor gets
  through --- open, closed or locked --- and the relocator determines where it arrives, instead of the cell beyond.
  A staircase is the same case with an `Access` that is always open, from its type.
- **A crypt door is two doors, one in each zone,** each with one door facing on a wall of its own zone and a relocator
  with a level offset of 0, bound to each other. One entity cannot span two zones: an entity has one parent and one
  zone, a wall address means something only in its own zone, and each zone unloads and saves on its own.
- **The two doors' `Access` is kept equal by a rule,** not shared, since each is in its own zone. The rule is open
  (`work_plan.md`, `### Regions`).
- **A door within one zone keeps its two door facings** on the walls of its doorway, with one `Access` for both sides.

## Saved for later

- **Transparent zone and region boundaries.** Contiguous zones joined edge to edge, so an actor walks from one zone
  into the next, across region boundaries too. Until then, zones connect only through relocators: waypoints, and
  dungeons with more than one entrance.
- **Making a new region** when an unbound relocator finds nothing, instead of a zone in its own or its parent region.
- **Unloading by region** and **region files**, in `work_plan.md` (`## Regions`).

## Steps

- [x] **1. The region entity.** A `Region` with a persistent id and a parent region, created by `World`, as the parent
      of its zones; unloading a region deletes its zones and child regions, and `zone_of` is unchanged.
- [x] **2. Level.** A zone's level as a component, set by the loader from the file and by a generator, and a region's
      generation settings, given the level of the zone to make.
- [x] **3. The level offset.** An unbound relocator's level offset, in the format, read by `stratum_format`, written by
      both generators, and decoded by the loader, replacing the `down` and `up` destination names. A door entity may
      have a relocator, and `layout.components.access_points` may refer to it.
- [x] **4. Finding and making the zone at an offset,** for `addressing_plan.md` step 8: the loaded zones at the target
      level searched for an unbound relocator with the opposite offset, in the regions `## Relocators with a level
      offset` names, and otherwise a new zone made in the region those rules choose.
- [x] **5. Relocator targets as strings,** `[region/]zone[@position]` (`## Relocator targets`), replacing the numeric
      level offset: the reader's grammar and its errors, the writer, both generators, the loader, `Relocator` in
      `stratum_format` and the sim, `resolve_relocator_target` resolving relative and absolute zones in the regions `.` and
      `..` or with the region omitted, and the checked-in zone files and reference files converted. Named regions, named
      zones, and positions are parsed and stored, and resolving them waits on what their names and a position are. With it, the access points of `## Access points`:
      the relocator's `entry` renamed `placement` in the format, `stratum_format`, and `AccessPoint`; the access types
      `up` and `down` replaced by `staircase`, and `door` added; the generators numbering all staircases in one order;
      and the entry point chosen among the access points of the arrival type as one that leads back.

## Open

The open items are tracked in `work_plan.md`, under Regions.
