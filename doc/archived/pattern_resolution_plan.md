# Pattern Resolution --- Plan

Pattern resolution evaluates the patterns in a spec and replaces each with its value, so a resolved spec has only
literals. It is the work of **Pattern expressions** in `work_plan.md`. `region_spec_plan.md` (`## Patterns`) defines
patterns and their value forms, and `world_structure_plan.md` (`## Design`) defines when a region spec is composed.
Patterns are written in Lua (`scripting_language_plan.md`).

## Design

- **A pattern is a value that is Lua code,** marked by a leading `&` that is not part of the code. A value without it
  is literal text, and a value that starts with `&&` is literal text that starts with `&`, as in `&&Co` for `&Co`.
- **A pattern is an expression or a sequence of statements.** Its text is compiled as `return <text>`, and as a
  sequence of statements that returns the value if it is not an expression.
- **`&nil` is a pattern whose value is nil.** A nil value removes its key, so composition layers values and resolution
  removes the keys (`world_structure_plan.md`, `## Design`).
- **A pattern's value is a boolean, a number, a string, or nil,** stored as text in the spec's store: a boolean as
  `true` or `false`, an integer as an integer, and a float as `std::format` writes it.
- **A region spec is resolved once it is composed,** parents first, so a pattern's `parent` has the parent's resolved
  values.
- **A zone's patterns are resolved when the zone is created:** the region spec's `zone.*` entries, since they
  use the zone's position and zone counter.
- **Resolution draws from a random engine of its own.** A region spec's resolution random engine is seeded from the
  spec's seed, and a zone's from the zone's seed. It is separate from the random engine that draws seeds, so a change to
  one pattern changes the values drawn from it and no seed.
- **A region spec's `zone.name` is the name of each zone in the region,** literal text or a string pattern such as
  `&region_name .. '_' .. x .. '_' .. y`. A zone in a region whose spec has none is named
  `&region_name .. '_' .. zone_counter`, and a zone whose name resolves to nil is named for its region and its
  coordinates.

### The pattern resolver

- **`PatternResolver` resolves the patterns of one spec,** a `RegionSpec` or a `ZoneSpec`: one resolver for each
  instance of each.
- **It is constructed with the symbols and the random engine,** the random engine separate from the symbols.
- **It resolves in a sandboxed environment:** the symbols, and globals that fall through to the sandbox's functions and
  libraries (`scripting_language_plan.md`, `## Lua 5.5 prototype`). A global the environment and the sandbox do not
  have is an error, and a global a pattern sets is set in its own environment.
- **It is independent of the spec types.** It resolves a single value or a whole store, any value that starts with `&`
  being a pattern, and a function for each spec type feeds it that spec's values, the typed members and the store's
  entries alike. The function leaves out the fields that are never resolved: a `RegionSpec`'s `seed`, which seeds the
  resolution random engine, and its `parent` and `region_spec`, which composition reads before resolution.
- **It is in `stratum_sim`,** and moves to a library of its own if it turns out to be general enough.

### Symbols

The symbols are the globals of a pattern's environment. A pattern refers only to these predefined symbols, never to
another entry of the spec being resolved.

| Symbol | Type | Value | Where |
| --- | --- | --- | --- |
| `x`, `y`, `z` | Integer | The zone's position | A zone's patterns |
| `zone_counter` | Integer | The zone's count, from 0, in the order in which zones are created | A zone's patterns |
| `region_name` | String | The name of the region the zone is in | A zone's patterns |
| `region` | Table | The entries of the region's resolved spec, such as `region.difficulty` | A zone's patterns |
| `parent` | Table | The entries of the parent region's resolved spec | A subregion's patterns |

`region` and `parent` are plain tables, so an entry they do not have is nil, as in `region.biome or 'plain'`.

### Functions

A pattern has the functions of Lua's base, `math`, `string`, `table`, and `utf8` libraries that the sandbox exposes,
and `random`, with the interface of Lua's `math.random`: a float in [0, 1) for `random()`, an integer in [1, m] for
`random(m)`, and an integer in [m, n] for `random(m, n)`, as in `&random(3, 6)`. It draws from the resolver's random
engine.

## Steps

- [x] 1. **The pattern resolver.** `PatternResolver`, independent of the spec types, with its symbols and random
  engine, resolves a single value or a store, a value that starts with `&` being a pattern.
- [x] 2. **Region spec resolution.** `resolve_region_spec` feeds a `RegionSpec`'s `name`, `display_name`, and store
  entries to a resolver, leaving the zone entries for each zone. **Region spec trees** resolves each composed region
  spec with it (`world_structure_plan.md`, step 4).
- [x] 3. **Zone spec resolution.** `resolve_zone_spec` feeds a `ZoneSpec`'s store to a resolver, and `World` resolves
  each zone's spec with the zone's symbols when it creates the zone, the zone's name from the region spec's
  `zone.name`.
- [x] 4. **Lua patterns.** `PatternResolver` resolves in a sandboxed Lua state, with its symbols in the pattern's
  environment, and the patterns of `region_specs.json`, `World`'s default zone name, and the tests are written in Lua.
- [x] 5. **`random`,** drawing from the resolver's random engine.

## Open questions

- **Can a `RegionSpec`'s `extent` and `zone_size` be patterns?** Their values are not text, so the resolver cannot take
  them as they are.
- **What type does a store entry have as a symbol?** A store value is text, so `region.difficulty` with the value `3`
  could be a number, a string, or a number only when its text parses as one.
- **Which symbols does a region spec's own resolution have?** The table's symbols are a zone's, and a subregion's
  `parent`; a region's own are undecided.
