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
#include "types.hpp"
#include "quetzal/stratum_core/TransitEndpoint.hpp"
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
        using transit_endpoints_type = std::vector<stratum_core::TransitEndpoint>;

        Zone(id_type id, const std::string& name, discrete_vector2_type extent, CellType cell_type = CellType::Null);
        Zone(const Zone&) = default;
        ~Zone() = default;

        Zone& operator=(const Zone&) = default;

        id_type id() const;

        std::string name() const;
        void set_name(const std::string& name);

        discrete_vector2_type extent() const;

        const Cells& cells() const;
        Cells& cells();

        const Cell& cell(discrete_point2_type position) const;
        Cell& cell(discrete_point2_type position);

        const rooms_type& rooms() const;
        rooms_type& rooms();

        const Room& room(id_type id) const;
        Room& room(id_type id);

        void create_room(const discrete_box2_type& rectangle); // name? ...
        // The room's rectangle is the bounding box of the cells
        void create_room(const std::vector<discrete_point2_type>& cell_positions);
        void expand_room(id_type room_id, const discrete_box2_type& rectangle);

        const doors_type& doors() const;
        doors_type& doors();

        const Door& door(id_type id) const;
        Door& door(id_type id);

        void create_door(discrete_point2_type position); // name? facing ...

        const transit_endpoints_type& transit_endpoints() const;

        void create_staircase(const discrete_directed_point2_type& placement, stratum_core::Directionality directionality);

        void render(std::ostream& os) const;
        void print(std::ostream& os) const;

    private:

        id_type m_id;
        std::string m_name;
        Cells m_cells;
        rooms_type m_rooms;
        doors_type m_doors;
        transit_endpoints_type m_transit_endpoints;
    };

    std::ostream& operator<<(std::ostream& os, const Zone& zone);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_ZONE_HPP
