#if !defined(QUETZAL_DUNGEN_PLACE_STAIRS_HPP)
#define QUETZAL_DUNGEN_PLACE_STAIRS_HPP
//------------------------------------------------------------------------------
// dungen
// place_stairs.hpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include "StairPlacements.hpp"
#include "types.hpp"
#include <vector>

namespace quetzal::dungen
{

    // Turns as many of the candidates into stairs as the placement mode asks for: Paired is
    // the two a level needs to be entered and left, and Multiple is three plus one per
    // thousand cells, drawn at random. The candidates are shuffled first, so they may be
    // supplied in any order, and running out of them places fewer than were asked for
    // Each algorithm supplies the cells its own layout allows a stair to stand in
    void place_stairs(Level& level, std::vector<position_type> candidates, StairPlacements placements, random_type& random);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_PLACE_STAIRS_HPP
