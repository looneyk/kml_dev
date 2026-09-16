#if !defined(QUETZAL_STRATUM_SIM_FLAT_HPP)
#define QUETZAL_STRATUM_SIM_FLAT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Flat.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <map>
#include <optional>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // A horizontal surface bounding the layout, one below and one above. The two differ by
    // role and nothing else, a stalactite and a stalagmite being one shape inverted
    // A height at the three levels a level file states it: across the whole flat, per room and per cell, each overriding
    // the one above. The ordinary flat is one height and two empty tables. The layout resolves a cell's height, the room
    // covering a cell being the layout's
    class Flat
    {
    public:

        using room_heights_type = std::map<id_type, value_type>;
        using cell_heights_type = std::map<discrete_point_type, value_type>;

        Flat() = default;
        explicit Flat(value_type height, bool ceiling = false);

        bool ceiling() const; // Role, floor otherwise
        void set_ceiling(bool b);

        value_type height() const;
        void set_height(value_type height);

        std::optional<value_type> room_height(id_type room_id) const;
        void set_room_height(id_type room_id, value_type height);

        std::optional<value_type> cell_height(const discrete_point_type& cell_position) const;
        void set_cell_height(const discrete_point_type& cell_position, value_type height);

    private:

        value_type m_height = value_type(0);
        bool m_ceiling = false;
        room_heights_type m_room_heights;
        cell_heights_type m_cell_heights;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_FLAT_HPP
