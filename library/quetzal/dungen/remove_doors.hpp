#if !defined(QUETZAL_DUNGEN_REMOVE_DOORS_HPP)
#define QUETZAL_DUNGEN_REMOVE_DOORS_HPP
//------------------------------------------------------------------------------
// dungen
// remove_doors.hpp
//------------------------------------------------------------------------------

#include "Zone.hpp"
#include "types.hpp"

namespace quetzal::dungen
{

    // Keeps each door with probability door_probability, leaving a removed door's doorway open as corridor. Run after every other pass
    void remove_doors(Zone& zone, double door_probability, random_type& random);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_REMOVE_DOORS_HPP
