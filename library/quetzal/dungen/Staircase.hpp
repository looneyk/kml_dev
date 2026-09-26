#if !defined(QUETZAL_DUNGEN_STAIRCASE_HPP)
#define QUETZAL_DUNGEN_STAIRCASE_HPP
//------------------------------------------------------------------------------
// dungen
// Staircase.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <iosfwd>
#include <optional>
#include <string>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    enum class Rise
    {
        Down,
        Up
    };

    std::string to_string(Rise rise);

    //--------------------------------------------------------------------------
    class Staircase
    {
    public:

        Staircase(id_type id, const std::string& name, directed_point_type directed_position, Rise rise);
        Staircase(const Staircase&) = default;
        ~Staircase() = default;

        Staircase& operator=(const Staircase&) = default;

        id_type id() const;

        std::string name() const;
        void set_name(const std::string& name);

        directed_point_type directed_position() const;
        void set_directed_position(directed_point_type directed_position);

        point_type position() const;
        vector_type direction() const;

        Rise rise() const;

        void print(std::ostream& os) const;

    private:

        id_type m_id;
        std::string m_name;
        directed_point_type m_directed_position;
        Rise m_rise;
    };

    std::ostream& operator<<(std::ostream& os, const Staircase& staircase);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_STAIRCASE_HPP
