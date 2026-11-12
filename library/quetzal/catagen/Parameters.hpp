#if !defined(QUETZAL_CATAGEN_PARAMETERS_HPP)
#define QUETZAL_CATAGEN_PARAMETERS_HPP
//------------------------------------------------------------------------------
// catagen
// Parameters.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/math/Random.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <cstddef>

namespace quetzal::catagen
{

    //--------------------------------------------------------------------------
    // The layout algorithm
    enum class Algorithm
    {
        Partition1, // The extent partitioned into rooms, joined by corridors and doors
        Surface1    // Open floor inside a solid border, with its staircases
    };

    // The command line and file header spelling, lower case
    std::string to_string(Algorithm algorithm);

    // Throws std::invalid_argument quoting s when it is not an algorithm
    Algorithm to_algorithm(std::string_view s);

    //--------------------------------------------------------------------------
    // The generation parameters of one run, written into the zone file so the zone can be reproduced. scale, spacing,
    // density and loop are partition1's
    struct Parameters
    {
        using seed_type = math::Random<math::portable_random_engine_type>::seed_type;

        Algorithm algorithm = Algorithm::Partition1;
        vector_type extent {48, 32}; // The size of the zone, in cells

        // Target room size in cells. The partition splits while a room exceeds the upper bound
        // and never below the lower, so a room falls within the range unless the range is too
        // narrow to split into -- an upper below twice the lower lets rooms exceed it
        math::Interval<coord_type> scale {4, 8};

        // Solid cells between rooms. At zero, adjoining rooms share a wall
        math::Interval<coord_type> spacing {0, 0};

        // Fraction of the extent kept as rooms
        double density = 1.0;
        double loop = 0.1; // Probability of a door beyond the ones connectivity needs

        // How many staircases lead down and how many up, each a count drawn from its closed range
        math::Interval<size_t> staircase_down_count {1, 1};
        math::Interval<size_t> staircase_up_count {1, 1};

        // The region spec of the worldspace each staircase leading down leads to, the kth of them named <region spec>_<k>
        // from 1, and level -1 there. Each leads one level down in its own worldspace if unassigned
        std::optional<std::string> staircase_down_region_spec;

        // The zone's position in its region, written into the zone file if assigned
        std::optional<zone_position_type> position;

        // Empty until assigned; validate rejects an empty seed
        std::optional<seed_type> seed;
    };

    // Sets the parameter the key refers to, and returns false if the key is not a parameter
    // Throws std::invalid_argument for a value that is not of the parameter's type
    bool set_parameter(Parameters& parameters, const std::string& key, const std::string& text);

    // Sets each parameter a token refers to, the keys being those of the zone file's generator parameters
    // Throws std::invalid_argument for a key that is not a parameter or a value that is not of the parameter's type
    void set_parameters(Parameters& parameters, const KeyValueStore& tokens);

    // Throws std::invalid_argument identifying the parameter at fault
    void validate(const Parameters& parameters);

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_PARAMETERS_HPP
