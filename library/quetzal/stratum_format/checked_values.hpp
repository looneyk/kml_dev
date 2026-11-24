#if !defined(QUETZAL_STRATUM_FORMAT_CHECKED_VALUES_HPP)
#define QUETZAL_STRATUM_FORMAT_CHECKED_VALUES_HPP
//------------------------------------------------------------------------------
// stratum_format
// checked_values.hpp
//------------------------------------------------------------------------------

#include "ZoneDocument.hpp"
#include "types.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

// The checked reading of the values that the zone and game files share
// An invalid value throws std::runtime_error, stating the path within the document and error details
namespace quetzal::stratum_format::detail
{

    // A path into the document, for error messages
    std::string member_path(const std::string& path, std::string_view key);
    std::string element_path(const std::string& path, size_t i);

    [[noreturn]] void fail(const std::string& path, std::string_view what);

    // Returns the value at a key, or null if the key is absent or set to null
    const raw_values_type* member(const raw_values_type& v, std::string_view key);

    const raw_values_type& required_member(const raw_values_type& v, const std::string& path, std::string_view key);
    const raw_values_type& object(const raw_values_type& v, const std::string& path);
    const raw_values_type& array(const raw_values_type& v, const std::string& path);
    std::string string_value(const raw_values_type& v, const std::string& path);

    // Returns std::nullopt if the key is absent or null
    std::optional<std::string> optional_string(const raw_values_type& v, std::string_view key, const std::string& path = {});

    // An object whose every value is text
    KeyValueStore text_values(const raw_values_type& v, const std::string& path);

    // Requires every key of the object to be one of the keys, naming the file in the error for any other
    void check_keys(const raw_values_type& v, const std::string& path, std::span<const std::string_view> keys, std::string_view file_description);
    value_type number_value(const raw_values_type& v, const std::string& path);
    bool boolean_value(const raw_values_type& v, const std::string& path);

    // A whole number within the coordinate range; a fraction is an error
    coord_type coord_value(const raw_values_type& v, const std::string& path);

    size_t count_value(const raw_values_type& v, const std::string& path);

    // A whole, non-negative number
    stored_entity_id_type id_value(const raw_values_type& v, const std::string& path);

    // A seed is written as text, since it spans a range wider than a JSON number represents exactly
    seed_type seed_value(const raw_values_type& v, const std::string& path);

    const raw_values_type& fixed_array(const raw_values_type& v, const std::string& path, size_t n);
    discrete_point2_type read_cell_position(const raw_values_type& v, const std::string& path);
    point_type read_position(const raw_values_type& v, const std::string& path);
    surface_point_type read_surface_position(const raw_values_type& v, const std::string& path);

    // A cardinal direction
    discrete_vector2_type read_direction(const raw_values_type& v, const std::string& path);

    // The cell the wall belongs to, and the normal pointing out of the wall's surface into that cell
    WallPlacement read_wall_placement(const raw_values_type& v, const std::string& path);

    // Returns null if the entry has no components
    raw_values_type read_components(const raw_values_type& v, const std::string& path);

    // Requires the file's header to have the format and version that a reader reads
    void check_file_header(const raw_values_type& v, std::string_view required_format, std::string_view required_version);

    // Parses the stream as JSON, prefixing a parse error with the stream's name
    raw_values_type parse_json(std::istream& is, const std::string& stream_name);

    // Parses the stream and reads the document from it, prefixing every error with the stream's name
    template<typename D>
    D read_document(std::istream& is, const std::string& stream_name, const std::function<D(const raw_values_type&)>& read);

    // Opens the file and reads the document from it, prefixing every error with the file's path
    template<typename D>
    D read_document(const std::filesystem::path& filepath, const std::function<D(const raw_values_type&)>& read);

} // namespace quetzal::stratum_format::detail

//------------------------------------------------------------------------------
template<typename D>
D quetzal::stratum_format::detail::read_document(std::istream& is, const std::string& stream_name, const std::function<D(const raw_values_type&)>& read)
{
    raw_values_type v = parse_json(is, stream_name);

    try
    {
        return read(v);
    }
    catch (const std::runtime_error& e)
    {
        throw std::runtime_error(std::format("{}: {}", stream_name, e.what()));
    }
}

//------------------------------------------------------------------------------
template<typename D>
D quetzal::stratum_format::detail::read_document(const std::filesystem::path& filepath, const std::function<D(const raw_values_type&)>& read)
{
    std::ifstream is(filepath, std::ios::binary);
    if (!is)
    {
        throw std::runtime_error(std::format("{}: cannot be opened", filepath.string()));
    }

    return read_document(is, filepath.string(), read);
}

#endif // QUETZAL_STRATUM_FORMAT_CHECKED_VALUES_HPP
