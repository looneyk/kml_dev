#if !defined(QUETZAL_STRATUM_FORMAT_WRITE_ZONE_HPP)
#define QUETZAL_STRATUM_FORMAT_WRITE_ZONE_HPP
//------------------------------------------------------------------------------
// stratum_format
// write_zone.hpp
//------------------------------------------------------------------------------

#include "Document.hpp"
#include <filesystem>
#include <iosfwd>

namespace quetzal::stratum_format
{

    // Writes the document in the written form the format specifies: one grid row per line, one
    // entry per line, and coordinate arrays inline on the line that uses them
    void write(const std::filesystem::path& filepath, const Document& document);
    void write(std::ostream& os, const Document& document);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WRITE_ZONE_HPP
