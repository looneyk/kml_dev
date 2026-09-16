#if !defined(QUETZAL_DUNGEN_REMOVE_DOORS_HPP)
#define QUETZAL_DUNGEN_REMOVE_DOORS_HPP
//------------------------------------------------------------------------------
// dungen
// remove_doors.hpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include "types.hpp"

namespace quetzal::dungen
{

    // Keeps each door with probability door_probability, the doorway of each one removed left open as corridor. Run
    // after every other pass, an algorithm placing its doorways as doors and reading them as such until then
    void remove_doors(Level& level, double door_probability, random_type& random);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_REMOVE_DOORS_HPP
