#if !defined(QUETZAL_CATAGEN_PARAMETERS_HPP)
#define QUETZAL_CATAGEN_PARAMETERS_HPP
//------------------------------------------------------------------------------
// catagen
// Parameters.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/math/Random.hpp"
#include <optional>
#include <cstddef>

namespace quetzal::catagen
{

    //--------------------------------------------------------------------------
    // The generation parameters of one run, written into the zone file so the zone can be reproduced
    struct Parameters
    {
        using seed_type = math::portable_random_engine_type::result_type;

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

        // The zone's position in its region, written into the zone file if assigned
        std::optional<zone_position_type> position;

        // Empty until assigned; validate rejects an empty seed
        std::optional<seed_type> seed;
    };

    // A seed drawn from the platform's entropy source
    Parameters::seed_type random_seed();

    // Throws std::invalid_argument identifying the parameter at fault
    void validate(const Parameters& parameters);

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_PARAMETERS_HPP
