#if !defined(QUETZAL_GEOMETRY_PROJECTION_HPP)
#define QUETZAL_GEOMETRY_PROJECTION_HPP
//------------------------------------------------------------------------------
// geometry
// Projection.hpp
//------------------------------------------------------------------------------

// Orthographic and perspective projection
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

        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;
        using matrix_type = transformation_matrix_type<Traits>;
        using projected_traits = Traits::template resized_traits<Traits::dimension - 1>;
        using projected_type = Point<projected_traits>;

        // orthographic and perspective projections have orientation in opposite directions ...

        Projection(); // orthographic projection onto xy-plane along z-axis
        Projection(const vector_type& normal); // orthographic projection onto the most perpendicular axis plane, then transformed to xy-plane
        Projection(const Orientation<Traits>& orientation); // orthographic projection where orientation direction and right are transformed to z-axis and x-axis respectively
        Projection(const OrientedPoint<Traits>& oriented_point); // orthographic projection as above, but centered at the oriented point position
        Projection(const OrientedPoint<Traits>& viewpoint, value_type fov); // perspective projection, and strictly that, no clipping or scaling
        Projection(const Projection&) = default;
        Projection(Projection&&) = default;

        Projection& operator=(const Projection&) = default;
        Projection& operator=(Projection&&) = default;

        void init(const vector_type& normal);
        void init(const Orientation<Traits>& orientation);
        void init(const OrientedPoint<Traits>& oriented_point);
        void init(const OrientedPoint<Traits>& viewpoint, value_type fov);

        projected_type operator()(const point_type& point) const;

        // most of these already use a matrix, just store and expose it ...
        // matrices would have to be 3D -> 2D and the static project functions would need to be converted to use matrix ...
//        matrix_type matrix() const;

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
quetzal::geometry::Projection<Traits>::Projection(const OrientedPoint<Traits>& viewpoint, value_type fov) :
    m_project{}
{
    init(viewpoint, fov);
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
        assert(math::float_eq(transformed.w(), Traits::value_type(1)));
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

    auto matrix = alignment(oriented_point, {});
    m_project = [matrix](const point_type& point) -> projected_type {
        auto transformed = point * matrix;
        assert(math::float_eq(transformed.w(), Traits::value_type(1)));
        return {transformed.x(), transformed.y()};
    };

    return;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
void quetzal::geometry::Projection<Traits>::init(const OrientedPoint<Traits>& viewpoint, value_type fov)
{
    assert(viewpoint.orientation().direction().unit());
    assert(viewpoint.orientation().right().unit());

    OrientedPoint<Traits> target = {};
    target.orientation().direction() = -vector_type::z_direction();
    auto matrix = alignment(viewpoint, target) * standard_perspective(fov);
    m_project = [matrix](const point_type& point) -> projected_type {
        auto transformed = projective_divide(point * matrix);
        return {transformed.x(), transformed.y()};
    };

    return;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::operator()(const point_type& point) const
{
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
