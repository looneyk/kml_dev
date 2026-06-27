//------------------------------------------------------------------------------
// model_pattern
// main.cpp
//------------------------------------------------------------------------------

#include "Pages.hpp"
#include "Pane.hpp"
#include "Panes.hpp"
#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/geometry.hpp"
#include "quetzal/common/Exception.hpp"
//#include "quetzal/common/string_util.hpp"
#include "quetzal/geometry/Accessor.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/Projection.hpp"
#include "quetzal/geometry/points_util.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/math_util.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <sstream>
#include <cassert>

#include "quetzal/brep/validation.hpp"
#include "quetzal/geometry/point_util.hpp"

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using vertex_attributes_type = geometry::PositionNormalTexture<vector_traits>; // Only requires position and texture ...
    using mesh_traits = brep::MeshTraits<vector_traits, vertex_attributes_type>;
    using mesh_type = brep::Mesh<mesh_traits>;
    using face_type = mesh_type::face_type;
    using vertex_type = mesh_type::vertex_attributes_type;
    using vertices_type = vector<vertex_type>;
    using matrix_type = geometry::transformation_matrix_type<vector_traits>;
    using point_type = mesh_type::point_type;
    using vector_type = point_type::vector_type;

    using vector2_traits = vector_traits::template resized_traits<vector_traits::dimension - 1>;
    using matrix2_type = geometry::transformation_matrix_type<vector2_traits>;
    using point2_type = vertex_attributes_type::texcoord_type;
    using vector2_type = point2_type::vector_type;

    using matrix3_type = geometry::transformation_matrix_type<vector_traits>;
    using point3_type = point_type;
    using vector3_type = point3_type::vector_type;

    namespace g = quetzal::geometry;

    const vector2_type page_size_Letter = {215.9, 279.4};
    const vector2_type page_size_Legal = {215.9, 355.6};
    const vector2_type page_size_A4 = {210.0, 297.0};

    //--------------------------------------------------------------------------
    value_type to_inches(value_type mm)
    {
        return mm / (25.4 * 25.4);
    }

    //--------------------------------------------------------------------------
    filesystem::path output_path(filesystem::path input_path)
    {
        if (input_path.extension() == ".obj")
        {
            input_path = input_path.stem();
        }

        return input_path / ".pdf";
    }

    //--------------------------------------------------------------------------
    template<typename T>
    struct geometry::PositionAccessor<vector<T>>
    {
        using value_type = vertices_type::value_type::value_type;
        using point_type = vertices_type::value_type::point_type;
        using vector_type = vertices_type::value_type::vector_type;

        static size_t position_count(const vector<T>& t)
        {
            return t.size();
        }

        static point_type position(const vector<T>& t, size_t n)
        {
            return t[n].position();
        }
    };

    //--------------------------------------------------------------------------
    matrix2_type texcoord_position_matrix(span<const point2_type> texcoords, span<const point2_type> positions)
    {
        // this matches affine_mapping ...
        // this is twice as fast as affine_mapping, consider using this in affine_mapping ...

        auto u0 = texcoords[0].x();
        auto v0 = texcoords[0].y();
        auto u1 = texcoords[1].x();
        auto v1 = texcoords[1].y();
        auto u2 = texcoords[2].x();
        auto v2 = texcoords[2].y();

        auto x0 = positions[0].x();
        auto y0 = positions[0].y();
        auto x1 = positions[1].x();
        auto y1 = positions[1].y();
        auto x2 = positions[2].x();
        auto y2 = positions[2].y();

        auto denom = u1 * v0 - u2 * v0 - u0 * v1 + u2 * v1 + u0 * v2 - u1 * v2;

        auto a = -(v1 * x0 - v2 * x0 - v0 * x1 + v2 * x1 + v0 * x2 - v1 * x2) / denom;
        auto b = -(-u1 * x0 + u2 * x0 + u0 * x1 - u2 * x1 - u0 * x2 + u1 * x2) / denom;
        auto c = -(-u2 * v1 * x0 + u1 * v2 * x0 + u2 * v0 * x1 - u0 * v2 * x1 - u1 * v0 * x2 + u0 * v1 * x2) / denom;
        auto d = -(v1 * y0 - v2 * y0 - v0 * y1 + v2 * y1 + v0 * y2 - v1 * y2) / denom;
        auto e = -(-u1 * y0 + u2 * y0 + u0 * y1 - u2 * y1 - u0 * y2 + u1 * y2) / denom;
        auto f = -(-u2 * v1 * y0 + u1 * v2 * y0 + u2 * v0 * y1 - u0 * v2 * y1 - u1 * v0 * y2 + u0 * v1 * y2) / denom;

        return
        {
            a, d, 0.0,
            b, e, 0.0,
            c, f, 1.0,
        };
    }

    //--------------------------------------------------------------------------
    vector2_type grain_direction_affine(span<const vertex_type> vertices, const g::Projection<vector_traits>& projection, vector2_type texture_direction)
    {
        assert(vertices.size() >= 3);

        auto from = std::ranges::to<std::vector>(ranges::take_view(vertices, 3) | std::views::transform([](const auto& vertex) -> point2_type { return vertex.texcoord(); }));
        auto to = std::ranges::to<std::vector>(ranges::take_view(vertices, 3) | std::views::transform([&projection](const auto& vertex) -> point2_type { return projection(vertex.position()); }));

        // The latter is twice as fast. Consider doing it in affine_mapping as a special case.
        //auto matrix = quetzal::geometry::affine_mapping<point2_type>(from, to);
        auto matrix = texcoord_position_matrix(from, to);

        return normalize(texture_direction * matrix);
    }

    //--------------------------------------------------------------------------
    vector2_type grain_direction_projective(span<const vertex_type> vertices, const g::Projection<vector_traits>& projection, vector2_type texture_direction)
    {
        assert(vertices.size() >= 4);

        auto from = std::ranges::to<std::vector>(ranges::take_view(vertices, 4) | std::views::transform([](const auto& vertex) -> point2_type { return vertex.texcoord(); }));
        auto to = std::ranges::to<std::vector>(ranges::take_view(vertices, 4) | std::views::transform([&projection](const auto& vertex) -> point2_type { return projection(vertex.position()); }));

        auto matrix = quetzal::geometry::projective_mapping<point2_type>(from, to);
        return normalize(texture_direction * matrix);
    }

    //--------------------------------------------------------------------------
    // Return the unit direction vector in position space corresponding to the given direction in texture space
    // There are probably many cases where affine can be used with 4 or more vertices, faces that were created with calculate_..._texcoords for example
    // So a simple vertex count is not a good condition, although projective should produce the same transformation
    vector2_type grain_direction(span<const vertex_type> vertices, const g::Projection<vector_traits>& projection, vector2_type texture_direction)
    {
        assert(vertices.size() >= 3);

        return vertices.size() >= 4 ?
            grain_direction_projective(vertices, projection, texture_direction) :
            grain_direction_affine(vertices, projection, texture_direction);
    }

    //--------------------------------------------------------------------------
    Pane transform_face(const face_type& face)
    {
        auto vertices = to_vertices(face);
        auto matrix = g::slerp_unit(face.attributes().normal(), {0.0, 0.0, 1.0}, 1.0) * g::scaling(1.0, 1.0, 0.0);
        ranges::for_each(vertices, [&matrix](auto& vertex) { vertex.transform_position(matrix); } );

        vector2_type texture_up_direction = -vector2_type::y_direction();
        vector2_type target_direction = vector2_type::x_direction(); // alternatively try {0, 1}, really the best fitting direction should be determined based on face and pages ...
        // really we can just rotate each independently for best fit

        g::Projection<vector_traits> projection;

        // Align grain direction
        auto matrix2 = g::translation(-to_vector(projection(centroid(vertices))));
        auto direction = grain_direction(vertices, projection, texture_up_direction);
        matrix2 *= g::rotation_2(angle_full_unit(direction, target_direction));

        Pane::polygon_type polygon(vertices.size());
        ranges::transform(vertices, polygon.vertices().begin(), [&matrix2, &projection](auto& vertex) { return projection(vertex.position()) * matrix2; });

        return Pane(face.id(), polygon, target_direction);
    }

    //--------------------------------------------------------------------------
    Panes transform_mesh(const mesh_type& mesh)
    {
        Panes panes;

//        size_t n = 0;

        for (const auto& face : mesh.faces())
        {
//            dump_face<mesh_type>(mesh, face.id(), n++);

            Pane pane = transform_face(face);
//cout << face.id() << "\t" << pane.vertex_count() << "\t" << pane.direction() << "\t[" << pane.vertex(0).position() << "]\t[" << pane.vertex(1).position() << "]\t[" << pane.vertex(2).position() << "]" << endl;
            auto i = panes.find(pane);
            if (i != panes.end())
            {
                i->insert_reference(face.id());
            }
            else
            {
                panes.insert(pane);
            }
        }

        return panes;
    }

    //--------------------------------------------------------------------------
    std::tuple<Pages, size_t> lay_out_panes(Panes& panes)
    {
        panes.sort();

        // Allocate panes to pages

        size_t nErrors = 0;
        Pages pages(page_size_Letter, 5.0, 5.0, "Helvetica", 12.0);
        for (const auto& pane : panes)
        {
            bool b = pages.insert(pane);
            if (!b)
            {
                // too large for single page, split and add as multiple parts ...
                // Won't be an error once that's done
                ++nErrors;
            }
        }

        return {pages, nErrors};
    }

    //--------------------------------------------------------------------------
    void write_pages(const Pages& pages, const filesystem::path& output_path)
    {
        ofstream ofs(output_path);
        if (!ofs)
        {
            ostringstream oss;
            oss << "Error opening file " << output_path;
            throw(quetzal::Exception(oss.str(), __FILE__, __LINE__));
        }

        // Draw pages
        quetzal::pdf::Document document;
        bool bFirst = true;

        for (const auto& page : pages)
        {
            if (bFirst)
            {
                bFirst = false;
            }
            else
            {
                document.advance_page();
            }

            page.draw(document);
        }

        ofs << document;
        return;
    }

    //--------------------------------------------------------------------------
    void show_statistics(const Panes& panes, const Pages& pages, size_t nErrors)
    {
        // Show statistics
        cout << pages.size() << " pages" << endl;
        cout << panes.reference_count() << " panes" << endl;
        cout << panes.instance_count() << " unique panes" << endl;
        cout << nErrors << " panes too large for page" << endl;

        size_t i = 0;
        for (const auto& pane : panes)
        {
            cout << "Pane " << i++ << " (" << pane.reference_count() << "):";
            for (auto id : pane.references())
            {
                cout << " " << id;
            }
            cout << "; " << pane.area() << "\t" << to_inches(pane.area()) << "\t" << to_inches(pane.reference_count() * pane.area()) << endl;
        }

        cout << fixed << setprecision(2) << panes.area() << " mm2 (rectangular bounds)" << endl;
        cout << fixed << setprecision(2) << to_inches(panes.area()) << " in2 (rectangular bounds)" << endl;
        return;
    }

    //--------------------------------------------------------------------------
    void illustrate_model(const filesystem::path& input_path, const filesystem::path& output_path)
    {
        mesh_type mesh;
        model::read_obj(mesh, input_path);

        Panes panes = transform_mesh(mesh);
        auto [pages, nErrors] = lay_out_panes(panes);
        write_pages(pages, output_path);

        show_statistics(panes, pages, nErrors);
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cout << "Error: Too few arguments" << endl;
        cout << "usage: " << argv[0] << " in_path_obj [out_path_pdf]" << endl;
    }

    filesystem::path in_path = argv[1];
    filesystem::path out_path = argc >= 2 ? argv[2] : output_path(in_path);

    try
    {
        illustrate_model(in_path, out_path);

        // Display resulting PDF file
//        ShellExecute(GetDesktopWindow(), L"open", quetzal::to_type<wstring>(out_path).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    catch (exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
