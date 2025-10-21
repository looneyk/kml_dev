#if !defined(QUETZAL_HEX_LEVEL_LEVEL_HPP)
#define QUETZAL_HEX_LEVEL_LEVEL_HPP
//------------------------------------------------------------------------------
// hex_level
// Level.hpp
//------------------------------------------------------------------------------

#include "Position.hpp"
#include "Size.hpp"
#include "Cell.hpp"
#include <vector>

namespace quetzal
{

namespace hex_level
{

    //--------------------------------------------------------------------------
    class Level
    {
    public:

        typedef std::vector<Cell> cells_type;

        Level();
        Level(float x, float y, float z, float dx, float dy, float dz);
        Level(const Level&) = default;
        ~Level() = default;

        Level& operator=(const Level&) = default;

        Position position() const;
        Size size() const;
        const cells_type& cells() const;

        void set_position(float x, float y, float z);
        void set_size(float dx, float dy, float dz);
        cells_type& cells();

        Cell& cell(int x, int y);

    private:

        Position m_position;
        Size m_size;
        cells_type m_cells;
    };

} // namespace hex_level

} // namespace quetzal

#endif // QUETZAL_HEX_LEVEL_LEVEL_HPP
