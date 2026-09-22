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
    enum class StaircaseDirection
    {
        Down,
        Up
    };

    //--------------------------------------------------------------------------
    class Staircase
    {
    public:

        Staircase(id_type id, const std::string& name, point_type position, StaircaseDirection direction);
        Staircase(const Staircase&) = default;
        ~Staircase() = default;

        Staircase& operator=(const Staircase&) = default;

        id_type id() const;

        std::string name() const;
        void set_name(const std::string& name);

        point_type position() const;
        void set_position(point_type position);

        StaircaseDirection direction() const;

        // The direction from its cell into the space it opens onto
        const std::optional<vector_type>& facing() const;
        void set_facing(vector_type facing);

        void print(std::ostream& os) const;

    private:

        id_type m_id;
        std::string m_name;
        point_type m_position;
        StaircaseDirection m_direction;
        std::optional<vector_type> m_facing;
    };

    std::ostream& operator<<(std::ostream& os, const Staircase& staircase);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_STAIRCASE_HPP
