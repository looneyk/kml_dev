#if !defined(QUETZAL_ROGUELIKE_CELL_HPP)
#define QUETZAL_ROGUELIKE_CELL_HPP
//------------------------------------------------------------------------------
// roguelike
// Cell.hpp
//------------------------------------------------------------------------------

#include "Container.hpp"
#include "Wall.hpp"
#include <vector>
#include <memory>

namespace quetzal
{

namespace roguelike
{

    class Level;

    //--------------------------------------------------------------------------
    class Cell : public Container
    {
    public:

        typedef std::vector<std::shared_ptr<Wall>> walls_type;

        Cell();
        Cell(const Cell&) = default;
        ~Cell() = default;

        Cell& operator=(const Cell&) = default;

        Level* level() const;
        const walls_type& walls() const;

        void set_level(Level* plevel);
        walls_type& walls();

    private:

        Level* m_plevel;
        std::vector<std::shared_ptr<Wall>> m_walls;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_CELL_HPP
