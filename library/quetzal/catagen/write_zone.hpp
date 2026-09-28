#if !defined(QUETZAL_CATAGEN_WRITE_ZONE_HPP)
#define QUETZAL_CATAGEN_WRITE_ZONE_HPP
//------------------------------------------------------------------------------
// catagen
// write_zone.hpp
//------------------------------------------------------------------------------

#include "Zone.hpp"
#include "Parameters.hpp"
#include <iosfwd>

namespace quetzal::catagen
{

    // Writes a discrete layout file in the wall notation, with the generator's parameters written as the tokens their
    // options take, so that the file reproduces its own zone, and the zone's name, if it has one, as the layout's name
    // Throws std::bad_optional_access when no seed was assigned
    void write_zone(std::ostream& os, const Zone& zone, const Parameters& parameters);

    // The grid rows alone, one per line
    void write_grid(std::ostream& os, const Zone& zone);

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_WRITE_ZONE_HPP
