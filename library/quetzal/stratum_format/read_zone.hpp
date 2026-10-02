#if !defined(QUETZAL_STRATUM_FORMAT_READ_ZONE_HPP)
#define QUETZAL_STRATUM_FORMAT_READ_ZONE_HPP
//------------------------------------------------------------------------------
// stratum_format
// read_zone.hpp
//------------------------------------------------------------------------------

#include "Document.hpp"
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <string>

namespace quetzal::stratum_format
{

    // Called with each discrepancy that is not an error, stating what it is and where
    using warning_function_type = std::function<void(const std::string& warning)>;

    // Writes a warning to std::clog
    void log_warning(const std::string& warning);

    // Reads and validates a layout file. Throws std::runtime_error stating what was wrong and where
    Document read(const std::filesystem::path& filepath, const warning_function_type& warning = log_warning);

    // name identifies the stream in error and warning messages
    Document read(std::istream& is, const std::string& name, const warning_function_type& warning = log_warning);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_READ_ZONE_HPP
