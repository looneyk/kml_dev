//------------------------------------------------------------------------------
// wimble
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using point_type = typename mesh_type::point_type;
    using vector_type = typename point_type::vector_type;
    using size_type = size_t; // ...

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "wimble";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    geometry::Polygon<vector_traits> create_cross_section_circular(size_t nFlutes, size_t nt, float rInner, float rOuter)
    {
        assert(rOuter > rInner);

        size_t nPointsFlute = 2 * nt + 1;
        geometry::Polygon<vector_traits> flute(nFlutes * nPointsFlute);

        float wBase = math::PiTwo<float> * rInner / nFlutes;
        float rArc = (rOuter * rOuter - rInner * rInner) / (2.0f * rOuter);
        float phi0 = -math::PiHalf<float>;
        float phi1 = atan2(rArc, rInner);
        phi1 += math::PiTwo<float> / (nFlutes * nFlutes);
        point_type center = {rInner, rArc, 0.0f};

        for (size_t j = 0; j <= nt; ++j)
        {
            float tj = float(j) / nt;
            float phi = math::lerp(phi0, phi1, tj);
            float w2 = 0.5f * math::lerp(wBase, 0.0f, tj);
            float cos_phi = cos(phi);
            float sin_phi = sin(phi);

            if (j < nt)
            {
                flute.vertex(j) = center + vector_type{(rArc + w2) * cos_phi, (rArc + w2) * sin_phi, 0.0f};
                flute.vertex(nPointsFlute - 1 - j) = center + vector_type{(rArc - w2) * cos_phi, (rArc - w2) * sin_phi, 0.0f};
            }
            else
            {
                flute.vertex(j) = center + vector_type{rArc * cos_phi, rArc * sin_phi, 0.0f};
            }
        }

        geometry::Polygon<vector_traits> polygon(nFlutes * nPointsFlute);
        typename geometry::Polygon<vector_traits>::sections_type::value_type sections;

        for (size_t i = 0; i < nFlutes; ++i)
        {
            size_t n = i * nPointsFlute;
            float phi = math::PiTwo<float> * i / nFlutes;
            auto matrix = geometry::rotation_z(phi);

            for (size_t j = 0; j < nPointsFlute; ++j)
            {
                polygon.vertex(n + j) = flute.vertex(j) * matrix;
            }

            sections.push_back(n);
            sections.push_back(n + nt);
        }
        
        polygon.sections() = std::move(sections);
        return polygon;
    }

    //--------------------------------------------------------------------------
    geometry::Polygon<vector_traits> create_cross_section_spiral(size_t nFlutes, size_t nt, float rInner, float rOuter, float arc)
    {
        assert(rOuter > rInner);
        assert(arc > 0.0f);

        size_t nPointsFlute = 2 * nt + 1;
        geometry::Polygon<vector_traits> polygon(nFlutes * nPointsFlute);

        float dr = rOuter - rInner;
        float a = dr / arc; // maybe pass in a directly instead of arc ...
        float wBase = math::PiTwo<float> * rInner / nFlutes;
cout << dr << "\t" << a << "\t" << wBase << endl;

        for (size_t i = 0; i < nFlutes; ++i)
        {
            float ti = float(i) / nFlutes;
            size_t ni = i * nPointsFlute;

            // looks like tangent is bad below, big surprize, but not too bad oddly enough ...

            for (size_t j = 0; j <= nt; ++j)
            {
                float tj = float(j) / nt;
                float r = rInner + a * tj;
r /= 2.0f;
                float t = ti + tj;
//                float phi = math::PiTwo<float> * t;
float phi = math::PiTwo<float> * ti + tj; // ...
                point_type point = {r * cos(phi), r * sin(phi), 0.0f};
                vector_type tangent = {a * cos(phi) - r * sin(phi) * math::PiTwo<float>, a * sin(phi) + r * cos(phi) * math::PiTwo<float>, 0.0f};
                vector_type offset = {-tangent.y(), tangent.x(), 0.0f};
                offset.normalize();
cout << j << "\t" << tj << "\t" << r << "\t" << t << "\t" << phi << endl;

                float w2 = 0.5f * math::lerp(wBase, 0.0f, tj);
                if (j < nt)
                {
                    polygon.vertex(ni + j) = point - w2 * offset;
cout << i << "\t" << ni << "\t" << ti << "\t" << j << "\t" << tj << "\t" << (ni + j) << "\t" << polygon.vertex(ni + j) << endl;
                    polygon.vertex(ni + nPointsFlute - 1 - j) = point + w2 * offset;
cout << i << "\t" << ni << "\t" << ti << "\t" << j << "\t" << tj << "\t" << (ni + nPointsFlute - 1 - j) << "\t" << polygon.vertex(ni + nPointsFlute - 1 - j) << endl;
                }
                else
                {
                    polygon.vertex(ni + j) = point;
cout << i << "\t" << ni << "\t" << ti << "\t" << j << "\t" << tj << "\t" << (ni + j) << "\t" << polygon.vertex(ni + j) << endl;
                }
            }
        }

        return polygon;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    FileValidator validator(dir_out, dir_ref, app_name);

    string name;
    mesh_type mesh;
    mesh_type m;
    geometry::Polygon<vector_traits> polygon;

    float da = 1.0f / 3.0f;

    name = "cylinder_4_4";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(4, 4, 10.0f, 40.0f);
    model::create_cylinder(mesh, name, polygon, 3, 0.0f, 100.0f,
        [](float) { return 1.0f; },
        [](float) { return 0.0f; }
    );
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_4_4_sections";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(4, 4, 10.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, polygon, 3, 0.0f, 100.0f,
        [](float) { return 1.0f; },
        [](float) { return 0.0f; }
    );
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "wimble_4_4_conical";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(4, 4, 10.0f, 40.0f);
    model::create_cylinder(mesh, name, polygon, 3, 0.0f, 100.0f,
        [](float t) { return 1.0f - t; },
        [](float) { return 0.0f; }
    );
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "wimble_4_4_conical_sections";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(4, 4, 10.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, polygon, 3, 0.0f, 100.0f,
        [](float t) { return 1.0f - t; },
        [](float) { return 0.0f; }
    );
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "wimble_6_10_conical";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(6, 20, 10.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, polygon, 6, 0.0f, 100.0f,
        [](float t) { return 1.0f - t; },
        [](float) { return 0.0f; }
    );
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "wimble_6_15_conical";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(6, 20, 15.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, polygon, 6, 0.0f, 100.0f,
        [](float t) { return 1.0f - t; },
        [](float) { return 0.0f; }
    );
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "wimble_6_20_conical";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(6, 20, 20.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, polygon, 6, 0.0f, 100.0f,
        [](float t) { return 1.0f - t; },
        [](float) { return 0.0f; }
    );
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "wimble_6_10_twisted";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(6, 20, 10.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, polygon, 20, 0.0f, 100.0f,
        [](float t) { return 1.0f - t; },
        [=](float t) { return da * t; });
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "wimble_6_15_twisted";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(6, 20, 15.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, polygon, 20, 0.0f, 100.0f,
        [](float t) { return 1.0f - t; },
        [=](float t) { return da * t; });
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "wimble_6_20_twisted";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(6, 20, 20.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, polygon, 20, 0.0f, 100.0f,
        [](float t) { return 1.0f - t; },
        [=](float t) { return da * t; });
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "wimble_6_10_bulbous";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(6, 20, 10.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, polygon, 20, 0.0f, 100.0f,
        [](float t) { return -25.0f / 9.0f * t * t + 20.0f / 9.0f * t + 5.0f / 9.0f; },
        [=](float t) { return da * t; });
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "wimble_6_15_bulbous";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(6, 20, 15.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, polygon, 20, 0.0f, 100.0f,
        [](float t) { return -25.0f / 9.0f * t * t + 20.0f / 9.0f * t + 5.0f / 9.0f; },
        [=](float t) { return da * t; });
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "wimble_6_20_bulbous";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(6, 20, 20.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, polygon, 20, 0.0f, 100.0f,
        [](float t) { return -25.0f / 9.0f * t * t + 20.0f / 9.0f * t + 5.0f / 9.0f; },
        [=](float t) { return da * t; });
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
/*
    const size_t resolution = 100;
    name = "wimble_6_15_high";
    cout << name << endl;
    mesh.clear();
    polygon = create_cross_section_circular(6, resolution, 15.0f, 40.0f);
    model::create_cylinder_sections(mesh, name, resolution, 0.0f, 100.0f, polygon,
//        [](float t) { return 1.0f - t; },
        [](float t) { return -25.0f / 9.0f * t * t + 20.0f / 9.0f * t + 5.0f / 9.0f; },
        [](float t) { return t / 3.0f; });

    // Base
    m.clear();
    model::create_geodesic_sphere(m, "base", 22.3f, 36);
    clip(m, geometry::Plane<vector_traits>({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}));
    model::transform(m, math::scaling(1.0f, 1.0f, 0.25f));
    mesh.insert(m);

    model::transform(mesh, math::scaling(0.75f));
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/
    validator.report_results();
    return 0;
}
