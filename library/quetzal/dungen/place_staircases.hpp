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

    // Turns candidates into staircases: a count leading up drawn from staircase_up_count, then a count leading down drawn
    // from staircase_down_count. The candidates are shuffled first, so they may be supplied in any order, and running out of
    // them places fewer than were asked for, those leading down going short before those leading up. A candidate whose
    // block would cut a passage, or leave a staircase nothing to face, is passed over
    // This should alternately place up and down staircases ...
    // Throws std::runtime_error where none can be placed, every staircase being an access point and a zone needing one
    // Each algorithm supplies the cells its own layout allows a staircase to be placed in
    void place_staircases(Level& level, std::vector<point_type> candidates, const math::Interval<size_t>& staircase_down_count,
        const math::Interval<size_t>& staircase_up_count, random_type& random);

    // Moves each staircase blocking a doorway one cell, in a random direction onto floor of its own space that is not a
    // doorway and blocks none, and leaves it where there is no such direction. A doorway is a door, or the corridor cell
    // beside room floor that a removed door leaves
    // Run after every pass that places or removes a door
    void move_staircases_off_doorways(Level& level, random_type& random);

    // A random facing for each staircase among the sides of its cell with floor beyond, or where there are none, with a door
    void face_staircases(Level& level, random_type& random);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_PLACE_STAIRCASES_HPP
