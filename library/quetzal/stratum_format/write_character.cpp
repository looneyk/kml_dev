//------------------------------------------------------------------------------
// stratum_format
// write_character.cpp
//------------------------------------------------------------------------------

#include "write_character.hpp"
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
    std::string document_text(const CharacterDocument& document)
    {
        std::string inventory = "\"inventory\": [";

        if (document.inventory.empty())
        {
            inventory += "]";
        }
        else
        {
            inventory += "\n";

            for (size_t i = 0; i < document.inventory.size(); ++i)
            {
                write_entry(inventory, entity_entry(document.inventory[i]), entry_indent + 2, i + 1 == document.inventory.size() ? "" : ",");
            }

            inventory += indentation(entry_indent) + "]";
        }

        std::vector<std::string> character
        {
            std::format("\"id\": {}", document.id),
            std::format("\"components\": {}", object_value_text(document.components)),
            std::format("\"zone\": {}", document.zone_id),
            std::format("\"placement\": {}", object_text({std::format("\"cell_position\": {}", cell_position_text(document.placement.position())),
                std::format("\"orientation\": {}", direction_text(document.placement.direction()))})),
            inventory
        };

        std::vector<std::string> sections = saved_game_file_sections();
        sections.push_back(object_section("character", character));

        return file_text(sections);
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(std::ostream& os, const CharacterDocument& document)
{
    os << document_text(document);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(const std::filesystem::path& filepath, const CharacterDocument& document)
{
    write_file(filepath, document_text(document));
    return;
}
