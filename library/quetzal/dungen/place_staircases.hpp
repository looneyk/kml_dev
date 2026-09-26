#if !defined(QUETZAL_DUNGEN_PLACE_STAIRCASES_HPP)
#define QUETZAL_DUNGEN_PLACE_STAIRCASES_HPP
//------------------------------------------------------------------------------
// dungen
// place_staircases.hpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include "types.hpp"
#include "quetzal/math/Interval.hpp"
#include <vector>
#include <cstddef>

namespace quetzal::dungen
{

    // Turns candidates into staircases, alternately leading up and leading down while both have some left of a count drawn
    // from up_count and down_count. The candidates are shuffled first, so they may be supplied in any order; one that
    // cannot take a staircase facing floor without blocking a doorway or cutting a passage in two is skipped, and running
    // out of them places fewer than were asked for, which is a warning.
    // Throws std::runtime_error where a rise with a count above zero has none placed, every staircase being an access point
    void place_staircases(Level& level, std::vector<point_type> candidates, const math::Interval<size_t>& up_count,
        const math::Interval<size_t>& down_count, random_type& random, const warning_function_type& warning);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_PLACE_STAIRCASES_HPP
