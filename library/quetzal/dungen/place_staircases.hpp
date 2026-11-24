#if !defined(QUETZAL_DUNGEN_PLACE_STAIRCASES_HPP)
#define QUETZAL_DUNGEN_PLACE_STAIRCASES_HPP
//------------------------------------------------------------------------------
// dungen
// place_staircases.hpp
//------------------------------------------------------------------------------

#include "Zone.hpp"
#include "types.hpp"
#include "quetzal/math/Interval.hpp"
#include <vector>
#include <cstddef>

namespace quetzal::dungen
{

    // Turns candidates into staircases, alternating between leading up and leading down while both have some left of the
    // counts drawn from up_count and down_count. The candidates are shuffled first, so they may be supplied in any order. A
    // staircase is on the floor of its cell, oriented toward a passable side, and a candidate that is not floor, is a
    // doorway, or is beside one is skipped. Running out of candidates places fewer staircases than were drawn, which is a warning, written to
    // the process-wide log.
    // Throws std::runtime_error if a rise with a count above zero has none placed, since every staircase is an access point
    void place_staircases(Zone& zone, std::vector<discrete_point2_type> candidates, const math::Interval<size_t>& up_count,
        const math::Interval<size_t>& down_count, random_type& random);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_PLACE_STAIRCASES_HPP
