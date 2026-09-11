#if !defined(QUETZAL_DUNGEN_LEVEL_HPP)
#define QUETZAL_DUNGEN_LEVEL_HPP
//------------------------------------------------------------------------------
// dungen
// Level.hpp
//------------------------------------------------------------------------------

#include "Cell.hpp"
#include "Cells.hpp"
#include "CellType.hpp"
#include "Room.hpp"
#include "Door.hpp"
#include "Stair.hpp"
#include "types.hpp"
#include <array>
#include <iosfwd>
#include <string>
#include <vector>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    class Level
    {
    public:

        using rooms_type = std::vector<Room>;
        using doors_type = std::vector<Door>;
        using stairs_type = std::vector<Stair>;

        Level(id_type id, const std::string& name, vector_type size, CellType cell_type = CellType::Null);
        Level(const Level&) = default;
        ~Level() = default;

        Level& operator=(const Level&) = default;

        id_type id() const;

        std::string name() const;
        void set_name(const std::string& name);

        vector_type size() const;

        const Cells& cells() const;
        Cells& cells();

        const Cell& cell(point_type position) const;
        Cell& cell(point_type position);

        const rooms_type& rooms() const;
        rooms_type& rooms();

        const Room& room(id_type id) const;
        Room& room(id_type id);

        void create_room(const box_type& rectangle); // name? ...
        void expand_room(id_type room_id, const box_type& rectangle);

        const doors_type& doors() const;
        doors_type& doors();

        const Door& door(id_type id) const;
        Door& door(id_type id);

        void create_door(point_type position); // name? facing ...

        const stairs_type& stairs() const;
        stairs_type& stairs();

        const Stair& stair(id_type id) const;
        Stair& stair(id_type id);

        void create_stair(point_type position); // name? direction ...

        void render(std::ostream& os) const;
        void print(std::ostream& os) const;

    private:

        id_type m_id;
        std::string m_name;
        Cells m_cells;
        rooms_type m_rooms;
        doors_type m_doors;
        stairs_type m_stairs;
    };

    std::ostream& operator<<(std::ostream& os, const Level& level);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_LEVEL_HPP
