#if !defined(QUETZAL_GEOMETRY_RANDOM_HPP)
#define QUETZAL_GEOMETRY_RANDOM_HPP
//------------------------------------------------------------------------------
// geometry
// random.hpp
//------------------------------------------------------------------------------

#include "quetzal/math/Random.hpp"
#include "quetzal/math/Vector.hpp"

namespace quetzal::geometry
{

    template<typename Traits>
    math::Vector<Traits> random_vector(typename Traits::value_type length, Random& random);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::geometry::random_vector(typename Traits::value_type length, Random& random)
{
    using T = typename Traits::value_type;

    if constexpr (Traits::dimension == 2)
    {
        T phi = math::PiTwo<T> * random.gen();
        return {length * cos(phi), length * sin(phi)};
    }
    else if constexpr (Traits::dimension == 3)
    {
        T phi = math::PiTwo<T> * random.gen();
        T theta = math::Pi<T> * (random.gen() - T(0.5));
        T r = length * cos(theta);
        return {r * cos(phi), r * sin(phi), length * sin(theta)};
    }
    else
    {
        static_assert(false, "Dimension not supported.");
    }
}

#endif // QUETZAL_GEOMETRY_RANDOM_HPP
