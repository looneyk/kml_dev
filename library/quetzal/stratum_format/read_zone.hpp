#if !defined(QUETZAL_STRATUM_FORMAT_READ_ZONE_HPP)
#define QUETZAL_STRATUM_FORMAT_READ_ZONE_HPP
//------------------------------------------------------------------------------
// stratum_format
// read_zone.hpp
//------------------------------------------------------------------------------

#include "ZoneDocument.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>

namespace quetzal::stratum_format
{

    // Reads and validates a layout file. Throws std::runtime_error stating what was wrong and where. Writes each
    // discrepancy that is not an error to the process-wide log as a warning, stating what it is and where
    ZoneDocument read_zone(const std::filesystem::path& filepath);

    // stream_name identifies the stream in error and warning messages
    ZoneDocument read_zone(std::istream& is, const std::string& stream_name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_READ_ZONE_HPP
