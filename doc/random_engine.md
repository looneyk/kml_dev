# Random Engine --- Plan

Date: 2026-09-27, revised 2026-10-01

Where random numbers come from in the game and in the generators: who owns each engine, how it is seeded, and what is
saved. Gameplay comes first. The engines are arranged for what a player experiences, and reproducibility is a
development tool for the suites, bug reports, and reference files, not a promise to players. The open items are in
`work_plan.md` (`### Random engine`).

## Where it is now

- **`Game` owns the gameplay engine,** a `math::Random<>` seeded from `std::random_device`, and `World` takes it
  by reference in its constructor; `World::resolve_relocator_target` draws from it. The seed is neither passed in nor
  logged.
- **dungen and catagen each seed one `math::Random<>` per run** from `Parameters::seed`, and pass it by reference from
  one function to the next.
- **A region's generator is given the level of the zone it makes,** as `ZoneLevel`, and no seed.
- **`math::Random<>` uses `std::default_random_engine`,** which is implementation-defined: MSVC's is `mt19937` and
  libstdc++'s is `minstd_rand0`. The standard distributions are implementation-defined too, so one seed makes different
  zones with different standard libraries.
- **The other documents state goals and no mechanism.** `game_system_principles.md` (Phase 5) calls for hierarchical
  seeding, `sim_zone_model.md` counts RNG streams as world state, and `placement_plan.md` hands placement a random engine
  without saying whose.

## What other games do

- **Dungeon Crawl Stone Soup** has several generators, chosen by purpose: gameplay, level generation, and the UI among
  them. A scoped subgenerator switches the current one, and level generation is seeded from the game seed and the
  level, so a seeded game builds the same dungeon however it is played.
- **Brogue CE** derives every level's seed from the game seed when the game starts. It keeps a substantive generator for
  everything that affects play and a cosmetic one for effects, so a flicker never changes the game.
- **NetHack 3.7** separates a core generator from a display generator for the same reason.

Each reaches its engine without passing it, through a global or a current generator that a scope selects.

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
- **Slay the Spire** --- shared seeds and a daily climb. Separate engines for the map, cards, potions, monsters, and
  events keep a choice in one from shifting the others.
- **The Binding of Isaac** --- seeds, special seeds that change the rules, and daily runs.
- **Spelunky and Spelunky 2** --- a daily challenge with one seed for everyone, and seeded runs in Spelunky 2.
- **Noita** and **Balatro** --- seeds that a player sets or shares.

Offering either here needs three things: each zone's seed derived from the world seed and the zone's place rather than
drawn (`## Generation`), an engine and distributions whose output is the same on every platform (`## The engine type`),
and a seed stored with the version of the generators that made it.

## Priorities

1. **Gameplay.** Variety from game to game, fast draws, and no accidental coupling: an animation must never shift a
   monster's attack.
2. **Development.** Any run can be reproduced from its seed. The suites seed explicitly, the tools already write the seed
   into each file, and a host logs its seed and accepts one.
3. **Reproducibility for players,** such as a seeded game shared between players, is not a goal. Nothing is designed
   against it, and whether to offer it is open.

## Ownership

- **`Game` owns the engines, as it owns the id allocator.** It seeds them from a seed that a host or a suite passes, or from `std::random_device` if none is passed,
  and the host logs the seed.
- **`World` takes the engines by reference in its constructor,** as it takes `IdAllocator&`, so every sim function that
  draws uses them through the world it already has, and no sim function takes an engine. `resolve_relocator_target` loses its
  parameter. A suite passes engines of its own, seeded explicitly, as it passes its own `CountingIdAllocator`.
- **A generation run owns its own engine,** seeded from the zone's seed (`## Generation`). In dungen and catagen, a run
  becomes an object with its parameters, its engine, and the state it builds, and the functions that now take a
  `random_type&` become its members. That ends the passing inside the generators, and it comes with their integration.
- **A view never draws from the simulation's engines.** A view that animates has a cosmetic engine of its own, so what is
  drawn on screen never changes what happens.
- **No global or ambient engine.** A scoped current generator, as in DCSS, avoids passing, but it hides which code draws,
  and two worlds in one process, as the suites have, would share it. Giving the engine to the object that already scopes
  the work, the game or a generation run, avoids passing without that cost.

## Streams

- **Gameplay and generation are separate engines,** both owned by `Game`, both seeded from the one world seed through
  `std::seed_seq`. Combat, AI, and every other draw during play use the gameplay engine. Each zone's seed is drawn from
  the generation engine. It costs one engine, and a bug in generation is then reproduced from the zone's seed without
  replaying the play that led to it.

## Generation

- **A zone's seed is one of its generation parameters,** which `World::create_zone(region, zone_spec)` draws from the
  generation engine and adds to the `ZoneSpec` before the generator runs (`world_generation_plan.md`). The generator
  reads the level and the seed from the `ZoneSpec`, and nothing from the world.
- **A generator selector's draws use the generation engine,** since they decide what a zone is.
- **A zone file records its seed** in `generator.parameters`, as the tools' files do now, so a zone reproduces from its
  file alone.
- **A retry draws a fresh sub-seed** from the run's engine, for Phase 5's reject-and-retry.
- **Placement draws from the run's engine** when it is part of generation (`placement_plan.md`), and from the gameplay
  engine when it happens during play, such as a denizen appearing over time.
- **A zone's seed is drawn, not derived.** It depends on the order in which zones are made, not only on the world seed.
  Deriving it from the world seed and the zone's place, as DCSS and Brogue do, builds the same world however it is
  explored, which matters only for reproducibility for players.

## The engine type

- **The engine is named explicitly** in place of `std::default_random_engine`, so one seed makes one sequence on every
  standard library. stratum's `random_type` and the generators' become `math::Random<E>` with the chosen engine;
  `math::Random`'s own default, which other projects use, is unchanged. Changing the generators' engine changes their
  output, so their reference files are regenerated.
- **The values drawn through the standard distributions can still differ** between standard libraries, since their
  algorithms are implementation-defined (`## Open`).
- **Draw speed does not decide it.** Every candidate below draws in a few nanoseconds, and a turn-based game makes
  hundreds of draws a turn at most. What differs is the size of the state, which sets the cost of seeding an engine and
  of saving one, and whether the output is specified exactly.

| Engine | State | Seeding and saving | Where it is |
| --- | --- | --- | --- |
| `std::mt19937_64` | 312 words, about 2.5 KB | Seeding fills the whole state; the saved state is about 6 KB of text | The standard, which specifies its output exactly |
| xoshiro256** | 4 words, 32 bytes | Seeded through splitmix64; the saved state is four numbers | About 20 lines of public-domain code, specified by its reference implementation; Rust `rand`'s small fast generator is from the family |
| PCG (pcg64, or pcg32 with 8 bytes of state) | 16 bytes | Seeded directly; the saved state is two numbers | The header-only pcg-cpp library, or a short implementation of its own; Godot and DCSS use PCG |
| Jenkins small fast (jsf64) | 4 words, 32 bytes | Seeded directly; the saved state is four numbers | About 15 lines of public-domain code; Brogue uses it |

- **xoshiro256** is recommended.** A generation run seeds a new engine, and a retry another, so seeding cost and state
  size count, and each of the game's engines has its state saved with every save. `std::mt19937_64` is the heaviest at both. The
  three small engines are close; xoshiro256** needs no library, and its reference implementation fixes its output on
  every platform. Each of them meets the standard's uniform random bit generator requirements, so it works as
  `math::Random`'s engine and through the standard distributions.

## Saving

- **`Game` saves the engines' state** with the id allocator's count in the saved game's `game` part, and restores
  both when it loads the game (`game_file_spec.md`, `## game`). A standard engine
  writes and reads its state as text through `<<` and `>>`, and a small engine's state is its few words.
- **Restoring the state repeats the same outcomes after a reload,** which keeps a player from reloading until a roll
  succeeds. Reseeding on load allows it. The player's settings choose which, through `Settings::random_seeding_on_open`.

## Open

The open items are tracked in `work_plan.md`, under `### Random engine`.
