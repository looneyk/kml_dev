#if !defined(QUETZAL_CATAGEN_TYPES_HPP)
#define QUETZAL_CATAGEN_TYPES_HPP
//------------------------------------------------------------------------------
// catagen
// types.hpp
//------------------------------------------------------------------------------

#include <compare>

namespace quetzal::catagen
{

    using coord_type = int;

    //--------------------------------------------------------------------------
    // Origin is lower left
    struct CellPosition
    {
        coord_type i = 0;
        coord_type j = 0;

        friend auto operator<=>(const CellPosition&, const CellPosition&) = default;
    };

    //--------------------------------------------------------------------------
    struct Extent
    {
        coord_type nx = 0;
        coord_type ny = 0;

        friend bool operator==(const Extent&, const Extent&) = default;
    };

    //--------------------------------------------------------------------------
    // A half open cell range
    struct Rectangle
    {
        CellPosition position;
        Extent extent;

        friend bool operator==(const Rectangle&, const Rectangle&) = default;
    };

    //--------------------------------------------------------------------------
    enum class CellType
    {
        Solid,
        Open,
        StairsDown,
        StairsUp
    };

    //--------------------------------------------------------------------------
    enum class WallType
    {
        Open,
        Solid,
        Door
    };

    //--------------------------------------------------------------------------
    enum class Direction
    {
        East,
        North,
        West,
        South
    };

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_TYPES_HPP
