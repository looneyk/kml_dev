#if !defined(QUETZAL_DUNGEN_ROOM_HPP)
#define QUETZAL_DUNGEN_ROOM_HPP
//------------------------------------------------------------------------------
// dungen
// Room.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <iosfwd>
#include <string>
#include <vector>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    class Room
    {
    public:

        using door_ids_type = std::vector<id_type>;

        Room(id_type id, const std::string& name, const box_type& rectangle);
        Room(const Room&) = default;
        ~Room() = default;

        Room& operator=(const Room&) = default;

        id_type id() const;

        std::string name() const;
        void set_name(const std::string& name);

        const box_type& rectangle() const;
        void set_rectangle(const box_type& rectangle);

        const door_ids_type& door_ids() const;
        door_ids_type& door_ids();

        void attach_door(id_type door_id);

        void print(std::ostream& os) const;

    private:

        id_type m_id;
        std::string m_name;
        box_type m_rectangle;
        door_ids_type m_door_ids; // not just ids, but also wall attachment, or should room/wall attachment be stored with door? or at all? ...
    };

    std::ostream& operator<<(std::ostream& os, const Room& room);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_ROOM_HPP
