//------------------------------------------------------------------------------
// triangulation_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/testing/named_checks.hpp"
#include "quetzal/triangulation/triangulation.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <optional>
#include <vector>

using namespace std;
using namespace quetzal::triangulation;

namespace
{

    //--------------------------------------------------------------------------
    const vector<Point> square = {{0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}};

    //--------------------------------------------------------------------------
    optional<Error> triangulation_error(const Region& region, const Options& options = {})
    {
        try
        {
            triangulate(region, options);
        }
        catch (const Error& error)
        {
            return error;
        }

        return nullopt;
    }

    //--------------------------------------------------------------------------
    Point position(const Region& region, const Triangulation& triangulation, size_t slot)
    {
        for (const auto& boundary : region.boundaries)
        {
            if (slot < boundary.size())
            {
                return boundary[slot];
            }

            slot -= boundary.size();
        }

        if (slot < region.interior_points.size())
        {
            return region.interior_points[slot];
        }

        return triangulation.added_points[slot - region.interior_points.size()].position;
    }

    //--------------------------------------------------------------------------
    bool counterclockwise(const Region& region, const Triangulation& triangulation)
    {
        return ranges::all_of(triangulation.triangles, [&](const triangle_type& triangle)
        {
            const Point a = position(region, triangulation, triangle[0]);
            const Point b = position(region, triangulation, triangle[1]);
            const Point c = position(region, triangulation, triangle[2]);
            return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0.0;
        });
    }

    //--------------------------------------------------------------------------
    TEST_CASE("simple regions")
    {
        Region region{{square}, {}};
        Triangulation triangulation = triangulate(region);
        CHECK_CASE_EQUAL("a square has two triangles", triangulation.triangles.size(), 2);
        CHECK_CASE("the square's triangles are counterclockwise", counterclockwise(region, triangulation));
        CHECK_CASE("the square adds no points", triangulation.added_points.empty());

        Region region_hole{{square, {{0.5, 0.5}, {0.5, 1.5}, {1.5, 1.5}, {1.5, 0.5}}}, {}};
        Triangulation triangulation_hole = triangulate(region_hole);
        CHECK_CASE_EQUAL("a square with a square hole has eight triangles", triangulation_hole.triangles.size(), 8);
        CHECK_CASE("the hole's winding does not matter", counterclockwise(region_hole, triangulation_hole));

        Region region_interior{{square}, {{1.0, 1.0}}};
        Triangulation triangulation_interior = triangulate(region_interior);
        CHECK_CASE_EQUAL("an interior point divides the square into four triangles", triangulation_interior.triangles.size(), 4);
        CHECK_CASE("each triangle has the interior point",
            ranges::all_of(triangulation_interior.triangles, [](const triangle_type& triangle) { return ranges::contains(triangle, size_t(4)); }));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("duplicate points")
    {
        // The hole's first point, point 4, is at the square's first point
        Region region{{square, {{0.0, 0.0}, {1.0, 1.5}, {1.5, 1.0}}}, {}};

        optional<Error> error = triangulation_error(region);
        REQUIRE_MESSAGE(error, "duplicate points are an error");
        CHECK_CASE("the condition is duplicate_points", error->condition() == Error::Condition::duplicate_points);
        vector<size_t> point_slots = error->point_slots();
        ranges::sort(point_slots);
        CHECK_CASE("the error lists both points", point_slots == vector<size_t>{0, 4});

        Options options;
        options.duplicate_points_merged = true;
        Triangulation triangulation = triangulate(region, options);
        CHECK_CASE("point 4 is merged into point 0", triangulation.merged_point_slots ==vector<array<size_t, 2>>{{4, 0}});
        CHECK_CASE("no triangle has the merged point",
            ranges::none_of(triangulation.triangles, [](const triangle_type& triangle) { return ranges::contains(triangle, size_t(4)); }));
        CHECK_CASE("the merged region has triangles", !triangulation.triangles.empty());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("crossing edges")
    {
        Region region{{{{0.0, 0.0}, {2.0, 2.0}, {2.0, 0.0}, {0.0, 2.0}}}, {}};

        optional<Error> error = triangulation_error(region);
        REQUIRE_MESSAGE(error, "crossing edges are an error");
        CHECK_CASE("the condition is crossing_edges", error->condition() == Error::Condition::crossing_edges);
        CHECK_CASE_EQUAL("the error lists both edges", error->point_slots().size(), 4);

        Options options;
        options.crossing_edges_split = true;
        Triangulation triangulation = triangulate(region, options);
        REQUIRE_MESSAGE(triangulation.added_points.size() == 1, "the crossing adds one point");
        const AddedPoint& added = triangulation.added_points.front();
        CHECK_CASE("the added point is at the crossing", added.position.x == 1.0 && added.position.y == 1.0);
        CHECK_CASE("the added point divides edges 0-1 and 2-3", added.edges == vector<edge_type>{{0, 1}, {2, 3}});
        CHECK_CASE_EQUAL("the bow tie has two triangles", triangulation.triangles.size(), 2);
        CHECK_CASE("the bow tie's triangles are counterclockwise", counterclockwise(region, triangulation));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("points on edges")
    {
        // The hole's first point, point 4, is on the square's edge 0-1
        Region region{{square, {{1.0, 0.0}, {0.5, 1.0}, {1.5, 1.0}}}, {}};

        optional<Error> error = triangulation_error(region);
        REQUIRE_MESSAGE(error, "a point on an edge is an error");
        CHECK_CASE("the condition is point_on_edge", error->condition() == Error::Condition::point_on_edge);
        CHECK_CASE("the error lists the point and the edge", error->point_slots() == vector<size_t>{4, 0, 1});

        Options options;
        options.edges_split_at_points = true;
        Triangulation triangulation = triangulate(region, options);
        CHECK_CASE("the divided region has triangles", !triangulation.triangles.empty());
        CHECK_CASE("dividing an edge at an input point adds no points", triangulation.added_points.empty());

        Region region_near{{square, {{1.0, 1e-9}, {0.5, 1.0}, {1.5, 1.0}}}, {}};
        CHECK_CASE("a point near an edge is not on it without a tolerance", !triangulation_error(region_near));

        Options options_tolerance;
        options_tolerance.point_on_edge_tolerance = 1e-6;
        optional<Error> error_tolerance = triangulation_error(region_near, options_tolerance);
        CHECK_CASE("a point within the tolerance is on the edge", error_tolerance && error_tolerance->condition() == Error::Condition::point_on_edge);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("overlapping edges")
    {
        // Merging point 4 into point 1 and point 7 into point 2 makes edge 1-2 an edge of both squares
        Region region{{{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}}, {{1.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {1.0, 1.0}}}, {}};

        Options options;
        options.duplicate_points_merged = true;
        optional<Error> error = triangulation_error(region, options);
        REQUIRE_MESSAGE(error, "an edge in two boundaries is an error");
        CHECK_CASE("the condition is overlapping_edges", error->condition() == Error::Condition::overlapping_edges);
        CHECK_CASE("the error lists the edge", error->point_slots() == vector<size_t>{1, 2});

        options.overlapping_edges_permitted = true;
        Triangulation triangulation = triangulate(region, options);
        CHECK_CASE("the overlapping region has triangles", !triangulation.triangles.empty());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("added points")
    {
        // The hole's long edge 4-5 has the hole's apex just above it and the square's corners far below, so no empty circle passes
        // through its ends
        Region region{{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}}, {{1.0, 5.0}, {9.0, 5.0}, {5.0, 5.1}}}, {}};
        CHECK_CASE("a constrained triangulation adds no points", triangulate(region).added_points.empty());

        Options options;
        options.conforming = true;
        Triangulation triangulation = triangulate(region, options);
        CHECK_CASE("a conforming triangulation adds points", !triangulation.added_points.empty());
        CHECK_CASE("each added point divides an input edge",
            ranges::all_of(triangulation.added_points, [](const AddedPoint& added) { return added.edges.size() == 1; }));
        CHECK_CASE("an added point divides edge 4-5", ranges::any_of(triangulation.added_points, [](const AddedPoint& added)
        {
            return added.edges == vector<edge_type>{{4, 5}};
        }));
        CHECK_CASE("the conforming triangles are counterclockwise", counterclockwise(region, triangulation));

        Region region_thin{{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 1.0}, {0.0, 1.0}}}, {}};
        CHECK_CASE_EQUAL("a thin rectangle is two triangles", triangulate(region_thin).triangles.size(), 2);

        Options options_refined;
        options_refined.refinement = Refinement{100};
        Triangulation triangulation_refined = triangulate(region_thin, options_refined);
        CHECK_CASE("refinement adds points", !triangulation_refined.added_points.empty());
        CHECK_CASE("refinement adds triangles", triangulation_refined.triangles.size() > 2);
        CHECK_CASE("the refined triangles are counterclockwise", counterclockwise(region_thin, triangulation_refined));

        return;
    }

} // namespace
