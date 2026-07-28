#if !defined(QUETZAL_GEOMETRY_LOCUS_HPP)
#define QUETZAL_GEOMETRY_LOCUS_HPP
//------------------------------------------------------------------------------
// geometry
// Locus.hpp
//------------------------------------------------------------------------------

#include <format>
#include <iosfwd>
#include <string>
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

    std::ostream& operator<<(std::ostream& os, const Locus& locus);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
std::string quetzal::geometry::to_string(const Locus& locus)
{
    switch (locus)
    {
        case Locus::Empty:
            return "Empty";
        case Locus::Point:
            return "Point";
        case Locus::Line:
            return "Line";
        case Locus::Ray:
            return "Ray";
        case Locus::Segment:
            return "Segment";
        case Locus::Polyline:
            return "Polyline";
        case Locus::Plane:
            return "Plane";
        case Locus::Circle:
            return "Circle";
        case Locus::Polygon:
            return "Polygon";
        case Locus::Surface:
            return "Surface";
        case Locus::Sphere:
            return "Sphere";
        case Locus::Polyhedron:
            return "Polyhedron";
        default:
            assert(false && "Invalid Locus value");
            return "";
    }
}

//------------------------------------------------------------------------------
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Locus& locus)
{
    os << to_string(locus);
    return os;
}

//------------------------------------------------------------------------------
template <>
struct std::formatter<quetzal::geometry::Locus> : std::formatter<std::string>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::Locus& locus, FormatContext& ctx) const
    {
        return std::formatter<std::string>::format(to_string(locus), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_LOCUS_HPP
