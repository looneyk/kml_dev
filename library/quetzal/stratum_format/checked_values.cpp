//------------------------------------------------------------------------------
// stratum_format
// checked_values.cpp
//------------------------------------------------------------------------------

#include "checked_values.hpp"
#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <istream>
#include <limits>

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::member_path(const std::string& path, std::string_view key)
{
    return path.empty() ? std::string(key) : std::format("{}.{}", path, key);
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::element_path(const std::string& path, size_t i)
{
    return std::format("{}[{}]", path, i);
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::detail::fail(const std::string& path, std::string_view what)
{
    throw std::runtime_error(path.empty()
        ? std::string(what)
        : std::format("{}: {}", path, what));
}

//------------------------------------------------------------------------------
const quetzal::stratum_format::raw_values_type* quetzal::stratum_format::detail::member(const raw_values_type& v, std::string_view key)
{
    auto i = v.find(key);
    return i != v.end() && !i->is_null() ? &*i : nullptr;
}

//------------------------------------------------------------------------------
const quetzal::stratum_format::raw_values_type& quetzal::stratum_format::detail::required_member(const raw_values_type& v, const std::string& path,
    std::string_view key)
{
    const raw_values_type* p = member(v, key);
    if (p == nullptr)
    {
        fail(member_path(path, key), "required and absent");
    }

    return *p;
}

//------------------------------------------------------------------------------
const quetzal::stratum_format::raw_values_type& quetzal::stratum_format::detail::object(const raw_values_type& v, const std::string& path)
{
    if (!v.is_object())
    {
        fail(path, "an object was expected");
    }

    return v;
}

//------------------------------------------------------------------------------
const quetzal::stratum_format::raw_values_type& quetzal::stratum_format::detail::array(const raw_values_type& v, const std::string& path)
{
    if (!v.is_array())
    {
        fail(path, "an array was expected");
    }

    return v;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::string_value(const raw_values_type& v, const std::string& path)
{
    if (!v.is_string())
    {
        fail(path, "a string was expected");
    }

    return v.get<std::string>();
}

//------------------------------------------------------------------------------
std::optional<std::string> quetzal::stratum_format::detail::optional_string(const raw_values_type& v, std::string_view key, const std::string& path)
{
    const raw_values_type* p = member(v, key);
    return p != nullptr ? std::make_optional(string_value(*p, member_path(path, key))) : std::nullopt;
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore quetzal::stratum_format::detail::text_values(const raw_values_type& v, const std::string& path)
{
    KeyValueStore values;
    for (const auto& [key, value] : object(v, path).items())
    {
        values.insert(key, string_value(value, member_path(path, key)));
    }

    return values;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::detail::check_keys(const raw_values_type& v, const std::string& path, std::span<const std::string_view> keys,
    std::string_view file_description)
{
    for (const auto& [key, value] : v.items())
    {
        if (std::ranges::find(keys, key) == keys.end())
        {
            fail(member_path(path, key), std::format("not a key of the {}", file_description));
        }
    }

    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_format::value_type quetzal::stratum_format::detail::number_value(const raw_values_type& v, const std::string& path)
{
    if (!v.is_number())
    {
        fail(path, "a number was expected");
    }

    return static_cast<value_type>(v.get<double>());
}

//------------------------------------------------------------------------------
bool quetzal::stratum_format::detail::boolean_value(const raw_values_type& v, const std::string& path)
{
    if (!v.is_boolean())
    {
        fail(path, "true or false was expected");
    }

    return v.get<bool>();
}

//------------------------------------------------------------------------------
quetzal::stratum_format::coord_type quetzal::stratum_format::detail::coord_value(const raw_values_type& v, const std::string& path)
{
    if (!v.is_number())
    {
        fail(path, "a number was expected");
    }

    double d = v.get<double>();
    if (d < std::numeric_limits<coord_type>::min() || d > std::numeric_limits<coord_type>::max())
    {
        fail(path, "outside the coordinate range");
    }

    if (d != static_cast<double>(static_cast<coord_type>(d)))
    {
        fail(path, "a whole number was expected");
    }

    return static_cast<coord_type>(d);
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_format::detail::count_value(const raw_values_type& v, const std::string& path)
{
    coord_type n = coord_value(v, path);
    if (n < 0)
    {
        fail(path, "a count cannot be negative");
    }

    return static_cast<size_t>(n);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::stored_entity_id_type quetzal::stratum_format::detail::id_value(const raw_values_type& v, const std::string& path)
{
    if (!v.is_number_unsigned())
    {
        fail(path, "a whole, non-negative number was expected");
    }

    return v.get<stored_entity_id_type>();
}

//------------------------------------------------------------------------------
quetzal::stratum_format::seed_type quetzal::stratum_format::detail::seed_value(const raw_values_type& v, const std::string& path)
{
    const std::string text = string_value(v, path);
    const char* end = text.data() + text.size();

    seed_type seed = 0;
    if (auto [last, ec] = std::from_chars(text.data(), end, seed); ec != std::errc() || last != end)
    {
        fail(path, "a seed was expected");
    }

    return seed;
}

//------------------------------------------------------------------------------
const quetzal::stratum_format::raw_values_type& quetzal::stratum_format::detail::fixed_array(const raw_values_type& v, const std::string& path, size_t n)
{
    const raw_values_type& a = array(v, path);
    if (a.size() != n)
    {
        fail(path, std::format("{} numbers were expected, and there are {}", n, a.size()));
    }

    return a;
}

//------------------------------------------------------------------------------
quetzal::stratum_format::discrete_point2_type quetzal::stratum_format::detail::read_cell_position(const raw_values_type& v, const std::string& path)
{
    const raw_values_type& a = fixed_array(v, path, 2);
    return {coord_value(a[0], element_path(path, 0)), coord_value(a[1], element_path(path, 1))};
}

//------------------------------------------------------------------------------
quetzal::stratum_format::point_type quetzal::stratum_format::detail::read_position(const raw_values_type& v, const std::string& path)
{
    const raw_values_type& a = fixed_array(v, path, 3);
    return {number_value(a[0], element_path(path, 0)),
        number_value(a[1], element_path(path, 1)),
        number_value(a[2], element_path(path, 2))};
}

//------------------------------------------------------------------------------
quetzal::stratum_format::surface_point_type quetzal::stratum_format::detail::read_surface_position(const raw_values_type& v, const std::string& path)
{
    const raw_values_type& a = fixed_array(v, path, 2);
    return {number_value(a[0], element_path(path, 0)), number_value(a[1], element_path(path, 1))};
}

//------------------------------------------------------------------------------
quetzal::stratum_format::discrete_vector2_type quetzal::stratum_format::detail::read_direction(const raw_values_type& v, const std::string& path)
{
    const raw_values_type& a = fixed_array(v, path, 2);
    discrete_vector2_type direction{coord_value(a[0], element_path(path, 0)), coord_value(a[1], element_path(path, 1))};

    if (std::abs(direction.x()) + std::abs(direction.y()) != 1)
    {
        fail(path, "not one of the four directions a cell's sides define");
    }

    return direction;
}

//------------------------------------------------------------------------------
quetzal::stratum_format::WallPlacement quetzal::stratum_format::detail::read_wall_placement(const raw_values_type& v, const std::string& path)
{
    object(v, path);
    return {read_cell_position(required_member(v, path, "cell_position"), member_path(path, "cell_position")),
        read_direction(required_member(v, path, "normal"), member_path(path, "normal"))};
}

//------------------------------------------------------------------------------
quetzal::stratum_format::raw_values_type quetzal::stratum_format::detail::read_components(const raw_values_type& v, const std::string& path)
{
    const raw_values_type* p = member(v, "components");
    return p != nullptr ? object(*p, member_path(path, "components")) : raw_values_type();
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::detail::check_file_header(const raw_values_type& v, std::string_view required_format, std::string_view required_version)
{
    object(v, "");

    std::string format = string_value(required_member(v, "", "format"), "format");
    if (format != required_format)
    {
        fail("format", std::format("'{}' is not this format", format));
    }

    std::string version = string_value(required_member(v, "", "version"), "version");
    if (version != required_version)
    {
        fail("version", std::format("'{}' is not a version this reads", version));
    }

    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_format::raw_values_type quetzal::stratum_format::detail::parse_json(std::istream& is, const std::string& stream_name)
{
    try
    {
        return raw_values_type::parse(is);
    }
    catch (const raw_values_type::parse_error& e)
    {
        throw std::runtime_error(std::format("{}: {}", stream_name, e.what()));
    }
}
