#if !defined(QUETZAL_DUNGEN_WRITE_ZONE_HPP)
#define QUETZAL_DUNGEN_WRITE_ZONE_HPP
//------------------------------------------------------------------------------
// dungen
// write_zone.hpp
//------------------------------------------------------------------------------

#include "Zone.hpp"
#include "Parameters.hpp"
#include "quetzal/stratum_format/ZoneDocument.hpp"
#include <iosfwd>

namespace quetzal::dungen
{

    // Returns the zone as a discrete layout document in the block notation, with the generator's parameters as the tokens
    // their options take, so that the document reproduces its own zone. Only the chosen algorithm's group is included. A
    // zone with a name has it as the layout's name
    // Throws std::bad_optional_access when no seed was assigned
    stratum_format::ZoneDocument make_document(const Zone& zone, const Parameters& parameters);

    // Writes the zone's document as a discrete layout file
    void write_zone(std::ostream& os, const Zone& zone, const Parameters& parameters);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_WRITE_ZONE_HPP
