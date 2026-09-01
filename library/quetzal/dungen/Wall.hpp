#if !defined(QUETZAL_DUNGEN_WALL_HPP)
#define QUETZAL_DUNGEN_WALL_HPP
//------------------------------------------------------------------------------
// dungen
// Wall.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <iosfwd>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    class Wall
    {
    public:

        Wall();
        Wall(const Wall&) = default;
        Wall(Wall&&) = default;
        ~Wall() = default;

        Wall& operator=(const Wall&) = default;
        Wall& operator=(Wall&&) = default;

        id_type cell_id() const;
        void set_cell_id(id_type cell_id);

//        const Properties& properties() const;
//        Properties& properties();

        void print(std::ostream& os) const;

    private:

        id_type m_cell_id;
/*
        decorations
        contents
        interactions
*/
    };

    std::ostream& operator<<(std::ostream& os, const Wall& wall);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_WALL_HPP
