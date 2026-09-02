#if !defined(QUETZAL_GEOMETRY_MINPROJECTION_HPP)
#define QUETZAL_GEOMETRY_MINPROJECTION_HPP
//------------------------------------------------------------------------------
// geometry
// MinProjection.hpp
//------------------------------------------------------------------------------

// Projects 3D points or vectors onto the 2D xy-plane
// Minimalist orthographic projection along a prominent coordinate axis
// Does not preserve shape, might have different scale factors in different directions

#include "Point.hpp"
#include "quetzal/math/Vector.hpp"
#include <functional>
#include <type_traits>
#include <cmath>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename T> requires (T::dimension == 3)
    class MinProjection
    {
    public:

        using traits_type = T::traits_type;
        using value_type = traits_type::value_type;
        using vector_type = math::Vector<traits_type>;
        using projected_traits = traits_type::template resized_traits<traits_type::dimension - 1>;
        using projected_type = std::conditional_t<std::is_same_v<T, vector_type>, math::Vector<projected_traits>, Point<projected_traits>>;

        MinProjection(); // orthographic projection onto xy-plane along z-axis
        MinProjection(const vector_type& normal); // orthographic projection onto the most perpendicular axis plane, then transformed to xy-plane
        MinProjection(const MinProjection&) = default;
        MinProjection(MinProjection&&) = default;
        ~MinProjection() = default;

        MinProjection& operator=(const MinProjection&) = default;
        MinProjection& operator=(MinProjection&&) = default;

        void init(const vector_type& normal);

        projected_type operator()(const T& t) const;

    private:

        using projection_function_type = std::function<projected_type(const T&)>;
        projection_function_type m_project;

        static projected_type project_x_yz(const T& t);
        static projected_type project_x_zy(const T& t);
        static projected_type project_y_zx(const T& t);
        static projected_type project_y_xz(const T& t);
        static projected_type project_z_xy(const T& t);
        static projected_type project_z_yx(const T& t);
    };

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename T> requires (T::dimension == 3)
quetzal::geometry::MinProjection<T>::MinProjection() :
    m_project{project_z_xy}
{
}

//------------------------------------------------------------------------------
template<typename T> requires (T::dimension == 3)
quetzal::geometry::MinProjection<T>::MinProjection(const vector_type& normal) :
    m_project{}
{
    init(normal);
}

//------------------------------------------------------------------------------
template<typename T> requires (T::dimension == 3)
void quetzal::geometry::MinProjection<T>::init(const vector_type& normal)
{
    assert(normal.unit());

    switch (normal.longest_component())
    {
        case 0:
            m_project = normal.x() > 0 ? project_x_yz : project_x_zy;
            break;

        case 1:
            m_project = normal.y() > 0 ? project_y_zx : project_y_xz;
            break;

        case 2:
            m_project = normal.z() > 0 ? project_z_xy : project_z_yx;
            break;

        default:
            assert(false);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename T> requires (T::dimension == 3)
typename quetzal::geometry::MinProjection<T>::projected_type quetzal::geometry::MinProjection<T>::operator()(const T& t) const
{
    return m_project(t);
}

//------------------------------------------------------------------------------
template<typename T> requires (T::dimension == 3)
typename quetzal::geometry::MinProjection<T>::projected_type quetzal::geometry::MinProjection<T>::project_x_yz(const T& t)
{
    return {t.y(), t.z()};
}

//------------------------------------------------------------------------------
template<typename T> requires (T::dimension == 3)
typename quetzal::geometry::MinProjection<T>::projected_type quetzal::geometry::MinProjection<T>::project_x_zy(const T& t)
{
    return {-t.y(), t.z()};
}

//------------------------------------------------------------------------------
template<typename T> requires (T::dimension == 3)
typename quetzal::geometry::MinProjection<T>::projected_type quetzal::geometry::MinProjection<T>::project_y_zx(const T& t)
{
    return {-t.x(), t.z()};
}

//------------------------------------------------------------------------------
template<typename T> requires (T::dimension == 3)
typename quetzal::geometry::MinProjection<T>::projected_type quetzal::geometry::MinProjection<T>::project_y_xz(const T& t)
{
    return {t.x(), t.z()};
}

//------------------------------------------------------------------------------
template<typename T> requires (T::dimension == 3)
typename quetzal::geometry::MinProjection<T>::projected_type quetzal::geometry::MinProjection<T>::project_z_xy(const T& t)
{
    return {t.x(), t.y()};
}

//------------------------------------------------------------------------------
template<typename T> requires (T::dimension == 3)
typename quetzal::geometry::MinProjection<T>::projected_type quetzal::geometry::MinProjection<T>::project_z_yx(const T& t)
{
    return {-t.x(), t.y()};
}

#endif // QUETZAL_GEOMETRY_MINPROJECTION_HPP
