#if !defined(QUETZAL_STRATUM_SIM_DIRECTIONMASK_HPP)
#define QUETZAL_STRATUM_SIM_DIRECTIONMASK_HPP
//------------------------------------------------------------------------------
// stratum_sim
// DirectionMask.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <cstdint>
#include <utility>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Any of the four directions of a flat grid, a bit each in CardinalDirection's order
    enum class DirectionMask : uint8_t
    {
        None = 0,
        East = 1 << 0,
        North = 1 << 1,
        West = 1 << 2,
        South = 1 << 3
    };

    constexpr DirectionMask operator|(DirectionMask a, DirectionMask b);
    constexpr DirectionMask operator&(DirectionMask a, DirectionMask b);
    constexpr DirectionMask& operator|=(DirectionMask& a, DirectionMask b);
    constexpr bool any(DirectionMask mask);

    constexpr DirectionMask direction_mask(CardinalDirection direction);

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
constexpr quetzal::stratum_sim::DirectionMask quetzal::stratum_sim::operator|(DirectionMask a, DirectionMask b)
{
    return static_cast<DirectionMask>(std::to_underlying(a) | std::to_underlying(b));
}

//------------------------------------------------------------------------------
constexpr quetzal::stratum_sim::DirectionMask quetzal::stratum_sim::operator&(DirectionMask a, DirectionMask b)
{
    return static_cast<DirectionMask>(std::to_underlying(a) & std::to_underlying(b));
}

//------------------------------------------------------------------------------
constexpr quetzal::stratum_sim::DirectionMask& quetzal::stratum_sim::operator|=(DirectionMask& a, DirectionMask b)
{
    a = a | b;
    return a;
}

//------------------------------------------------------------------------------
constexpr bool quetzal::stratum_sim::any(DirectionMask mask)
{
    return mask != DirectionMask::None;
}

//------------------------------------------------------------------------------
constexpr quetzal::stratum_sim::DirectionMask quetzal::stratum_sim::direction_mask(CardinalDirection direction)
{
    return static_cast<DirectionMask>(1 << std::to_underlying(direction));
}

#endif // QUETZAL_STRATUM_SIM_DIRECTIONMASK_HPP
