#if !defined(QUETZAL_DUNGEN_DOOR_HPP)
#define QUETZAL_DUNGEN_DOOR_HPP
//------------------------------------------------------------------------------
// dungen
// Door.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <iosfwd>
#include <string>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    class Door
    {
    public:

        Door(id_type id, const std::string& name, point_type position);
        Door(const Door&) = default;
        ~Door() = default;

        Door& operator=(const Door&) = default;

        id_type id() const;

        std::string name() const;
        void set_name(const std::string& name);

        point_type position() const;
        void set_position(point_type position);

        void print(std::ostream& os) const;

    private:

        id_type m_id;
        std::string m_name;
        point_type m_position;

        id_type m_room_id_outer = nullid;
        id_type m_room_id_inner = nullid;
    };

    std::ostream& operator<<(std::ostream& os, const Door& door);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_DOOR_HPP
