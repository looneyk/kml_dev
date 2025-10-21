//------------------------------------------------------------------------------
// hyper_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/math/math_util_dx.hpp"
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
	using mesh_type = brep::Mesh<brep::MeshTraits<value_type>>;

	const std::string dirOut = "out/";

	//--------------------------------------------------------------------------
	template<typename M>
	void create_hyper0(M& mesh, model::size_type nAzimuth, model::size_type nz, model::value_type<M> radius, model::value_type<M> dz)
	{
		M m0;
		model::create_cylinder(m0, nAzimuth, nz, radius, 0.0f, 0.0f, dz); // dz based on ring? ...

        value_type tPhi0 = 0.0f;
        value_type tTheta = -0.2f;

        // Ring loop
        for (size_t i = 0; i < 6; ++i)
        {
            value_type rBase = dz * (6 - i) / 24.0f;

            // Ring spike loop
            for (size_t j = 0; j < 6; ++j)
            {
                value_type tPhi = tPhi0 + j * 1.0f / 6.0f;
                value_type xBase = rBase * cos(tPhi * math::PiTwo<value_type>);
                value_type yBase = rBase * sin(tPhi * math::PiTwo<value_type>);

                M m = m0;
                math::Matrix<value_type> mTheta = math::rotation_normal({0.0f, 1.0f, 0.0f}, tTheta * math::PiTwo<value_type>);
                math::Matrix<value_type> mPhi = math::rotation_normal({0.0f, 0.0f, 1.0f}, tPhi * math::PiTwo<value_type>);
                math::Matrix<value_type> mTranslation = math::translation(xBase, yBase, 0.0f);
                model::transform(m, mTheta);
                model::transform(m, mPhi);
                model::transform(m, mTranslation);

                mesh.insert(m);
            }

            tPhi0 += 1.0f / 36.0f;
            tTheta += 0.04f;
        }

		mesh.check();
		return;
	}

	//--------------------------------------------------------------------------
	template<typename M>
	void create_hyper1(M& mesh, model::size_type nAzimuth, model::size_type nz, model::value_type<M> radius, model::value_type<M> scale, model::value_type<M> dz0 = 1.0f, model::value_type<M> dz1 = 1.0f, model::value_type<M> theta0 = -0.2f, model::value_type<M> theta1 = 0.0f)
	{
        value_type tPhi0 = 0.0f;
        value_type tTheta = theta0;
        value_type dTheta = (theta1 - theta0) / 5.0f;

        // Ring loop
        for (size_t i = 0; i < 6; ++i)
        {
            value_type length = scale;
            length *= dz0 + (dz1 - dz0) * (i / 5.0f); // 5 = max i

    		M m0;
    		model::create_cylinder(m0, nAzimuth, nz, radius, 0.0f, 0.0f, length);

            value_type rBase = scale * (6 - i) / 24.0f;

            // Ring spike loop
            for (size_t j = 0; j < 6; ++j)
            {
                value_type tPhi = tPhi0 + j * 1.0f / 6.0f;
                value_type xBase = rBase * cos(tPhi * math::PiTwo<value_type>);
                value_type yBase = rBase * sin(tPhi * math::PiTwo<value_type>);

                M m = m0;
                math::Matrix<value_type> mTheta = math::rotation_normal({0.0f, 1.0f, 0.0f}, tTheta * math::PiTwo<value_type>);
                math::Matrix<value_type> mPhi = math::rotation_normal({0.0f, 0.0f, 1.0f}, tPhi * math::PiTwo<value_type>);
                math::Matrix<value_type> mTranslation = math::translation(xBase, yBase, 0.0f);
                model::transform(m, mTheta);
                model::transform(m, mPhi);
                model::transform(m, mTranslation);

                mesh.insert(m);
            }

            tPhi0 += 1.0f / 36.0f;
            tTheta += dTheta;
        }

		mesh.check();
		return;
	}

	//--------------------------------------------------------------------------
	template<typename M>
	void create_hyper2(M& mesh, model::size_type nAzimuth, model::size_type nz, model::value_type<M> radius, model::value_type<M> scale, model::value_type<M> dz0 = 1.0f, model::value_type<M> dz1 = 1.0f, model::value_type<M> theta0 = -0.2f, model::value_type<M> theta1 = 0.0f)
	{
        value_type tPhi0 = 0.0f;
        value_type tTheta = theta0;
        value_type dTheta = (theta1 - theta0) / 5.0f;

        // Ring loop
        for (size_t i = 0; i < 6; ++i)
        {
            value_type length = scale;
            length *= dz0 + (dz1 - dz0) * (i / 5.0f); // 5 = max i

    		M m0;
    		model::create_cylinder(m0, nAzimuth, nz, radius, 0.25f * radius, 0.0f, length);

            value_type rBase = scale * (6 - i) / 24.0f;

            // Ring spike loop
            for (size_t j = 0; j < 6; ++j)
            {
                value_type tPhi = tPhi0 + j * 1.0f / 6.0f;
                value_type xBase = rBase * cos(tPhi * math::PiTwo<value_type>);
                value_type yBase = rBase * sin(tPhi * math::PiTwo<value_type>);

                M m = m0;
                math::Matrix<value_type> mTheta = math::rotation_normal({0.0f, 1.0f, 0.0f}, tTheta * math::PiTwo<value_type>);
                math::Matrix<value_type> mPhi = math::rotation_normal({0.0f, 0.0f, 1.0f}, tPhi * math::PiTwo<value_type>);
                math::Matrix<value_type> mTranslation = math::translation(xBase, yBase, 0.0f);
                model::transform(m, math::scaling(0.25f, 1.0f, 1.0f)); // Flatten leaf blades
                model::transform(m, math::translation(0.0f, radius, 0.0f)); // Offset so spikes don't meet at center
                model::transform(m, mTheta);
                model::transform(m, mPhi);
                model::transform(m, mTranslation);

                mesh.insert(m);
            }

            tPhi0 += 1.0f / 36.0f;
            tTheta += dTheta;
        }

		mesh.check();
		return;
	}

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	argc;
	argv;

	string name;
	mesh_type mesh;

	name = "hyper0.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper0(mesh, 3, 10, 2.5f, 150.0f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper1_scale_full.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper1(mesh, 3, 10, 2.5f, 150.0f, 1.0f, 1.0f, -0.2f, 0.0f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper1_scale_half.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper1(mesh, 3, 10, 2.5f, 150.0f, 0.5f, 1.0f, -0.2f, 0.0f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper1_scale_third.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper1(mesh, 3, 10, 2.5f, 150.0f, 0.333f, 1.0f, -0.2f, 0.0f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper1_scale_quarter.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper1(mesh, 3, 10, 2.5f, 150.0f, 0.25f, 1.0f, -0.2f, 0.0f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper1_scale_full_3.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper1(mesh, 3, 10, 2.5f, 150.0f, 1.0f, 1.0f, -0.2f, 0.05f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper1_scale_half_3.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper1(mesh, 3, 10, 2.5f, 150.0f, 0.5f, 1.0f, -0.2f, 0.05f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper1_scale_third_3.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper1(mesh, 3, 10, 2.5f, 150.0f, 0.333f, 1.0f, -0.2f, 0.05f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper1_scale_quarter_3.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper1(mesh, 3, 10, 2.5f, 150.0f, 0.25f, 1.0f, -0.2f, 0.05f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper2_scale_full.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper2(mesh, 6, 10, 2.5f, 150.0f, 1.0f, 1.0f, -0.2f, 0.0f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper2_scale_half.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper2(mesh, 6, 10, 2.5f, 150.0f, 0.5f, 1.0f, -0.2f, 0.0f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper2_scale_third.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper2(mesh, 6, 10, 2.5f, 150.0f, 0.333f, 1.0f, -0.2f, 0.0f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper2_scale_quarter.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper2(mesh, 6, 10, 2.5f, 150.0f, 0.25f, 1.0f, -0.2f, 0.0f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper2_scale_full_3.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper2(mesh, 6, 10, 2.5f, 150.0f, 1.0f, 1.0f, -0.2f, 0.05f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper2_scale_half_3.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper2(mesh, 6, 10, 2.5f, 150.0f, 0.5f, 1.0f, -0.2f, 0.05f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper2_scale_third_3.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper2(mesh, 6, 10, 2.5f, 150.0f, 0.333f, 1.0f, -0.2f, 0.05f);
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	name = "hyper2_scale_quarter_3.obj";
	cout << name << endl;
	mesh.clear();
	create_hyper2(mesh, 6, 10, 2.5f, 150.0f, 0.25f, 0.5f, -0.2f, 0.05f); // 0.5f ...
	triangulate(mesh);
	write_obj(mesh, dirOut + name);

	return 0;
}
