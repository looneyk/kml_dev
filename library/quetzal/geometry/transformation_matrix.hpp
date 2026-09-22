#if !defined(QUETZAL_GEOMETRY_TRANSFORMATION_MATRIX_HPP)
#define QUETZAL_GEOMETRY_TRANSFORMATION_MATRIX_HPP
//------------------------------------------------------------------------------
// geometry
// transformation_matrix.hpp
//
// Transformation matrices for pre-multiplication of row vectors: v' = v * M
// Angles in radians
//------------------------------------------------------------------------------

#include "Orientation.hpp"
#include "OrientedPoint.hpp"
#include "quetzal/math/Matrix.hpp"
#include "quetzal/math/Rotation.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include "quetzal/math/math_xm.hpp"
#include <array>
#include <concepts>
#include <iostream>
#include <span>
#include <cmath>
#include <cassert>

namespace quetzal::geometry
{

    template<size_t N>
    concept TransformationMatrixSize = (N == 2 || N == 3 || N == 4);

    template<typename M>
    concept TransformationMatrix = (M::square && TransformationMatrixSize<M::rows>);

//    template<typename Traits> requires (math::Homogeneous<Traits> && TransformationMatrixSize<Traits::size>)
//    using transformation_matrix_type = math::Matrix<typename Traits::value_type, Traits::size, Traits::size>;
    template<typename Traits> requires (TransformationMatrixSize<Traits::dimension + 1>)
    using transformation_matrix_type = math::Matrix<typename Traits::value_type, Traits::dimension + 1, Traits::dimension + 1>;

    template<TransformationMatrix M>
    static constexpr size_t transformation_dimension = M::rows - 1;

    // The functions here with either zero or one T parameter are ambiguous as far as dimensionality ...

    // Clears any translation component of the transformation
    template<TransformationMatrix M>
    M direction_matrix(M matrix);

    template<typename Traits> requires (math::Homogeneous<Traits>)
    transformation_matrix_type<Traits> translation(const math::Vector<Traits>& v);

    template<typename T>
    math::Matrix<T, 2> translation(T tx);

    template<typename T>
    math::Matrix<T, 3> translation(T tx, T ty);

    template<typename T>
    math::Matrix<T, 4> translation(T tx, T ty, T tz);

//    // Uniform scaling, cleaner template parameters, but requires explicit matrix type to be provided
//    template<TransformationMatrix M>
//    M scaling(typename M::value_type factor);
    template<typename T, size_t N = 3>
    math::Matrix<T, N + 1> scaling(T factor);

    template<typename T>
    math::Matrix<T, 3> scaling(T sx, T sy);

    template<typename T>
    math::Matrix<T, 4> scaling(T sx, T sy, T sz);

    template<typename V> requires (math::Homogeneous<typename V::traits_type>)
    transformation_matrix_type<typename V::traits_type> scaling_axis(const V& axis, typename V::value_type scale_directional, typename V::value_type scale_transverse);

    // 2D
    template<typename T>
    math::Matrix<T, 3> rotation_2(T angle);

    template<typename T>
    math::Matrix<T, 4> rotation_x(T angle);

    template<typename T>
    math::Matrix<T, 4> rotation_y(T angle);

    template<typename T>
    math::Matrix<T, 4> rotation_z(T angle);

    // experimental ...
    template<typename T>
    math::Matrix<T, 4> rotation_dz(T angle);

    template<typename T>
    math::Matrix<T, 4> rotation(math::Rotation<T> rotation);

    template<typename V> requires (math::Homogeneous<typename V::traits_type>)
    transformation_matrix_type<typename V::traits_type> rotation_axis(const V& axis, typename V::value_type angle);

    template<typename V> requires (math::Homogeneous<typename V::traits_type>)
    transformation_matrix_type<typename V::traits_type> rotation_axis_unit(const V& axis, typename V::value_type angle);

    // Rotation matrix resulting from rotation of unit vector from a to b
    template<typename V> requires (math::Homogeneous<typename V::traits_type>)
    transformation_matrix_type<typename V::traits_type> slerp_unit(const V& a, const V& b, typename V::value_type t);

    // Rotation matrix resulting from rotation of unit vector from a to b
    // The given axis is used as the rotation axis and to determine which of the explementary angles formed by a and b will be traversed by t
    template<typename V> requires (math::Homogeneous<typename V::traits_type>)
    transformation_matrix_type<typename V::traits_type> slerp_unit(const V& axis, const V& a, const V& b, typename V::value_type t);

    template<typename T>
    math::Matrix<T, 2> reflection_x1();

    template<typename T>
    math::Matrix<T, 3> reflection_x2();

    template<typename T>
    math::Matrix<T, 3> reflection_y2();

    template<typename T>
    math::Matrix<T, 4> reflection_x3();

    template<typename T>
    math::Matrix<T, 4> reflection_y3();

    template<typename T>
    math::Matrix<T, 4> reflection_z3();

//    template<typename Traits> requires (Traits::dimension == 2)
//    transformation_matrix_type<Traits> reflection(const geometry::Point<Traits>& point);

//    template<typename Traits> requires (Traits::dimension == 2)
//    transformation_matrix_type<Traits> reflection(const geometry::Line<Traits>& line);

//    template<typename Traits> requires (Traits::dimension == 3)
//    transformation_matrix_type<Traits> reflection(const geometry::Point<Traits>& point);

//    template<typename Traits> requires (Traits::dimension == 3)
//    transformation_matrix_type<Traits> reflection(const geometry::Line<Traits>& line);

//    template<typename Traits> requires (Traits::dimension == 3)
//    transformation_matrix_type<Traits> reflection(const geometry::Plane<Traits>& plane);

    // Transformation matrix for rotating from to coincide with to
    // vectors need to be unit length
    // (internally does an axis/angle rotation) ...
    template<typename V> requires (math::Homogeneous<typename V::traits_type>)
    transformation_matrix_type<typename V::traits_type> alignment(const V& from, const V& to);

    // Transformation matrix for rotating from_direction and from_right to coincide with to_direction and to_right
    // up and right vectors need to be unit length
    template<typename V> requires (math::Homogeneous<typename V::traits_type>)
    transformation_matrix_type<typename V::traits_type> alignment(const V& from_direction, V from_right, const V& to_direction, V to_right);

    // Fewer operations, would expect to be more accurate ...
    template<typename V> requires (math::Homogeneous<typename V::traits_type>)
    transformation_matrix_type<typename V::traits_type> alignment2(const V& from_direction, const V& from_right, const V& to_direction, const V& to_right);

    template<typename Traits> requires (math::Homogeneous<Traits>)
    transformation_matrix_type<Traits> alignment(const Orientation<Traits>& from, const Orientation<Traits>& to);

    template<typename Traits> requires (math::Homogeneous<Traits>)
    transformation_matrix_type<Traits> alignment(const OrientedPoint<Traits>& from, const OrientedPoint<Traits>& to);

    // Transformation matrix for perspective
    // projective divide will collapse to the xy-plane at z = 1
    template<typename Traits> requires (math::Homogeneous<Traits>)
    transformation_matrix_type<Traits> perspective(const OrientedPoint<Traits>& viewpoint, typename Traits::value_type fov);

    // Transformation matrix for perspective scaled to frustrum
    template<typename Traits> requires (math::Homogeneous<Traits>)
    transformation_matrix_type<Traits> perspective(const OrientedPoint<Traits>& viewpoint, typename Traits::value_type fov, typename Traits::value_type aspect_ratio, typename Traits::value_type z_near, typename Traits::value_type z_far);

    // Transformation matrix for a viewpoint at the origin and directed down the negative z-axis
    template<typename T>
    math::Matrix<T, 4> standard_perspective(T fov);

    // Transformation matrix for a viewpoint at the origin and directed down the negative z-axis scaled using the given frustrum parameters
    template<typename T>
    math::Matrix<T, 4> frustrum_perspective(T fov, T aspect_ratio, T z_near, T z_far);

// deprecated ...
// enabled only until affine_mapping is implemented ...
    // Requires dimension sets of homogeneous points (basis vectors) specified individually
    template<typename V> requires (math::Homogeneous<typename V::traits_type> && V::dimension == 2)
    transformation_matrix_type<typename V::traits_type> linear_mapping(const V& a0, const V& a1, const V& b0, const V& b1);

    // Requires dimension sets of homogeneous points
    template<typename P> requires (math::Homogeneous<typename P::traits_type>)
    transformation_matrix_type<typename P::traits_type> linear_mapping(std::span<const P, P::dimension> from, std::span<const P, P::dimension> to);

    // Requires dimension + 1 sets of homogeneous points
    template<typename P> requires (math::Homogeneous<typename P::traits_type>)
    transformation_matrix_type<typename P::traits_type> affine_mapping(std::span<const P, P::dimension + 1> from, std::span<const P, P::dimension + 1> to);

    // Requires dimension + 2 sets of homogeneous points
    // Four correspondences, two dimensional only: a homography, defined by mapping a quadrilateral to a quadrilateral, both planar
    template<typename P> requires (math::Homogeneous<typename P::traits_type> && P::dimension == 2)
    transformation_matrix_type<typename P::traits_type> projective_mapping(std::span<const P> from, std::span<const P> to);

    // experimental ...
    template<typename V> requires (math::Homogeneous<typename V::traits_type> && V::dimension == 3)
    transformation_matrix_type<typename V::traits_type> skew_symmetric_matrix(const V& axis);

    // Build a transformation (square) matrix with size appropriate for points of type P (dimension + 1 square)
    template<typename P> requires (math::Homogeneous<typename P::traits_type>)
    quetzal::geometry::transformation_matrix_type<typename P::traits_type> to_transformation_matrix(std::span<const P> points);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<quetzal::geometry::TransformationMatrix M>
M quetzal::geometry::direction_matrix(M matrix)
{
    // Clear last row of any translation, is this really correct? ...
    for (size_t i = 0; i < M::cols - 1; ++i)
    {
        matrix(M::rows - 1, i) = M::value_type(0);
    }
    // cover both ways ...
    for (size_t i = 0; i < M::rows - 1; ++i)
    {
        matrix(i, M::cols - 1) = M::value_type(0);
    }
    assert(matrix(M::rows - 1, M::cols - 1) == M::value_type(1));

    return transpose(inverse(matrix));
}

//------------------------------------------------------------------------------
template<typename Traits> requires (quetzal::math::Homogeneous<Traits>)
quetzal::geometry::transformation_matrix_type<Traits> quetzal::geometry::translation(const math::Vector<Traits>& v)
{
    transformation_matrix_type<Traits> matrix(1);
    for (size_t i = 0; i < Traits::dimension; ++i)
    {
        matrix(Traits::dimension, i) = v[i];
    }

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 2> quetzal::geometry::translation(T tx)
{
    math::Matrix<T, 2> matrix(1);
    matrix(2, 0) = tx;
    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 3> quetzal::geometry::translation(T tx, T ty)
{
    math::Matrix<T, 3> matrix(1);
    matrix(2, 0) = tx;
    matrix(2, 1) = ty;
    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::translation(T tx, T ty, T tz)
{
    math::Matrix<T, 4> matrix(1);
    matrix(3, 0) = tx;
    matrix(3, 1) = ty;
    matrix(3, 2) = tz;
    return matrix;
}

//------------------------------------------------------------------------------
//template<quetzal::geometry::TransformationMatrix M>
//M quetzal::geometry::scaling(typename M::value_type factor)
template<typename T, size_t N>
quetzal::math::Matrix<T, N + 1> quetzal::geometry::scaling(T factor)
{
    using M = math::Matrix<T, N + 1>;
    M matrix(1);

    auto data = matrix.data();
    size_t k = 0;
    constexpr size_t stride = M::cols + 1;
    for (size_t i = 0; i < M::rows - 1; ++i)
    {
        data[k] = factor;
        k += stride;
    }

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 3> quetzal::geometry::scaling(T sx, T sy)
{
    math::Matrix<T, 3> matrix(1);
    matrix(0, 0) = sx;
    matrix(1, 1) = sy;
    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::scaling(T sx, T sy, T sz)
{
    math::Matrix<T, 4> matrix(1);
    matrix(0, 0) = sx;
    matrix(1, 1) = sy;
    matrix(2, 2) = sz;
    return matrix;
}

//------------------------------------------------------------------------------
template<typename V> requires (quetzal::math::Homogeneous<typename V::traits_type>)
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::scaling_axis(const V& axis, typename V::value_type scale_directional, typename V::value_type scale_transverse)
{
    assert(axis.unit());

    typename V::value_type scale_diff = scale_directional - scale_transverse;

    transformation_matrix_type<typename V::traits_type> matrix(1);
    matrix(0, 0) = scale_transverse + scale_diff * axis.x() * axis.x();
    matrix(1, 0) = scale_diff * axis.x() * axis.y();
    matrix(2, 0) = scale_diff * axis.x() * axis.z();
    matrix(0, 1) = scale_diff * axis.y() * axis.x();
    matrix(1, 1) = scale_transverse + scale_diff * axis.y() * axis.y();
    matrix(2, 1) = scale_diff * axis.y() * axis.z();
    matrix(0, 2) = scale_diff * axis.z() * axis.x();
    matrix(1, 2) = scale_diff * axis.z() * axis.y();
    matrix(2, 2) = scale_transverse + scale_diff * axis.z() * axis.z();
    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 3> quetzal::geometry::rotation_2(T angle)
{
    T c = cos(angle);
    T s = sin(angle);

    math::Matrix<T, 3> matrix(1);
    matrix(0, 0) = c;
    matrix(0, 1) = s;
    matrix(1, 0) = -s;
    matrix(1, 1) = c;

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::rotation_x(T angle)
{
    T c = cos(angle);
    T s = sin(angle);

    math::Matrix<T, 4> matrix(1);
    matrix(1, 1) = c;
    matrix(1, 2) = s;
    matrix(2, 1) = -s;
    matrix(2, 2) = c;

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::rotation_y(T angle)
{
    T c = cos(angle);
    T s = sin(angle);

    math::Matrix<T, 4> matrix(1);
    matrix(2, 2) = c;
    matrix(2, 0) = s;
    matrix(0, 2) = -s;
    matrix(0, 0) = c;

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::rotation_z(T angle)
{
    T c = cos(angle);
    T s = sin(angle);

    math::Matrix<T, 4> matrix(1);
    matrix(0, 0) = c;
    matrix(0, 1) = s;
    matrix(1, 0) = -s;
    matrix(1, 1) = c;

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::rotation_dz(T angle)
{
    T c = cos(angle);
    T s = sin(angle);

    math::Matrix<T, 4> matrix(1);
    matrix(0, 0) = -s;
    matrix(0, 1) = c;
    matrix(1, 0) = -c;
    matrix(1, 1) = -s;

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::rotation(math::Rotation<T> rotation)
{
    math::Matrix<T, 4> matrix;
    
    const auto& q = rotation.rep();
    T xx = q[0] * q[0];
    T yy = q[1] * q[1];
    T zz = q[2] * q[2];
    T xy = q[0] * q[1];
    T yz = q[1] * q[2];
    T zx = q[2] * q[0];
    T wx = q[3] * q[0];
    T wy = q[3] * q[1];
    T wz = q[3] * q[2];
    
    matrix(0, 0) = T(1) - T(2) * (yy + zz);
    matrix(0, 1) = T(2) * (xy - wz);
    matrix(0, 2) = T(2) * (zx + wy);
    
    matrix(1, 0) = T(2) * (xy + wz);
    matrix(1, 1) = T(1) - T(2) * (xx + zz);
    matrix(1, 2) = T(2) * (yz - wx);
    
    matrix(2, 0) = T(2) * (zx - wy);
    matrix(2, 1) = T(2) * (yz + wx);
    matrix(2, 2) = T(1) - T(2) * (xx + yy);
    
    matrix(3, 3) = T(1);

    return matrix;
}

//------------------------------------------------------------------------------
template<typename V> requires (quetzal::math::Homogeneous<typename V::traits_type>)
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::rotation_axis(const V& axis, typename V::value_type angle)
{
    return rotation_axis_unit(normalize(axis), angle);
}

//------------------------------------------------------------------------------
template<typename V> requires (quetzal::math::Homogeneous<typename V::traits_type>)
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::rotation_axis_unit(const V& axis, typename V::value_type angle)
{
    assert(axis.unit());

    using matrix_type = transformation_matrix_type<typename V::traits_type>;

    if constexpr (std::same_as<typename V::value_type, float>)
    {
        // DirectXMath is about 15 times faster. Try using SSE, Eigen ...
        return rotation_axis_unit_xm(axis, angle);
    }
    else
    {
        // Rotation matrix using Rodrigues' rotation formula

        using T = V::value_type;

        matrix_type mcross =
        {
            T(0), axis[2], -axis[1], T(0),
            -axis[2], T(0), axis[0], T(0),
            axis[1], -axis[0], T(0), T(0),
            T(0), T(0), T(0), T(0),
        };

//        matrix_type mcross = transpose(skew_symmetric_matrix(axis)); // if transpose, is skew symmetric correct for the multiplication here? ...

        matrix_type mcross2 =
        {
            -(axis[1] * axis[1] + axis[2] * axis[2]), axis[0] * axis[1], axis[2] * axis[0], T(0),
            axis[0] * axis[1], -(axis[2] * axis[2] + axis[0] * axis[0]), axis[1] * axis[2], T(0),
            axis[2] * axis[0], axis[1] * axis[2], -(axis[0] * axis[0] + axis[1] * axis[1]), T(0),
            T(0), T(0), T(0), T(0),
        };

        return matrix_type::Identity() + sin(angle) * mcross + (1 - cos(angle)) * mcross2;
    }
}

//------------------------------------------------------------------------------
template<typename V> requires (quetzal::math::Homogeneous<typename V::traits_type>)
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::slerp_unit(const V& a, const V& b, typename V::value_type t)
{
    if (math::float_eq(a, b))
    {
        return transformation_matrix_type<typename V::traits_type>::Identity();
    }

    if (math::float_eq(a, -b))
    {
        auto axis = perpendicular_unit_vector(a);
        return rotation_axis(axis, t * math::Pi<typename V::value_type>);
    }

    return rotation_axis(cross(a, b), t * angle_between_unit(a, b));
}

//------------------------------------------------------------------------------
template<typename V> requires (quetzal::math::Homogeneous<typename V::traits_type>)
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::slerp_unit(const V& axis, const V& a, const V& b, typename V::value_type t)
{
    if (math::float_eq(a, b))
    {
        return transformation_matrix_type<typename V::traits_type>::Identity();
    }

    if (math::float_eq(a, -b))
    {
        return rotation_axis(perpendicular_unit_vector(a), t * math::Pi<typename V::value_type>); // The selection of axis is arbitrary ...
    }

    return rotation_axis(axis, t * angle_full_unit(a, b, axis));
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 2> quetzal::geometry::reflection_x1()
{
    math::Matrix<T, 2> matrix(1);
    matrix(0, 0) = T(-1);
    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 3> quetzal::geometry::reflection_x2()
{
    math::Matrix<T, 3> matrix(1);
    matrix(0, 0) = T(-1);
    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 3> quetzal::geometry::reflection_y2()
{
    math::Matrix<T, 3> matrix(1);
    matrix(1, 1) = T(-1);
    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::reflection_x3()
{
    math::Matrix<T, 4> matrix(1);
    matrix(0, 0) = T(-1);
    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::reflection_y3()
{
    math::Matrix<T, 4> matrix(1);
    matrix(1, 1) = T(-1);
    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::reflection_z3()
{
    math::Matrix<T, 4> matrix(1);
    matrix(2, 2) = T(-1);
    return matrix;
}

//------------------------------------------------------------------------------
template<typename V> requires (quetzal::math::Homogeneous<typename V::traits_type>)
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::alignment(const V& from_direction, const V& to_direction)
{
    assert(from_direction.unit());
    assert(to_direction.unit());

    if (math::float_eq(from_direction, to_direction))
    {
        return transformation_matrix_type<typename V::traits_type>::Identity();
    }

    return rotation_axis_unit(normalize(cross(from_direction, to_direction)), angle_between_unit(from_direction, to_direction));
}

//------------------------------------------------------------------------------
template<typename V> requires (quetzal::math::Homogeneous<typename V::traits_type>)
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::alignment(const V& from_direction, V from_right, const V& to_direction, V to_right)
{
    assert(from_direction.unit());
    assert(from_right.unit());
    assert(to_direction.unit());
    assert(to_right.unit());
    assert(perpendicular(from_direction, from_right));
    assert(perpendicular(to_direction, to_right));

    using T = V::value_type;

    // Transformation to rotate direction and right From vectors to z-axis and x-axis respectively

    T thetaFrom = angle_between_unit(from_direction, V::z_direction());
    V axisFrom;
    if (math::float_eq(thetaFrom, math::Pi<T>))
    {
        axisFrom = from_right;
    }
    else if (math::float_ne0(thetaFrom))
    {
        axisFrom = normalize(cross(from_direction, V::z_direction()));
        from_right *= rotation_axis_unit(axisFrom, thetaFrom);
    }

    assert(math::float_eq0(from_right.z()));
    T phiFrom = atan2(from_right.y(), from_right.x());

    // Transformation to rotate direction and right To vectors to z-axis and x-axis respectively

    T thetaTo = angle_between_unit(to_direction, V::z_direction());
    V axisTo;
    if (math::float_eq(thetaTo, math::Pi<T>))
    {
        axisTo = to_right;
    }
    else if (math::float_ne0(thetaTo))
    {
        axisTo = normalize(cross(to_direction, V::z_direction()));
        to_right *= rotation_axis_unit(axisTo, thetaTo);
    }

    assert(math::float_eq0(to_right.z()));
    T phiTo = atan2(to_right.y(), to_right.x());
    T dphi = phiTo - phiFrom;

    // Transformation matrix

    transformation_matrix_type<typename V::traits_type> matrix(1);

    // Rotate From to z-axis orientation
    if (math::float_ne0(thetaFrom))
    {
        matrix = geometry::rotation_axis_unit(axisFrom, thetaFrom);
    }

    // z-axis rotation
    if (math::float_ne0(dphi))
    {
        matrix *= geometry::rotation_z(dphi);
    }

    // Rotate To from z-axis orientation
    if (math::float_ne0(thetaTo))
    {
        matrix *= geometry::rotation_axis_unit(axisTo, -thetaTo);
    }

    return matrix;
}

//------------------------------------------------------------------------------
template<typename V> requires (quetzal::math::Homogeneous<typename V::traits_type>)
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::alignment2(const V& from_direction, const V& from_right, const V& to_direction, const V& to_right)
{
    assert(from_direction.unit());
    assert(from_right.unit());
    assert(to_direction.unit());
    assert(to_right.unit());
    assert(perpendicular(from_direction, from_right));
    assert(perpendicular(to_direction, to_right));

    using T = V::value_type;

    transformation_matrix_type<typename V::traits_type> matrix(1);

    V rightFromAligned = from_right;
    T dtheta = angle_between_unit(from_direction, to_direction);
    if (math::float_ne0(dtheta))
    {
        V axis;
        if (math::float_eq(dtheta, math::Pi<T>))
        {
            axis = to_right;
        }
        else
        {
            axis = normalize(cross(from_direction, to_direction));
        }

        matrix = rotation_axis_unit(axis, dtheta);
        rightFromAligned = normalize(from_right * matrix);
    }

//    assert(perpendicular(to_direction, rightFromAligned));

    T dphi = angle_full_unit(rightFromAligned, to_right, to_direction);
    if (math::float_ne0(dphi))
    {
        matrix *= rotation_axis_unit(to_direction, dphi);
    }

//    assert(math::float_eq(from_direction * matrix, to_direction));
//    assert(math::float_eq(from_right * matrix, to_right));

    return matrix;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (quetzal::math::Homogeneous<Traits>)
quetzal::geometry::transformation_matrix_type<Traits> quetzal::geometry::alignment(const Orientation<Traits>& from, const Orientation<Traits>& to)
{
    return alignment(from.direction(), from.right(), to.direction(), to.right());
}

//------------------------------------------------------------------------------
template<typename Traits> requires (quetzal::math::Homogeneous<Traits>)
quetzal::geometry::transformation_matrix_type<Traits> quetzal::geometry::alignment(const OrientedPoint<Traits>& from, const OrientedPoint<Traits>& to)
{
    return translation(-from.position().vector()) * alignment(from.orientation(), to.orientation()) * translation(to.position().vector());
}

//------------------------------------------------------------------------------
template<typename Traits> requires (quetzal::math::Homogeneous<Traits>)
quetzal::geometry::transformation_matrix_type<Traits> quetzal::geometry::perspective(const OrientedPoint<Traits>& viewpoint, typename Traits::value_type fov)
{
    using vector_type = math::Vector<Traits>;
    OrientedPoint<Traits> target = {{}, {-vector_type::z_direction(), vector_type::x_direction()}};
    return alignment(viewpoint, target) * standard_perspective(fov);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (quetzal::math::Homogeneous<Traits>)
quetzal::geometry::transformation_matrix_type<Traits> quetzal::geometry::perspective(const OrientedPoint<Traits>& viewpoint, typename Traits::value_type fov, typename Traits::value_type aspect_ratio, typename Traits::value_type z_near, typename Traits::value_type z_far)
{
    using vector_type = math::Vector<Traits>;
    OrientedPoint<Traits> target = {{}, {-vector_type::z_direction(), vector_type::x_direction()}};
    return alignment(viewpoint, target) * frustrum_perspective(fov, aspect_ratio, z_near, z_far);
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::standard_perspective(T fov)
{
    T s = T(1) / tan(fov / T(2));

    quetzal::math::Matrix<T, 4> matrix;
    matrix(0, 0) = s;
    matrix(1, 1) = s;
    matrix(2, 2) = T(-1);
    matrix(2, 3) = T(-1);
    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::frustrum_perspective(T fov, T aspect_ratio, T z_near, T z_far)
{
    assert(false); // not implemented yet ...

    T s = T(1) / tan(fov / T(2));

    quetzal::math::Matrix<T, 4> matrix;
    matrix(0, 0) = s;
    matrix(1, 1) = s;
    matrix(2, 2) = T(-1);
    matrix(2, 3) = T(-1);
    return matrix;
}

//------------------------------------------------------------------------------
template<typename V> requires (quetzal::math::Homogeneous<typename V::traits_type> && V::dimension == 2)
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::linear_mapping(const V& a0, const V& a1, const V& b0, const V& b1)
{
    // [a] X = [b]
    // X = inverse([a]) [b]

    auto d = a0.x() * a1.y() - a0.y() * a1.x();

    transformation_matrix_type<typename V::traits_type> matrix(1);
    matrix(0, 0) = (a1.y() * b0.x() - a0.y() * b1.x()) / d;
    matrix(0, 1) = (a1.y() * b0.y() - a0.y() * b1.y()) / d;
    matrix(1, 0) = (a0.x() * b1.x() - a1.x() * b0.x()) / d;
    matrix(1, 1) = (a0.x() * b1.y() - a1.x() * b0.y()) / d;

    return matrix;
}

//------------------------------------------------------------------------------
template<typename P> requires (quetzal::math::Homogeneous<typename P::traits_type>)
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::linear_mapping(std::span<const P, P::dimension> from, std::span<const P, P::dimension> to)
{
    constexpr size_t N = P::dimension;

    // A linear map has no translation, so dimension correspondences determine it, in the leading dimension x dimension
    // block. to_transformation_matrix reads one more point than a linear map is given
    // consider changing to_transfoormation_matrix for this, cleaner code, less efficient ...

    // [from] X = [to]
    // X = inverse([from]) [to]

    math::Matrix<typename P::value_type, N, N> mapped_from;
    math::Matrix<typename P::value_type, N, N> mapped_to;

    for (size_t i = 0; i < N; ++i)
    {
        for (size_t j = 0; j < N; ++j)
        {
            mapped_from(i, j) = from[i][j];
            mapped_to(i, j) = to[i][j];
        }
    }

    auto x = inverse(mapped_from) * mapped_to;

    transformation_matrix_type<typename P::traits_type> matrix(1);
    for (size_t i = 0; i < N; ++i)
    {
        for (size_t j = 0; j < N; ++j)
        {
            matrix(i, j) = x(i, j);
        }
    }

    return matrix;
}

//------------------------------------------------------------------------------
template<typename P> requires (quetzal::math::Homogeneous<typename P::traits_type>)
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::affine_mapping(std::span<const P, P::dimension + 1> from, std::span<const P, P::dimension + 1> to)
{
    // [from] X = [to]
    // X = inverse([from]) [to]

    auto mapped_from = to_transformation_matrix<P>(from);
    auto mapped_to = to_transformation_matrix<P>(to);

    return inverse(mapped_from) * mapped_to;
}

//------------------------------------------------------------------------------
// Direct linear transformation. A homography has eight degrees of freedom, and each correspondence has its own scale,
// [from]i H = ki [to]i. Each correspondence contributes two equations in the eight unknowns, the ninth element fixed at
// 1. Four correspondences determine the map exactly; more are fitted by least squares
template<typename P> requires (quetzal::math::Homogeneous<typename P::traits_type> && P::dimension == 2)
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping(std::span<const P> from, std::span<const P> to)
{
    using T = P::value_type;

    assert(from.size() >= 4);
    assert(from.size() == to.size());

    const size_t n = from.size();

    // The two rows one correspondence contributes, coefficients then right hand side.
    // For (x, y, 1) * M = w (u, v, 1) with unknowns m00, m10, m20, m01, m11, m21, m02, m12:
    //     x m00 + y m10 + m20 - u x m02 - u y m12 = u
    //     x m01 + y m11 + m21 - v x m02 - v y m12 = v
    auto equations = [](const P& f, const P& t) -> std::array<std::array<T, 9>, 2>
    {
        T x = f[0], y = f[1];
        T u = t[0], v = t[1];

        return {{{x, y, T(1), T(0), T(0), T(0), -u * x, -u * y, u},
                 {T(0), T(0), T(0), x, y, T(1), -v * x, -v * y, v}}};
    };

    math::Matrix<T, 8, 9> system;

    if (n == 4)
    {
        // Exactly determined, so solved as it stands; the normal equations would square the condition number
        for (size_t i = 0; i < n; ++i)
        {
            auto rows = equations(from[i], to[i]);
            for (size_t k = 0; k < rows.size(); ++k)
            {
                for (size_t j = 0; j < 9; ++j)
                {
                    system(2 * i + k, j) = rows[k][j];
                }
            }
        }
    }
    else
    {
        // Overdetermined, so least squares through the normal equations, transpose(A) A x = transpose(A) b, accumulated
        // directly into the 8 by 9, as the height of the 2n by 8 system is not known at compile time
        for (size_t a = 0; a < 8; ++a)
        {
            for (size_t b = 0; b < 9; ++b)
            {
                system(a, b) = T(0);
            }
        }

        for (size_t i = 0; i < n; ++i)
        {
            for (const auto& row : equations(from[i], to[i]))
            {
                for (size_t a = 0; a < 8; ++a)
                {
                    for (size_t b = 0; b < 8; ++b)
                    {
                        system(a, b) += row[a] * row[b];
                    }

                    system(a, 8) += row[a] * row[8];
                }
            }
        }
    }

    auto [reduced, nonsingular] = math::gauss_jordan_elimination(system);
    assert(nonsingular); // Degenerate correspondences, no homography maps one set onto the other

    return {reduced(0, 8), reduced(3, 8), reduced(6, 8),
            reduced(1, 8), reduced(4, 8), reduced(7, 8),
            reduced(2, 8), reduced(5, 8), T(1)};
}

//------------------------------------------------------------------------------
template<typename V> requires (quetzal::math::Homogeneous<typename V::traits_type> && V::dimension == 3)
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::skew_symmetric_matrix(const V& axis)
{
    using T = V::value_type;

    return
    {
        T(0), -axis[2], axis[1],
        axis[2], T(0), -axis[0],
        -axis[1], axis[0], T(0),
    };
}

//------------------------------------------------------------------------------
template<typename P> requires (quetzal::math::Homogeneous<typename P::traits_type>)
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::to_transformation_matrix(std::span<const P> points)
{
    assert(points.size() >= P::component_count);

    transformation_matrix_type<typename P::traits_type> matrix(1);
    for (size_t i = 0; i < P::component_count; ++i)
    {
        for (size_t j = 0; j < P::component_count; ++j)
        {
            matrix(i, j) = points[i][j];
        }
    }

    return matrix;
}

#endif // QUETZAL_GEOMETRY_TRANSFORMATION_MATRIX_HPP
