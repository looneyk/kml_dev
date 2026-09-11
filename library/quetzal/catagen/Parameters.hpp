#if !defined(QUETZAL_CATAGEN_PARAMETERS_HPP)
#define QUETZAL_CATAGEN_PARAMETERS_HPP
//------------------------------------------------------------------------------
// catagen
// Parameters.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/math/Interval.hpp"
#include <optional>
#include <random>

namespace quetzal::catagen
{

    //--------------------------------------------------------------------------
    // The generation parameters of one run, written into the level file so that a level
    // names what produced it and can be produced again
    struct Parameters
    {
        using seed_type = std::default_random_engine::result_type;

        vector_type extent {48, 32}; // The area to fill, in cells

        // Target room size in cells. The partition splits while a room exceeds the upper bound
        // and never below the lower, so a room falls within the range unless the range is too
        // narrow to split into -- an upper below twice the lower leaves rooms oversized
        math::Interval<coord_type> scale {4, 8};

        // Rock left between rooms, in cells. Zero has them share a wall; anything more sets
        // them apart, and what separates them is what the corridors then cross
        math::Interval<coord_type> spacing {0, 0};

        // Fraction of the extent the rooms are drawn from; 1.0 keeps every one of them, so
        // with no spacing it leaves nothing solid at all
        double density = 1.0;
        double loop = 0.1; // Probability of a door beyond the ones connectivity needs

        // Empty until one is assigned, so that a caller who never did is caught by validate
        // rather than quietly handed the same level every run
        std::optional<seed_type> seed;
    };

    // A seed drawn from the platform's entropy source, for a caller wanting a different
    // level each run. Generation itself takes the seed it is given, so that a written
    // seed reproduces a level exactly
    Parameters::seed_type random_seed();

    // Throws std::invalid_argument naming the parameter at fault
    void validate(const Parameters& parameters);

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_PARAMETERS_HPP
