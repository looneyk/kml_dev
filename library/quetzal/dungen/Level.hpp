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
#include "Shaft.hpp"
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
        using shafts_type = std::vector<Shaft>;

        Level(id_type id, const std::string& name, dimensions_type size, CellType cell_type = CellType::Null);
        Level(const Level&) = default;
        ~Level() = default;

        Level& operator=(const Level&) = default;

        id_type id() const;

        std::string name() const;
        void set_name(const std::string& name);

        dimensions_type size() const;

        const Cells& cells() const;
        Cells& cells();

        const Cell& cell(position_type position) const;
        Cell& cell(position_type position);

        const Cell& cell(coord_type x, coord_type y) const;
        Cell& cell(coord_type x, coord_type y);

        const rooms_type& rooms() const;
        rooms_type& rooms();

        const Room& room(id_type id) const;
        Room& room(id_type id);

        void create_room(position_type position, dimensions_type size); // name? ...
        void expand_room(id_type room_id, position_type position, dimensions_type size);

        const doors_type& doors() const;
        doors_type& doors();

        const Door& door(id_type id) const;
        Door& door(id_type id);

        void create_door(position_type position); // name? facing ...

        const shafts_type& shafts() const;
        shafts_type& shafts();

        const Shaft& shaft(id_type id) const;
        Shaft& shaft(id_type id);

        void create_shaft(position_type position); // name? direction ...

        void render(std::ostream& os) const;
        void print(std::ostream& os) const;

    private:

        id_type m_id;
        std::string m_name;
        dimensions_type m_size;
        Cells m_cells;
        rooms_type m_rooms;
        doors_type m_doors;
        shafts_type m_shafts;
    };

    std::ostream& operator<<(std::ostream& os, const Level& level);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_LEVEL_HPP
