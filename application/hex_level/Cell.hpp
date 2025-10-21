#if !defined(QUETZAL_HEX_LEVEL_CELL_HPP)
#define QUETZAL_HEX_LEVEL_CELL_HPP
//------------------------------------------------------------------------------
// hex_level
// Cell.hpp
//------------------------------------------------------------------------------

#include "Position.hpp"
#include "Size.hpp"
#include "Wall.hpp"
#include <vector>

namespace quetzal
{

namespace hex_level
{

    class Level;

    //--------------------------------------------------------------------------
    class Cell
    {
    public:

        typedef std::vector<Wall> walls_type;

        Cell();
        Cell(const Cell&) = default;
        ~Cell() = default;

        Cell& operator=(const Cell&) = default;

        Level* level() const;
        Position position() const;
        const walls_type& walls() const;

        void set_level(Level* plevel);
        void set_position(float x, float y, float z);
        walls_type& walls();

    private:

        Level* m_plevel;
        Position m_position;
        std::vector<Wall> m_walls;
    };

} // namespace hex_level

} // namespace quetzal

#endif // QUETZAL_HEX_LEVEL_CELL_HPP
