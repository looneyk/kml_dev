//------------------------------------------------------------------------------
// mapping_test
// main.cpp
//
// Exercises geometry::Mapping and geometry::PolygonMapping, the position -> texcoord maps.
//
// The edge cases check that PolygonMapping, which create_edge_internal and create_edge_partial use for a new vertex's
// texcoord, agrees with split_edge's interpolation along the edge
//------------------------------------------------------------------------------

#include "quetzal/geometry/Attributes.hpp"
#include "quetzal/geometry/Mapping.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/PolygonMapping.hpp"
#include "quetzal/geometry/Projection.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <format>
#include <string>
#include <vector>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using point_type = geometry::Point<vector_traits>;
    using vector_type = point_type::vector_type;
    using projection_type = geometry::Projection<vector_traits>;
    using texcoord_type = projection_type::projected_type;
    using mapping_type = geometry::Mapping<point_type, texcoord_type, projection_type>;

    //--------------------------------------------------------------------------
    // An absolute tolerance, for an error that would show in a model
    bool near(const texcoord_type& a, const texcoord_type& b, value_type tolerance = 1.0e-9)
    {
        return abs(a.x() - b.x()) <= tolerance && abs(a.y() - b.y()) <= tolerance;
    }

    //--------------------------------------------------------------------------
    string to_string(const texcoord_type& t)
    {
        return std::format("({:.9f}, {:.9f})", t.x(), t.y());
    }

    //--------------------------------------------------------------------------
    // Every correspondence the mapping was built from must map back exactly
    void check_correspondences(const string& name, const mapping_type& mapping, const vector<point_type>& positions, const vector<texcoord_type>& texcoords)
    {
        for (size_t i = 0; i < positions.size(); ++i)
        {
            texcoord_type t = mapping(positions[i]);
            CHECK_CASE(std::format("{}: corner {} round trips", name, i), near(t, texcoords[i]),
                std::format("got {} expected {}", to_string(t), to_string(texcoords[i])));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Along an edge the mapping must agree with lerp of the edge endpoints, as split_edge relies on
    void check_edge_agrees_with_lerp(const string& name, const mapping_type& mapping, const point_type& p0, const point_type& p1, const texcoord_type& t0, const texcoord_type& t1)
    {
        for (value_type t : {0.25, 0.5, 0.75})
        {
            texcoord_type mapped = mapping(lerp(p0, p1, t));
            texcoord_type interpolated = lerp(t0, t1, t);
            CHECK_CASE(std::format("{}: edge t={} mapping matches lerp", name, t), near(mapped, interpolated),
                std::format("mapping {} lerp {}", to_string(mapped), to_string(interpolated)));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A homography maps straight lines to straight lines, unlike a bilinear patch, but not their parameterization, so
    // agreement with lerp is checked for PolygonMapping instead
    void check_straightness(const string& name, const mapping_type& mapping, const point_type& p0, const point_type& p1)
    {
        texcoord_type a = mapping(p0);
        texcoord_type b = mapping(lerp(p0, p1, 0.5));
        texcoord_type c = mapping(p1);

        value_type area = (b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x());
        CHECK_CASE(std::format("{}: collinear maps collinear", name), abs(area) <= 1.0e-9,
            std::format("area {:.12f}", area));
        return;
    }

    //--------------------------------------------------------------------------
    // Three correspondences give an affine transform, which must be exact
    TEST_CASE("triangle, affine mapping")
    {
        vector<point_type> positions = {{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {0.0, 3.0, 0.0}};
        vector<texcoord_type> texcoords = {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}};

        mapping_type mapping(positions, texcoords, projection_type(vector_type(0.0, 0.0, 1.0)));

        check_correspondences("triangle", mapping, positions, texcoords);
        check_edge_agrees_with_lerp("triangle edge 0-1", mapping, positions[0], positions[1], texcoords[0], texcoords[1]);
        check_edge_agrees_with_lerp("triangle edge 0-2", mapping, positions[0], positions[2], texcoords[0], texcoords[2]);
        return;
    }

    //--------------------------------------------------------------------------
    // A parallelogram with affine texcoords, which the four point projective path must reproduce
    TEST_CASE("quad, affine texcoords")
    {
        vector<point_type> positions = {{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {2.0, 3.0, 0.0}, {0.0, 3.0, 0.0}};
        vector<texcoord_type> texcoords = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}};

        mapping_type mapping(positions, texcoords, projection_type(vector_type(0.0, 0.0, 1.0)));

        check_correspondences("quad affine", mapping, positions, texcoords);
        check_edge_agrees_with_lerp("quad affine edge 0-1", mapping, positions[0], positions[1], texcoords[0], texcoords[1]);
        check_edge_agrees_with_lerp("quad affine edge 1-2", mapping, positions[1], positions[2], texcoords[1], texcoords[2]);
        CHECK_CASE(std::format("quad affine: center maps to (0.5, 0.5)"), near(mapping(point_type(1.0, 1.5, 0.0)), texcoord_type(0.5, 0.5)),
            std::format("got {}", to_string(mapping(point_type(1.0, 1.5, 0.0)))));
        return;
    }

    //--------------------------------------------------------------------------
    // A trapezoid, which no affine transform reproduces
    TEST_CASE("quad, projective (trapezoid)")
    {
        vector<point_type> positions = {{0.0, 0.0, 0.0}, {4.0, 0.0, 0.0}, {3.0, 2.0, 0.0}, {1.0, 2.0, 0.0}};
        vector<texcoord_type> texcoords = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}};

        mapping_type mapping(positions, texcoords, projection_type(vector_type(0.0, 0.0, 1.0)));

        check_correspondences("quad projective", mapping, positions, texcoords);

        check_straightness("quad projective edge 0-1", mapping, positions[0], positions[1]);
        check_straightness("quad projective diagonal 0-2", mapping, positions[0], positions[2]);
        return;
    }

    //--------------------------------------------------------------------------
    // More than four consistent correspondences, which least squares must fit exactly
    TEST_CASE("quad, overdetermined (six correspondences)")
    {
        vector<point_type> positions = {{0.0, 0.0, 0.0}, {4.0, 0.0, 0.0}, {3.0, 2.0, 0.0}, {1.0, 2.0, 0.0}};
        vector<texcoord_type> texcoords = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}};

        projection_type projection(vector_type(0.0, 0.0, 1.0));
        mapping_type mapping_4(positions, texcoords, projection);

        // Two more points, on the map by construction
        for (const point_type& p : {point_type(2.0, 1.0, 0.0), point_type(1.5, 0.5, 0.0)})
        {
            positions.push_back(p);
            texcoords.push_back(mapping_4(p));
        }

        mapping_type mapping_6(positions, texcoords, projection);

        CHECK_CASE("overdetermined: six correspondences accepted", true);
        check_correspondences("quad overdetermined", mapping_6, positions, texcoords);

        // and it must agree with the exactly determined fit everywhere, not just at the samples
        for (const point_type& p : {point_type(2.0, 0.5, 0.0), point_type(2.5, 1.5, 0.0)})
        {
            CHECK_CASE(std::format("overdetermined: agrees with exact fit at {:.1f},{:.1f}", p.x(), p.y()),
                near(mapping_6(p), mapping_4(p)),
                std::format("six {} four {}", to_string(mapping_6(p)), to_string(mapping_4(p))));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A quad off the xy-plane, as the mesh_operations torus faces are, with texcoords in the range of the box_torus4
    // outputs
    TEST_CASE("quad, off axis plane (torus-like)")
    {
        vector<point_type> positions = {{2.0, 0.0, 1.0}, {2.5, 0.0, 2.0}, {2.5, 1.0, 2.0}, {2.0, 1.0, 1.0}};
        vector<texcoord_type> texcoords = {{0.389499, 0.474156}, {0.610499, 0.471134}, {0.616667, 0.500000}, {0.395667, 0.503000}};

        vector_type normal = normalize(cross(positions[1] - positions[0], positions[3] - positions[0]));
        mapping_type mapping(positions, texcoords, projection_type(normal));

        check_correspondences("quad off plane", mapping, positions, texcoords);
        check_straightness("quad off plane edge 0-1", mapping, positions[0], positions[1]);
        return;
    }

    //--------------------------------------------------------------------------
    // PolygonMapping, the bilinear mapping
    //--------------------------------------------------------------------------

    using attributes_type = geometry::PositionNormalTexture<vector_traits>;
    using polygon_mapping_type = geometry::PolygonMapping<attributes_type>;

    //--------------------------------------------------------------------------
    attributes_type corner(const point_type& position, const vector_type& normal, const texcoord_type& texcoord)
    {
        return attributes_type(position, normal, texcoord);
    }

    //--------------------------------------------------------------------------
    void check_polygon_corners(const string& name, const polygon_mapping_type& mapping, const vector<attributes_type>& corners)
    {
        for (size_t i = 0; i < mapping.vertex_count(); ++i)
        {
            texcoord_type t = mapping(corners[i].position());
            CHECK_CASE(std::format("{}: corner {} round trips", name, i), near(t, corners[i].texcoord()),
                std::format("got {} expected {}", to_string(t), to_string(corners[i].texcoord())));
        }

        return;
    }

    //--------------------------------------------------------------------------
    void check_polygon_edge(const string& name, const polygon_mapping_type& mapping, const attributes_type& a, const attributes_type& b)
    {
        for (value_type t : {0.25, 0.5, 0.75})
        {
            texcoord_type mapped = mapping(lerp(a.position(), b.position(), t));
            texcoord_type interpolated = lerp(a.texcoord(), b.texcoord(), t);
            CHECK_CASE(std::format("{}: edge t={} matches lerp", name, t), near(mapped, interpolated),
                std::format("mapping {} lerp {}", to_string(mapped), to_string(interpolated)));
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("PolygonMapping")
    {
        vector_type n(0.0, 0.0, 1.0);

        // Triangle, affine path
        vector<attributes_type> triangle = {
            corner({0.0, 0.0, 0.0}, n, {0.0, 0.0}),
            corner({2.0, 0.0, 0.0}, n, {1.0, 0.0}),
            corner({0.0, 3.0, 0.0}, n, {0.0, 1.0})};
        polygon_mapping_type mapping_triangle(triangle, n);
        check_polygon_corners("triangle", mapping_triangle, triangle);
        check_polygon_edge("triangle edge 0-1", mapping_triangle, triangle[0], triangle[1]);

        // Parallelogram, where the bilinear twist term vanishes and it degenerates to affine
        vector<attributes_type> parallelogram = {
            corner({0.0, 0.0, 0.0}, n, {0.0, 0.0}),
            corner({2.0, 0.0, 0.0}, n, {1.0, 0.0}),
            corner({2.0, 3.0, 0.0}, n, {1.0, 1.0}),
            corner({0.0, 3.0, 0.0}, n, {0.0, 1.0})};
        polygon_mapping_type mapping_parallelogram(parallelogram, n);
        check_polygon_corners("parallelogram", mapping_parallelogram, parallelogram);
        check_polygon_edge("parallelogram edge 1-2", mapping_parallelogram, parallelogram[1], parallelogram[2]);
        CHECK_CASE("parallelogram: center", near(mapping_parallelogram(point_type(1.0, 1.5, 0.0)), texcoord_type(0.5, 0.5)),
            std::format("got {}", to_string(mapping_parallelogram(point_type(1.0, 1.5, 0.0)))));

        // Trapezoid, which no affine map reproduces
        vector<attributes_type> trapezoid = {
            corner({0.0, 0.0, 0.0}, n, {0.0, 0.0}),
            corner({4.0, 0.0, 0.0}, n, {1.0, 0.0}),
            corner({3.0, 2.0, 0.0}, n, {1.0, 1.0}),
            corner({1.0, 2.0, 0.0}, n, {0.0, 1.0})};
        polygon_mapping_type mapping_trapezoid(trapezoid, n);
        check_polygon_corners("trapezoid", mapping_trapezoid, trapezoid);
        check_polygon_edge("trapezoid edge 0-1", mapping_trapezoid, trapezoid[0], trapezoid[1]);
        check_polygon_edge("trapezoid edge 2-3", mapping_trapezoid, trapezoid[2], trapezoid[3]);
        check_polygon_edge("trapezoid edge 3-0", mapping_trapezoid, trapezoid[3], trapezoid[0]);

        return;
    }

    //--------------------------------------------------------------------------
    // The dodecahedron top face from the primitives model. The bilinear patch through four of its five corners
    // reproduces the similarity between a regular pentagon and its uv pentagon, so the center maps to the uv centroid
    // whichever corner the perimeter starts at
    //
    // At float, as models are built, the cancellation noise in the pentagon's zero twist term is large enough to send the
    // solve down the quadratic branch
    template<typename T>
    void check_pentagon(const string& precision, T tolerance)
    {
        using traits = math::HomogeneousTraits<T, 3>;
        using pentagon_point_type = geometry::Point<traits>;
        using pentagon_vector_type = pentagon_point_type::vector_type;
        using pentagon_texcoord_type = geometry::Projection<traits>::projected_type;
        using pentagon_attributes_type = geometry::PositionNormalTexture<traits>;

        pentagon_vector_type n(T(0), T(0), T(1));
        const T z = T(7.94654);

        vector<pentagon_attributes_type> pentagon = {
            {pentagon_point_type(T( 4.91123), T( 3.56822), z), n, pentagon_texcoord_type(T(0.5),      T(0.0489435))},
            {pentagon_point_type(T(-1.87593), T( 5.7735),  z), n, pentagon_texcoord_type(T(0.0),      T(0.412215))},
            {pentagon_point_type(T(-6.07062), T( 0.0),     z), n, pentagon_texcoord_type(T(0.190983), T(1.0))},
            {pentagon_point_type(T(-1.87593), T(-5.7735),  z), n, pentagon_texcoord_type(T(0.809017), T(1.0))},
            {pentagon_point_type(T( 4.91123), T(-3.56822), z), n, pentagon_texcoord_type(T(1.0),      T(0.412215))}};

        pentagon_texcoord_type centroid(T(0.5), T(0.574675));
        pentagon_point_type center(T(0), T(0), z);

        for (size_t start = 0; start < pentagon.size(); ++start)
        {
            vector<pentagon_attributes_type> rotated;
            for (size_t i = 0; i < pentagon.size(); ++i)
            {
                rotated.push_back(pentagon[(start + i) % pentagon.size()]);
            }

            geometry::PolygonMapping<pentagon_attributes_type> mapping(rotated, n);

            for (size_t i = 0; i < mapping.vertex_count(); ++i)
            {
                pentagon_texcoord_type tc = mapping(rotated[i].position());
                bool round_trips = abs(tc.x() - rotated[i].texcoord().x()) <= tolerance && abs(tc.y() - rotated[i].texcoord().y()) <= tolerance;
                CHECK_CASE(std::format("pentagon {} start {}: corner {} round trips", precision, start, i), round_trips,
                    std::format("got ({:.6f}, {:.6f}) expected ({:.6f}, {:.6f})", tc.x(), tc.y(), rotated[i].texcoord().x(), rotated[i].texcoord().y()));
            }

            pentagon_texcoord_type t = mapping(center);
            bool b = abs(t.x() - centroid.x()) <= tolerance && abs(t.y() - centroid.y()) <= tolerance;
            CHECK_CASE(std::format("pentagon {} start {}: center maps to uv centroid", precision, start), b,
                std::format("got ({:.6f}, {:.6f}) expected ({:.6f}, {:.6f})", t.x(), t.y(), centroid.x(), centroid.y()));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The error of the inverse bilinear solve. A known (u, v) is mapped through the forward patch in double, and the
    // mapping recovers it at the precision under test. Near parallelogram quads have a small twist term and an ill
    // conditioned quadratic
    template<typename T>
    double inverse_bilinear_error(const std::array<std::pair<double, double>, 4>& xy, const std::array<std::pair<double, double>, 4>& uv)
    {
        using traits = math::HomogeneousTraits<T, 3>;
        using pt = geometry::Point<traits>;
        using tc = geometry::Projection<traits>::projected_type;
        using att = geometry::PositionNormalTexture<traits>;

        typename pt::vector_type n(T(0), T(0), T(1));

        vector<att> corners;
        for (size_t i = 0; i < 4; ++i)
        {
            corners.push_back({pt(T(xy[i].first), T(xy[i].second), T(0)), n, tc(T(uv[i].first), T(uv[i].second))});
        }

        geometry::PolygonMapping<att> mapping(corners, n);

        auto blend = [](const std::array<std::pair<double, double>, 4>& c, double u, double v) {
            return std::pair<double, double>{
                c[0].first * (1 - u) * (1 - v) + c[1].first * u * (1 - v) + c[2].first * u * v + c[3].first * (1 - u) * v,
                c[0].second * (1 - u) * (1 - v) + c[1].second * u * (1 - v) + c[2].second * u * v + c[3].second * (1 - u) * v};
        };

        double worst = 0.0;
        for (int iu = 0; iu <= 8; ++iu)
        {
            for (int iv = 0; iv <= 8; ++iv)
            {
                double u = iu / 8.0;
                double v = iv / 8.0;
                auto p = blend(xy, u, v);
                auto expected = blend(uv, u, v);

                tc got = mapping(pt(T(p.first), T(p.second), T(0)));
                worst = std::max(worst, std::max(std::abs(double(got.x()) - expected.first), std::abs(double(got.y()) - expected.second)));
            }
        }

        return worst;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("regular pentagon, more corners than the patch takes, double")
    {
        check_pentagon<double>("double", 1.0e-6);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("regular pentagon, more corners than the patch takes, float")
    {
        check_pentagon<float>("float", 1.0e-4f);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("inverse bilinear accuracy, near parallelogram quads")
    {
        std::array<std::pair<double, double>, 4> uv = {{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}}};

        // Twist shrinking toward zero, so the quadratic gets steadily worse conditioned
        for (double twist : {1.0, 0.1, 0.01, 0.001, 0.0001})
        {
            std::array<std::pair<double, double>, 4> xy = {{{0.0, 0.0}, {10.0, 0.0}, {10.0 + twist, 5.0}, {0.0, 5.0}}};

            double ef = inverse_bilinear_error<float>(xy, uv);
            double ed = inverse_bilinear_error<double>(xy, uv);

            // Tight enough to fail on any conditioning regression; the stable solve is near one ulp at both precisions
            CHECK_CASE(std::format("twist {:<8g} float error {:.3e}, double error {:.3e}", twist, ef, ed), ef >= 0.0 && ef < 1.0e-6 && ed < 1.0e-14);
        }

        return;
    }

} // namespace
