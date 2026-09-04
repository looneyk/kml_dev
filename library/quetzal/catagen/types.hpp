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
    // A cell coordinate within a Level's extent, cell (0, 0) being its lower left
    struct CellPosition
    {
        coord_type i = 0;
        coord_type j = 0;

        friend auto operator<=>(const CellPosition&, const CellPosition&) = default;
    };

    //--------------------------------------------------------------------------
    // Cell counts, not coordinates
    struct Extent
    {
        coord_type nx = 0;
        coord_type ny = 0;

        friend bool operator==(const Extent&, const Extent&) = default;
    };

    //--------------------------------------------------------------------------
    // A half open cell range, position to position plus extent exclusive
    struct Rectangle
    {
        CellPosition position;
        Extent extent;

        friend bool operator==(const Rectangle&, const Rectangle&) = default;
    };

    //--------------------------------------------------------------------------
    // What occupies a cell. Rock is Solid; everything else is floor, the stairs
    // differing from open floor only in what stands on them
    enum class CellType
    {
        Solid,
        Open,
        StairsDown,
        StairsUp
    };

    //--------------------------------------------------------------------------
    // What stands on the edge between two cells. Solid stops a mover, Door does not, and
    // Open is the absence of a wall rather than a wall that happens to be passable
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
