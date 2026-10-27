#if !defined(QUETZAL_GEOMETRY_ORIENTATION_HPP)
#define QUETZAL_GEOMETRY_ORIENTATION_HPP
//------------------------------------------------------------------------------
// geometry
// Orientation.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include "quetzal/math/Vector.hpp"
#include <array>
#include <charconv>
#include <format>
#include <iostream>
#include <string_view>
#include <system_error>

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
        using vector_type = math::Vector<Traits>;

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

    template<typename Traits>
    std::istream& operator>>(std::istream& is, Orientation<Traits>& orientation);

    template<typename Traits>
    std::from_chars_result from_chars(const char* first, const char* last, Orientation<Traits>& orientation, const FormatSpec& spec = {});

    template<typename Traits>
    std::to_chars_result to_chars(char* first, char* last, const Orientation<Traits>& orientation, const FormatSpec& spec = {});

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
    os << quetzal::to_string(orientation, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::geometry::operator>>(std::istream& is, Orientation<Traits>& orientation)
{
    typename Orientation<Traits>::vector_type direction;
    typename Orientation<Traits>::vector_type right;
    if (quetzal::read_components(is,direction, right))
    {
        orientation = Orientation<Traits>(direction, right);
    }

    return is;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, Orientation<Traits>& orientation, const FormatSpec& spec)
{
    typename Orientation<Traits>::vector_type direction;
    typename Orientation<Traits>::vector_type right;
    auto result = quetzal::components_from_chars(first, last, spec,direction, right);
    if (result.ec == std::errc {})
    {
        orientation = Orientation<Traits>(direction, right);
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const Orientation<Traits>& orientation, const FormatSpec& spec)
{
    return quetzal::components_to_chars(first, last, spec,orientation.direction(), orientation.right());
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Orientation<Traits>> : quetzal::series_formatter<quetzal::math::Vector<Traits>>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::Orientation<Traits>& orientation, FormatContext& ctx) const
    {
        const std::array<quetzal::math::Vector<Traits>, 2> vectors {orientation.direction(), orientation.right()};
        return this->format_series(vectors, ctx);
    }
};

#endif // QUETZAL_GEOMETRY_ORIENTATION_HPP
