#if !defined(QUETZAL_DUNGEN_STAIRCASE_HPP)
#define QUETZAL_DUNGEN_STAIRCASE_HPP
//------------------------------------------------------------------------------
// dungen
// Staircase.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <iosfwd>
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

        Staircase(id_type id, const std::string& name, directed_point_type placement, Rise rise);
        Staircase(const Staircase&) = default;
        Staircase(Staircase&&) = default;
        ~Staircase() = default;

        Staircase& operator=(const Staircase&) = default;
        Staircase& operator=(Staircase&&) = default;

        id_type id() const;
        std::string name() const;
        directed_point_type placement() const;
        Rise rise() const;

        void print(std::ostream& os) const;

    private:

        id_type m_id;
        std::string m_name;
        directed_point_type m_placement;
        Rise m_rise;
    };

    std::ostream& operator<<(std::ostream& os, const Staircase& staircase);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_STAIRCASE_HPP
