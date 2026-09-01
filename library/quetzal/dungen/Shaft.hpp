#if !defined(QUETZAL_DUNGEN_SHAFT_HPP)
#define QUETZAL_DUNGEN_SHAFT_HPP
//------------------------------------------------------------------------------
// dungen
// Shaft.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <iosfwd>
#include <string>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    class Shaft
    {
    public:

        Shaft(id_type id, const std::string& name, position_type position);
        Shaft(const Shaft&) = default;
        ~Shaft() = default;

        Shaft& operator=(const Shaft&) = default;

        id_type id() const;

        std::string name() const;
        void set_name(const std::string& name);

        position_type position() const;
        void set_position(position_type position);

        void print(std::ostream& os) const;

    private:

        id_type m_id;
        std::string m_name;
        position_type m_position;
    };

    std::ostream& operator<<(std::ostream& os, const Shaft& shaft);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_SHAFT_HPP
