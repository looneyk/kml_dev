#if !defined(QUETZAL_GEOMETRY_POINT_UTIL_HPP)
#define QUETZAL_GEOMETRY_POINT_UTIL_HPP
//------------------------------------------------------------------------------
// geometry
// cartesian.hpp
//------------------------------------------------------------------------------

#include "Point.hpp"
#include "quetzal/math/CartesianTraits.hpp"
//#include "quetzal/math/floating_point.hpp"
//#include <concepts>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace quetzal::geometry
{

    template<typename Traits>
    using cartesian_traits = math::CartesianTraits<typename Traits::value_type, Traits::dimension>;

    template<typename Traits>
    Point<cartesian_traits<Traits>> to_cartesian(const Point<Traits>& p);

    template<typename Traits>
    std::string to_fixed_width(const Point<Traits>& point, int width = 24, int prec = std::numeric_limits<typename Traits::value_type>::max_digits10 - 1);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<quetzal::geometry::cartesian_traits<Traits>> quetzal::geometry::to_cartesian(const Point<Traits>& p)
{
    //if constexpr (std::same_as<Traits, cartesian_traits<Traits>>)
    if constexpr (Traits::size == Traits::dimension) // not as comprehensive ...
    {
        return p;
    }
    else
    {
        // projective_divide ...
        // prepare ...
        // but good that this assertion works ...
        //assert(p[Traits::dimension] == Traits::value_type(1));
//            assert(math::float_eq(p[Traits::dimension], Traits::value_type(1)));

        // This could be a metaprogrammed Traits operation. Should provide it for vector as well ...
        Point<cartesian_traits<Traits>> c;
        for (size_t i = 0; i < Traits::dimension; ++i)
        {
            c[i] = p[i];
        }

        return c;
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
std::string quetzal::geometry::to_fixed_width(const Point<Traits>& point, int width, int prec)
{
    std::ostringstream oss(std::ostringstream::binary);

    for (size_t i = 0; i < Traits::size; ++i)
    {
        std::ostringstream oss_temp;
        oss_temp << std::setprecision(prec) << point[i];
        auto n = oss_temp.str().size();
        oss << std::string(width - n, ' ') << oss_temp.str();
    }

    return oss.str();
}

#endif // QUETZAL_GEOMETRY_POINT_UTIL_HPP
