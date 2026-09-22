#if !defined(QUETZAL_DUNGEN_PARAMETERS_HPP)
#define QUETZAL_DUNGEN_PARAMETERS_HPP
//------------------------------------------------------------------------------
// dungen
// Parameters.hpp
//------------------------------------------------------------------------------

#include "Algorithm.hpp"
#include "types.hpp"
#include "quetzal/math/Interval.hpp"
#include <optional>
#include <random>
#include <cstddef>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    // Rooms dropped at random until the density is met, a maze flooded through every cell
    // they left, then dead ends and loops pruned back
    struct Maze1Parameters
    {
        // A room dimension is the base plus a value below the variation, forced odd, so the
        // rooms sit on the same odd lattice the maze is flooded along
        coord_type room_dimension_base = 3;
        coord_type room_dimension_variation = 8;

        double room_density = 0.25;
        bool room_merge = false; // Whether a room overlapping one other room grows it rather than being dropped

        double corridor_straightness = 0.5; // Probability the maze keeps its heading where it can

        double probability_dead_end_removal = 0.5;
        double probability_loop_removal = 1.0;

        double door_probability = 1.0; // Probability of a corridor exiting a room having a door
    };

    //--------------------------------------------------------------------------
    // A room inside most sectors of a coarse grid, orthogonally adjacent sectors joined by
    // corridors that bend once in the rock between
    struct Grid1Parameters
    {
        vector_type divisions {3, 3}; // Sectors across and up

        // Fraction of sectors holding a room. A sector without one still joins its neighbors,
        // through a single cell rather than a room
        double room_density = 1.0;

        coord_type room_dimension_min = 3; // Smallest room, before its own sector limits it further
        coord_type room_margin = 1; // Rock kept between a room and the edge of its sector

        double loop = 0.15; // Probability of a corridor beyond the ones connectivity needs
        double door_probability = 1.0; // Probability of a corridor exiting a room having a door
    };

    //--------------------------------------------------------------------------
    // Noise smoothed into caverns by a cellular automaton over the eight neighbors of a cell,
    // the largest connected region kept and the rest filled back in
    struct Cave1Parameters
    {
        double fill = 0.45; // Fraction of cells solid before any smoothing

        size_t smoothing_passes = 4;
        size_t birth_threshold = 5; // Solid neighbors that turn an open cell solid
        size_t survival_threshold = 4; // Solid neighbors that keep a solid cell solid
    };

    //--------------------------------------------------------------------------
    // The generation parameters of one run, written into the zone file so that a zone names
    // what produced it and can be produced again
    // Only the group belonging to the chosen algorithm is read, so the others may hold
    // whatever a command line or a header put there
    struct Parameters
    {
        using seed_type = std::default_random_engine::result_type;

        Algorithm algorithm = Algorithm::Maze1;
        vector_type size {32, 32};

        // How many staircases lead down and how many up, each a count drawn from its closed range
        math::Interval<size_t> staircase_down_count {1, 1};
        math::Interval<size_t> staircase_up_count {1, 1};

        Maze1Parameters maze1;
        Grid1Parameters grid1;
        Cave1Parameters cave1;

        // Empty until one is assigned, so that a caller who never did is caught by validate
        // rather than quietly handed the same zone every run
        std::optional<seed_type> seed;
    };

    // A seed drawn from the platform's entropy source, for a caller wanting a different zone
    // each run. Generation itself takes the seed it is given, so that a written seed
    // reproduces a zone exactly
    Parameters::seed_type random_seed();

    // The shared parameters and the chosen algorithm's group, the other groups going unread
    // Throws std::invalid_argument naming the parameter at fault
    void validate(const Parameters& parameters);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_PARAMETERS_HPP
