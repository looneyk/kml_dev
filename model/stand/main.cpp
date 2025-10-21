//------------------------------------------------------------------------------
// stand
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/transformation_matrix.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <string>
#include <iostream>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename(const string& name)
    {
        return dirOut + name + ".obj";
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void create_stand_(M& mesh, size_t nStruts, value_type rMajorBottom, value_type rMinorBottom, value_type rMajorTop, value_type rMinorTop, value_type dzStrut)
    {
//        size_t nMajorBottom = 320;
//        size_t nMinorBottom = 50;
//        size_t nMajorTop = 200;
//        size_t nMinorTop = 25;
//        size_t nAzimuthStrut = 40;

        size_t nMajorBottom = 200;
        size_t nMinorBottom = 40;
        size_t nMajorTop = 200;
        size_t nMinorTop = 30;
        size_t nAzimuthStrut = 40;

        M m;
        math::Matrix<value_type> matrix;

        m.clear();
        model::create_torus(m, "base", nMajorBottom, nMinorBottom, rMajorBottom, rMinorBottom);
//model::create_torus(m, nMajorBottom / 2, nMinorBottom / 2, rMajorBottom, rMinorBottom);
//model::create_torus(m, nMajorBottom, nMinorBottom, rMajorBottom, rMinorBottom, model::ExtentEndsFlat(-0.05f, 0.05f));
//model::create_torus(m, nMajorBottom, nMinorBottom, rMajorBottom, rMinorBottom, model::ExtentEndsFlat(0.28f, 0.38f));
//model::create_torus(m, nMajorBottom, nMinorBottom, rMajorBottom, rMinorBottom, model::ExtentEndsFlat(0.62f, 0.72f));
//model::create_torus(m, nMajorBottom, nMinorBottom, rMajorBottom, rMinorBottom, model::ExtentEndsFlat(0.0f, 0.999f));
        mesh.insert(m);

        m.clear();
        model::create_torus(m, "top", nMajorTop, nMinorTop, rMajorTop, rMinorTop);
        matrix = math::translation(0.0f, 0.0f, dzStrut * sqrt(3.0f) / 2.0f);
        model::transform_position(m, matrix);
        mesh.insert(m);

        M mStrut;
        model::create_cylinder(mStrut, "strut", nAzimuthStrut, 1, rMinorBottom, rMinorTop, 0.0f, dzStrut);
dump(mStrut);
        typename M::vector_type axis_y = {0.0f, 1.0f, 0.0f};
        matrix = math::rotation_axis(axis_y, -1.0f / 12.0f * math::PiTwo<value_type>); // pi ...
        model::transform(mStrut, matrix);
        matrix = math::translation(rMajorBottom, 0.0f, 0.0f);
        model::transform_position(mStrut, matrix);

        for (size_t i = 0; i < nStruts; ++i)
        {
            m = mStrut;
            m.rename_submesh(0, "strut_" + to_string(i));
            typename M::vector_type axis_z = {0.0f, 0.0f, 1.0f};
            matrix = math::rotation_axis(axis_z, value_type(i) / value_type(nStruts) * math::PiTwo<value_type>); // pi ...
            model::transform(m, matrix);
            mesh.insert(m);
        }

        mesh_type::point_type point{0.0f, 0.0f, 0.0f};
//mesh_type::point_type point{0.0f, 0.0f, -1.5f};
        clip(mesh, geometry::Plane(point, {0.0f, 0.0f, -1.0f}), "slice");

        mesh.check();
        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void create_stand(M& mesh, size_t nStruts, value_type rMajorBottom, value_type rMinorBottom, value_type rMajorTop, value_type rMinorTop, value_type height)
    {
        size_t nMajorBottom = 200;
        size_t nMinorBottom = 40;
        size_t nMajorTop = 200;
        size_t nMinorTop = 30;
        size_t nAzimuthStrut = 40;

//nMajorBottom = 40;
//nMinorBottom = 10;
//nMajorTop = 40;
//nMinorTop = 10;
//nAzimuthStrut = 10;

        M m;
        math::Matrix<value_type> matrix;

        m.clear();
        model::create_torus(m, "base", nMajorBottom, nMinorBottom, rMajorBottom, rMinorBottom);
        mesh.insert(m);

        m.clear();
        model::create_torus(m, "top", nMajorTop, nMinorTop, rMajorTop, rMinorTop);
        matrix = math::translation(0.0f, 0.0f, height);
        model::transform_position(m, matrix);
        mesh.insert(m);

        typename M::vector_type vStrutBottom = {rMajorBottom, 0.0f, 0.0f};
        typename M::vector_type vStrutTop = {rMajorTop, 0.0f, height};
        value_type dz = (vStrutTop - vStrutBottom).norm();

        M mStrut;
        model::create_cylinder(mStrut, "strut", nAzimuthStrut, 1, rMinorBottom, rMinorTop, 0.0f, dz);
        typename M::vector_type axis_y = {0.0f, 1.0f, 0.0f};
        value_type angle = atan2(vStrutTop.x() - vStrutBottom.x(), vStrutTop.z() - vStrutBottom.z());
        matrix = math::rotation_axis(axis_y, angle) * math::translation(rMajorBottom, 0.0f, 0.0f);
        model::transform(mStrut, matrix);

        for (size_t i = 0; i < nStruts; ++i)
        {
            m = mStrut;
            m.rename_submesh(0, "strut_" + to_string(i));
            matrix = math::rotation_z3(value_type(i) / value_type(nStruts) * math::PiTwo<value_type>);
            model::transform(m, matrix);
            mesh.insert(m);
        }

        clip(mesh, geometry::HalfSpace<M::vector_traits>({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}));

        mesh.check();
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    FileValidator validator(dirOut, dirReference);

    string name;
    mesh_type mesh;
/*
    name = "stand_50";
    cout << name << endl;
    mesh.clear();
    create_stand_(mesh, 1, 40.0f, 5.0f, 25.0f, 2.5f, 30.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "stand_65";
    cout << name << endl;
    mesh.clear();
    create_stand_(mesh, 1, 40.0f, 3.75f, 32.5f, 2.5f, 15.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "stand";
    cout << name << endl;
    mesh.clear();
    create_stand(mesh, 3, 40.0f, 3.75f, 32.5f, 2.5f, 15.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/
    name = "stand_25";
    cout << name << endl;
    mesh.clear();
    create_stand(mesh, 3, 12.0f, 2.5f, 9.0f, 1.8f, 12.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
