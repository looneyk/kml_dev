#if !defined(QUETZAL_ROGUELIKE_DUNGEON_HPP)
#define QUETZAL_ROGUELIKE_DUNGEON_HPP
//------------------------------------------------------------------------------
// roguelike
// Dungeon.hpp
//------------------------------------------------------------------------------

#include "DiscreteSize.hpp"
#include "Level.hpp"
#include <vector>

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class Dungeon
    {
    public:

        Dungeon();
        Dungeon(const Dungeon&) = default;
        ~Dungeon() = default;

        Dungeon& operator=(const Dungeon&) = default;

        DiscreteSize size() const;
        const std::vector<Level>& levels() const;

        std::vector<Level>& levels();
        Level& level(int z);

    private:

        std::vector<Level> m_levels;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_DUNGEON_HPP
