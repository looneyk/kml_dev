//------------------------------------------------------------------------------
// math_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/math/Matrix.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/VectorTraits.hpp"
#include "quetzal/math/math_util.hpp"
#include "quetzal/math/transformation_matrix.hpp"
#include <string>
#include <iostream>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector3_traits = math::VectorTraits<value_type, 3>;
    using vector3_type = math::Vector<vector3_traits>;
    using matrix_type = math::Matrix<value_type>;

    using vector2_traits = math::VectorTraits<value_type, 2>;
    using vector2_type = math::Vector<vector2_traits>;

}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

//    vector3_type v2 {0.0f, 0.0f};
    vector3_type v3 {0.0f, 0.0f, 0.0f};
//    vector3_type v4 {0.0f, 0.0f, 0.0f, 0.0f};

    value_type x = v3.get<0>();
    value_type y = v3.get<1>();
    value_type z = v3.get<2>();
//    value_type w = v3.get<3>();

    v3.set<0>(0.0f);
    v3.set<1>(0.0f);
    v3.set<2>(0.0f);
//    v3.set<3>(0.0f);

    x = v3.x();
    y = v3.y();
    z = v3.z();
//    w = v3.w();

    v3.set_x(0.0f);
    v3.set_y(0.0f);
    v3.set_z(0.0f);
//    v3.set_w(0.0f);

//    v3.set(0.0f);
//    v3.set(0.0f, 0.0f);
    v3.set(0.0f, 0.0f, 0.0f);
//    v3.set(0.0f, 0.0f, 0.0f, 0.0f);

    vector3_type position0 = {0.0f, 0.0f, 0.0f};
    vector3_type position1 = {1.0f, 0.0f, 0.0f};
    vector3_type position2 = {0.0f, 1.0f, 0.0f};
    vector3_type normal = {0.0f, 0.0f, 1.0f};

    auto v0 = position1 - position0;
    auto v1 = position2 - position1;
    cout << "[" << v0  << "] [" << v1 << "]" << endl;
    auto c = cross(v0, v1);
    cout << "c [" << c << "]" << endl;
    auto d = dot(normal, c);
    cout << "d " << d << endl;

    v1 = position1 - position0;
    v0 = position2 - position1;
    cout << "[" << v0  << "] [" << v1 << "]" << endl;
    c = cross(v0, v1);
    cout << "c [" << c << "]" << endl;
    d = dot(normal, c);
    cout << "d " << d << endl;

//    return dot(mesh.halfedge(id).face_value().normal(), cross(position1 - position0, position2 - position1)) < 0;

    vector3_type axisX = {1.0f, 0.0f, 0.0f};
    vector3_type axisY = {0.0f, 1.0f, 0.0f};
    vector3_type axisZ = {0.0f, 0.0f, 1.0f};

    matrix_type matrixX = math::rotation_x(math::PiHalf<value_type>);
    matrix_type matrixY = math::rotation_y(math::PiHalf<value_type>);
    matrix_type matrixZ = math::rotation_z(math::PiHalf<value_type>);

    vector3_type axis = {1.0f, 1.0f, 1.0f};
    matrix_type matrix = math::rotation_axis(axis, math::PiTwoThird<value_type>);

    vector3_type rx = axisX * matrixZ;
    vector3_type ry = axisY * matrixX;
    vector3_type rz = axisZ * matrixY;

    vector3_type rotX = axisX * matrix;
    vector3_type rotY = axisY * matrix;
    vector3_type rotZ = axisZ * matrix;

    assert(vector_eq(rx, axisY));
    assert(vector_eq(ry, axisZ));
    assert(vector_eq(rz, axisX));

    assert(vector_eq(rotX, axisY));
    assert(vector_eq(rotY, axisZ));
    assert(vector_eq(rotZ, axisX));

    vector2_type p0 = {1.0f, 2.0f};
    vector2_type p1 = {3.0f, 4.0f};

    vector2_type t0 = {0.25f, 0.5f};
    vector2_type t1 = {0.75f, 1.0f};

    auto m = change_of_basis(p0, p1, t0, t1);
    assert(vector_eq(p0 * m, t0));
    assert(vector_eq(p1 * m, t1));

    string s;
    getline(cin, s);

    return 0;
}
