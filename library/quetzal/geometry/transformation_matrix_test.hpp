#if !defined(QUETZAL_GEOMETRY_TRANSFORMATION_MATRIX_HPP)
#define QUETZAL_GEOMETRY_TRANSFORMATION_MATRIX_HPP
//------------------------------------------------------------------------------
// geometry
// transformation_matrix.hpp
//
// Transformation matrices for pre-multiplication: v' = v * M
// Angles in radians
//------------------------------------------------------------------------------

#include "quetzal/math/Matrix.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include "quetzal/math/math_xm.hpp"
#include <concepts>
#include <iomanip>
#include <iostream>
#include <span>
#include <cmath>
#include <cassert>

namespace quetzal::geometry
{

    // The functions here with either zero or one T parameter are ambiguous as far as dimensionality ...

    template<typename Traits> requires(Traits::dimension == 2 || Traits::dimension == 3)
    using transformation_matrix_type = math::Matrix<typename Traits::value_type, Traits::size>;

    // Clears any translation component of the transformation
    template<typename A> requires A::square
    A direction_matrix(A matrix);

    template<typename Traits> requires (Traits::size == Traits::dimension + 1)
    transformation_matrix_type<Traits> translation(const math::Vector<Traits>& v);

    template<typename T>
    math::Matrix<T, 2> translation(T tx);

    template<typename T>
    math::Matrix<T, 3> translation(T tx, T ty);

    template<typename T>
    math::Matrix<T, 4> translation(T tx, T ty, T tz);

    template<typename T, size_t N = 3>
    math::Matrix<T, N + 1> scaling(T factor);

    template<typename T>
    math::Matrix<T, 3> scaling(T sx, T sy);

    template<typename T>
    math::Matrix<T, 4> scaling(T sx, T sy, T sz);

    template<typename V>
    transformation_matrix_type<typename V::traits_type> scaling_axis(const V& axis, typename V::value_type sDirectional, typename V::value_type sTransverse);

    // 2D
    template<typename T>
    math::Matrix<T, 3> rotation_2(T angle);

    template<typename T>
    math::Matrix<T, 4> rotation_x3(T angle);

    template<typename T>
    math::Matrix<T, 4> rotation_y3(T angle);

    template<typename T>
    math::Matrix<T, 4> rotation_z3(T angle);

    // experimental ...
    template<typename T>
    math::Matrix<T, 4> rotation_dz(T angle);

    template<typename V>
    transformation_matrix_type<typename V::traits_type> rotation_axis(const V& axis, typename V::value_type angle);

    template<typename V>
    transformation_matrix_type<typename V::traits_type> rotation_axis_unit(const V& axis, typename V::value_type angle);

    // Rotation matrix resulting from rotation of unit vector from a to b
    template<typename V>
    transformation_matrix_type<typename V::traits_type> slerp_unit(const V& a, const V& b, typename V::value_type t);

    // Rotation matrix resulting from rotation of unit vector from a to b
    // The given axis is used as the rotation axis and to determine which of the explementary angles formed by a and b will be traversed by t
    template<typename V>
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
//    transformation_matrix_type<Traits> reflection(const geometry::Line<Traits>& line);

//    template<typename Traits> requires (Traits::dimension == 2)
//    transformation_matrix_type<Traits> reflection(const geometry::Point<Traits>& point);

//    template<typename Traits> requires (Traits::dimension == 3)
//    transformation_matrix_type<Traits> reflection(const geometry::Plane<Traits>& plane);

//    template<typename Traits> requires (Traits::dimension == 3)
//    transformation_matrix_type<Traits> reflection(const geometry::Line<Traits>& line);

//    template<typename Traits> requires (Traits::dimension == 3)
//    transformation_matrix_type<Traits> reflection(const geometry::Point<Traits>& point);

    // Transformation matrix for rotating directionFrom to coincide with directionTo
    // vectors need to be unit length
    // (internally does an axis/angle rotation) ...
    template<typename V>
    transformation_matrix_type<typename V::traits_type> alignment(const V& directionFrom, const V& directionTo);

    // Transformation matrix for rotating upFrom and rightFrom to coincide with upTo and rightTo
    // up and right vectors need to be unit length
    template<typename V>
    transformation_matrix_type<typename V::traits_type> alignment(const V& directionFrom, V rightFrom, const V& directionTo, V rightTo);

    // Fewer operations, would expect to be more accurate ...
    template<typename V>
    transformation_matrix_type<typename V::traits_type> alignment2(const V& directionFrom, const V& rightFrom, const V& directionTo, const V& rightTo);

// deprecated ...
// enabled only until affine_mapping is implemented ...
    // Requires dimension sets of homogeneous points (basis vectors) specified individually
    template<typename V> requires (V::dimension == 2)
    transformation_matrix_type<typename V::traits_type> linear_mapping(const V& a0, const V& a1, const V& b0, const V& b1);

    // Requires dimension sets of homogeneous points (basis vectors)
    template<typename P>
    transformation_matrix_type<typename P::traits_type> linear_mapping(std::span<const P> from, std::span<const P> to);

    // Requires dimension + 1 sets of homogeneous points
    template<typename P>
    transformation_matrix_type<typename P::traits_type> affine_mapping(std::span<const P> from, std::span<const P> to);

    // Requires dimension + 2 sets of homogeneous points
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping(std::span<const P> from, std::span<const P> to);

    // Requires dimension + 2 sets of homogeneous points
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping_i(std::span<const P> from, std::span<const P> to);

    // Requires dimension + 2 sets of homogeneous points
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping_i_e(std::span<const P> from, std::span<const P> to);

    // Requires dimension + 2 sets of homogeneous points
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping_g(std::span<const P> from, std::span<const P> to);

    // Requires dimension + 2 sets of homogeneous points
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping_g_(std::span<const P> from, std::span<const P> to);

    // Requires dimension + 2 sets of homogeneous points
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping_e(std::span<const P> from, std::span<const P> to);

    // Requires dimension + 2 sets of homogeneous points
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping_svd_e(std::span<const P> from, std::span<const P> to);
/*
    // Requires dimension + 2 sets of homogeneous points
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping_o_sim(std::span<const P> from, std::span<const P> to);

    // Requires dimension + 2 sets of homogeneous points
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping_svd_o(std::span<const P> from, std::span<const P> to);

    // Requires dimension + 2 sets of homogeneous points
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping_jsvd_o(std::span<const P> from, std::span<const P> to);
*/
    // Requires 4 sets of dimension 2 homogeneous points
//    template<typename P> requires (P::dimension == 2 && P::size == 3)
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping_h(std::span<const P> from, std::span<const P> to);

    // Requires 4 sets of dimension 2 homogeneous points
//    template<typename P> requires (P::dimension == 2 && P::size == 3)
    template<typename P>
    transformation_matrix_type<typename P::traits_type> projective_mapping_h2(std::span<const P> from, std::span<const P> to);

    // experimental ...
    template<typename V> requires (V::dimension == 3)
    transformation_matrix_type<typename V::traits_type> skew_symmetric_matrix(const V& axis);

    // Build an M x N matrix from points where M is the number of points and N is the size of the point object
    template<typename P, size_t M>
    quetzal::math::Matrix<typename P::value_type, M, P::component_count> to_matrix(std::span<const P> points);

    // Build a transformation (square) matrix with size appropriate for points of type P (dimension + 1 square)
    template<typename P>
    quetzal::geometry::transformation_matrix_type<typename P::traits_type> to_transformation_matrix(std::span<const P> points);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename A> requires A::square
A quetzal::geometry::direction_matrix(A matrix)
{
    // Clear last row of any translation, is this really correct? ...
    for (size_t i = 0; i < A::cols - 1; ++i)
    {
        matrix(A::rows - 1, i) = A::value_type(0);
    }
    assert(matrix(A::rows - 1, A::cols - 1) == A::value_type(1));

    return transpose(inverse(matrix));
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::size == Traits::dimension + 1)
quetzal::geometry::transformation_matrix_type<Traits> quetzal::geometry::translation(const math::Vector<Traits>& v)
{
    auto matrix = transformation_matrix_type<Traits>::Identity();
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
template<typename T, size_t N>
quetzal::math::Matrix<T, N + 1> quetzal::geometry::scaling(T factor)
{
    auto matrix = math::Matrix<T, N + 1>::Identity();
    for (size_t i = 0; i < N; ++i)
    {
        matrix(i, i) = factor;
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
template<typename V>
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::scaling_axis(const V& axis, typename V::value_type sDirectional, typename V::value_type sTransverse)
{
    assert(axis.unit());

    typename V::value_type sDiff = sDirectional - sTransverse;

    auto matrix = transformation_matrix_type<typename V::traits_type>::Identity();
    matrix(0, 0) = sTransverse + sDiff * axis.x() * axis.x();
    matrix(1, 0) = sDiff * axis.x() * axis.y();
    matrix(2, 0) = sDiff * axis.x() * axis.z();
    matrix(0, 1) = sDiff * axis.y() * axis.x();
    matrix(1, 1) = sTransverse + sDiff * axis.y() * axis.y();
    matrix(2, 1) = sDiff * axis.y() * axis.z();
    matrix(0, 2) = sDiff * axis.z() * axis.x();
    matrix(1, 2) = sDiff * axis.z() * axis.y();
    matrix(2, 2) = sTransverse + sDiff * axis.z() * axis.z();
    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 3> quetzal::geometry::rotation_2(T angle)
{
    T c = cos(angle);
    T s = sin(angle);

    auto matrix = math::Matrix<T, 3>::Identity();
    matrix(0, 0) = c;
    matrix(0, 1) = s;
    matrix(1, 0) = -s;
    matrix(1, 1) = c;

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::rotation_x3(T angle)
{
    T c = cos(angle);
    T s = sin(angle);

    auto matrix = math::Matrix<T>::Identity();
    matrix(1, 1) = c;
    matrix(1, 2) = s;
    matrix(2, 1) = -s;
    matrix(2, 2) = c;

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::rotation_y3(T angle)
{
    T c = cos(angle);
    T s = sin(angle);

    auto matrix = math::Matrix<T>::Identity();
    matrix(2, 2) = c;
    matrix(2, 0) = s;
    matrix(0, 2) = -s;
    matrix(0, 0) = c;

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Matrix<T, 4> quetzal::geometry::rotation_z3(T angle)
{
    T c = cos(angle);
    T s = sin(angle);

    auto matrix = math::Matrix<T>::Identity();
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

    auto matrix = math::Matrix<T>::Identity();
    matrix(0, 0) = -s;
    matrix(0, 1) = c;
    matrix(1, 0) = -c;
    matrix(1, 1) = -s;

    return matrix;
}

//------------------------------------------------------------------------------
template<typename V>
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::rotation_axis(const V& axis, typename V::value_type angle)
{
    return rotation_axis_unit(normalize(axis), angle);
}

//------------------------------------------------------------------------------
template<typename V>
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
template<typename V>
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
template<typename V>
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
template<typename V>
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::alignment(const V& directionFrom, const V& directionTo)
{
    assert(directionFrom.unit());
    assert(directionTo.unit());

    if (math::float_eq(directionFrom, directionTo))
    {
        return transformation_matrix_type<typename V::traits_type>::Identity();
    }

    return rotation_axis_unit(normalize(cross(directionFrom, directionTo)), angle_between_unit(directionFrom, directionTo));
}

//------------------------------------------------------------------------------
template<typename V>
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::alignment(const V& directionFrom, V rightFrom, const V& directionTo, V rightTo)
{
    assert(directionFrom.unit());
    assert(rightFrom.unit());
    assert(directionTo.unit());
    assert(rightTo.unit());
    assert(perpendicular(directionFrom, rightFrom));
    assert(perpendicular(directionTo, rightTo));

    using T = V::value_type;

    // Transformation to rotate direction and right From vectors to z-axis and x-axis respectively

    T thetaFrom = angle_between_unit(directionFrom, V::z_direction());
    V axisFrom;
    if (math::float_eq(thetaFrom, math::Pi<T>))
    {
        axisFrom = rightFrom;
    }
    else if (math::float_ne0(thetaFrom))
    {
        axisFrom = normalize(cross(directionFrom, V::z_direction()));
        rightFrom *= rotation_axis_unit(axisFrom, thetaFrom);
    }

    assert(math::float_eq0(rightFrom.z()));
    T phiFrom = atan2(rightFrom.y(), rightFrom.x());

    // Transformation to rotate direction and right To vectors to z-axis and x-axis respectively

    T thetaTo = angle_between_unit(directionTo, V::z_direction());
    V axisTo;
    if (math::float_eq(thetaTo, math::Pi<T>))
    {
        axisTo = rightTo;
    }
    else if (math::float_ne0(thetaTo))
    {
        axisTo = normalize(cross(directionTo, V::z_direction()));
        rightTo *= rotation_axis_unit(axisTo, thetaTo);
    }

    assert(math::float_eq0(rightTo.z()));
    T phiTo = atan2(rightTo.y(), rightTo.x());
    T dphi = phiTo - phiFrom;

    // Transformation matrix

    auto matrix = transformation_matrix_type<typename V::traits_type>::Identity();

    // Rotate From to z-axis orientation
    if (math::float_ne0(thetaFrom))
    {
        matrix = geometry::rotation_axis_unit(axisFrom, thetaFrom);
    }

    // z-axis rotation
    if (math::float_ne0(dphi))
    {
        matrix *= geometry::rotation_z3(dphi);
    }

    // Rotate To from z-axis orientation
    if (math::float_ne0(thetaTo))
    {
        matrix *= geometry::rotation_axis_unit(axisTo, -thetaTo);
    }

    return matrix;
}

//------------------------------------------------------------------------------
template<typename V>
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::alignment2(const V& directionFrom, const V& rightFrom, const V& directionTo, const V& rightTo)
{
    assert(directionFrom.unit());
    assert(rightFrom.unit());
    assert(directionTo.unit());
    assert(rightTo.unit());
    assert(perpendicular(directionFrom, rightFrom));
    assert(perpendicular(directionTo, rightTo));

    using T = V::value_type;

    auto matrix = transformation_matrix_type<typename V::traits_type>::Identity();
    V rightFromAligned = rightFrom;

    T dtheta = angle_between_unit(directionFrom, directionTo);
    if (math::float_ne0(dtheta))
    {
        V axis;
        if (math::float_eq(dtheta, math::Pi<T>))
        {
            axis = rightTo;
        }
        else
        {
            axis = normalize(cross(directionFrom, directionTo));
        }

        matrix = rotation_axis_unit(axis, dtheta);
        rightFromAligned = normalize(rightFrom * matrix);
    }

//    assert(perpendicular(directionTo, rightFromAligned));

    T dphi = angle_full_unit(rightFromAligned, rightTo, directionTo);
    if (math::float_ne0(dphi))
    {
        matrix *= rotation_axis_unit(directionTo, dphi);
    }

//    assert(math::float_eq(directionFrom * matrix, directionTo));
//    assert(math::float_eq(rightFrom * matrix, rightTo));

    return matrix;
}

//------------------------------------------------------------------------------
template<typename V> requires (V::dimension == 2)
quetzal::geometry::transformation_matrix_type<typename V::traits_type> quetzal::geometry::linear_mapping(const V& a0, const V& a1, const V& b0, const V& b1)
{
    // [a] X = [b]
    // X = inverse([a]) [b]

    auto d = a0.x() * a1.y() - a0.y() * a1.x();

    auto m = transformation_matrix_type<typename V::traits_type>::Identity();
    m(0, 0) = (a1.y() * b0.x() - a0.y() * b1.x()) / d;
    m(0, 1) = (a1.y() * b0.y() - a0.y() * b1.y()) / d;
    m(1, 0) = (a0.x() * b1.x() - a1.x() * b0.x()) / d;
    m(1, 1) = (a0.x() * b1.y() - a1.x() * b0.y()) / d;

    return m;
}

//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::linear_mapping(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= P::dimension);
    assert(to.size() >= P::dimension);

    // [from] X = [to]
    // X = inverse([from]) [to]

    auto mapped_from = to_transformation_matrix<P>(from);
    auto mapped_to = to_transformation_matrix<P>(to);

    return inverse(mapped_from) * mapped_to;
}

//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::affine_mapping(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= P::dimension + 1);
    assert(to.size() >= P::dimension + 1);

    // [from] X = [to]
    // X = inverse([from]) [to]

    auto mapped_from = to_transformation_matrix<P>(from);
    auto mapped_to = to_transformation_matrix<P>(to);

    return inverse(mapped_from) * mapped_to;
}

//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping(std::span<const P> from, std::span<const P> to)
{
/*
    assert(from.size() >= P::dimension + 2);
    assert(to.size() >= P::dimension + 2);

    auto mapped_from = to_matrix<P, P::dimension + 2>(from);
    auto mapped_to = to_matrix<P, P::dimension + 2>(to);
*/
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    auto mapped_from = to_matrix<P, 4>(from);
    auto mapped_to = to_matrix<P, 4>(to);

    // [from] X = [to]
    // X = inverse([from]) [to]

//std::cout << "from" << std::endl << to_fixed_width(mapped_from);
auto pif = pseudo_inverse(mapped_from);
std::cout << "pinv(from)" << std::endl << to_fixed_width(pif);
    return pif * mapped_to;
}

//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping_i(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    auto mapped_from = to_matrix<P, 4>(from);
    auto mapped_to = to_matrix<P, 4>(to);

    // [from] X = [to]
    // X = inverse([from]) [to]

//std::cout << "from" << std::endl << to_fixed_width(mapped_from);
auto pif = inverse(mapped_from);
std::cout << "inv(from)" << std::endl << to_fixed_width(pif);
    return pif * mapped_to;
}

//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping_i_e(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    auto mapped_from = to_matrix<P, 4>(from);
    auto mapped_to = to_matrix<P, 4>(to);

    // [from] X = [to]
    // X = inverse([from]) [to]

//std::cout << "from" << std::endl << to_fixed_width(mapped_from);
auto pif = inverse_e(mapped_from);
std::cout << "inv(from)" << std::endl << to_fixed_width(pif);
    return pif * mapped_to;
}

//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping_g(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    auto mapped_from = to_matrix<P, 4>(from);
    auto mapped_to = to_matrix<P, 4>(to);

//std::cout << "from" << std::endl << to_fixed_width(mapped_from);
auto pif = pseudo_inverse_g(mapped_from);
std::cout << "pinv(from)" << std::endl << to_fixed_width(pif);
    return pif * mapped_to;
}

//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping_g_(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    auto mapped_from = to_matrix<P, 4>(from);
    auto mapped_to = to_matrix<P, 4>(to);

//std::cout << "from" << std::endl << to_fixed_width(mapped_from);
auto pif = pseudo_inverse_g_(mapped_from);
std::cout << "pinv(from)" << std::endl << to_fixed_width(pif);
    return pif * mapped_to;
}

//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping_e(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    auto mapped_from = to_matrix<P, 4>(from);
    auto mapped_to = to_matrix<P, 4>(to);

//std::cout << "from" << std::endl << to_fixed_width(mapped_from);
auto pif = pseudo_inverse_e(mapped_from);
std::cout << "pinv(from)" << std::endl << to_fixed_width(pif);
    return pif * mapped_to;
}

//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping_svd_e(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    auto mapped_from = to_matrix<P, 4>(from);
    auto mapped_to = to_matrix<P, 4>(to);

//std::cout << "from" << std::endl << to_fixed_width(mapped_from);
auto pif = pseudo_inverse_svd_e(mapped_from);
std::cout << "pinv(from)" << std::endl << to_fixed_width(pif);
    return pif * mapped_to;
}
/*
//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping_o_sim(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    return
    {
//        -5000.447296456286, -5.287020966843596e-13, 0, 2.91038304567337e-11,
//        -18661.99398561024, -5.89546813381036e-12, 0, 0,
//        -18661.99398561022, -0.004551601645727838, 0, 0,
//        2549275.283419319, 0.5608800803303161, 0, 1,

        -5000.447322853361, -2.892530948300874e-11, 0, -1.164153218269348e-10,
        -18661.99408412591, -1.073048550134144e-10, 0, -3.492459654808044e-10,
       -18661.99408412589, -0.004551601751991744, 0, -2.328306436538696e-10,
        2549275.296876805, 0.5608800949692169, 0, 1.000000029802322,
    };
}

//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping_svd_o(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    auto mapped_from = to_matrix<P, 4>(from);
    auto mapped_to = to_matrix<P, 4>(to);

//std::cout << "from" << std::endl << to_fixed_width(mapped_from);
auto pif = pseudo_inverse_svd_o(mapped_from);
std::cout << "pinv(from)" << std::endl << to_fixed_width(pif);
    return pif * mapped_to;
}

//------------------------------------------------------------------------------
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping_jsvd_o(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    auto mapped_from = to_matrix<P, 4>(from);
    auto mapped_to = to_matrix<P, 4>(to);

//std::cout << "from" << std::endl << to_fixed_width(mapped_from);
auto pif = pseudo_inverse_jsvd_o(mapped_from);
std::cout << "pinv(from)" << std::endl << to_fixed_width(pif);
    return pif * mapped_to;
}
*/
//------------------------------------------------------------------------------
//template<typename P> requires (P::dimension == 2 && P::size == 3)
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping_h(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    using T = P::value_type;

    // Construct the matrix for 8 equations and 9 unknowns
    Eigen::Matrix<T, 8, 9> A;
    for (int i = 0; i < 4; ++i)
    {
        T x = from[i].x();
        T y = from[i].y();
        T x_prime = to[i].x();
        T y_prime = to[i].y();

        A.row(2 * i) << -x, -y, -1, 0, 0, 0, x * x_prime, y * x_prime, x_prime;
        A.row(2 * i + 1) << 0, 0, 0, -x, -y, -1, x * y_prime, y * y_prime, y_prime;
    }

    Eigen::JacobiSVD<Eigen::Matrix<T, 8, 9>> svd(A, Eigen::ComputeFullV);
    Eigen::Matrix<T, 9, 1> h = svd.matrixV().col(8); // The last column of V gives the solution

    transformation_matrix_type<typename P::traits_type> matrix;
    auto data = matrix.data();
    for (int i = 0; i < 8; ++i)
    {
        data[i] = h[i] / h[8];
    }
    data[8] = T(1);

    return matrix.transpose();
}

//------------------------------------------------------------------------------
//template<typename P> requires (P::dimension == 2 && P::size == 3)
template<typename P>
quetzal::geometry::transformation_matrix_type<typename P::traits_type> quetzal::geometry::projective_mapping_h2(std::span<const P> from, std::span<const P> to)
{
    assert(from.size() >= 4);
    assert(to.size() >= 4);

    using T = P::value_type;

    // Construct the A matrix
    Eigen::Matrix<T, 8, 8> A;
    Eigen::Vector<T, 8> b;
    for (int i = 0; i < 4; ++i)
    {
        T x = from[i].x();
        T y = from[i].y();
        T x_prime = to[i].x();
        T y_prime = to[i].y();

        A.row(2 * i) << x, y, 1, 0, 0, 0, -x * x_prime, -y * x_prime;
        b(2 * i) = x_prime;

        A.row(2 * i + 1) << 0, 0, 0, x, y, 1, -x * y_prime, -y * y_prime;
        b(2 * i + 1) = y_prime;
    }

    // Solve the linear system A * h = b
    Eigen::Vector<T, Eigen::Dynamic> h = A.colPivHouseholderQr().solve(b);

    transformation_matrix_type<typename P::traits_type> matrix;
    auto data = matrix.data();
    for (int i = 0; i < 8; ++i)
    {
        data[i] = h[i];
    }
    data[8] = T(1);

    return matrix.transpose();
}

//------------------------------------------------------------------------------
template<typename V> requires (V::dimension == 3)
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
template<typename P, size_t M>
quetzal::math::Matrix<typename P::value_type, M, P::component_count> quetzal::geometry::to_matrix(std::span<const P> points)
{
    assert(points.size() >= M);

    math::Matrix<typename P::value_type, M, P::component_count> matrix;
    for (size_t i = 0; i < M; ++i)
    {
        for (size_t j = 0; j < P::component_count; ++j)
        {
            matrix(i, j) = points[i][j];
        }
    }

    return matrix;
}

//------------------------------------------------------------------------------
template<typename P>
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
