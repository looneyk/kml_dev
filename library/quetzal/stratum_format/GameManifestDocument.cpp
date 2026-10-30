//------------------------------------------------------------------------------
// stratum_format
// GameManifestDocument.cpp
//------------------------------------------------------------------------------

#include "GameManifestDocument.hpp"
#include "checked_values.hpp"
#include <algorithm>
#include <array>
#include <format>
#include <optional>
#include <string_view>
#include <utility>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    constexpr std::array<std::string_view, 7> file_keys {"format", "version", "notation", "views", "view_menu", "default_view", "settings"};

    //--------------------------------------------------------------------------
    GameManifestDocument read_game_manifest_document(const raw_values_type& v)
    {
        check_file_header(v, GameManifestDocument::format_name, GameManifestDocument::format_version);
        check_keys(v, "", file_keys, "game manifest file");

        GameManifestDocument document;
        document.version = std::string(GameManifestDocument::format_version);

        const std::string notation_name = string_value(required_member(v, "", "notation"), "notation");
        const std::optional<Notation> notation = to_notation(notation_name);
        if (!notation)
        {
            fail("notation", std::format("'{}' is not a notation", notation_name));
        }

        document.notation = *notation;

        const raw_values_type& views = array(required_member(v, "", "views"), "views");
        for (size_t i = 0; i < views.size(); ++i)
        {
            const std::string path = element_path("views", i);
            std::string view = string_value(views[i], path);
            if (std::ranges::find(document.views, view) != document.views.end())
            {
                fail(path, std::format("'{}' is listed more than once", view));
            }

            document.views.push_back(std::move(view));
        }

        if (document.views.empty())
        {
            fail("views", "lists no view");
        }

        document.view_menu = boolean_value(required_member(v, "", "view_menu"), "view_menu");

        document.default_view = string_value(required_member(v, "", "default_view"), "default_view");
        if (std::ranges::find(document.views, document.default_view) == document.views.end())
        {
            fail("default_view", std::format("'{}' is not one of the views", document.default_view));
        }

        if (const raw_values_type* settings = member(v, "settings"); settings != nullptr)
        {
            document.settings = text_values(*settings, "settings");
        }

        return document;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::GameManifestDocument quetzal::stratum_format::read_game_manifest(std::istream& is, const std::string& stream_name)
{
    return read_document<GameManifestDocument>(is, stream_name, read_game_manifest_document);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::GameManifestDocument quetzal::stratum_format::read_game_manifest(const std::filesystem::path& filepath)
{
    return read_document<GameManifestDocument>(filepath, read_game_manifest_document);
}
