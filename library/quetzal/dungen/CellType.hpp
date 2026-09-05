#if !defined(QUETZAL_DUNGEN_CELLTYPE_HPP)
#define QUETZAL_DUNGEN_CELLTYPE_HPP
//------------------------------------------------------------------------------
// dungen
// CellType.hpp
//------------------------------------------------------------------------------

#include <iosfwd>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    enum class CellType
    {
        Null,
        Solid,
        Room,
        Corridor,
        Door,
        Stair
    };

    std::ostream& operator<<(std::ostream& os, const CellType& type);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_CELLTYPE_HPP
