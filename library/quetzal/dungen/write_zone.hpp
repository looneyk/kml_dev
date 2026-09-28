#if !defined(QUETZAL_DUNGEN_WRITE_ZONE_HPP)
#define QUETZAL_DUNGEN_WRITE_ZONE_HPP
//------------------------------------------------------------------------------
// dungen
// write_zone.hpp
//------------------------------------------------------------------------------

#include "Zone.hpp"
#include "Parameters.hpp"
#include <iosfwd>

namespace quetzal::dungen
{

    // Writes a discrete layout file in the block notation, with the generator's parameters written as the tokens their
    // options take, so that the file reproduces its own zone. Only the chosen algorithm's group is written. A zone with a
    // name has it written as the layout's name
    // Throws std::bad_optional_access when no seed was assigned
    void write_zone(std::ostream& os, const Zone& zone, const Parameters& parameters);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_WRITE_ZONE_HPP
