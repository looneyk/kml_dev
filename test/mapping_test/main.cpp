//------------------------------------------------------------------------------
// mapping_test
// main.cpp
//
// Exercises geometry::Mapping and geometry::PolygonMapping, the position -> texcoord maps.
//
// PolygonMapping is what create_edge_internal/create_edge_partial use to give a new vertex its
// texcoord, while split_edge instead interpolates along the edge being split. Those two have to
// agree wherever both apply, or which one a vertex happens to go through changes the result.
// That agreement is what the edge cases here check.
//------------------------------------------------------------------------------

#include "quetzal/geometry/Attributes.hpp"
#include "quetzal/geometry/Mapping.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/PolygonMapping.hpp"
#include "quetzal/geometry/Projection.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/floating_point.hpp"
#include <format>
#include <iostream>
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

    size_t n_cases = 0;
    size_t n_failures = 0;

    //--------------------------------------------------------------------------
    void check(const string& name, bool b, const string& detail = {})
    {
        ++n_cases;
        if (!b)
        {
            ++n_failures;
        }

        cout << (b ? "pass  " : "FAIL  ") << name;
        if (!detail.empty())
        {
            cout << "  " << detail;
        }

        cout << endl;
        return;
    }

    //--------------------------------------------------------------------------
    // Absolute rather than ulp comparison, since what matters here is whether a texcoord is wrong
    // by an amount that would show in a model, not whether it is bit exact
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
    // Every correspondence the mapping was built from has to come back out of it. A fit that cannot
    // reproduce its own input points is not a mapping of that kind at all
    void check_correspondences(const string& name, const mapping_type& mapping, const vector<point_type>& positions, const vector<texcoord_type>& texcoords)
    {
        for (size_t i = 0; i < positions.size(); ++i)
        {
            texcoord_type t = mapping(positions[i]);
            check(std::format("{}: corner {} round trips", name, i), near(t, texcoords[i]),
                std::format("got {} expected {}", to_string(t), to_string(texcoords[i])));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Along an edge of the face the mapping and a plain interpolation of the edge endpoints must
    // agree -- this is exactly the invariant split_edge relies on when it lerps a split vertex
    void check_edge_agrees_with_lerp(const string& name, const mapping_type& mapping, const point_type& p0, const point_type& p1, const texcoord_type& t0, const texcoord_type& t1)
    {
        for (value_type t : {0.25, 0.5, 0.75})
        {
            texcoord_type mapped = mapping(lerp(p0, p1, t));
            texcoord_type interpolated = lerp(t0, t1, t);
            check(std::format("{}: edge t={} mapping matches lerp", name, t), near(mapped, interpolated),
                std::format("mapping {} lerp {}", to_string(mapped), to_string(interpolated)));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A homography maps every straight line to a straight line, which is what distinguishes it from a
    // bilinear patch. It does NOT preserve the parameterization along that line -- equal steps become
    // unequal, transformed fractional-linearly -- so agreement with lerp is a bilinear property and is
    // checked against PolygonMapping instead, not here
    void check_straightness(const string& name, const mapping_type& mapping, const point_type& p0, const point_type& p1)
    {
        texcoord_type a = mapping(p0);
        texcoord_type b = mapping(lerp(p0, p1, 0.5));
        texcoord_type c = mapping(p1);

        value_type area = (b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x());
        check(std::format("{}: collinear maps collinear", name), abs(area) <= 1.0e-9,
            std::format("area {:.12f}", area));
        return;
    }

    //--------------------------------------------------------------------------
    // A triangle gives three correspondences, so Mapping builds an affine transform. This is the
    // case that should be exactly right, and it is the baseline the quad cases are judged against
    void test_triangle_affine()
    {
        cout << endl << "triangle, affine mapping" << endl;

        vector<point_type> positions = {{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {0.0, 3.0, 0.0}};
        vector<texcoord_type> texcoords = {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}};

        mapping_type mapping(positions, texcoords, projection_type(vector_type(0.0, 0.0, 1.0)));

        check_correspondences("triangle", mapping, positions, texcoords);
        check_edge_agrees_with_lerp("triangle edge 0-1", mapping, positions[0], positions[1], texcoords[0], texcoords[1]);
        check_edge_agrees_with_lerp("triangle edge 0-2", mapping, positions[0], positions[2], texcoords[0], texcoords[2]);
        return;
    }

    //--------------------------------------------------------------------------
    // A parallelogram with texcoords assigned affinely. Four correspondences send Mapping down the
    // projective path, but the correct answer here is still affine, so this isolates whether the
    // four point path can reproduce a case the three point path handles
    void test_quad_affine()
    {
        cout << endl << "quad, affine texcoords" << endl;

        vector<point_type> positions = {{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {2.0, 3.0, 0.0}, {0.0, 3.0, 0.0}};
        vector<texcoord_type> texcoords = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}};

        mapping_type mapping(positions, texcoords, projection_type(vector_type(0.0, 0.0, 1.0)));

        check_correspondences("quad affine", mapping, positions, texcoords);
        check_edge_agrees_with_lerp("quad affine edge 0-1", mapping, positions[0], positions[1], texcoords[0], texcoords[1]);
        check_edge_agrees_with_lerp("quad affine edge 1-2", mapping, positions[1], positions[2], texcoords[1], texcoords[2]);
        check(std::format("quad affine: center maps to (0.5, 0.5)"), near(mapping(point_type(1.0, 1.5, 0.0)), texcoord_type(0.5, 0.5)),
            std::format("got {}", to_string(mapping(point_type(1.0, 1.5, 0.0)))));
        return;
    }

    //--------------------------------------------------------------------------
    // A trapezoid, the case that genuinely needs a projective map. The four corners cannot be
    // reproduced by any affine transform, so this separates a real homography from a least squares
    // affine fit through four points
    void test_quad_projective()
    {
        cout << endl << "quad, projective (trapezoid)" << endl;

        vector<point_type> positions = {{0.0, 0.0, 0.0}, {4.0, 0.0, 0.0}, {3.0, 2.0, 0.0}, {1.0, 2.0, 0.0}};
        vector<texcoord_type> texcoords = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}};

        mapping_type mapping(positions, texcoords, projection_type(vector_type(0.0, 0.0, 1.0)));

        check_correspondences("quad projective", mapping, positions, texcoords);

        check_straightness("quad projective edge 0-1", mapping, positions[0], positions[1]);
        check_straightness("quad projective diagonal 0-2", mapping, positions[0], positions[2]);
        return;
    }

    //--------------------------------------------------------------------------
    // More than four correspondences overdetermine the homography. Consistent data has an exact
    // solution, which least squares must find rather than merely approach, so every correspondence
    // still has to round trip. The extra points are generated through the four point map, so they
    // lie on it by construction
    void test_quad_overdetermined()
    {
        cout << endl << "quad, overdetermined (six correspondences)" << endl;

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

        check("overdetermined: six correspondences accepted", true);
        check_correspondences("quad overdetermined", mapping_6, positions, texcoords);

        // and it must agree with the exactly determined fit everywhere, not just at the samples
        for (const point_type& p : {point_type(2.0, 0.5, 0.0), point_type(2.5, 1.5, 0.0)})
        {
            check(std::format("overdetermined: agrees with exact fit at {:.1f},{:.1f}", p.x(), p.y()),
                near(mapping_6(p), mapping_4(p)),
                std::format("six {} four {}", to_string(mapping_6(p)), to_string(mapping_4(p))));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The mesh_operations torus faces are quads off the xy-plane, which is the configuration where
    // the splitter disagreed with itself. Texcoord values are in the range the box_torus4 outputs
    // showed, where mapping and lerp derived vertices differed by about 1.8e-3
    void test_quad_off_plane()
    {
        cout << endl << "quad, off axis plane (torus-like)" << endl;

        vector<point_type> positions = {{2.0, 0.0, 1.0}, {2.5, 0.0, 2.0}, {2.5, 1.0, 2.0}, {2.0, 1.0, 1.0}};
        vector<texcoord_type> texcoords = {{0.389499, 0.474156}, {0.610499, 0.471134}, {0.616667, 0.500000}, {0.395667, 0.503000}};

        vector_type normal = normalize(cross(positions[1] - positions[0], positions[3] - positions[0]));
        mapping_type mapping(positions, texcoords, projection_type(normal));

        check_correspondences("quad off plane", mapping, positions, texcoords);
        check_straightness("quad off plane edge 0-1", mapping, positions[0], positions[1]);
        return;
    }

    //--------------------------------------------------------------------------
    // PolygonMapping, the bilinear replacement for the four point path
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
            check(std::format("{}: corner {} round trips", name, i), near(t, corners[i].texcoord()),
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
            check(std::format("{}: edge t={} matches lerp", name, t), near(mapped, interpolated),
                std::format("mapping {} lerp {}", to_string(mapped), to_string(interpolated)));
        }

        return;
    }

    //--------------------------------------------------------------------------
    void test_polygon_mapping()
    {
        cout << endl << "PolygonMapping" << endl;

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
        check("parallelogram: center", near(mapping_parallelogram(point_type(1.0, 1.5, 0.0)), texcoord_type(0.5, 0.5)),
            std::format("got {}", to_string(mapping_parallelogram(point_type(1.0, 1.5, 0.0)))));

        // Trapezoid, the case the least squares affine fit could not reproduce
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

        // Colinear vertices, too few vertices and default construction are all rejected at compile
        // time or by assert now, so none of them has a runtime rejection to test here
        return;
    }

    //--------------------------------------------------------------------------
    // The dodecahedron top face, verbatim from the primitives model. Five corners, so the bilinear
    // patch is fit through four of them and the fifth is dropped. That costs nothing here: a regular
    // pentagon and its regular uv pentagon are related by a similarity, and a bilinear patch through
    // any four corners of an affinely related pair reproduces the whole relation. So the center must
    // map to the uv centroid, whichever corner the perimeter happens to start at.
    //
    // Run at float as well as double. A regular pentagon makes the inverse bilinear twist term
    // exactly zero, and at float the cancellation noise left in its place is large enough to be taken
    // for a real value, which sends the solve down the quadratic branch and returns a wrong texcoord
    // rather than a slightly imprecise one. Models are built at float
    template<typename T>
    void test_pentagon(const string& precision, T tolerance)
    {
        cout << endl << "regular pentagon, more corners than the patch takes, " << precision << endl;

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
                check(std::format("pentagon {} start {}: corner {} round trips", precision, start, i), round_trips,
                    std::format("got ({:.6f}, {:.6f}) expected ({:.6f}, {:.6f})", tc.x(), tc.y(), rotated[i].texcoord().x(), rotated[i].texcoord().y()));
            }

            pentagon_texcoord_type t = mapping(center);
            bool b = abs(t.x() - centroid.x()) <= tolerance && abs(t.y() - centroid.y()) <= tolerance;
            check(std::format("pentagon {} start {}: center maps to uv centroid", precision, start), b,
                std::format("got ({:.6f}, {:.6f}) expected ({:.6f}, {:.6f})", t.x(), t.y(), centroid.x(), centroid.y()));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Accuracy of the inverse bilinear solve, measured rather than assumed.
    //
    // A known (u, v) is pushed through the forward patch in double to get a position and the texcoord
    // that position must map back to. The mapping then re-derives (u, v) from that position at the
    // precision under test, so the gap is the inversion's own error. Near parallelogram quads are the
    // hard case: the twist term is small, the quadratic is ill conditioned, and the smaller root comes
    // out of the direct formula as a difference of near equal terms
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
    void test_inverse_bilinear_accuracy()
    {
        cout << endl << "inverse bilinear accuracy, near parallelogram quads" << endl;

        std::array<std::pair<double, double>, 4> uv = {{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}}};

        // Twist shrinking toward zero, so the quadratic gets steadily worse conditioned
        for (double twist : {1.0, 0.1, 0.01, 0.001, 0.0001})
        {
            std::array<std::pair<double, double>, 4> xy = {{{0.0, 0.0}, {10.0, 0.0}, {10.0 + twist, 5.0}, {0.0, 5.0}}};

            double ef = inverse_bilinear_error<float>(xy, uv);
            double ed = inverse_bilinear_error<double>(xy, uv);

            // Tight enough to fail as soon as the conditioning regresses at all: the stable solve holds
            // near one ulp at both precisions, while the direct formula misses these by orders
            check(std::format("twist {:<8g} float error {:.3e}, double error {:.3e}", twist, ef, ed), ef >= 0.0 && ef < 1.0e-6 && ed < 1.0e-14);
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    test_triangle_affine();
    test_quad_affine();
    test_quad_projective();
    test_quad_overdetermined();
    test_quad_off_plane();
    test_polygon_mapping();
    test_pentagon<double>("double", 1.0e-6);
    test_pentagon<float>("float", 1.0e-4f);
    test_inverse_bilinear_accuracy();

    cout << endl << (n_failures == 0 ? "OK: " : "Error: ") << (n_cases - n_failures) << " of " << n_cases << " cases passed" << endl;
    return n_failures == 0 ? 0 : 1;
}
