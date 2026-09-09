# dungen --- Plan

Status: complete
Date: 2026-09-04

`dungen`'s generation algorithm lives in `tool/dungen/main.cpp`; the library holds only
the structures it writes into. Two more layouts are wanted, so the algorithm moves into the library
as one file per layout, behind a shared `Parameters` and a `generate` that dispatches on which was
asked for. `catagen` is the shape to match: `types` / `Parameters` / `Level` / `generate` /
`write_level`.

`dungen_reference.md` describes what exists.

## The three layouts

| Name    | Layout |
| ------- | ------ |
| `maze1` | Rooms dropped at random until the density is met, a maze flooded through every cell they left, doors punched into the rooms, then dead ends and loops pruned back. The algorithm that exists today. |
| `grid1` | Rogue's. A coarse grid of sectors, a room somewhere inside most of them, orthogonally adjacent sectors joined by corridors that bend once in the rock between. Sparse rooms, solid rock, short direct corridors. |
| `cave1` | Noise smoothed into caverns by a cellular automaton, the largest connected region kept and the rest filled back in. No rooms, no doors --- open floor and rock. |

The names are the `--algorithm` values, the `Algorithm` enumerators (`Maze1`, `Grid1`, `Cave1`), and
the suffix on the file each lives in.

## Files

Added to `library/quetzal/dungen`:

- `Algorithm.hpp/.cpp` --- the enumeration and its spelling.
- `StairPlacements.hpp/.cpp` --- the enumeration already in `main.cpp`, and its spelling.
- `Parameters.hpp/.cpp` --- the shared parameters, one nested group per algorithm, `random_seed`
  and `validate`.
- `generate.hpp/.cpp` --- dispatch on `parameters.algorithm`.
- `generate_maze1.hpp/.cpp`, `generate_grid1.hpp/.cpp`, `generate_cave1.hpp/.cpp` --- one layout each,
  and nothing shared between them beyond what is named here.
- `place_stairs.hpp/.cpp` --- turning candidate cells into stairs, each algorithm supplying the
  candidates its layout allows.
- `write_level.hpp/.cpp` --- the header row and the grid.

Unchanged: `Cell`, `Cells`, `CellType`, `CellTypeProperties`, `Door`, `Level`, `Pattern`, `Room`,
`Stair`, `Wall`, `types`.

## Interfaces

```cpp
enum class Algorithm { Maze1, Grid1, Cave1 };

std::string to_string(Algorithm algorithm);          // The command line and header spelling
Algorithm to_algorithm(std::string_view s);          // Throws std::invalid_argument

enum class StairPlacements { None, Paired, Multiple };

std::string to_string(StairPlacements placements);
StairPlacements to_stair_placements(std::string_view s);
```

```cpp
// Rooms dropped at random, a maze flooded through what they left, then pruned back
struct Maze1Parameters
{
    coord_type room_dimension_base = 3;
    coord_type room_dimension_variation = 8;
    double room_density = 0.25;
    bool room_merge = false;
    double corridor_straightness = 0.5;
    double probability_dead_end_removal = 0.5;
    double probability_loop_removal = 1.0;
};

// A room inside most sectors of a coarse grid, adjacent sectors joined
struct Grid1Parameters
{
    dimensions_type divisions {3, 3};   // Sectors across and up
    double room_density = 1.0;          // Fraction of sectors holding a room, the rest bare junctions
    coord_type room_dimension_min = 3;  // Smallest room, before its sector limits it further
    coord_type room_margin = 1;         // Rock kept between a room and its sector's edge
    double loop = 0.15;                 // Probability of a corridor beyond the ones connectivity needs
};

// Noise smoothed into caverns
struct Cave1Parameters
{
    double fill = 0.45;                 // Fraction of cells solid before smoothing
    size_t smoothing_passes = 4;
    size_t birth_threshold = 5;         // Solid neighbors that turn an open cell solid
    size_t survival_threshold = 4;      // Solid neighbors that keep a solid cell solid
};

struct Parameters
{
    using seed_type = std::default_random_engine::result_type;

    Algorithm algorithm = Algorithm::Maze1;
    dimensions_type size {32, 32};
    StairPlacements stair_placements = StairPlacements::Paired;

    Maze1Parameters maze1;
    Grid1Parameters grid1;
    Cave1Parameters cave1;

    std::optional<seed_type> seed; // Empty until assigned, which validate rejects
};

Parameters::seed_type random_seed();
void validate(const Parameters& parameters); // Throws std::invalid_argument naming the parameter
```

```cpp
Level generate(id_type id, const std::string& name, const Parameters& parameters);

Level generate_maze1(id_type id, const std::string& name, const Parameters& parameters);
Level generate_grid1(id_type id, const std::string& name, const Parameters& parameters);
Level generate_cave1(id_type id, const std::string& name, const Parameters& parameters);
```

Each reads the shared parameters and its own group, seeds its own `math::Random` from
`parameters.seed`, and is deterministic in what it was given.

```cpp
void place_stairs(Level& level, std::vector<position_type> candidates,
    StairPlacements placements, math::Random<>& random);
```

```cpp
constexpr auto format_version = "0.2";
void write_level(std::ostream& os, const Level& level, const Parameters& parameters);
```

## The file format

Unchanged, and the version stays `0.2`. The grid glyphs mean what they meant, so
`stratum_sim/load_layout_dungen.cpp` reads all three layouts as they are. The header row gains
`algorithm=` and holds only the active algorithm's group, which a reader that keeps `key=value`
in a map already tolerates.

## Command line

`dungen` keeps every option it has, each setting the group it belongs to, and gains
`--algorithm maze1|grid1|cave1`. Options belonging to an algorithm that was not asked for are read
and ignored rather than rejected, so an option may be given before the algorithm it belongs to.

## Stages

- [x] **1. Enumerations and parameters.** `Algorithm`, `StairPlacements`, `Parameters`,
      `write_level`, and `generate` dispatching to three skeletons.
- [x] **2. `maze1` moved,** out of `main.cpp` into `generate_maze1.cpp` unchanged in what it
      produces, with `place_stairs` split out. `main.cpp` left with the command line and file I/O
      only.
- [x] **3. `grid1`.**
- [x] **4. `cave1`.**
- [x] **5. `dungen_reference.md`.**
