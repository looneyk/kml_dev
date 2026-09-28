#if !defined(QUETZAL_DUNGEN_ZONE_HPP)
#define QUETZAL_DUNGEN_ZONE_HPP
//------------------------------------------------------------------------------
// dungen
// Zone.hpp
//------------------------------------------------------------------------------

#include "Cell.hpp"
#include "Cells.hpp"
#include "CellType.hpp"
#include "Room.hpp"
#include "Door.hpp"
#include "Staircase.hpp"
#include "types.hpp"
#include <array>
#include <iosfwd>
#include <string>
#include <vector>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    class Zone
    {
    public:

        using rooms_type = std::vector<Room>;
        using doors_type = std::vector<Door>;
        using staircases_type = std::vector<Staircase>;

        Zone(id_type id, const std::string& name, vector_type extent, CellType cell_type = CellType::Null);
        Zone(const Zone&) = default;
        ~Zone() = default;

        Zone& operator=(const Zone&) = default;

        id_type id() const;

        std::string name() const;
        void set_name(const std::string& name);

        vector_type extent() const;

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

        const staircases_type& staircases() const;
        staircases_type& staircases();

        const Staircase& staircase(id_type id) const;
        Staircase& staircase(id_type id);

        void create_staircase(const std::string& name, const directed_point_type& directed_position, Rise rise);

        void render(std::ostream& os) const;
        void print(std::ostream& os) const;

    private:

        id_type m_id;
        std::string m_name;
        Cells m_cells;
        rooms_type m_rooms;
        doors_type m_doors;
        staircases_type m_staircases;
    };

    std::ostream& operator<<(std::ostream& os, const Zone& zone);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_ZONE_HPP
