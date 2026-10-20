//------------------------------------------------------------------------------
// stratum_format
// read_game.cpp
//------------------------------------------------------------------------------

#include "read_game.hpp"
#include "checked_values.hpp"
#include "saved_game_format.hpp"
#include "SettingsDocument.hpp"

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    //--------------------------------------------------------------------------
    GameDocument read_game_document(const raw_values_type& v)
    {
        check_file_header(v, saved_game_format_name, saved_game_format_version);

        GameDocument document;
        document.version = std::string(saved_game_format_version);

        const raw_values_type& game = object(required_member(v, "", "game"), "game");
        document.next_id = id_value(required_member(game, "game", "next_id"), "game.next_id");
        document.seed = string_value(required_member(game, "game", "seed"), "game.seed");

        const raw_values_type& random_engines = object(required_member(game, "game", "random_engines"), "game.random_engines");
        for (const auto& [stream, state] : random_engines.items())
        {
            document.random_engines[stream] = string_value(state, member_path("game.random_engines", stream));
        }

        document.settings_snapshot = read_snapshot(v);
        return document;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::GameDocument quetzal::stratum_format::read_game(std::istream& is, const std::string& stream_name)
{
    return read_document<GameDocument>(is, stream_name, read_game_document);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::GameDocument quetzal::stratum_format::read_game(const std::filesystem::path& filepath)
{
    return read_document<GameDocument>(filepath, read_game_document);
}
