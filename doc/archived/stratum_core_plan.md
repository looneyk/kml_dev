# stratum_core --- Plan

`quetzal_stratum_core` is the library of stratum's data types: the records that the game and zone files store and that
`stratum_sim` builds the world from, with their operations, and no file format and no ECS. `stratum_format` reads and
writes them, and `stratum_sim`, dungen, and catagen use them as they are. The work is **stratum_core** in
`work_plan.md`.

## Design

- **A data model separate from its serialization** follows LLVM, whose `IR` library is separate from its `Bitcode`
  reader and writer, OpenUSD, whose `Sdf` data model is separate from its file format plugins, and Assimp, whose
  `aiScene` structures are separate from its importers.
- **The library is `library/quetzal/stratum_core`,** namespace `quetzal::stratum_core`, target `quetzal_stratum_core`.
  It links `quetzal_common` and `quetzal_math`, includes the `quetzal/geometry` headers, and uses neither flecs nor
  nlohmann.
  `stratum_format`, `stratum_sim`, dungen, and catagen link it.
- **stratum_core contains the types that `stratum_format` and `stratum_sim` share:**
  - the aliases common to both `types.hpp` files, such as `coord_type`, `discrete_point_type`,
    `discrete_vector_type`, `point_type`, and `seed_type`;
  - `PersistentId`, and the zone's position in its worldspace, `zone_position_type`, in place of `stratum_sim::Position`;
  - `AccessPoint` and `RegionSpecName`, whose bound destination is a `PersistentId`, and `ZoneExtendedName`, with the
    parsing and formatting of their text;
  - `Directionality`, `Notation`, and `TransitEndpoint`, with the text forms of the enumerations;
  - `RegionExtent`, `RegionSpec`, and `ZoneSpec`;
  - the world's records: `Worldspace`, `Region`, `RegionTree`, and a zone's record.
- **`stratum_format` keeps what depends on the file format:** the documents, `raw_values_type`, the readers and writers,
  `checked_values.hpp`, `json_util.hpp`, `references.hpp`, the reading of a `RegionSpec` and a `RegionExtent`, and
  `grid.hpp`, which works on a `ZoneDocument`'s cells.
- **`stratum_sim` keeps what depends on the ECS or on scripting:** `EntityReference`, which has a `flecs::entity`, the
  zone components, and the pattern resolution of region specs and zone specs.
- **`WorldDocument` contains the world's records,** so `World::open` and `World::save` read and write them without
  copying them into other structures.
- **A shared record has only stored state.** The lookups derived from it, such as zones by position and by name, are
  built by `stratum_sim` when the world is read and are never written.
- **A value that only `stratum_sim` sets** is in a `stratum_sim` structure beside the shared record, never an optional
  member of the record that is set after it is read.

## Steps

- [x] 1. **The library.** `library/quetzal/stratum_core`, in the root `CMakeLists.txt`, with `stratum_core_test`
  labeled `stratum`. `Directionality`, `Notation`, `ZoneExtendedName`, `AccessPoint`, `TransitEndpoint`,
  `RegionExtent`, and `RegionSpec` move from `stratum_format`, with their tests, and the shared aliases move from
  `stratum_format/types.hpp`. `stratum_format`, `stratum_sim`, dungen, and catagen link it.
- [x] 2. **One set of shared types.** `stratum_sim/types.hpp` takes the shared aliases from stratum_core.
  `PersistentId` moves to stratum_core, and an `AccessPoint`'s bound destination is a `PersistentId`. `zone_position_type`
  replaces `stratum_sim::Position`.
- [x] 3. **`ZoneSpec` in stratum_core.** The `ZoneSpec` structure and its position moves, and `resolve_zone_spec` stays
  in `stratum_sim`.
- [x] 4. **The world's records shared.** `Worldspace`, `Region`, `RegionTree`, and a zone's record move to
  stratum_core, without the lookups derived from them, and `WorldDocument` contains them. `World::open` builds the
  lookups from what it reads, and `World::save` writes the records as they are.

## Open questions

- **Is a worldspace's record of the zones it created, and of the links set between worldspaces, a structure of its own
  apart from the root region spec?**
