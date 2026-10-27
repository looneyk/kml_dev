#if !defined(QUETZAL_GEOMETRY_LOCUS_HPP)
#define QUETZAL_GEOMETRY_LOCUS_HPP
//------------------------------------------------------------------------------
// geometry
// Locus.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <format>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    enum class Locus
    {
        Empty,
        Point,
        Line,
        Ray,
        Segment,
        Polyline,
        Plane,
        Circle,
        Polygon,
        Surface,
        Sphere,
        Polyhedron
    };

    std::string to_string(const Locus& locus);

    std::optional<Locus> to_locus(std::string_view name);

    std::istream& operator>>(std::istream& is, Locus& locus);

    std::ostream& operator<<(std::ostream& os, const Locus& locus);

    std::from_chars_result from_chars(const char* first, const char* last, Locus& locus);

    std::to_chars_result to_chars(char* first, char* last, const Locus& locus);

    namespace detail
    {

        inline constexpr std::array<std::string_view, 12> locus_names {"Empty", "Point", "Line", "Ray", "Segment", "Polyline", "Plane",
            "Circle", "Polygon", "Surface", "Sphere", "Polyhedron"};

        static_assert(locus_names.size() == static_cast<size_t>(Locus::Polyhedron) + 1);

        constexpr std::string_view locus_name(Locus locus)
        {
            const auto slot = static_cast<size_t>(locus);
            assert(slot < locus_names.size());
            return locus_names[slot];
        }

    } // namespace detail

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
inline std::string quetzal::geometry::to_string(const Locus& locus)
{
    return std::string(detail::locus_name(locus));
}

//------------------------------------------------------------------------------
inline std::optional<quetzal::geometry::Locus> quetzal::geometry::to_locus(std::string_view name)
{
    const auto i = std::ranges::find(detail::locus_names, name);
    if (i == detail::locus_names.end())
    {
        return std::nullopt;
    }

    return static_cast<Locus>(i - detail::locus_names.begin());
}

//------------------------------------------------------------------------------
inline std::istream& quetzal::geometry::operator>>(std::istream& is, Locus& locus)
{
    std::istream::sentry sentry(is);
    if (!sentry)
    {
        return is;
    }

    std::string name;
    for (std::istream::int_type c = is.peek(); std::isalpha(c); c = is.peek())
    {
        name += static_cast<char>(is.get());
    }

    std::optional<Locus> named = to_locus(name);
    if (!named)
    {
        is.setstate(std::ios::failbit);
        return is;
    }

    locus = *named;
    return is;
}

//------------------------------------------------------------------------------
inline std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Locus& locus)
{
    os << detail::locus_name(locus);
    return os;
}

//------------------------------------------------------------------------------
inline std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, Locus& locus)
{
    const char* name_first = quetzal::skip_whitespace(first, last);
    const char* name_last = std::find_if_not(name_first, last, [](unsigned char c) { return std::isalpha(c); });

    std::optional<Locus> named = to_locus(std::string_view(name_first, name_last));
    if (!named)
    {
        return {first, std::errc::invalid_argument};
    }

    locus = *named;
    return {name_last, std::errc {}};
}

//------------------------------------------------------------------------------
inline std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const Locus& locus)
{
    const std::string_view name = detail::locus_name(locus);
    if (static_cast<size_t>(last - first) < name.size())
    {
        return {last, std::errc::value_too_large};
    }

    return {std::ranges::copy(name, first).out, std::errc {}};
}

//------------------------------------------------------------------------------
template <>
struct std::formatter<quetzal::geometry::Locus> : std::formatter<std::string_view>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::Locus& locus, FormatContext& ctx) const
    {
        return std::formatter<std::string_view>::format(quetzal::geometry::detail::locus_name(locus), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_LOCUS_HPP
