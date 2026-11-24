//------------------------------------------------------------------------------
// stratum_core
// ZoneExtendedName.cpp
//------------------------------------------------------------------------------

#include "ZoneExtendedName.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>

namespace
{

    constexpr std::string_view relative_zone_prefix = "../[";
    constexpr std::string_view relative_zone_suffix = "]";

    //--------------------------------------------------------------------------
    bool digits(std::string_view text)
    {
        return !text.empty() && std::ranges::all_of(text, [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; });
    }

    //--------------------------------------------------------------------------
    void remove_sign(std::string_view& text)
    {
        if (text.starts_with('+') || text.starts_with('-'))
        {
            text.remove_prefix(1);
        }

        return;
    }

    //--------------------------------------------------------------------------
    template<typename T>
    std::optional<T> integer_value(std::string_view text)
    {
        std::string_view magnitude = text;
        remove_sign(magnitude);

        if (!digits(magnitude))
        {
            return std::nullopt;
        }

        // from_chars accepts a leading - but not a leading +
        const std::string_view number = text.starts_with('+') ? text.substr(1) : text;

        T value{};
        const auto [end, error] = std::from_chars(number.data(), number.data() + number.size(), value);
        return error == std::errc{} && end == number.data() + number.size() ? std::optional<T>(value) : std::nullopt;
    }

    //--------------------------------------------------------------------------
    // Tests for a string representing a number
    bool coordinate(std::string_view text)
    {
        remove_sign(text);
        const size_t point = text.find('.');

        return point == std::string_view::npos ? digits(text) : digits(text.substr(0, point)) && digits(text.substr(point + 1));
    }

    //--------------------------------------------------------------------------
    // Tests for a string representing a point: (coordinate,coordinate)
    bool position(std::string_view text)
    {
        if (!text.starts_with('(') || !text.ends_with(')'))
        {
            return false;
        }

        text = text.substr(1, text.size() - 2);
        const size_t comma = text.find(',');

        return comma != std::string_view::npos && coordinate(text.substr(0, comma)) && coordinate(text.substr(comma + 1));
    }

    //--------------------------------------------------------------------------
    // Test for a segment containing a zone offset: [k] or [=n]
    bool zone_offset_segment(std::string_view segment)
    {
        if (!segment.starts_with('[') || !segment.ends_with(']'))
        {
            return false;
        }

        std::string_view level = segment.substr(1, segment.size() - 2);
        if (level.starts_with('='))
        {
            level.remove_prefix(1);
        }

        return integer_value<std::int32_t>(level).has_value();
    }

    //--------------------------------------------------------------------------
    // Test for a segment containing a persistent id
    bool persistent_id_segment(std::string_view segment)
    {
        return segment.starts_with('#') && digits(segment.substr(1)) && integer_value<std::uint64_t>(segment.substr(1)).has_value();
    }

    //--------------------------------------------------------------------------
    // Validate the segments individually
    bool valid_segments(std::string_view segments, bool absolute)
    {
        bool valid = true;
        bool last = false;

        for (size_t slot = 0; valid && !last; ++slot)
        {
            const size_t separator = segments.find('/');
            const std::string_view segment = segments.substr(0, separator);
            last = separator == std::string_view::npos;

            valid = segment == ".." || quetzal::stratum_core::valid_segment_name(segment)
                || (slot == 0 && !absolute && persistent_id_segment(segment)) || (last && zone_offset_segment(segment));

            segments.remove_prefix(last ? segments.size() : separator + 1);
        }

        return valid;
    }

} // namespace

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_core::ZoneExtendedName> quetzal::stratum_core::parse_zone_extended_name(std::string_view text)
{
    std::string_view segments = text;

    if (const size_t at = segments.find('@'); at != std::string_view::npos)
    {
        if (!position(segments.substr(at + 1)))
        {
            return std::nullopt;
        }

        segments = segments.substr(0, at);
    }

    const bool absolute = segments.starts_with('/');
    if (absolute)
    {
        segments.remove_prefix(1);
    }

    return valid_segments(segments, absolute) ? std::optional<ZoneExtendedName>(ZoneExtendedName{std::string(text)}) : std::nullopt;
}

//------------------------------------------------------------------------------
quetzal::stratum_core::ZoneExtendedName quetzal::stratum_core::relative_zone_extended_name(std::int32_t k)
{
    return ZoneExtendedName{std::format("{}{:+}{}", relative_zone_prefix, k, relative_zone_suffix)};
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_core::discrete_vector3_type> quetzal::stratum_core::zone_offset(std::string_view extended_name)
{
    if (!extended_name.starts_with(relative_zone_prefix) || !extended_name.ends_with(relative_zone_suffix))
    {
        return std::nullopt;
    }

    const std::optional<coord_type> z = integer_value<coord_type>(extended_name.substr(relative_zone_prefix.size(),
        extended_name.size() - relative_zone_prefix.size() - relative_zone_suffix.size()));

    return z.has_value() ? std::optional<discrete_vector3_type>(discrete_vector3_type{0, 0, *z}) : std::nullopt;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_core::valid_segment_name(std::string_view text)
{
    return !text.empty() && (std::isalpha(static_cast<unsigned char>(text.front())) || text.front() == '_')
        && std::ranges::all_of(text, [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; });
}
