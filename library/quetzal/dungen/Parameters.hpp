#if !defined(QUETZAL_DUNGEN_PARAMETERS_HPP)
#define QUETZAL_DUNGEN_PARAMETERS_HPP
//------------------------------------------------------------------------------
// dungen
// Parameters.hpp
//------------------------------------------------------------------------------

#include "Algorithm.hpp"
#include "types.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/math/Random.hpp"
#include <optional>
#include <string>
#include <cstddef>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    // Rooms dropped at random until the density is met, a maze flooded through every cell
    // they left, then dead ends and loops pruned back
    struct Maze1Parameters
    {
        // A room dimension is the base plus a value below the variation, forced odd to align rooms with the maze's odd
        // lattice
        coord_type room_dimension_base = 3;
        coord_type room_dimension_variation = 8;

        double room_density = 0.25;
        bool room_merge = false; // True if a room overlapping one other room grows it rather than being dropped

        double corridor_straightness = 0.5; // Probability the maze keeps its heading where it can

        double probability_dead_end_removal = 0.5;
        double probability_loop_removal = 1.0;

        double door_probability = 1.0; // Probability of a corridor exiting a room having a door
    };

    //--------------------------------------------------------------------------
    // A room inside most sectors of a coarse grid, orthogonally adjacent sectors joined by corridors that bend once in the
    // solid cells between
    struct Grid1Parameters
    {
        vector_type divisions {3, 3}; // Sectors across and up

        // Fraction of sectors with a room. A sector without one joins its neighbors through a single cell
        double room_density = 1.0;

        coord_type room_dimension_min = 3; // Smallest room, before its own sector limits it further
        coord_type room_margin = 1; // Solid cells between a room and the edge of its sector

        double loop = 0.15; // Probability of a corridor beyond the ones connectivity needs
        double door_probability = 1.0; // Probability of a corridor exiting a room having a door
    };

    //--------------------------------------------------------------------------
    // Noise smoothed into caverns by a cellular automaton over the eight neighbors of a cell,
    // the largest connected area kept and the rest filled back in
    struct Cave1Parameters
    {
        double fill = 0.45; // Fraction of cells solid before any smoothing

        size_t smoothing_passes = 4;
        size_t birth_threshold = 5; // Solid neighbors that turn an open cell solid
        size_t survival_threshold = 4; // Solid neighbors that keep a solid cell solid
    };

    //--------------------------------------------------------------------------
    // The generation parameters of one run, written into the zone file so the zone can be reproduced. Only the chosen
    // algorithm's group is read
    struct Parameters
    {
        using seed_type = random_type::seed_type;

        Algorithm algorithm = Algorithm::Maze1;
        vector_type extent {32, 32};

        // How many staircases lead down and how many up, each a count drawn from its closed range
        math::Interval<size_t> staircase_down_count {1, 1};
        math::Interval<size_t> staircase_up_count {1, 1};

        // The region spec of the worldspace each staircase leading down leads to, the kth of them named <region spec>_<k>
        // from 1, and level -1 there. Each leads one level down in its own worldspace if unassigned
        std::optional<std::string> staircase_down_region_spec;

        Maze1Parameters maze1;
        Grid1Parameters grid1;
        Cave1Parameters cave1;

        // The zone's position in its region, written into the zone file if assigned
        std::optional<zone_position_type> position;

        // Empty until assigned; validate rejects an empty seed
        std::optional<seed_type> seed;
    };

    // Sets the parameter the key refers to, and returns false if the key is not a parameter
    // Throws std::invalid_argument for a value that is not of the parameter's type
    bool set_parameter(Parameters& parameters, const std::string& key, const std::string& text);

    // Sets each parameter a token refers to, the keys being those of the zone file's generator parameters. room_density and
    // door_probability set the value in both groups that have one
    // Throws std::invalid_argument for a key that is not a parameter or a value that is not of the parameter's type
    void set_parameters(Parameters& parameters, const KeyValueStore& tokens);

    // Validates the shared parameters and the chosen algorithm's group
    // Throws std::invalid_argument identifying the parameter at fault
    void validate(const Parameters& parameters);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_PARAMETERS_HPP
