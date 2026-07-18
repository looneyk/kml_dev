#if !defined(QUETZAL_GEOMETRY_ORIENTATION_HPP)
#define QUETZAL_GEOMETRY_ORIENTATION_HPP
//------------------------------------------------------------------------------
// geometry
// Orientation.hpp
//------------------------------------------------------------------------------

#include "Vector.hpp"
#include "quetzal/common/string_util.hpp"
#include <format>
#include <iostream>

namespace quetzal::geometry
{

    // By default, direction is along the z-axis and right is along the x-axis (a vertical or up orientation)

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Orientation
    {
    public:

		static_assert(Traits::dimension >= 3);

        using value_type = Traits::value_type;
        using vector_type = Vector<Traits>;

        Orientation();
        Orientation(const vector_type& direction, const vector_type& right);
        Orientation(const Orientation&) = default;
        Orientation(Orientation&&) = default;
        ~Orientation() = default;

        Orientation& operator=(const Orientation&) = default;
        Orientation& operator=(Orientation&&) = default;

        // Direction may have different physical interpretations depending on the use case (mesh (up) vs. camera (forward), for example)
        vector_type direction() const;
        vector_type& direction();
        void set_direction(const vector_type& v);

        vector_type right() const;
        vector_type& right();
        void set_right(const vector_type& v);

        // Set direction to be oriented forward, right is calculated using the given up vector
        // This is the form that is used by Projection, no, but that would be more logical, see how orientation is used and consider following Camera axes ...
        // aside from projection, does direction up make more sense in other usages? ...
        void set_forward_up(const vector_type& forward, const vector_type& up);

        // Reorients right so that it is perpendicular to direction and normalizes the resulting vectors
        void normalize();

        // Sets direction and right to align with z-axis and x-axis respectively
        void clear();

    private:

        vector_type m_direction;
        vector_type m_right;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, Orientation<Traits> orientation);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Orientation<Traits>::Orientation() :
    Orientation(vector_type::z_direction(), vector_type::x_direction())
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Orientation<Traits>::Orientation(const vector_type& direction, const vector_type& right) :
    m_direction(direction),
    m_right(right)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Orientation<Traits>::vector_type quetzal::geometry::Orientation<Traits>::direction() const
{
    return m_direction;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Orientation<Traits>::vector_type& quetzal::geometry::Orientation<Traits>::direction()
{
    return m_direction;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Orientation<Traits>::set_direction(const vector_type& v)
{
    m_direction = v;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Orientation<Traits>::vector_type quetzal::geometry::Orientation<Traits>::right() const
{
    return m_right;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Orientation<Traits>::vector_type& quetzal::geometry::Orientation<Traits>::right()
{
    return m_right;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Orientation<Traits>::set_right(const vector_type& v)
{
    m_right = v;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Orientation<Traits>::set_forward_up(const vector_type& forward, const vector_type& up)
{
    m_direction = math::normalize(forward);
    m_right = math::normalize(cross(forward, up));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Orientation<Traits>::normalize()
{
    auto front = cross(m_direction, m_right);
    m_right = math::normalize(cross(front, m_direction));
    math::normalize(m_direction);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Orientation<Traits>::clear()
{
    m_direction = vector_type::z_direction();
    m_right = vector_type::x_direction();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, Orientation<Traits> orientation)
{
    os << to_array_string(orientation, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Orientation<Traits>>
{
    // Orientation has no formatting knobs of its own; the whole spec belongs to (and is entirely
    // consumed by) the Vector formatter, reused verbatim for direction and right.
    std::formatter<quetzal::math::Vector<Traits>> m_vector_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_vector_formatter.parse(ctx);
    }

    auto format(const quetzal::geometry::Orientation<Traits>& orientation, std::format_context& ctx) const
    {
        auto out = m_vector_formatter.format(orientation.direction(), ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        return m_vector_formatter.format(orientation.right(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_ORIENTATION_HPP
