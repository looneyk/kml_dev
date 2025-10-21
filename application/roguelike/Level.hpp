#if !defined(QUETZAL_ROGUELIKE_LEVEL_HPP)
#define QUETZAL_ROGUELIKE_LEVEL_HPP
//------------------------------------------------------------------------------
// roguelike
// Level.hpp
//------------------------------------------------------------------------------

#include "DiscretePosition.hpp"
#include "DiscreteSize.hpp"
#include "Cell.hpp"
#include <vector>

namespace quetzal
{

namespace roguelike
{

    class Dungeon;

    //--------------------------------------------------------------------------
    class Level
    {
    public:

        typedef std::vector<Cell> cells_type;

        Level();
        Level(Dungeon* pdungeon, int z, int dx, int dy);
        Level(const Level&) = default;
        ~Level() = default;

        Level& operator=(const Level&) = default;

        Dungeon* dungeon() const;
        DiscretePosition position() const;
        DiscreteSize size() const;
        const cells_type& cells() const;

        void set_dungeon(Dungeon* pdungeon);
        void set_position(int x, int y, int z);
        void set_size(int dx, int dy);
        cells_type& cells();
        Cell& cell(int x, int y);

    private:

        Dungeon* m_pdungeon;
        DiscretePosition m_position;
        DiscreteSize m_size;
        cells_type m_cells;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_LEVEL_HPP
