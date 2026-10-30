#if !defined(QUETZAL_STRATUM_FORMAT_GAMEMANIFESTDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_GAMEMANIFESTDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// GameManifestDocument.hpp
//------------------------------------------------------------------------------

#include "Notation.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    struct GameManifestDocument
    {
        static constexpr std::string_view format_name = "quetzal.stratum_game_manifest";
        static constexpr std::string_view format_version = "0.1";

        std::string version;
        Notation notation = Notation::Block;
        std::vector<std::string> views;
        bool view_menu = false; // notation/view added to main menu, development option
        std::string default_view;
        KeyValueStore settings; // game settings for this particular configuration
    };

    // Throws std::runtime_error on error or an unsupported key, stream_name used in error messages
    GameManifestDocument read_game_manifest(const std::filesystem::path& filepath);
    GameManifestDocument read_game_manifest(std::istream& is, const std::string& stream_name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_GAMEMANIFESTDOCUMENT_HPP
