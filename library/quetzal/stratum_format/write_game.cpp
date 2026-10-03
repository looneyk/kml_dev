//------------------------------------------------------------------------------
// stratum_format
// write_game.cpp
//------------------------------------------------------------------------------

#include "write_game.hpp"
#include "written_form.hpp"
#include <format>
#include <ostream>
#include <string>
#include <vector>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    //--------------------------------------------------------------------------
    std::string document_text(const GameDocument& document)
    {
        std::vector<std::string> engines;
        for (const auto& [stream, state] : document.engines)
        {
            engines.push_back(std::format("{}: {}", string_text(stream), string_text(state)));
        }

        std::vector<std::string> game
        {
            std::format("\"next_id\": {}", document.next_id),
            std::format("\"seed\": {}", string_text(document.seed)),
            std::format("\"engines\": {}", object_text(engines))
        };

        std::vector<std::string> sections = file_header_sections();
        sections.push_back(std::format("{}\"game\": {}", indentation(section_indent), object_text(game)));
        sections.push_back(std::format("{}\"settings\": {}", indentation(section_indent), object_or_empty_text(document.settings)));

        return top_level_object_text(sections);
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(std::ostream& os, const GameDocument& document)
{
    os << document_text(document);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(const std::filesystem::path& filepath, const GameDocument& document)
{
    write_file(filepath, document_text(document));
    return;
}
