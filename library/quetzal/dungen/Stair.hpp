#if !defined(QUETZAL_DUNGEN_STAIR_HPP)
#define QUETZAL_DUNGEN_STAIR_HPP
//------------------------------------------------------------------------------
// dungen
// Stair.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <iosfwd>
#include <string>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    class Stair
    {
    public:

        Stair(id_type id, const std::string& name, point_type position);
        Stair(const Stair&) = default;
        ~Stair() = default;

        Stair& operator=(const Stair&) = default;

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
    };

    std::ostream& operator<<(std::ostream& os, const Stair& stair);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_STAIR_HPP
