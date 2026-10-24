# Random Engine --- Plan

Date: 2026-09-27, revised 2026-10-05

Where random numbers come from in the game and in the generators: who owns each random engine, how it is seeded, and
what is saved. Gameplay comes first. The random engines are arranged for what a player experiences, and
reproducibility is a development tool for the tests, bug reports, and reference files, not a promise to players. The
open items are in `work_plan.md` (`### Random engine`).

## Current state

- **The gameplay random engine is process-wide,** a `RandomStream` that `gameplay_random()` returns, though nothing
  draws from it yet. `Game` seeds it with the world seed, which `Game` is passed or draws, and writes the seed to the
  log. Reseeding on open draws a new seed for the gameplay random engine and keeps the world seed.
- **Region and zone seeds are derived** (`## Generation`). `World::create_zone(region, zone_spec)` adds the zone's
  seed to the `ZoneSpec` under `seed_key` before the selector runs, and the host passes it to the generator in the
  `ZoneRequest`, with the zone's position.
- **dungen and catagen each seed one `math::Random` per run** from `Parameters::seed`, and pass it by reference from
  one function to the next.
- **The random engine type is chosen** (`## The random engine type`): xoshiro256**, with the distributions and
  shuffle implemented in `math::Random`, so one seed makes the same zones with every standard library.
- **The other documents state goals and no mechanism.** `game_system_principles.md` (Phase 5) calls for hierarchical
  seeding, `sim_zone_model.md` counts RNG streams as world state, and `placement_plan.md` passes placement a random engine
  without stating which one.

## Other games

- **Dungeon Crawl Stone Soup** keeps one generator for each purpose in its `rng` namespace: gameplay, level generation,
  and the UI among them. The draw functions, such as `random2(n)`, draw from whichever generator is current. A scope
  makes another one current by declaring an `rng::generator` object for it, and the previous one is current again when
  the scope ends; level generation declares one for level generation. A level's generator is seeded from the game seed
  and the level, so a seeded game builds the same dungeon however it is played. No generator is passed to a function.
- **Brogue CE** keeps two generator states in a global array, substantive and cosmetic, and a global selector,
  `rogue.RNG`, that decides which one `rand_range` draws from. Code that draws for an effect calls
  `assureCosmeticRNG` to select the cosmetic generator, and `restoreRNG` to select the previous one again, so a flicker
  never changes the game. Every level's seed is drawn from the game seed when the game starts and stored with the
  level.
- **NetHack 3.7** has two global generators: `rn2` and the functions built on it draw from the core one, and
  `rn2_on_display_rng` from the display one, for the same reason as Brogue's cosmetic generator.
- **Slay the Spire** keeps its generators as static fields of its dungeon class, one for each purpose: cards, monsters,
  events, relics, potions, treasure, the merchant, shuffling, monster AI, monster hit points, and the rest. Code draws
  from the field for its purpose without being passed it. Each is seeded from the run seed, and some are seeded again
  on each floor from the run seed and the floor number, so a choice in one never shifts the draws of another.
- **Godot** has global functions, such as `randi`, `randf` and `randf_range`, that draw from one global random engine,
  a PCG. A `RandomNumberGenerator` object has a seed and state of its own, for a stream that must be reproducible apart
  from the rest.
- **Unreal Engine** has the global `FMath::Rand` and `FMath::FRand`, and `FRandomStream`, an object with a seed of its
  own that a procedural generator or a gameplay system owns when its draws must be reproducible.

Each reaches its gameplay random engines without passing them: through a global, a static field for each purpose, or a
current generator that a scope selects. Where an object owns a random engine, as Godot's `RandomNumberGenerator` and
Unreal's `FRandomStream`, it is for a stream that must be reproducible apart from the rest, which a generation run here
is.

### Seeds that players share

**World seeds** reproduce the generated world and not the play in it, so players who share a seed explore the same map.

- **Minecraft** --- the world seed fixes terrain, biomes, and structures. Players trade seeds for good spawns and speedrun
  routes, and a change to generation in a new version changes what a seed makes.
- **Terraria** --- world seeds, and secret seeds that change the rules.
- **Factorio** --- a map exchange string includes the seed.
- **No Man's Sky** --- one fixed seed for the whole universe, the same for every player.

**Run seeds** reproduce a run's dungeon, items, and encounters, and what happens then depends on the player's choices.
They are used for daily challenges, races, and tournaments.

- **Dungeon Crawl Stone Soup** --- seeded games, with an option to generate every level in advance, so the dungeon does
  not depend on the order in which it is explored.
- **Brogue** --- seeds are shown and shared, with catalogs of notable seeds and weekly seeded challenges.
- **Slay the Spire** --- shared seeds and a daily climb. Separate random engines for the map, cards, potions, monsters,
  and events keep a choice in one from shifting the others.
- **The Binding of Isaac** --- seeds, special seeds that change the rules, and daily runs.
- **Spelunky and Spelunky 2** --- a daily challenge with one seed for everyone, and seeded runs in Spelunky 2.
- **Noita** and **Balatro** --- seeds that a player sets or shares.

Offering either requires each zone's seed derived from the world seed and the zone's place rather than drawn
(`## Generation`), a random engine and distributions whose output is the same on every platform (`## The random engine
type`), and a seed stored with the version of the generators that made it.

## Priorities

1. **Gameplay.** Variety from game to game, fast draws, and no accidental coupling: an animation must never shift a
   monster's attack.
2. **Development.** Any run can be reproduced from its seed. The tests seed explicitly, the tools already write the seed
   into each file, and a host logs its seed and accepts one.
3. **Replayability.** A world seed, passed or drawn, rebuilds the same world however it is explored, so players can share
   one, and a seed passed for a region or a zone rebuilds that region or zone alone. A new game with no seed passed is
   as random as before, from a world seed drawn and logged.

## Ownership

- **The gameplay random engine is process-wide,** reached through a function, as in DCSS, Brogue and NetHack, and passed
  to no function or constructor (`session_plan.md`, `## Settings, the log, and the random engines`). `World` does not
  take it. Generation draws from random engines seeded from derived seeds (`## Generation`).
- **Each process-wide random engine is thread-safe.** A draw locks the random engine's mutex for the whole draw, the
  distribution included, so threads that draw at the same time never corrupt its state. The values each thread draws
  then depend on the order in which the threads draw, so the draws that must be reproducible from a seed are made on
  one thread.
- **Its thread safety is internal to it.** A process-wide random engine is a `stratum_sim::RandomStream`, which
  has `math::Random`'s members for seeding and drawing --- `seed`, `random`, `random_closed`, `gaussian`,
  `triangular`, `choose` and `shuffle` --- and its state as text, `state` and `set_state`, for saving. Its mutex and
  its `math::Random` are private, and no member returns either, so locking each draw can change to a debug check of the
  owning thread, or to anything else, without changing a caller. Code moving between a run's own `math::Random` and a
  process-wide random engine changes only the object it draws from. `quetzal::math` has no locking.

  ```cpp
  namespace quetzal::stratum_sim
  {
      RandomStream& gameplay_random();
  }
  ```

- **`math::Random` keeps its state as text too.** `state` and `set_state` replace `engine()`, through which `Game` now
  writes and reads the random engine's state, so no caller reaches the random engine itself.
- **`Game` has the world seed,** which a host or a test passes, or which `Game` draws from `std::random_device` if none
  is passed, and logs. The gameplay random engine is seeded from it, and every region's and zone's seed derives from it.
  A test seeds the gameplay random engine explicitly before each test case.
- **A generation run owns its own random engine,** seeded from the zone's seed (`## Generation`). In dungen and
  catagen, a run becomes an object with its parameters, its random engine, and the state it builds, and the functions
  that now take a `random_type&` become its members. The generators' functions then no longer pass a random engine to one another, and the change comes with their
  integration. A run's random engine
  is used by one thread and needs no lock.
- **A view never draws from the simulation's random engines.** A view that animates has a cosmetic random engine of its
  own, so what is drawn on screen never changes what happens.

## Streams

- **Gameplay and generation draw from separate random engines.** Combat, AI, and every other draw during play use the
  gameplay random engine, seeded from the world seed. Generation never draws from it: each zone is generated from its
  own derived seed, so play never changes the world, and a bug in generation is reproduced from the zone's seed without
  replaying the play that led to it.

## Generation

- **Seeds are derived from the world seed and coordinates, hierarchically.** A region's seed derives from the world
  seed and the region's coordinates in the world, `(x, y)`, and a zone's from its region's seed and the zone's
  coordinates in the region, `(x, y, z)`. The coordinates are signed, with the starting region and
  zone at the origin, within the world's and region's sizes (`settings_plan.md`, `### A new game's settings`). A seed
  is derived through `std::seed_seq` over the seed and the coordinates as 32-bit words, since the standard specifies its
  output and a zone's position in the world, about 10^21 places, does not fit one 64-bit number. How coordinates are
  assigned to new regions and zones is a matter for generation, apart from the derivation.
- **A seed passed takes precedence.** A `RegionSpec` or `ZoneSpec` with a seed under `seed_key` uses it in place of the
  derived one, so a region or a zone can be rebuilt alone, such as from a zone file's `generator.parameters`.
- **A zone's seed is one of its generation parameters,** which `World::create_zone(region, zone_spec)` adds to the
  `ZoneSpec`, under `seed_key`, before the generator runs (`world_generation_plan.md`). The generator is passed the
  level and the seed.
- **A generator selector's draws use a random engine seeded from the zone's seed,** since they decide what the zone is.
- **A zone file stores its seed** in `generator.parameters`, so a zone reproduces from its
  file alone.
- **A retry draws a fresh sub-seed** from the run's random engine, for Phase 5's reject-and-retry.
- **Placement draws from the run's random engine** when it is part of generation (`placement_plan.md`), and from the
  gameplay random engine when it happens during play, such as a denizen appearing over time.
- **A zone's seed is derived, not drawn,** as DCSS's level seeds are, so it does not depend on the order in which zones
  are made, and the world is the same however it is explored.

## The random engine type

- **The random engine is named explicitly** in place of `std::default_random_engine`, so one seed makes one sequence
  on every standard library. One alias in `quetzal/math`, `portable_random_engine_type`, is the chosen random engine,
  and stratum's `random_type`, dungen's and catagen's are each `math::Random<math::portable_random_engine_type>`, so
  changing the random engine changes that alias alone. It is also `math::Random`'s default, since its distributions
  require a 64-bit random engine that uses its whole range. Changing the random engine changes the generators' output,
  so their reference files are regenerated.
- **An expectation written into a test from one seed's outcome changes with the random engine,** such as
  `stratum_sim_test`'s "At this seed the door is in the top row of the shared wall", and is rewritten by hand, unlike a
  reference file. A check of a property that holds for any seed, or a reference file, needs no rewriting.
- **The distributions and the shuffle are implemented in `math::Random`:** a bounded integer by Lemire's method,
  a real from the top 53 bits of a 64-bit value, and a Fisher-Yates shuffle in place of `std::shuffle`. The standard
  distributions and `std::shuffle` are implementation-defined, and checked-in reference files must match across
  standard libraries, so one seed makes one sequence of values on every platform. They come with the random engine, so
  the reference files are regenerated once.
- **Draw speed does not decide it.** Every candidate below draws in a few nanoseconds, and a turn-based game makes
  hundreds of draws a turn at most. What differs is the size of the state, which sets the cost of seeding a random
  engine and of saving one, and whether the output is specified exactly.

| Random engine | State | Seeding and saving | Where it is |
| --- | --- | --- | --- |
| `std::mt19937_64` | 312 words, about 2.5 KB | Seeding fills the whole state; the saved state is about 6 KB of text | The standard, which specifies its output exactly |
| xoshiro256** | 4 words, 32 bytes | Seeded through splitmix64; the saved state is four numbers | About 20 lines of public-domain code, specified by its reference implementation; Rust `rand`'s small fast generator is from the family |
| PCG (pcg64, or pcg32 with 8 bytes of state) | 16 bytes | Seeded directly; the saved state is two numbers | The header-only pcg-cpp library, or a short implementation of its own; Godot and DCSS use PCG |
| Jenkins small fast (jsf64) | 4 words, 32 bytes | Seeded directly; the saved state is four numbers | About 15 lines of public-domain code; Brogue uses it |

- **xoshiro256** is the random engine.** A generation run seeds a new random engine, and a retry another, so seeding cost and
  state size count, and each of the game's random engines has its state saved with every save. `std::mt19937_64` is
  the heaviest at both. The three small random engines are close; xoshiro256** needs no library, and its reference
  implementation fixes its output on every platform. Each of them meets the standard's uniform random bit generator
  requirements, so it works as `math::Random`'s random engine.

## Saving

- **`Game` saves the random engines' state** with the id allocator's count in the saved game's `game` part, and
  restores both when it loads the game (`game_file_spec.md`, `## game`). A standard random engine writes and reads its
  state as text through `<<` and `>>`, and a small random engine's state is its few words.
- **Restoring the state repeats the same outcomes after a reload,** which keeps a player from reloading until a roll
  succeeds. Reseeding on load allows it. The player's settings choose which, through `Settings::random_seeding_on_open`.

## Open

The open items are tracked in `work_plan.md`, under `### Random engine`.
