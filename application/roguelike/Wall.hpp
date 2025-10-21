#if !defined(QUETZAL_ROGUELIKE_WALL_HPP)
#define QUETZAL_ROGUELIKE_WALL_HPP
//------------------------------------------------------------------------------
// Wall.hpp
//------------------------------------------------------------------------------

#include "Container.hpp"

namespace quetzal
{

namespace roguelike
{

    class Cell;

    //--------------------------------------------------------------------------
    class Wall : public Container
    {
    public:

        Wall();
        Wall(const Wall&) = default;
        ~Wall() = default;

        Wall& operator=(const Wall&) = default;

        Cell* cell() const;

        void set_cell(Cell* pcell);

    private:

        Cell* m_pcell;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_WALL_HPP
