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
    template<Homogeneous Traits> requires (Traits::dimension == 3)
    class Projection
    {
    public:

        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;
        using matrix_type = transformation_matrix_type<Traits>;
        using projected_traits = Traits::template resized_traits<Traits::dimension - 1>;
        using projected_type = Point<projected_traits>;

        // orthographic and perspective projections have orientation in opposite directions, this needs to be resolved ...

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

        // This is the projection type dependent depth scale factor
        // Provides transformed (but not projected) z value for perspective, constant for orthographic
        // (z_order would return the z value for both)
        value_type z_scale(const point_type& point) const;

    private:

        // The transformed z value before projective divide and/or dimension reduction, independent of projection type
        // Note that the sign is inverted in the perspective case as its matrix inverts sign of z for proper depth scaling due to its required model positioning
        value_type transformed_z(const point_type& point) const;

        using projection_function_type = std::function<projected_type(const point_type& point)>;
        using z_value_function_type = std::function<value_type(const point_type& point)>;

        matrix_type m_matrix;
        projection_function_type m_project;
        z_value_function_type m_z_scale;

        z_value_function_type m_z_scale_orthographic = [](const point_type&) { return value_type(1); };
        z_value_function_type m_z_scale_perspective = [this](const point_type& point) { return transformed_z(point); };

        static matrix_type matrix_x_yz;
        static matrix_type matrix_x_zy;
        static matrix_type matrix_y_zx;
        static matrix_type matrix_y_xz;
        static matrix_type matrix_z_xy;
        static matrix_type matrix_z_yx;

        static projected_type project_x_yz(const point_type& point);
        static projected_type project_x_zy(const point_type& point);
        static projected_type project_y_zx(const point_type& point);
        static projected_type project_y_xz(const point_type& point);
        static projected_type project_z_xy(const point_type& point);
        static projected_type project_z_yx(const point_type& point);
    };

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
quetzal::geometry::Projection<Traits>::Projection() :
    m_project{project_z_xy}
{
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
quetzal::geometry::Projection<Traits>::Projection(const vector_type& normal) :
    m_project{}
{
    init(normal);
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
quetzal::geometry::Projection<Traits>::Projection(const Orientation<Traits>& orientation) :
    m_project{}
{
    init(orientation);
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
quetzal::geometry::Projection<Traits>::Projection(const OrientedPoint<Traits>& oriented_point) :
    m_project{}
{
    init(oriented_point);
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
quetzal::geometry::Projection<Traits>::Projection(const OrientedPoint<Traits>& viewpoint, value_type fov) :
    m_project{}
{
    init(viewpoint, fov);
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
void quetzal::geometry::Projection<Traits>::init(const vector_type& normal)
{
    assert(normal.unit());

    switch (normal.longest_component())
    {
        case 0:
            m_matrix = normal.x() > 0 ? matrix_x_yz : matrix_x_zy;
            m_project = normal.x() > 0 ? project_x_yz : project_x_zy;
            break;

        case 1:
            m_matrix = normal.y() > 0 ? matrix_y_zx : matrix_y_xz;
            m_project = normal.y() > 0 ? project_y_zx : project_y_xz;
            break;

        case 2:
            m_matrix = normal.z() > 0 ? matrix_z_xy : matrix_z_yx;
            m_project = normal.z() > 0 ? project_z_xy : project_z_yx;
            break;

        default:
            assert(false);
    }

    m_z_scale = m_z_scale_orthographic;

    return;
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
void quetzal::geometry::Projection<Traits>::init(const Orientation<Traits>& orientation)
{
    m_matrix = alignment(orientation, {});
    m_project = [this](const point_type& point) -> projected_type {
        auto transformed = point * m_matrix;
        assert(math::float_eq(transformed.w(), Traits::value_type(1)));
        return {transformed.x(), transformed.y()};
    };
    m_z_scale = m_z_scale_orthographic;

    return;
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
void quetzal::geometry::Projection<Traits>::init(const OrientedPoint<Traits>& oriented_point)
{
    m_matrix = alignment(oriented_point, {});
    m_project = [this](const point_type& point) -> projected_type {
        auto transformed = point * m_matrix;
        assert(math::float_eq(transformed.w(), Traits::value_type(1)));
        return {transformed.x(), transformed.y()};
    };
    m_z_scale = m_z_scale_orthographic;

    return;
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
void quetzal::geometry::Projection<Traits>::init(const OrientedPoint<Traits>& viewpoint, value_type fov)
{
    m_matrix = perspective(viewpoint, fov);
    m_project = [this](const point_type& point) -> projected_type {
        auto transformed = projective_divide(point * m_matrix);
        return {transformed.x(), transformed.y()};
    };
    m_z_scale = m_z_scale_perspective;

    return;
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::operator()(const point_type& point) const
{
    return m_project(point);
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::value_type quetzal::geometry::Projection<Traits>::z_scale(const point_type& point) const
{
    return m_z_scale(point);
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::value_type quetzal::geometry::Projection<Traits>::transformed_z(const point_type& point) const
{
    auto transformed = point * m_matrix;
    return transformed.z();
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::matrix_type quetzal::geometry::Projection<Traits>::matrix_x_yz = 
{
    0, 0, 0, 0,
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 1,
};

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::matrix_type quetzal::geometry::Projection<Traits>::matrix_x_zy =
{
    0, 0, 0, 0,
    -1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 1,
};

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::matrix_type quetzal::geometry::Projection<Traits>::matrix_y_zx =
{
    -1, 0, 0, 0,
    0, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 1,
};

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::matrix_type quetzal::geometry::Projection<Traits>::matrix_y_xz =
{
    1, 0, 0, 0,
    0, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 1,
};

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::matrix_type quetzal::geometry::Projection<Traits>::matrix_z_xy =
{
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 1,
};

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::matrix_type quetzal::geometry::Projection<Traits>::matrix_z_yx =
{
    -1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 1,
};

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_x_yz(const point_type& point)
{
    return {point.y(), point.z()};
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_x_zy(const point_type& point)
{
    return {-point.y(), point.z()};
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_y_zx(const point_type& point)
{
    return {-point.x(), point.z()};
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_y_xz(const point_type& point)
{
    return {point.x(), point.z()};
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_z_xy(const point_type& point)
{
    return {point.x(), point.y()};
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Homogeneous Traits> requires (Traits::dimension == 3)
typename quetzal::geometry::Projection<Traits>::projected_type quetzal::geometry::Projection<Traits>::project_z_yx(const point_type& point)
{
    return {-point.x(), point.y()};
}

#endif // QUETZAL_GEOMETRY_PROJECTION_HPP
