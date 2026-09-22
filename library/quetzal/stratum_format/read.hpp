#if !defined(QUETZAL_STRATUM_FORMAT_READ_HPP)
#define QUETZAL_STRATUM_FORMAT_READ_HPP
//------------------------------------------------------------------------------
// stratum_format
// read.hpp
//------------------------------------------------------------------------------

#include "Document.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>

namespace quetzal::stratum_format
{

    // Reads a layout file. Throws std::runtime_error stating what was wrong and where, and returns
    // nothing until every check the format requires has passed
    Document read(const std::filesystem::path& filepath);

    // The same over a stream already open
    // name is for error identification
    Document read(std::istream& is, const std::string& name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_READ_HPP
