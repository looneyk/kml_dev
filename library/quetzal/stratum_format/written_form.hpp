#if !defined(QUETZAL_STRATUM_FORMAT_WRITTEN_FORM_HPP)
#define QUETZAL_STRATUM_FORMAT_WRITTEN_FORM_HPP
//------------------------------------------------------------------------------
// stratum_format
// written_form.hpp
//------------------------------------------------------------------------------

#include "Document.hpp"
#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>
#include <cassert>
#include <cmath>

// The text of the written form that the zone file and the game files share, for their writers
namespace quetzal::stratum_format::detail
{

    constexpr size_t section_indent = 2;
    constexpr size_t entry_indent = 4;

    std::string indentation(size_t indent);
    std::string string_text(std::string_view s);

    // The shortest digits that read back as the same value, in fixed notation, and with a fraction always shown
    template<std::floating_point T>
    std::string number_text(T value);

    // Arrays of objects are spaced inside their brackets, as objects are inside their braces; arrays of anything else are not
    std::string inline_text(const raw_values_type& v);

    // Joins the members of an object or the elements of an array with separators
    std::string joined(const std::vector<std::string>& parts);

    std::string object_text(const std::vector<std::string>& members);
    std::string cell_position_text(const discrete_point_type& cell_position);
    std::string direction_text(const discrete_vector_type& direction);
    std::string address_text(const WallAddress& address);
    std::string reference_text(stored_entity_id_type id);

    // A member per line, each value inline
    std::string object_section(std::string_view key, const std::vector<std::string>& members);

    //--------------------------------------------------------------------------
    // One member of an entry. A member with entries is an array of entries, written an entry per line
    struct EntryMember
    {
        std::string key;
        std::string value;
        std::vector<std::vector<EntryMember>> entries;
    };

    using entry_type = std::vector<EntryMember>;

    // Writes the entry on one line, unless it has members with entries. Each of those begins its own line, aligned inside
    // the brace
    void write_entry(std::string& text, const entry_type& entry, size_t indent, std::string_view suffix);

    // site, parent and relocator are written into components, ahead of the rest
    entry_type entity_entry(const Entity& entity);

    // An absent object is written as an empty one
    std::string object_or_empty_text(const raw_values_type& values);

    // The header that opens each file of a saved game but its zones: the format and the version
    std::vector<std::string> file_header_sections();
    std::vector<std::string> file_header_sections(std::string_view header_format, std::string_view header_version);

    // The sections of a file, each on lines of its own, in the file's one top-level object
    std::string top_level_object_text(const std::vector<std::string>& sections);

    void write_file(const std::filesystem::path& filepath, const std::string& text);

} // namespace quetzal::stratum_format::detail

//------------------------------------------------------------------------------
template<std::floating_point T>
std::string quetzal::stratum_format::detail::number_text(T value)
{
    assert(std::isfinite(value));

    std::array<char, 512> buffer;
    auto [end, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::fixed);
    assert(ec == std::errc());

    std::string text(buffer.data(), end);
    if (text.find('.') == std::string::npos)
    {
        text += ".0";
    }

    return text;
}

#endif // QUETZAL_STRATUM_FORMAT_WRITTEN_FORM_HPP
