#if !defined(QUETZAL_STRATUM_FORMAT_REGIONSPECSDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_REGIONSPECSDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// RegionSpecsDocument.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/KeyValueStore.hpp"
#include <filesystem>
#include <iosfwd>
#include <map>
#include <string>
#include <string_view>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // The game's region specs, each a set of keys and values
    struct RegionSpecsDocument
    {
        static constexpr std::string_view format_name = "quetzal.stratum_region_specs";
        static constexpr std::string_view format_version = "0.1";

        // Relative to the assets or a mod directory
        static constexpr std::string_view filepath = "data/region_specs.json";

        std::string version;

        // <region spec name, keys>
        std::map<std::string, KeyValueStore> region_specs;
    };

    // Throws std::runtime_error on error or an unsupported key, stream_name used in error messages
    RegionSpecsDocument read_region_specs(const std::filesystem::path& filepath);
    RegionSpecsDocument read_region_specs(std::istream& is, const std::string& stream_name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_REGIONSPECSDOCUMENT_HPP
