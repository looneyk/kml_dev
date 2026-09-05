#if !defined(QUETZAL_DUNGEN_CELL_HPP)
#define QUETZAL_DUNGEN_CELL_HPP
//------------------------------------------------------------------------------
// dungen
// Cell.hpp
//------------------------------------------------------------------------------

#include "CellType.hpp"
#include "CellTypeProperties.hpp"
#include "Wall.hpp"
#include "types.hpp"
#include <array>
#include <iosfwd>

namespace quetzal::dungen
{

    class CellTypeProperties;

    //--------------------------------------------------------------------------
    class Cell
    {
    public:

        Cell(CellType type = CellType::Null, id_type room_id = nullid);
        Cell(const Cell&) = default;
        ~Cell() = default;

        Cell& operator=(const Cell&) = default;

        CellType type() const;
        void set_type(CellType type);

        const CellTypeProperties& properties() const;
        CellTypeProperties& properties();

        id_type room_id() const;
        void set_room_id(id_type room_id);

        const Wall& wall(size_t i) const;
        Wall& wall(size_t i);
        void set_wall(const Wall& wall);

        void render(std::ostream& os) const;

        void print(std::ostream& os) const;

    private:

        CellType m_type;
        id_type m_room_id;
        std::array<Wall, 4> m_walls;
/*
        room, corridor, door, stair

        material/hardness/...
        feature (door type, stair type (up/down, stairs/ramp), ...)
        decorations
        (illuminated)
        marked/visited
        contents
*/
    };

    std::ostream& operator<<(std::ostream& os, const Cell& cell);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_CELL_HPP
