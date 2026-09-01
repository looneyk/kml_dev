#if !defined(QUETZAL_MATH_TRAITS_HPP)
#define QUETZAL_MATH_TRAITS_HPP
//------------------------------------------------------------------------------
// math
// traits.hpp
//------------------------------------------------------------------------------

namespace quetzal::math
{

    template<typename Traits>
    concept Cartesian = (Traits::size == Traits::dimension);

    template<typename Traits>
    concept Homogeneous = (Traits::size == Traits::dimension + 1);

} // namespace quetzal::math

#endif // QUETZAL_MATH_TRAITS_HPP
