#if !defined(QUETZAL_GEOMETRY_PROJECTION_HPP)
#define QUETZAL_GEOMETRY_PROJECTION_HPP
//------------------------------------------------------------------------------
// geometry
// Projection.hpp
//------------------------------------------------------------------------------

// Orthographic projection
// Projects 3D points onto the 2D xy-plane

#include "Orientation.hpp"
#include "OrientedPoint.hpp"
#include "Point.hpp"
#include "transformation_matrix.hpp"
#include <functional>
#include <cmath>
#include <cassert>

//#include "quetzal/math/floating_point.hpp"

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits> requires (Traits::dimension == 3)
    class Projection
    {
    public:

        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;
        using projected_traits = Traits::template resized_traits<Traits::dimension - 1>;
        using projected_type = Point<projected_traits>;

        Projection(); // orthographic projection onto xy-plane along z-axis
        Projection(const vector_type& normal); // orthographic projection onto the most perpendicular axis plane, then transformed to xy-plane
        Projection(const Orientation<Traits>& orientation); // orthographic projection where orientation direction and right are transformed to z-axis and x-axis respectively
        Projection(const OrientedPoint<Traits>& oriented_point); // orthographic projection as above, but centered at the oriented point position
        Projection(const Projection&) = default;
        Projection(Projection&&) = default;

        Projection& operator=(const Projection&) = default;
        Projection& operator=(Projection&&) = default;

        void init(const vector_type& normal);
        void init(const Orientation<Traits>& orientation);
        void init(const OrientedPoint<Traits>& oriented_point);

        projected_type operator()(const point_type& point) const;

    private:

        using projection_function_type = std::function<projected_type(const point_type& point)>;

        projection_function_type m_project;

        static projected_type project_x_yz(const point_type& point);
        static projected_type project_x_zy(const point_type& point);
        static projected_type project_y_zx(const point_type& point);
        static projected_type project_y_xz(const point_type& point);
        static projected_type project_z_xy(const point_type& point);
        static projected_type project_z_yx(const point_type& point);
    };

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
quetzal::geometry::Projection<Traits>::Projection() :
    m_project{project_z_xy}
{
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
quetzal::geometry::Projection<Traits>::Projection(const vector_type& normal) :
    m_project{}
{
    init(normal);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
quetzal::geometry::Projection<Traits>::Projection(const Orientation<Traits>& orientation) :
    m_project{}
{
    init(orientation);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
quetzal::geometry::Projection<Traits>::Projection(const OrientedPoint<Traits>& oriented_point) :
    m_project{}
{
    init(oriented_point);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
void quetzal::geometry::Projection<Traits>::init(const vector_type& normal)
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
template<typename Traits> requires (Traits::dimension == 3)
void quetzal::geometry::Projection<Traits>::init(const Orientation<Traits>& orientation)
{
    assert(orientation.direction().unit());
    assert(orientation.right().unit());

    auto matrix = alignment(orientation, {});
    m_project = [matrix](const point_type& point) -> projected_type {
        auto transformed = point * matrix;
//        assert(math::float_eq0(transformed.z()));
//        assert(math::float_eq(transformed.w(), Traits::value_type(1)));
        return {transformed.x(), transformed.y()};
    };

    return;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
void quetzal::geometry::Projection<Traits>::init(const OrientedPoint<Traits>& oriented_point)
{
    assert(oriented_point.orientation().direction().unit());
    assert(oriented_point.orientation().right().unit());

    auto matrix = alignment(oriented_point, {}); // translation then alignment? ...
    m_project = [matrix](const point_type& point) -> projected_type {
        auto transformed = point * matrix;
        return {transformed.x(), transformed.y()};
    };

    return;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::operator()(const point_type& point) const
{
    // projective divide, probably should not be necessary with projection ...

    return m_project(point);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_x_yz(const point_type& point)
{
    return {point.y(), point.z()};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_x_zy(const point_type& point)
{
    return {-point.y(), point.z()};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_y_zx(const point_type& point)
{
    return {-point.x(), point.z()};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_y_xz(const point_type& point)
{
    return {point.x(), point.z()};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_z_xy(const point_type& point)
{
    return {point.x(), point.y()};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_z_yx(const point_type& point)
{
    return {-point.x(), point.y()};
}

#endif // QUETZAL_GEOMETRY_PROJECTION_HPP
