#if !defined(QUETZAL_CATAGEN_WRITE_ZONE_HPP)
#define QUETZAL_CATAGEN_WRITE_ZONE_HPP
//------------------------------------------------------------------------------
// catagen
// write_zone.hpp
//------------------------------------------------------------------------------

#include "Zone.hpp"
#include "Parameters.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include <iosfwd>

namespace quetzal::catagen
{

    // Returns the zone as a discrete layout document in the wall notation, with the generator's parameters as the tokens
    // their options take, so that the document reproduces its own zone. A zone with a name has it as the layout's name
    // Throws std::bad_optional_access when no seed was assigned
    stratum_format::Document make_document(const Zone& zone, const Parameters& parameters);

    // Writes the zone's document as a discrete layout file
    void write_zone(std::ostream& os, const Zone& zone, const Parameters& parameters);

    // The grid rows alone, one per line
    void write_grid(std::ostream& os, const Zone& zone);

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_WRITE_ZONE_HPP
