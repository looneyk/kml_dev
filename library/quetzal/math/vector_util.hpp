#if !defined(QUETZAL_MATH_VECTOR_UTIL_HPP)
#define QUETZAL_MATH_VECTOR_UTIL_HPP
//------------------------------------------------------------------------------
// math
// vector_util.hpp
//------------------------------------------------------------------------------

#include "CartesianTraits.hpp"
#include "Vector.hpp"
#include <concepts>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace quetzal::math
{

    template<typename Traits>
    using cartesian_traits = math::CartesianTraits<typename Traits::value_type, Traits::dimension>;

    template<typename Traits>
    Vector<cartesian_traits<Traits>> to_cartesian(const Vector<Traits>& v);

    template<typename Traits>
    std::string to_fixed_width(const Vector<Traits>& v, int width = 24, int prec = std::numeric_limits<typename Traits::value_type>::max_digits10 - 1);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<quetzal::math::cartesian_traits<Traits>> quetzal::math::to_cartesian(const Vector<Traits>& v)
{
    if constexpr (std::same_as<Traits, cartesian_traits<Traits>>)
    {
        return v;
    }
    else
    {
        assert(v[Traits::dimension] == Traits::value_type(0));

        Vector<cartesian_traits<Traits>> c;
        for (size_t i = 0; i < Traits::dimension; ++i)
        {
            c[i] = v[i];
        }

        return c;
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
std::string quetzal::math::to_fixed_width(const Vector<Traits>& v, int width, int prec)
{
    std::ostringstream oss(std::ostringstream::binary);

    for (size_t i = 0; i < Traits::size; ++i)
    {
        std::ostringstream oss_temp;
        oss_temp << std::setprecision(prec) << v[i];
        auto n = oss_temp.str().size();
        oss << std::string(width - n, ' ') << oss_temp.str();
    }

    return oss.str();
}

#endif // QUETZAL_MATH_VECTOR_UTIL_HPP
