//------------------------------------------------------------------------------
// triangulation
// triangulation.cpp
//------------------------------------------------------------------------------

#include "triangulation.hpp"
#include <CDT.h>
#include <algorithm>
#include <format>
#include <numeric>

using namespace std;
using namespace quetzal::triangulation;

namespace
{

    using triangulator_type = CDT::Triangulation<double>;

    //--------------------------------------------------------------------------
    // The triangulator's vertices omit the merged duplicates and are followed by the vertices it adds
    class SlotMapping
    {
    public:

        explicit SlotMapping(size_t point_count) :
            m_point_count(point_count),
            m_retained_point_slots(point_count)
        {
            iota(m_retained_point_slots.begin(), m_retained_point_slots.end(), size_t(0));
        }

        SlotMapping(size_t point_count, const CDT::DuplicatesInfo& duplicates) :
            m_point_count(point_count),
            m_retained_point_slots()
        {
            for (size_t point_slot = 0; point_slot < duplicates.mapping.size(); ++point_slot)
            {
                if (duplicates.mapping[point_slot] == m_retained_point_slots.size())
                {
                    m_retained_point_slots.push_back(point_slot);
                }
            }
        }

        CDT::VertInd input_vertex_count() const
        {
            return static_cast<CDT::VertInd>(m_retained_point_slots.size());
        }

        size_t point_slot(CDT::VertInd vertex_slot) const
        {
            if (vertex_slot < m_retained_point_slots.size())
            {
                return m_retained_point_slots[vertex_slot];
            }

            return m_point_count + (vertex_slot - m_retained_point_slots.size());
        }

        edge_type edge(const CDT::Edge& edge) const
        {
            return {point_slot(edge.v1()), point_slot(edge.v2())};
        }

    private:

        size_t m_point_count;
        vector<size_t> m_retained_point_slots; // <vertex slot, point slot>
    };

    //--------------------------------------------------------------------------
    vector<Point> region_points(const Region& region)
    {
        vector<Point> points;
        for (const auto& boundary : region.boundaries)
        {
            points.append_range(boundary);
        }

        points.append_range(region.interior_points);
        return points;
    }

    //--------------------------------------------------------------------------
    vector<CDT::Edge> region_edges(const Region& region)
    {
        vector<CDT::Edge> edges;
        CDT::VertInd boundary_begin = 0;
        for (const auto& boundary : region.boundaries)
        {
            const auto boundary_end = static_cast<CDT::VertInd>(boundary_begin + boundary.size());
            for (CDT::VertInd vertex_slot = boundary_begin; vertex_slot < boundary_end; ++vertex_slot)
            {
                edges.emplace_back(vertex_slot, vertex_slot + 1 < boundary_end ? vertex_slot + 1 : boundary_begin);
            }

            boundary_begin = boundary_end;
        }

        return edges;
    }

    //--------------------------------------------------------------------------
    // Until the triangulation is finalized, its first vertices are those of the super-triangle enclosing the input
    CDT::VertInd unfinalized_to_input(CDT::VertInd vertex_slot)
    {
        return vertex_slot - CDT::nSuperTriVerts;
    }

    //--------------------------------------------------------------------------
    CDT::Edge unfinalized_to_input(const CDT::Edge& edge)
    {
        return CDT::Edge(unfinalized_to_input(edge.v1()), unfinalized_to_input(edge.v2()));
    }

    //--------------------------------------------------------------------------
    string position_text(const Point& point)
    {
        return format("({}, {})", point.x, point.y);
    }

    //--------------------------------------------------------------------------
    CDT::RefinementCriterion::Enum refinement_criterion(Refinement::Criterion criterion)
    {
        switch (criterion)
        {
        case Refinement::Criterion::smallest_angle:
            return CDT::RefinementCriterion::SmallestAngle;
        case Refinement::Criterion::largest_area:
            return CDT::RefinementCriterion::LargestArea;
        }

        throw logic_error("Unknown refinement criterion");
    }

    //--------------------------------------------------------------------------
    // The triangulator divides an edge at each input vertex lying on it, and maps each piece to its original edge
    void check_points_on_edges(const triangulator_type& triangulator, const SlotMapping& slot_mapping, const vector<Point>& points)
    {
        for (const auto& [piece, originals] : triangulator.pieceToOriginals)
        {
            for (const auto& original : originals)
            {
                for (CDT::VertInd vertex_slot : {piece.v1(), piece.v2()})
                {
                    if (unfinalized_to_input(vertex_slot) < slot_mapping.input_vertex_count() && vertex_slot != original.v1() && vertex_slot != original.v2())
                    {
                        const size_t point_slot = slot_mapping.point_slot(unfinalized_to_input(vertex_slot));
                        const edge_type edge = slot_mapping.edge(unfinalized_to_input(original));
                        throw Error(Error::Condition::point_on_edge, {point_slot, edge[0], edge[1]},
                            format("Point {} at {} is on edge {}-{}", point_slot, position_text(points[point_slot]), edge[0], edge[1]));
                    }
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    void check_overlapping_edges(const triangulator_type& triangulator, const SlotMapping& slot_mapping)
    {
        if (!triangulator.overlapCount.empty())
        {
            const edge_type edge = slot_mapping.edge(unfinalized_to_input(triangulator.overlapCount.begin()->first));
            throw Error(Error::Condition::overlapping_edges, {edge[0], edge[1]},
                format("Edge {}-{} occurs more than once in the boundaries", edge[0], edge[1]));
        }

        return;
    }

    //--------------------------------------------------------------------------
    void insert_edges(triangulator_type& triangulator, const vector<CDT::Edge>& edges, const Options& options, const SlotMapping& slot_mapping,
        const vector<Point>& points)
    {
        try
        {
            if (options.conforming)
            {
                triangulator.conformToEdges(edges);
            }
            else
            {
                triangulator.insertEdges(edges);
            }
        }
        catch (const CDT::IntersectingConstraintsError& error)
        {
            const edge_type edge = slot_mapping.edge(error.e1());
            const edge_type edge_other = slot_mapping.edge(error.e2());
            throw Error(Error::Condition::crossing_edges, {edge[0], edge[1], edge_other[0], edge_other[1]},
                format("Edges {}-{} and {}-{} cross", edge[0], edge[1], edge_other[0], edge_other[1]));
        }
        catch (const CDT::InvalidEdgeSplitVertex& error)
        {
            const edge_type edge = slot_mapping.edge(error.e1());
            const edge_type edge_other = slot_mapping.edge(error.e2());
            throw Error(Error::Condition::unplaceable_intersection, {edge[0], edge[1], edge_other[0], edge_other[1]},
                format("The intersection of edges {}-{} and {}-{} cannot be placed without breaking the triangulation, at {}, {}, {}, {}",
                    edge[0], edge[1], edge_other[0], edge_other[1], position_text(points[edge[0]]), position_text(points[edge[1]]),
                    position_text(points[edge_other[0]]), position_text(points[edge_other[1]])));
        }

        if (!options.edges_split_at_points)
        {
            check_points_on_edges(triangulator, slot_mapping, points);
        }

        if (!options.overlapping_edges_permitted)
        {
            check_overlapping_edges(triangulator, slot_mapping);
        }

        return;
    }

    //--------------------------------------------------------------------------
    void finalize(triangulator_type& triangulator, const Options& options)
    {
        if (!options.refinement)
        {
            triangulator.eraseOuterTrianglesAndHoles();
            return;
        }

        const Refinement& refinement = *options.refinement;
        CDT::TriIndUSet outer_triangles = triangulator.collectOuterTrianglesAndHoles();
        triangulator.refineTriangles(static_cast<CDT::VertInd>(refinement.added_point_limit), refinement_criterion(refinement.criterion),
            refinement.threshold, &outer_triangles, refinement.edge_length_minimum);
        triangulator.finalizeTriangulation(outer_triangles);
        return;
    }

    //--------------------------------------------------------------------------
    vector<AddedPoint> added_points(const triangulator_type& triangulator, const SlotMapping& slot_mapping)
    {
        vector<AddedPoint> points;
        for (size_t vertex_slot = slot_mapping.input_vertex_count(); vertex_slot < triangulator.vertices.size(); ++vertex_slot)
        {
            points.push_back({{triangulator.vertices[vertex_slot].x, triangulator.vertices[vertex_slot].y}, {}});
        }

        for (const auto& [piece, originals] : triangulator.pieceToOriginals)
        {
            for (CDT::VertInd vertex_slot : {piece.v1(), piece.v2()})
            {
                if (vertex_slot >= slot_mapping.input_vertex_count())
                {
                    auto& edges = points[vertex_slot - slot_mapping.input_vertex_count()].edges;
                    for (const auto& original : originals)
                    {
                        const edge_type edge = slot_mapping.edge(original);
                        if (ranges::find(edges, edge) == edges.end())
                        {
                            edges.push_back(edge);
                        }
                    }
                }
            }
        }

        for (auto& point : points)
        {
            ranges::sort(point.edges);
        }

        return points;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::triangulation::Error::Error(Condition condition, const vector<size_t>& point_slots, const string& message) :
    runtime_error(message),
    m_condition(condition),
    m_point_slots(point_slots)
{
}

//------------------------------------------------------------------------------
quetzal::triangulation::Error::Condition quetzal::triangulation::Error::condition() const
{
    return m_condition;
}

//------------------------------------------------------------------------------
const vector<size_t>& quetzal::triangulation::Error::point_slots() const
{
    return m_point_slots;
}

//------------------------------------------------------------------------------
quetzal::triangulation::Triangulation quetzal::triangulation::triangulate(const Region& region, const Options& options)
{
    const vector<Point> points = region_points(region);

    vector<CDT::V2d<double>> vertices;
    vertices.reserve(points.size());
    for (const auto& point : points)
    {
        vertices.emplace_back(point.x, point.y);
    }

    vector<CDT::Edge> edges = region_edges(region);

    Triangulation result;
    SlotMapping slot_mapping(points.size());

    if (options.duplicate_points_merged)
    {
        const CDT::DuplicatesInfo duplicates = CDT::RemoveDuplicatesAndRemapEdges(vertices, edges);
        slot_mapping = SlotMapping(points.size(), duplicates);
        for (size_t duplicate_slot : duplicates.duplicates)
        {
            result.merged_point_slots.push_back({duplicate_slot, slot_mapping.point_slot(static_cast<CDT::VertInd>(duplicates.mapping[duplicate_slot]))});
        }
    }

    const auto crossing_edges = options.crossing_edges_split ? CDT::IntersectingConstraintEdges::TryResolve : CDT::IntersectingConstraintEdges::NotAllowed;
    triangulator_type triangulator(CDT::VertexInsertionOrder::Auto, crossing_edges, options.point_on_edge_tolerance);

    try
    {
        triangulator.insertVertices(vertices);
    }
    catch (const CDT::DuplicateVertexError& error)
    {
        const size_t point_slot = slot_mapping.point_slot(error.v1());
        const size_t point_slot_other = slot_mapping.point_slot(error.v2());
        throw Error(Error::Condition::duplicate_points, {point_slot, point_slot_other},
            format("Points {} and {} are both at {}", point_slot, point_slot_other, position_text(points[point_slot])));
    }

    insert_edges(triangulator, edges, options, slot_mapping, points);
    finalize(triangulator, options);

    for (const auto& triangle : triangulator.triangles)
    {
        result.triangles.push_back({slot_mapping.point_slot(triangle.vertices[0]), slot_mapping.point_slot(triangle.vertices[1]),
            slot_mapping.point_slot(triangle.vertices[2])});
    }

    result.added_points = added_points(triangulator, slot_mapping);
    return result;
}
