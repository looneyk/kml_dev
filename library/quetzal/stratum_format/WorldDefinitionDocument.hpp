#if !defined(QUETZAL_STRATUM_FORMAT_WORLDDEFINITIONDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_WORLDDEFINITIONDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// WorldDefinitionDocument.hpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_core/RegionSpec.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    struct WorldDefinitionDocument
    {
        static constexpr std::string_view format_name = "quetzal.stratum_world_definition";
        static constexpr std::string_view format_version = "0.1";

        // Relative to the assets or a mod directory
        static constexpr std::string_view filepath = "data/world_definition.json";

        using region_spec_tree_type = std::vector<stratum_core::RegionSpec>;

        std::string version;

        std::string starting_region; // New game region name

        std::vector<region_spec_tree_type> region_specs;
    };

    // Throws std::runtime_error on error or an unsupported key, stream_name used in error messages
    WorldDefinitionDocument read_world_definition(const std::filesystem::path& filepath);
    WorldDefinitionDocument read_world_definition(std::istream& is, const std::string& stream_name);

    // Writes each tree's region specs a line each
    void write(const std::filesystem::path& filepath, const WorldDefinitionDocument& document);
    void write(std::ostream& os, const WorldDefinitionDocument& document);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WORLDDEFINITIONDOCUMENT_HPP
