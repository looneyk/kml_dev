#if !defined(QUETZAL_GEOMETRY_CARDINALDIRECTION_HPP)
#define QUETZAL_GEOMETRY_CARDINALDIRECTION_HPP
//------------------------------------------------------------------------------
// geometry
// CardinalDirection.hpp
//------------------------------------------------------------------------------

#include "quetzal/math/DiscreteVector.hpp"
#include <array>
#include <concepts>
#include <utility>

namespace quetzal::geometry
{

    // Cardinal directions
    // Unit vectors along the 4 axes in conventional CCW order

    //--------------------------------------------------------------------------
    enum class CardinalDirection
    {
        East,
        North,
        West,
        South
    };

    template<std::signed_integral T, size_t N = 2>
    using cardinal_direction_type = math::DiscreteVector<T, N>;

    template<std::signed_integral T, size_t N = 2>
    using cardinal_directions_type = std::array<cardinal_direction_type<T, N>, 4>;

    template<std::signed_integral T, size_t N = 2>
    const cardinal_directions_type<T, N> cardinal_directions
    {
        cardinal_direction_type<T, N>{1, 0},   // East
        cardinal_direction_type<T, N>{0, 1},   // North
        cardinal_direction_type<T, N>{-1, 0},  // West
        cardinal_direction_type<T, N>{0, -1}   // South
    };

    template<std::signed_integral T, size_t N = 2>
    const cardinal_direction_type<T, N>& cardinal_direction(CardinalDirection direction);

    CardinalDirection opposite(CardinalDirection direction);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N>
const quetzal::geometry::cardinal_direction_type<T, N>& quetzal::geometry::cardinal_direction(CardinalDirection direction)
{
    return cardinal_directions<T, N>[std::to_underlying(direction)];
}

//------------------------------------------------------------------------------
inline quetzal::geometry::CardinalDirection quetzal::geometry::opposite(CardinalDirection direction)
{
    return static_cast<CardinalDirection>((std::to_underlying(direction) + 2) % 4);
}

#endif // QUETZAL_GEOMETRY_CARDINALDIRECTION_HPP
