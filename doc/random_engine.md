# Random Engine --- Plan

Date: 2026-09-27, revised 2026-10-05

Where random numbers come from in the game and in the generators: who owns each random engine, how it is seeded, and
what is saved. Gameplay comes first. The random engines are arranged for what a player experiences, and
reproducibility is a development tool for the tests, bug reports, and reference files, not a promise to players. The
open items are in `work_plan.md` (`### Random engine`).

## Where it is now

- **`Game` owns the gameplay random engine,** a `math::Random<>` seeded from `std::random_device`, and `World` takes it
  by reference in its constructor, though nothing draws from it yet. The seed is neither passed in nor logged.
- **dungen and catagen each seed one `math::Random<>` per run** from `Parameters::seed`, and pass it by reference from
  one function to the next.
- **A region's generator is given the level of the zone it makes,** as `ZoneLevel`, and no seed.
- **`math::Random<>` uses `std::default_random_engine`,** which is implementation-defined: MSVC's is `mt19937` and
  libstdc++'s is `minstd_rand0`. The standard distributions are implementation-defined too, so one seed makes different
  zones with different standard libraries.
- **The other documents state goals and no mechanism.** `game_system_principles.md` (Phase 5) calls for hierarchical
  seeding, `sim_zone_model.md` counts RNG streams as world state, and `placement_plan.md` passes placement a random engine
  without stating which one.

## What other games do

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

Offering either here needs three things: each zone's seed derived from the world seed and the zone's place rather than
drawn (`## Generation`), a random engine and distributions whose output is the same on every platform (`## The random
engine type`), and a seed stored with the version of the generators that made it.

## Priorities

1. **Gameplay.** Variety from game to game, fast draws, and no accidental coupling: an animation must never shift a
   monster's attack.
2. **Development.** Any run can be reproduced from its seed. The tests seed explicitly, the tools already write the seed
   into each file, and a host logs its seed and accepts one.
3. **Reproducibility for players,** such as a seeded game shared between players, is not a goal. Nothing is designed
   against it, and whether to offer it is open.

## Ownership

- **The gameplay and generation random engines are process-wide,** each reached through a function for its stream, as
  in DCSS, Brogue and NetHack, and passed to no function or constructor (`session_plan.md`, `## Settings, the log, and
  the random engines`). `World` does not take them, and `resolve_relocator_target` loses its parameter.
- **Each process-wide random engine is thread-safe.** A draw locks the random engine's mutex for the whole draw, the
  distribution included, so threads that draw at the same time never corrupt its state. The values each thread draws
  then depend on the order in which the threads draw, so the draws that must be reproducible from a seed are made on
  one thread.
- **`Game` seeds the process-wide random engines** from a seed that a host or a test passes, or from
  `std::random_device` if none is passed, and the host logs the seed. A test seeds them explicitly before each test
  case.
- **A generation run owns its own random engine,** seeded from the zone's seed (`## Generation`). In dungen and
  catagen, a run becomes an object with its parameters, its random engine, and the state it builds, and the functions
  that now take a `random_type&` become its members. The generators' functions then no longer pass a random engine to one another, and the change comes with their
  integration. A run's random engine
  is used by one thread and needs no lock.
- **A view never draws from the simulation's random engines.** A view that animates has a cosmetic random engine of its
  own, so what is drawn on screen never changes what happens.

## Streams

- **Gameplay and generation are separate random engines,** both seeded by `Game` from the one world seed through
  `std::seed_seq`. Combat, AI, and every other draw during play use the gameplay random engine. Each zone's seed is
  drawn from the generation random engine. It costs one random engine, and a bug in generation is then reproduced from
  the zone's seed without replaying the play that led to it.

## Generation

- **A zone's seed is one of its generation parameters,** which `World::create_zone(region, zone_spec)` draws from the
  generation random engine and adds to the `ZoneSpec`, under `seed_key`, before the generator runs (`world_generation_plan.md`). The
  generator is passed the level and the seed.
- **A generator selector's draws use the generation random engine,** since they decide what a zone is.
- **A zone file stores its seed** in `generator.parameters`, as the tools' files do now, so a zone reproduces from its
  file alone.
- **A retry draws a fresh sub-seed** from the run's random engine, for Phase 5's reject-and-retry.
- **Placement draws from the run's random engine** when it is part of generation (`placement_plan.md`), and from the
  gameplay random engine when it happens during play, such as a denizen appearing over time.
- **A zone's seed is drawn, not derived.** It depends on the order in which zones are made, not only on the world seed.
  Deriving it from the world seed and the zone's place, as DCSS and Brogue do, builds the same world however it is
  explored, which matters only for reproducibility for players.

## The random engine type

- **The random engine is named explicitly** in place of `std::default_random_engine`, so one seed makes one sequence
  on every standard library. One alias in `quetzal/math`, `portable_random_engine_type`, is the chosen random engine,
  and stratum's `random_type`, dungen's and catagen's are each `math::Random<math::portable_random_engine_type>`, so
  changing the random engine changes that alias alone. `math::Random`'s own default, which other projects use, is
  unchanged. Changing the random engine changes the generators' output, so their reference files are regenerated.
- **An expectation written into a test from one seed's outcome changes with the random engine,** such as
  `stratum_sim_test`'s "At this seed the door is in the top row of the shared wall", and is rewritten by hand, unlike a
  reference file. A check of a property that holds for any seed, or a reference file, needs no rewriting.
- **The distributions and the shuffle are the project's own,** in `math::Random`: a bounded integer by Lemire's method,
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

- **xoshiro256** is recommended.** A generation run seeds a new random engine, and a retry another, so seeding cost and
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
