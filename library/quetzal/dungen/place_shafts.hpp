#if !defined(QUETZAL_DUNGEN_PLACE_SHAFTS_HPP)
#define QUETZAL_DUNGEN_PLACE_SHAFTS_HPP
//------------------------------------------------------------------------------
// dungen
// place_shafts.hpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include "ShaftPlacements.hpp"
#include "types.hpp"
#include <vector>

namespace quetzal::dungen
{

    // Turns as many of the candidates into shafts as the placement mode asks for: Paired is
    // the two a level needs to be entered and left, and Multiple is three plus one per
    // thousand cells, drawn at random. The candidates are shuffled first, so they may be
    // supplied in any order, and running out of them places fewer than were asked for
    // Each algorithm supplies the cells its own layout allows a shaft to stand in
    void place_shafts(Level& level, std::vector<position_type> candidates, ShaftPlacements placements, random_type& random);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_PLACE_SHAFTS_HPP
