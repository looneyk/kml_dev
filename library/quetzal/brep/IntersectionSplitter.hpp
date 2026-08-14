#if !defined(QUETZAL_BREP_INTERSECTION_SPLITTER_HPP)
#define QUETZAL_BREP_INTERSECTION_SPLITTER_HPP
//------------------------------------------------------------------------------
// brep
// IntersectionSplitter.hpp
//------------------------------------------------------------------------------

// TEMP The high volume half of the trace logging: everything emitted per face pair, per halfedge, or
// per segment beyond the one line that identifies it. These go to the console during a run, so on the
// larger models they cost more in wall time than the work they describe. Define to get them back
//#define QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE

#include "Mesh.hpp"
#include "Submesh.hpp"
#include "Halfedge.hpp"
#include "id.hpp"
#include "AnnotatedIntersection.hpp"
#include "border.hpp"
#include "geometry.hpp"
#include "mesh_util.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/geometry_util.hpp"
#include "quetzal/geometry/Halfspace.hpp"
#include "quetzal/geometry/Intersection.hpp"
#include "quetzal/geometry/Plane.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Ray.hpp"
#include "quetzal/geometry/Segment.hpp"
#include "quetzal/geometry/intersect.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/floating_point.hpp"
#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <ranges>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "quetzal/brep/Visualizer.hpp"
#include "quetzal/common/file_util.hpp"
#include "debug.hpp"
#include "validation.hpp"
#include <span>
#include <iostream>

namespace quetzal::brep
{

    // This class produces a set of intersection traces which are identified by matched pairs of border halfedges, each of which split the submesh into two bordered fragments.
    // These border traces can then be used to produce any CSG operation, union, intersection, difference, and symmetric difference by stitching them back together in different ways.
    // A connection is a matched pair of submesh borders identified by the first border halfedges of each submesh's exterior and interior fragments.

    // This works in two passes. In the first pass, the set of all halfedge intersections with the opposite mesh is generated.
    // These are often abstract, such as where a halfedge passes through the interior of a face and there is no other geometry on the other side to reference directly.
    // This results in a set of TraceSegment's, pairs of TracePoint's.
    // The second pass connects the isolated trace segments in sequence taking advantage of halfedge boundary rep connectivity information.
    // It instantiates any abstract vertices and edges, and splits halfedges and faces as required, uncoupling trace halfedge pairs as the last step.
    // This results in a set of trace connections, which are just the initial connection_borders_type for each connection trace.
    // These connection borders can be matched up according to the desired operation and iterated along to stitch the meshes back together (mesh_operations).

    // Intersection points are clamped to vertex positions so that exact position comparisons can be made.

    // Note: Trace... types below are short for/represent intersection traces ...
    // connection_ types are short for/represent connection traces ...
    // connection_trace_ indicates an array of the full cycle of 4 border halfedge_ids
    // connection_ indicates only the starting 4 border halfedge_ids. This only works for simple traces, never more than one trace at any given intersection vertex (deprecated ...)

    //--------------------------------------------------------------------------
    template<typename M>
    class IntersectionSplitter
    {
    public:

        using connections_type = std::vector<connection_type>; // deprecated? ...

        IntersectionSplitter(M& mesh);
        IntersectionSplitter(const IntersectionSplitter&) = default;
        IntersectionSplitter(IntersectionSplitter&&) noexcept = default;
        ~IntersectionSplitter() = default;

        IntersectionSplitter& operator=(const IntersectionSplitter&) = default;
        IntersectionSplitter& operator=(IntersectionSplitter&&) noexcept = default;

        // A connection is an independent intersection trace that splits each of the two submeshes into exterior and interior fragments, a sequence of sequential border halfedges
        // Since submeshes are not necessarily convex, there may be several such connections
        // Returns a vector of connection traces, the full sequence of matching halfedges along the intersection {A exterior, A interior, B exterior, B interior}
        // Only the positions and texcoords of newly created vertices are valid, normals (currently set to face normal) will need to be calculated/recalculated for the border vertices
        // (Note that in the usual case where borders are or become surface edges, this could in fact calculate normals correctly ...)
        connection_traces_type connections(const id_type submesh_id_a, const id_type submesh_id_b);

    private:

        using value_type = M::value_type;
        using point_type = M::point_type;
        using vector_type = point_type::vector_type;
        using ray_type = geometry::Ray<typename M::vector_traits>;
        using segment_type = geometry::Segment<typename M::vector_traits>;
        using plane_type = geometry::Plane<typename M::vector_traits>;
        using polygon_type = geometry::Polygon<typename M::vector_traits>;
        using intersection_type = geometry::Intersection<typename M::vector_traits>;
        using halfedge_type = M::halfedge_type;
        using face_type = M::face_type;
        using vertex_type = M::vertex_type;

        //----------------------------------------------------------------------
        // Returned by find_face_intersections_linear (0..2 still?) ...
        struct HalfedgeIntersection
        {
            id_type m_halfedge_id_a = nullid;
            id_type m_halfedge_id_b = nullid;
            point_type m_position;
        };

        //----------------------------------------------------------------------
        // An point along an intersection trace between face_a and face_b.
        // A halfedge_id that is nullid indicates that the intersection occurs in the interior as opposed to on the boundary of the face (floating).
        struct TracePoint
        {
            point_type position;
            id_type face_id_a = nullid;
            id_type halfedge_id_a = nullid;
            id_type face_id_b = nullid;
            id_type halfedge_id_b = nullid;

        };

        //----------------------------------------------------------------------
        // The segment (often virtual) of intersection between faces in two submeshes, before connecting them into connection traces
        // invariants: p0.face_id_a == p1.face_id_a and p0.face_id_b == p1.face_id_b
        struct TraceSegment
        {
            TracePoint p0;
            TracePoint p1;
        };

        // invariants: trace[i].p1 == trace[i + 1 mod size].p0
        using TraceSegments = std::vector<TraceSegment>;
        using Traces = std::vector<TraceSegments>;

        //----------------------------------------------------------------------
        // A reference to the half of a TracePoint that corresponds to one of the two submeshes it references
        struct SubmeshTracePointRef
        {
            TracePoint* m_ptrace_point = nullptr;
            bool m_side_a = false;

            bool operator==(const SubmeshTracePointRef&) const = default;

            const point_type& position() const { return m_ptrace_point->position; }
            id_type& face_id() const { return m_side_a ? m_ptrace_point->face_id_a : m_ptrace_point->face_id_b; }
            id_type& halfedge_id() const { return m_side_a ? m_ptrace_point->halfedge_id_a : m_ptrace_point->halfedge_id_b; }
        };

        void clear();

        // Computes find_face_pair_segment for every bounding-box-overlapping pair of faces across the
        // two submeshes -- the full set of independently discovered trace segments, before connecting them
        TraceSegments find_intersections(id_type submesh_id_a, id_type submesh_id_b);

        // Computes the intersection trace segment between face_id_a and face_id_b, if any, in complete
        // isolation from any other pair -- no continuation/search logic, no mutation, no notion of
        // "current trace". Parallel (including coplanar) face pairs are out of scope, return nullopt
        std::optional<TraceSegment> find_face_intersections(id_type face_id_a, id_type face_id_b);

        // Try to find an intersection of the two faces, used when looking for a new connection trace
        // Returns {{id_a0, id_b0, position0}, {id_a1, id_b1, position1}} where the positions can be can be:
        //   no intersection (n == 0) (termination condition, no trace)
        //   a single point intersection (n == 1) (termination condition, no trace, currently single points are ignored, but maybe should split halfedges as required? ...)
        //   a virtual segment, two points on separate halfedges (n == 2) (id != nullid) or floating (ids == nullid)
        //   a segment of an existing halfedge (n == 2) (id0 == id1)
        // Note: only intersctions of convex faces are currently supported, so for now there should only be one segment of intersection ...
        std::vector<HalfedgeIntersection> find_face_intersections_linear(id_type face_id_a, id_type face_id_b, const vector_type& trace_direction);

        using AnnotatedPoint = quetzal::brep::AnnotatedPoint<typename M::vector_traits>;
        using AnnotatedSegment = quetzal::brep::AnnotatedSegment<typename M::vector_traits>;
        using AnnotatedIntersection = quetzal::brep::AnnotatedIntersection<typename M::vector_traits>;
        using annotated_intersections_type = quetzal::brep::annotated_intersections_type<typename M::vector_traits>;

        // Find the face intersection with the plane of the opposing face
        // Supports non-convex faces
        // Returns both segment and isolated halfedge vertex intersections
        // Returns
        //   nullopt: the face is in the plane (entire face intersection, no individual intersections)
        //   empty vector: no intersection
        //   otherwise: a sequence of annotated intersections (point/segment, halfedge_id) in trace_direction order
        std::optional<annotated_intersections_type> face_plane_intersection(const face_type& face, const plane_type& plane, const vector_type& trace_direction);

        // temp translation layer ...
        std::optional<AnnotatedSegment> face_plane_intersection_wrapper(const face_type& face, const plane_type& plane, const vector_type& trace_direction);

        intersection_type halfedge_plane_intersection(const halfedge_type& halfedge, const plane_type& plane);

        // Splits the edge every trace point stands on, so that each segment runs vertex to vertex
        void split_trace_segments(TraceSegments& segments);

        // Create a halfedge pair for each trace segment where at least one of its endpoint halfedge_id's is nullid
        // No connections are made, although endpoint vertices may be incident with other vertices
        // The edges created are two halfedge, mutually partnered, holes
        void create_isolated_intersection_point_edges(TraceSegments& segments);

        // Updates the halfedge_id in trace points after split_edge as required
        void update_trace_points(TraceSegments& segments, id_type halfedge_id);

        // Updates the halfedge_id in trace points after create_edge_partial as required
        // The spur is spliced in ahead of the anchor halfedge, so the loop steps out of the anchor
        // position through the spur before reaching the anchor halfedge itself. A segment ending at
        // the anchor holds the halfedge following its own, so every one arriving along the halfedge
        // the spur was spliced in after moves to the spur
        void update_trace_points_spur(TraceSegments& segments, bool side_a, id_type halfedge_id_anchor, id_type halfedge_id_spur);

        // Updates the halfedge_id in trace points after a segment has been instantiated as required
        // Instantiating a segment divides the face it lies in, leaving a halfedge at each of its ends
        // on either side of the division, so any segment still to be instantiated can be left holding
        // the one that went the other way. Every segment meeting this one in that face bounds the
        // region the trace cuts off, whichever trace it belongs to, so it lies on the interior side
        // and takes the halfedge of that loop at its own end. An end left behind shows up as the two
        // ends of its segment disagreeing on face, face ids being reassigned correctly by the division
        void update_trace_points_connection(Traces& traces, id_type halfedge_id_a_interior, id_type halfedge_id_b_interior);

        // Connects a set of face/face intersection segments into closed-loop intersection traces
        // Each segment is the intersection of one face pair, so a trace is a walk through face pairs and
        // the successor of a segment follows from mesh connectivity at the endpoint it leaves through:
        // crossing the interior of an edge moves to that edge's partner face, and the opposite side keeps
        // its face. Endpoints that land on a vertex are the only ones with a choice, since the trace may
        // continue into any face of that vertex fan, or stay in the same face having only touched it
        Traces order_intersections(const TraceSegments& segments);

        // Reference to one endpoint of one segment, used internally by order_intersections
        struct SegmentEndpointRef
        {
            size_t segment_pos;
            int endpoint_slot; // 0 = p0, 1 = p1
        };

        // Every segment end, grouped by the position it stands at, which is all the connectivity
        // ordering needs. Built once from the segments and not maintained, since ordering does not
        // modify anything
        using PositionIndex = std::map<point_type, std::vector<SegmentEndpointRef>>;

        PositionIndex index_positions(const TraceSegments& segments);

        // The end continuing the trace out of trace_point. A segment is the intersection of one face
        // pair, so leaving through an edge of one submesh moves to the adjacent face there while the
        // face on the other side carries over: the continuation is the end at this position sharing a
        // face id with trace_point. Only an original vertex meeting a vertex changes both at once, and
        // that is ambiguous only where more than one trace passes through the position
        std::optional<SegmentEndpointRef> successor_position(const TraceSegments& segments, const PositionIndex& position_index, const std::vector<bool>& used, const TracePoint& trace_point, size_t segment_pos) const;

        // Instantiates all intersection traces, producing connection traces
        connection_traces_type instantiate_connection_traces(Traces& traces);

        // Instantiates an intersection trace producing a single connection trace
        // Connects the ordered trace segments, adding and splitting faces and halfedges as required
        connection_trace_type instantiate_trace(Traces& traces, size_t trace_pos);

        // Instantiates one side of one segment, connecting its two ends if no edge runs between them yet
        // Returns the halfedge along the segment in exterior_direction
        // halfedge_id0 is resolved to the halfedge at its vertex in the face halfedge_id1 is in, which
        // differ where an earlier segment split that face
        id_type instantiate_segment_side(id_type halfedge_id0, id_type halfedge_id1, const vector_type& exterior_direction);

        // Closes a loop lying wholly inside a face, which bounds a region rather than dividing the face:
        // the loop the region is on becomes a face of its own and the face keeps a hole where it was
        // Returns the halfedge along the segment in exterior_direction
        id_type close_enclosed_loop(id_type hole_id, id_type halfedge_id0, id_type halfedge_id1, const vector_type& exterior_direction);

        void uncouple(connection_trace_type& connection);

        // The position to texcoord mapping of a face, created on the first request for it
        // Held so that it describes the face as it was found, since adding edges for the intersection
        // points in a face changes the face that any later request would be calculated from
        const geometry::PolygonMapping<typename M::vertex_attributes_type>& face_mapping(id_type face_id);

        // Create a pair of connected partner halfedges where neither originates at an existing vertex
        // Returns the id of the halfedge in the direction of the segment
        id_type create_edge_internal(id_type face_id, const segment_type& segment);

        // Create a pair of connected partner halfedges splitting the halfedge vertex and extending to a new vertex at position
        // Returns the id of the halfedge with its vertex at the original halfedge vertex position
        id_type create_edge_partial(id_type halfedge_id, const point_type& position);

        // Create a pair of connected partner halfedges between two existing vertices without the need for splitting the face
        // Used to connect an internal floating chain to an edge, thus removing the floating chain, but not creating a new face
        // Returns the id of the halfedge from the halfedge_id0 to the halfedge_id1 vertex position
        // compare this with mesh_util connect ...
        id_type create_edge_connect(id_type halfedge_id0, id_type halfedge_id1, bool coupled = true);

        void insert_trace_intersection(id_type halfedge_id_a_exterior, id_type halfedge_id_a_interior, id_type halfedge_id_b_exterior, id_type halfedge_id_b_interior);

        id_type exterior_halfedge(id_type halfedge_id, const vector_type& exterior_direction) const;

        id_type interior_partner(id_type halfedge_id_exterior, const vector_type& exterior_direction) const;

        id_type find_halfedge(id_type face_id, const point_type& position);

        // For every trace point at this position in this submesh, assign it the given halfedge_id
        void set_trace_points_halfedge(TraceSegments& segments, bool side_a, const point_type& position, id_type halfedge_id);

        // position_a is farther along than position_b in direction
        // position_a and position_b should both fall on a line with the given direction or else the result will be ambiguous
        bool farther(const point_type& position_a, const point_type& position_b, const vector_type& direction);

        // For every vertex position in intersection trace segments, for each vertex at that position,
        // set the position to the same "canonical" position (the value of the first position encountered there)
        void align_segment_positions(TraceSegments& segments);

        void validate_segments(const TraceSegments& segments);

        // Validates that each intersection trace segment is oriented in trace_direction and that consecutive segment endpoints match
        void validate_intersection_traces(const Traces& traces);

        void validate_connections(const connection_traces_type& connections);

        void dump_trace_comparison(const TraceSegments& segments, const connection_trace_type& connection_trace) const; // TEMP diagnostic

        M& m_mesh;

        // Used for validation
        std::map<id_type, geometry::PolygonMapping<typename M::vertex_attributes_type>> m_face_mappings;

        std::set<id_type> m_connection_halfedge_ids; // All of the halfedge ids that are part of any continuous connection trace
        std::set<point_type> m_connection_positions; // All of the vertex positons that are part of any continuous connection trace
        connection_traces_type m_connections;

        size_t m_stage_count = 0; // logging ...
    };

    //--------------------------------------------------------------------------
    // Add parameter for right vector ...
    template<typename M>
    void visualize(const M& mesh, std::span<id_type> submesh_ids, const typename M::point_type& eye, const typename M::point_type& target, std::optional<typename M::value_type> fov, const std::string& filename)
    {
        using point_type = M::point_type;

        auto forward = normalize(target - eye);
        auto up = M::vector_type::z_direction();
        auto right = normalize(cross(forward, up));
        if (math::float_eq0(right.norm_squared()))
        {
            // brute force vertical view correction, should support continuous rotation through vertical, but we don't have enough information here ...
            // should at least be perpendicular, not fixed ...
            right = M::vector_type::x_direction(); // ...
        }
        quetzal::geometry::Orientation<typename M::vector_traits> orientation = {forward, right};
        quetzal::geometry::Projection<typename M::vector_traits> projection;
        if (fov)
        {
            quetzal::geometry::OrientedPoint<typename M::vector_traits> viewpoint = {eye, orientation};
            projection.init(viewpoint, *fov);
        }
        else
        {
            projection.init(orientation);
        }
        quetzal::brep::Visualizer<M> visualizer(projection, 0.5, 0.25);
        for (id_type submesh_id : submesh_ids)
        {
            visualizer.draw_submesh(mesh.submesh(submesh_id));
        }
        visualizer.write(filename);
        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void visualize(const M& mesh, const std::string& name, std::array<id_type, 2> submesh_ids)
    {
        auto bounds_a = mesh.submesh(submesh_ids[0]).bounds();
        auto bounds_b = mesh.submesh(submesh_ids[1]).bounds();
        auto bounds = op_union(bounds_a, bounds_b);
        auto target = bounds.centroid();
        auto extent = bounds.extent();
        auto extent_max = extent[extent.longest_component()];

        typename M::point_type eye = {0.5f, -2.0f, 1.5f}; // norm about 2.5
        eye *= extent_max * M::val(0.5);

        // projection type selection ...
//        std::optional<typename M::value_type> fov = std::nullopt;
        std::optional<typename M::value_type> fov = math::PiHalf<typename M::value_type>;

        std::cout << std::format("[svg: {}]", name) << std::endl;
        auto submesh_ids_visualized = std::vector<id_type>{submesh_ids[0], submesh_ids[1]};
        visualize(mesh, submesh_ids_visualized, eye, target, fov, std::format("debug/{}.svg", name));
        submesh_ids_visualized = {submesh_ids[0]};
        visualize(mesh, submesh_ids_visualized, eye, target, fov, std::format("debug/{}_a.svg", name));
        submesh_ids_visualized = {submesh_ids[1]};
        visualize(mesh, submesh_ids_visualized, eye, target, fov, std::format("debug/{}_b.svg", name));
        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    ValidationState validate_dump(const M& mesh, const std::string& name)
    {
        std::cout << std::format("[dump: {}]", name) << std::endl;
        std::ofstream ofs(std::format("debug/{}.txt", name), std::ios_base::binary);
        OstreamRedirect redirect(std::cout, ofs);
        ValidationState state(true);
        state = validate(mesh);
        dump_faces(mesh);
        dump_submeshes(mesh);
        return state;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    ValidationState debug_log(const M& mesh, const std::string& name, std::array<id_type, 2> submesh_ids)
    {
        ValidationState state;
        if constexpr (debug_validate_dump)
        {
            state = validate_dump(mesh, name);
        }
        if constexpr (debug_visualize)
        {
            visualize(mesh, name, submesh_ids);
        }
        return state;
    }

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::IntersectionSplitter<M>::IntersectionSplitter(M& mesh) :
    m_mesh(mesh),
    m_connection_halfedge_ids(),
    m_connection_positions(),
    m_connections(),
    m_stage_count(0)
{
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::connection_traces_type quetzal::brep::IntersectionSplitter<M>::connections(const id_type submesh_id_a, const id_type submesh_id_b)
{
    clear();
std::array<id_type, 2> submesh_ids = {submesh_id_a, submesh_id_b};
std::cout << std::format("DEBUG connections() entry: submesh_id_a {} submesh_id_b {}, halfedge_store {}, vertex_store {}, face_store {}, surface_store {}, submesh_store {}, submesh_count {}", id_string(submesh_id_a), id_string(submesh_id_b), m_mesh.halfedge_store().size(), m_mesh.vertex_store().size(), m_mesh.face_store().size(), m_mesh.surface_store().size(), m_mesh.submesh_store().size(), m_mesh.submesh_count()) << std::endl;
#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
for (id_type face_id : m_mesh.submesh(submesh_id_a).face_ids()) { std::cout << std::format("    submesh_a face {}", id_string(face_id)) << std::endl; }
for (id_type face_id : m_mesh.submesh(submesh_id_b).face_ids()) { std::cout << std::format("    submesh_b face {}", id_string(face_id)) << std::endl; }
#endif
assert(debug_log(m_mesh, std::format("model{}", m_stage_count++), submesh_ids).value() >= 0);

    // Find all intersection traces in the original submeshes
    // Find all face/face intersection segments between the two submeshes
    // Order them into a set of set of distinct intersection traces
    // The submeshes are not modified at this stage
    auto segments = find_intersections(submesh_id_a, submesh_id_b);
    if (segments.empty())
    {
        return {};
    }

    align_segment_positions(segments); // Set vertices and intersection points at a common, likely imprecise, position to a canonical position value
//std::cout << "segments" << '\n' << segments << '\n';

    split_trace_segments(segments); // Split intersection halfedges so that all halfedge intersections are now on halfedge vertices

    create_isolated_intersection_point_edges(segments); // Create edges for intersection points in the interior of a face

    validate_segments(segments);

    // All intersections are now either regular edges, paired halfedge holes, or a pair of opposing vertices to be connected

    auto traces = order_intersections(segments);
//std::cout << "traces" << '\n' << traces << '\n';
    validate_intersection_traces(traces);

    // Instantiate each intersection trace, splitting and adding halfedges and faces as required
    // Uncouple the trace halfedges to produce the final connection traces of border halfedges
    m_connections = instantiate_connection_traces(traces);

    validate_connections(m_connections);

    return m_connections;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::clear()
{
    m_face_mappings.clear();
    m_connection_halfedge_ids.clear();
    m_connection_positions.clear();
    m_connections.clear();
    m_stage_count = 0;
    return;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::IntersectionSplitter<M>::TraceSegments quetzal::brep::IntersectionSplitter<M>::find_intersections(const id_type submesh_id_a, const id_type submesh_id_b)
{
    TraceSegments trace_segments;

    const auto face_ids_a = m_mesh.submesh(submesh_id_a).face_ids();
    const auto face_ids_b = m_mesh.submesh(submesh_id_b).face_ids();

    for (id_type face_id_a : face_ids_a)
    {
        for (id_type face_id_b : face_ids_b)
        {
            if (disjoint(m_mesh.face(face_id_a).bounds(), m_mesh.face(face_id_b).bounds()))
            {
                continue;
            }

            auto segment = find_face_intersections(face_id_a, face_id_b);
            if (segment)
            {
                trace_segments.push_back(*segment);
                // trace_segments.append_range(std::views::as_rvalue(segments));
            }
        }
    }

    return trace_segments;
}

//------------------------------------------------------------------------------
template<typename M>
std::optional<typename quetzal::brep::IntersectionSplitter<M>::TraceSegment> quetzal::brep::IntersectionSplitter<M>::find_face_intersections(const id_type face_id_a, const id_type face_id_b)
{
    if (parallel(m_mesh.face(face_id_a), m_mesh.face(face_id_b)))
    {
        return std::nullopt; // Coplanar/parallel handling is out of scope for this redesign ...
    }

    vector_type trace_direction = normalize(cross(m_mesh.face(face_id_a).attributes().normal(), m_mesh.face(face_id_b).attributes().normal()));
    auto ixs0 = find_face_intersections_linear(face_id_a, face_id_b, trace_direction);
    if (ixs0.empty() || ixs0.size() == 1)
    {
        return std::nullopt;
    }

    assert(ixs0.size() == 2); // Limited to convex faces for now, no more than a single segment of intersection ...
    TracePoint p0 = {ixs0[0].m_position, face_id_a, ixs0[0].m_halfedge_id_a, face_id_b, ixs0[0].m_halfedge_id_b};
    TracePoint p1 = {ixs0[1].m_position, face_id_a, ixs0[1].m_halfedge_id_a, face_id_b, ixs0[1].m_halfedge_id_b};
    return TraceSegment{p0, p1};
}

//------------------------------------------------------------------------------
template<typename M>
std::vector<typename quetzal::brep::IntersectionSplitter<M>::HalfedgeIntersection> quetzal::brep::IntersectionSplitter<M>::find_face_intersections_linear(id_type face_id_a, id_type face_id_b, const vector_type& trace_direction)
{
    using geometry::Locus;
#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << std::format("find_face_intersections_linear fa {}, fb {}", face_id_a, face_id_b) << std::endl;
#endif

    const auto& face_a = m_mesh.face(face_id_a);
    const auto& face_b = m_mesh.face(face_id_b);

    const auto plane_a = to_plane(face_a);
    const auto plane_b = to_plane(face_b);

    auto ix_a = face_plane_intersection_wrapper(face_a, plane_b, trace_direction);
    if (!ix_a)
    {
        return {};
    }

    id_type id_a0 = ix_a->m_halfedge_id0;
    id_type id_a1 = ix_a->m_halfedge_id1;
    segment_type segment_a = *ix_a;

    auto ix_b = face_plane_intersection_wrapper(face_b, plane_a, trace_direction);
    if (!ix_b)
    {
        return {};
    }

    id_type id_b0 = ix_b->m_halfedge_id0;
    id_type id_b1 = ix_b->m_halfedge_id1;
    segment_type segment_b = *ix_b;

    if (!intersects(segment_a, segment_b)) // "skew" polygons
    {
        return {};
    }

    // Intersect the two resulting segments with endpoints ordered by trace_direction

    assert(float_eq(segment_a.direction(), trace_direction));
    assert(float_eq(segment_b.direction(), trace_direction));
/*
std::cout << std::format("h a0 {}", vertex_position(m_mesh.halfedge(id_a0))) << std::endl;
std::cout << std::format("h a1 {}", vertex_position(m_mesh.halfedge(id_a1))) << std::endl;
std::cout << std::format("h b0 {}", vertex_position(m_mesh.halfedge(id_b0))) << std::endl;
std::cout << std::format("h b1 {}", vertex_position(m_mesh.halfedge(id_b1))) << std::endl;
std::cout << std::format("s a0 {}", segment_a.endpoint(0)) << std::endl;
std::cout << std::format("s a1 {}", segment_a.endpoint(1)) << std::endl;
std::cout << std::format("s b0 {}", segment_b.endpoint(0)) << std::endl;
std::cout << std::format("s b1 {}", segment_b.endpoint(1)) << std::endl;
*/
//assert(halfedge_id1 == nullid || ne_or_exact(vertex_position(m_mesh.halfedge(halfedge_id1)), segment.endpoint(1)));
//assert(ne_or_exact(segment_a.endpoint(0), segment_b.endpoint(0)));
//assert(ne_or_exact(segment_a.endpoint(1), segment_b.endpoint(1)));

    segment_type segment;

    if (float_eq(segment_a.endpoint(0), segment_b.endpoint(0)))
    {
        if (id_a0 != nullid && vertex_position(m_mesh.halfedge(id_a0)) == segment_a.endpoint(0))
        {
            segment.endpoint(0) = segment_a.endpoint(0);
        }
        else if (id_b0 != nullid && vertex_position(m_mesh.halfedge(id_b0)) == segment_b.endpoint(0))
        {
            segment.endpoint(0) = segment_b.endpoint(0);
        }
        else
        {
            segment.endpoint(0) = segment_a.endpoint(0);
        }
    }
    else if (farther(segment_b.endpoint(0), segment_a.endpoint(0), trace_direction))
    {
        segment.endpoint(0) = segment_b.endpoint(0);
        // The endpoint comes from the other face, so where it stands on this one has to be determined rather
        // than assumed: it is floating only when it lies within the face, which is exactly what
        // find_halfedge reports by returning nullid. It lies on the boundary whenever this face's own
        // intersection runs along an edge rather than crossing the interior
        id_a0 = find_halfedge(face_id_a, segment.endpoint(0));
    }
    else
    {
        segment.endpoint(0) = segment_a.endpoint(0);
        id_b0 = find_halfedge(face_id_b, segment.endpoint(0));
    }

    if (float_eq(segment_a.endpoint(1), segment_b.endpoint(1)))
    {
        if (id_a1 != nullid && vertex_position(m_mesh.halfedge(id_a1)) == segment_a.endpoint(1))
        {
            segment.endpoint(1) = segment_a.endpoint(1);
        }
        else if (id_b1 != nullid && vertex_position(m_mesh.halfedge(id_b1)) == segment_b.endpoint(1))
        {
            segment.endpoint(1) = segment_b.endpoint(1);
        }
        else
        {
            segment.endpoint(1) = segment_a.endpoint(1);
        }
    }
    else if (farther(segment_b.endpoint(1), segment_a.endpoint(1), trace_direction))
    {
        segment.endpoint(1) = segment_a.endpoint(1);
        id_b1 = find_halfedge(face_id_b, segment.endpoint(1));
    }
    else
    {
        segment.endpoint(1) = segment_b.endpoint(1);
        id_a1 = find_halfedge(face_id_a, segment.endpoint(1));
    }
//std::cout << std::format("s0 {}", segment.endpoint(0)) << std::endl;
//std::cout << std::format("s1 {}", segment.endpoint(1)) << std::endl;

    if (!float_eq(segment.direction(), trace_direction))
    {
        // The two per-face segments only share a single point along trace_direction (or a floating-point-scale
        // sliver whose sign is noise, from computing that point two different ways) -- not a real segment of
        // intersection. Report it as the single-point case the caller already knows how to ignore.
        return {{id_a0, id_b0, segment.endpoint(0)}};
    }

    HalfedgeIntersection ix0 = {id_a0, id_b0, segment.endpoint(0)};
    HalfedgeIntersection ix1 = {id_a1, id_b1, segment.endpoint(1)};
    return {ix0, ix1};
}

//------------------------------------------------------------------------------
template<typename M>
std::optional<typename quetzal::brep::IntersectionSplitter<M>::annotated_intersections_type> quetzal::brep::IntersectionSplitter<M>::face_plane_intersection(const face_type& face, const plane_type& plane, const vector_type& trace_direction)
{
#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << std::format("face_plane_intersection face {} {}, plane {}, trace_direction {}", face.id(), face.attributes().normal(), plane, trace_direction) << std::endl;
#endif
    using geometry::Locus;

    if (coplanar(plane, to_plane(face)))
    {
        return std::nullopt;
    }

    // Find all intersection points and segments and calculate a distance measurement with respect to trace_direction

    auto direction_key = [&](const point_type& position) -> value_type
    {
        return dot(position.vector(), trace_direction);
    };

    // tuple elements: AnnotatedIntersection, direction_key
    using ordered_annotation_intersection_type = std::tuple<AnnotatedIntersection, value_type>;
    std::vector<ordered_annotation_intersection_type> ixs;

    for (const auto& halfedge : face.halfedges_ext())
    {
        auto ix = halfedge_plane_intersection(halfedge, plane);
#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << std::format("face_plane_intersection halfedge_plane_intersection {}", ix) << std::endl;
#endif
        if (ix.empty())
        {
            continue;
        }

        if (ix.locus() == Locus::Point)
        {
            AnnotatedIntersection annotated{Locus::Point, {ix.point(), halfedge.id()}, {}};
            ixs.push_back({annotated, direction_key(ix.point())});
        }
        else if (ix.locus() == Locus::Segment)
        {
            // Intersection is an edge of the face, accept only the halfedge partner in the trace_direction
            auto segment = to_segment(halfedge);
            assert(ix.segment() == segment);
            if (direction_compare(ix.segment().direction(), trace_direction) <= 0)
            {
                continue;
            }

            AnnotatedIntersection annotated{Locus::Segment, {}, {segment, halfedge.id(), halfedge.next_id()}};
            ixs.push_back({annotated, direction_key(segment.endpoint(0))});
        }
        else
        {
            assert(false);
        }
    }

    if (ixs.empty())
    {
        return annotated_intersections_type();
    }

    // Sort them in trace_direction order
    std::ranges::sort(ixs, std::ranges::less{}, [](const auto& t) { return std::get<1>(t); });

#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << std::format("face_plane_intersection sorted {}", ixs) << std::endl;
#endif
    // Group them into segment and point intersections

    // We're looking for parts of the face that are either in or external to the plane, and creating a segment

    // Vertex positions in the face are characterized relative to the plane
    geometry::HalfSpace<typename M::vector_traits> halfspace(plane);
    auto interior = [&](const point_type& position)
    {
        return halfspace.compare(position) < 0;
    };

    annotated_intersections_type intersections;

    // Excursion as used here is a face halfedge making a plane crossing that starts an intersection segment.
    // The first crossing determines which side of the plane (exterior/interior to the other submesh) is used for reference
    // when creating intersection segments and points. This depends on the face's winding relative to the plane.
    auto next_is_interior = [&](const AnnotatedIntersection& ix)
    {
        id_type halfedge_id = ix.m_locus == Locus::Point ? ix.m_point.m_halfedge_id : ix.m_segment.m_halfedge_id1;
        return interior(m_mesh.halfedge(halfedge_id).next().vertex_attributes().position());
    };

    bool excursion_starts_on_interior = next_is_interior(std::get<0>(ixs.front()));
    auto is_excursion_start = [&](const AnnotatedIntersection& ix)
    {
        return ixs.size() > 1 && next_is_interior(ix) == excursion_starts_on_interior;
    };

    std::optional<AnnotatedPoint> ap_prev; // the start of a new segment, an intersection on a halfedge making an excursion

    for (const auto& [ix, _] : ixs)
    {
        if (ix.m_locus == Locus::Segment)
        {
            const auto& as = ix.m_segment;

            if (ap_prev)
            {
                segment_type segment = {*ap_prev, as.endpoint(0)};
                intersections.push_back({Locus::Segment, {}, {segment, ap_prev->m_halfedge_id, as.m_halfedge_id0}});
                ap_prev.reset();
            }

            ap_prev.emplace(as.endpoint(0), as.m_halfedge_id0);
            continue;
        }

        assert(ix.m_locus == Locus::Point);
        const auto& ap = ix.m_point;
        auto& halfedge = m_mesh.halfedge(ap.m_halfedge_id);

        if (ap_prev)
        {
            segment_type segment = {*ap_prev, ap};
            intersections.push_back({Locus::Segment, {}, {segment, ap_prev->m_halfedge_id, ap.m_halfedge_id}});
            ap_prev.reset();
        }
        else if (is_excursion_start(ix))
        {
            ap_prev.emplace(ap);
        }
        else if (ap == halfedge.vertex_attributes().position()) // A halfedge vertex intersection with the plane
        {
            intersections.push_back({Locus::Point, ap, {}});
        }
    }

    assert(!ap_prev);
    return intersections;
}

//------------------------------------------------------------------------------
template<typename M>
std::optional<typename quetzal::brep::IntersectionSplitter<M>::AnnotatedSegment> quetzal::brep::IntersectionSplitter<M>::face_plane_intersection_wrapper(const face_type& face, const plane_type& plane, const vector_type& trace_direction)
{
    using geometry::Locus;

    auto result = face_plane_intersection(face, plane, trace_direction);
#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << std::format("face_plane_intersection {}", result ? std::format("{}", std::span<const AnnotatedIntersection>(*result)) : std::string("nullopt")) << std::endl;
#endif
    if (!result || result->empty())
    {
        return std::nullopt; // coplanar or no intersection, same as the original for both
    }

    assert(result->size() == 1); // only convex faces are supported here, like the original face_plane_intersection

    const AnnotatedIntersection& annotated = result->front();
    if (annotated.m_locus == Locus::Point)
    {
        return std::nullopt; // single-vertex graze, not yet supported, same as the original
    }

    assert(annotated.m_locus == Locus::Segment);
    return annotated.m_segment;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::IntersectionSplitter<M>::intersection_type quetzal::brep::IntersectionSplitter<M>::halfedge_plane_intersection(const halfedge_type& halfedge, const plane_type& plane)
{
    using geometry::Locus;

    const auto segment = to_segment(halfedge);
//std::cout << std::format("halfedge_plane_intersection hid {} segment {} plane {}", halfedge.id(), segment, plane) << std::endl;
    const auto ix = intersection(segment, plane);
    if (ix.locus() == Locus::Point)
    {
        if (float_eq(ix.point(), segment.endpoint(1))) // Halfedges are half-open intervals
        {
            return {};
        }

        if (float_eq(ix.point(), segment.endpoint(0))) // Denoise
        {
            return segment.endpoint(0);
        }

        assert(segment.contains(ix.point()));
        return ix;
    }
    else if (ix.locus() == Locus::Segment)
    {
        assert(ix.segment() == segment);
        return ix;
    }

    assert(ix.empty());
    return {};
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::split_trace_segments(TraceSegments& segments)
{
    for (auto& segment : segments)
    {
        for (TracePoint* ptrace_point : {&segment.p0, &segment.p1})
        {
            const auto& position = ptrace_point->position;

            for (bool side_a : {true, false})
            {
                SubmeshTracePointRef ref = {ptrace_point, side_a};
                id_type halfedge_id = ref.halfedge_id();

                if (halfedge_id == nullid || vertex_position(m_mesh.halfedge(halfedge_id)) == position)
                {
                    continue;
                }

                assert(position != vertex_position(m_mesh.halfedge(halfedge_id).next())); // Verify the half-open invariant

                id_type partner_id_orig = m_mesh.halfedge(halfedge_id).partner_id();
                split_edge(m_mesh, halfedge_id, position);
                update_trace_points(segments, halfedge_id);
                update_trace_points(segments, partner_id_orig);
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::create_isolated_intersection_point_edges(TraceSegments& segments)
{
    for (auto& segment : segments)
    {
        for (bool side_a : {true, false})
        {
            SubmeshTracePointRef ref0 = {&segment.p0, side_a};
            SubmeshTracePointRef ref1 = {&segment.p1, side_a};

            bool floating0 = ref0.halfedge_id() == nullid;
            bool floating1 = ref1.halfedge_id() == nullid;

            if (!floating0 && !floating1)
            {
                continue;
            }

            id_type face_id = ref0.face_id();

            if (floating0 && floating1)
            {
                id_type halfedge_id = create_edge_internal(face_id, segment_type{ref0.position(), ref1.position()});
                m_mesh.face(face_id).create_hole(halfedge_id);
                set_trace_points_halfedge(segments, side_a, ref0.position(), halfedge_id);
                set_trace_points_halfedge(segments, side_a, ref1.position(), m_mesh.halfedge(halfedge_id).next_id());
                continue;
            }

            // Create the perimeter to isolated point edge and update the halfedge_id for the formerly isolated point
            const SubmeshTracePointRef& ref_anchor = floating0 ? ref1 : ref0;
            const SubmeshTracePointRef& ref_floating = floating0 ? ref0 : ref1;

            id_type halfedge_id = create_edge_partial(ref_anchor.halfedge_id(), ref_floating.position());

            // The spur goes in ahead of whichever occurrence of the anchor position holds its corner,
            // which is not necessarily the one the trace point named, and the tip halfedge runs back
            // into it
            id_type halfedge_id_anchor = m_mesh.halfedge(m_mesh.halfedge(halfedge_id).next_id()).next_id();

            set_trace_points_halfedge(segments, side_a, ref_floating.position(), m_mesh.halfedge(halfedge_id).next_id());
            update_trace_points_spur(segments, side_a, halfedge_id_anchor, halfedge_id);

            if (!floating0) // The spur runs along this segment, which leaves the anchor through it rather than through the anchor halfedge
            {
                ref0.halfedge_id() = halfedge_id;
            }
            else // The segment runs into the anchor along the spur, so it ends on the halfedge the tip runs back into
            {
                ref1.halfedge_id() = halfedge_id_anchor;
            }
        }
    }

    return;
}

template<typename M>
void quetzal::brep::IntersectionSplitter<M>::update_trace_points(TraceSegments& segments, const id_type halfedge_id)
{
    assert(halfedge_id != nullid);

    for (auto& segment : segments)
    {
        for (TracePoint* ptrace_point : {&segment.p0, &segment.p1})
        {
            for (bool side_a : {true, false})
            {
                SubmeshTracePointRef ref = {ptrace_point, side_a};
                if (ref.halfedge_id() == halfedge_id && !halfedge_contains(m_mesh.halfedge(halfedge_id), ref.position()))
                {
                    ref.halfedge_id() = m_mesh.halfedge(halfedge_id).next_id();
                    assert(halfedge_contains(m_mesh.halfedge(ref.halfedge_id()), ref.position()));
                }
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::update_trace_points_spur(TraceSegments& segments, const bool side_a, const id_type halfedge_id_anchor, const id_type halfedge_id_spur)
{
    for (auto& segment : segments)
    {
        SubmeshTracePointRef ref0 = {&segment.p0, side_a};
        SubmeshTracePointRef ref1 = {&segment.p1, side_a};

        if (ref1.halfedge_id() == halfedge_id_anchor && ref0.halfedge_id() != nullid && m_mesh.halfedge(ref0.halfedge_id()).next_id() == halfedge_id_spur)
        {
            ref1.halfedge_id() = halfedge_id_spur;
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::IntersectionSplitter<M>::Traces quetzal::brep::IntersectionSplitter<M>::order_intersections(const TraceSegments& segments)
{
    PositionIndex position_index = index_positions(segments);
    std::vector<bool> used(segments.size(), false);
    Traces traces;

    for (size_t pos = 0; pos < segments.size(); ++pos)
    {
        if (used[pos])
        {
            continue;
        }

        TraceSegments trace;
        const point_type position0 = segments[pos].p0.position;
        size_t segment_pos = pos;
        int endpoint_slot = 0;

        for (;;)
        {
            used[segment_pos] = true;
            const TraceSegment& segment = segments[segment_pos];
            const TracePoint& far_point = endpoint_slot == 0 ? segment.p1 : segment.p0;
            // Oriented so .p0 == current_point, .p1 == far_point in traversal order, keeping each
            // point's own face_id_a/face_id_b intact rather than collapsing to a single shared point
            trace.push_back(endpoint_slot == 0 ? segment : TraceSegment{segment.p1, segment.p0});

            if (far_point.position == position0)
            {
                break; // Loop closes back to trace[0] via this last segment
            }

            auto chosen = successor_position(segments, position_index, used, far_point, segment_pos);
            if (!chosen)
            {
                std::cout << std::format("ORDER no continuation at {}, trace of {} segments left open", far_point.position, trace.size()) << std::endl;
                break;
            }

            segment_pos = chosen->segment_pos;
            endpoint_slot = chosen->endpoint_slot;
        }

        traces.push_back(trace);
    }

    return traces;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::IntersectionSplitter<M>::PositionIndex quetzal::brep::IntersectionSplitter<M>::index_positions(const TraceSegments& segments)
{
    PositionIndex position_index;

    for (size_t pos = 0; pos < segments.size(); ++pos)
    {
        position_index[segments[pos].p0.position].push_back({pos, 0});
        position_index[segments[pos].p1.position].push_back({pos, 1});
    }

    return position_index;
}

//------------------------------------------------------------------------------
template<typename M>
std::optional<typename quetzal::brep::IntersectionSplitter<M>::SegmentEndpointRef> quetzal::brep::IntersectionSplitter<M>::successor_position(const TraceSegments& segments, const PositionIndex& position_index, const std::vector<bool>& used, const TracePoint& trace_point, const size_t segment_pos) const
{
    auto i = position_index.find(trace_point.position);
    if (i == position_index.end())
    {
        return std::nullopt;
    }

    std::vector<SegmentEndpointRef> matches;
    std::vector<SegmentEndpointRef> candidates; // Every end still available here, whatever its faces

    for (const auto& ref : i->second)
    {
        if (ref.segment_pos == segment_pos || used[ref.segment_pos])
        {
            continue;
        }

        candidates.push_back(ref);

        const TraceSegment& segment = segments[ref.segment_pos];
        const TracePoint& point = ref.endpoint_slot == 0 ? segment.p0 : segment.p1;

        // Leaving through an edge of one submesh moves to the adjacent face there while the face on
        // the other side carries over. Where the reduction has turned an edge meeting an edge into a
        // vertex meeting a vertex, both carry over. Either way at least one face id is shared
        if (point.face_id_a == trace_point.face_id_a || point.face_id_b == trace_point.face_id_b)
        {
            matches.push_back(ref);
        }
    }

    if (matches.size() == 1)
    {
        return matches.front();
    }

    // One end standing here is the continuation whether or not a face carries over. Where the position
    // is a vertex on both submeshes the trace moves to a new face on each side at once, and with
    // nothing else standing there the choice is given
    if (candidates.size() == 1)
    {
        return candidates.front();
    }

    if (matches.empty())
    {
        // Both faces change at once and there is more than one end to choose between, which is the one
        // case still without a rule, so nothing is chosen
        std::cout << std::format("ORDER no face carries over at {}, {} ends to choose between", trace_point.position, candidates.size()) << std::endl;
        return std::nullopt;
    }

    std::cout << std::format("ORDER {} candidates share a face at {}, {} ends there", matches.size(), trace_point.position, i->second.size()) << std::endl;
    std::cout << std::format("    ORDER arriving  seg -- (a {} h {}, b {} h {})", id_string(trace_point.face_id_a), id_string(trace_point.halfedge_id_a), id_string(trace_point.face_id_b), id_string(trace_point.halfedge_id_b)) << std::endl;

    for (const auto& ref : i->second)
    {
        const TraceSegment& segment = segments[ref.segment_pos];
        const TracePoint& point = ref.endpoint_slot == 0 ? segment.p0 : segment.p1;
        const TracePoint& far_point = ref.endpoint_slot == 0 ? segment.p1 : segment.p0;

        bool self = ref.segment_pos == segment_pos;
        bool match = std::ranges::any_of(matches, [&ref](const SegmentEndpointRef& m) { return m.segment_pos == ref.segment_pos && m.endpoint_slot == ref.endpoint_slot; });

        std::cout << std::format("    ORDER {:8} seg {} p{} (a {} h {}, b {} h {}) to {}{}", self ? "self" : (used[ref.segment_pos] ? "used" : (match ? "match" : "other")), ref.segment_pos, ref.endpoint_slot, id_string(point.face_id_a), id_string(point.halfedge_id_a), id_string(point.face_id_b), id_string(point.halfedge_id_b), far_point.position, self ? " <-- arriving" : "") << std::endl;
    }

    return matches.front();
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::connection_traces_type quetzal::brep::IntersectionSplitter<M>::instantiate_connection_traces(Traces& traces)
{
    connection_traces_type connections;
    for (size_t pos = 0; pos < traces.size(); ++pos)
    {
        connections.push_back(instantiate_trace(traces, pos));
    }

    return connections;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::connection_trace_type quetzal::brep::IntersectionSplitter<M>::instantiate_trace(Traces& traces, const size_t trace_pos)
{
    TraceSegments& segments = traces[trace_pos];
    assert(segments.size() >= 2);

    connection_trace_type connection_trace;
//dump_trace_comparison(segments, connection_trace); // TEMP diagnostic, segments as ordered, before any of them are instantiated

    const size_t n = segments.size();
    for (size_t i = 0; i < n; ++i)
    {
        // Each segment is self-consistent (both its own points share its own face_id_a/face_id_b, as
        // discovered), so both endpoints are always resolved from the SAME segment -- never mixed with
        // the neighboring segment's point at the same position, which may disagree on the exiting face
        const TracePoint& point0 = segments[i].p0;
        const TracePoint& point1 = segments[i].p1;

        id_type face_id_a = m_mesh.halfedge(point0.halfedge_id_a).face_id();
        id_type face_id_b = m_mesh.halfedge(point0.halfedge_id_b).face_id();

        vector_type trace_direction = normalize(cross(m_mesh.face(face_id_a).attributes().normal(), m_mesh.face(face_id_b).attributes().normal()));

        id_type halfedge_id_a_exterior = instantiate_segment_side(point0.halfedge_id_a, point1.halfedge_id_a, -trace_direction);
        id_type halfedge_id_b_exterior = instantiate_segment_side(point0.halfedge_id_b, point1.halfedge_id_b, trace_direction);

        id_type halfedge_id_a_interior = interior_partner(halfedge_id_a_exterior, -trace_direction);
        id_type halfedge_id_b_interior = interior_partner(halfedge_id_b_exterior, trace_direction);

        insert_trace_intersection(halfedge_id_a_exterior, halfedge_id_a_interior, halfedge_id_b_exterior, halfedge_id_b_interior);
        update_trace_points_connection(traces, halfedge_id_a_interior, halfedge_id_b_interior);
#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << std::format("instantiate_trace i {} of {} a_ext {} a_int {} b_ext {} b_int {}", i, n, id_string(halfedge_id_a_exterior), id_string(halfedge_id_a_interior), id_string(halfedge_id_b_exterior), id_string(halfedge_id_b_interior)) << std::endl;
#endif
        connection_trace.push_back({halfedge_id_a_exterior, halfedge_id_a_interior, halfedge_id_b_exterior, halfedge_id_b_interior});
    }

//dump_trace_comparison(segments, connection_trace); // TEMP diagnostic

    uncouple(connection_trace);
    return connection_trace;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::update_trace_points_connection(Traces& traces, const id_type halfedge_id_a_interior, const id_type halfedge_id_b_interior)
{
    for (bool side_a : {true, false})
    {
        // The interior halfedge and its next are the two halfedges of the interior loop standing at
        // the segment's two ends, whichever way round that loop runs on this side
        id_type halfedge_id_interior = side_a ? halfedge_id_a_interior : halfedge_id_b_interior;

        for (id_type halfedge_id : {halfedge_id_interior, m_mesh.halfedge(halfedge_id_interior).next_id()})
        {
            const point_type position = vertex_position(m_mesh.halfedge(halfedge_id));
            const id_type face_id = m_mesh.halfedge(halfedge_id).face_id();

            for (auto& trace : traces)
            {
                for (auto& segment : trace)
                {
                    SubmeshTracePointRef ref0 = {&segment.p0, side_a};
                    SubmeshTracePointRef ref1 = {&segment.p1, side_a};

                    if (m_mesh.halfedge(ref0.halfedge_id()).face_id() == m_mesh.halfedge(ref1.halfedge_id()).face_id())
                    {
                        continue; // Both ends on one loop, so the division left neither of them behind
                    }

                    for (const SubmeshTracePointRef& ref : {ref0, ref1})
                    {
                        if (ref.position() == position && m_mesh.halfedge(ref.halfedge_id()).face_id() != face_id)
                        {
                            ref.halfedge_id() = halfedge_id;
                        }
                    }
                }
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::instantiate_segment_side(id_type halfedge_id0, id_type halfedge_id1, const vector_type& exterior_direction)
{
    if (halfedge_id1 == m_mesh.halfedge(halfedge_id0).next_id()) // The segment is this edge
    {
        return exterior_halfedge(halfedge_id0, exterior_direction);
    }

    if (halfedge_id0 == m_mesh.halfedge(halfedge_id1).next_id()) // The segment is this edge, but in the opposite direction
    {
        return exterior_halfedge(halfedge_id1, exterior_direction);
    }

    // At this point, the connection segment needs to be created

    const id_type hole_id = m_mesh.halfedge(halfedge_id0).hole_id();
    if (hole_id != nullid && hole_id == m_mesh.halfedge(halfedge_id1).hole_id())
    {
        return close_enclosed_loop(hole_id, halfedge_id0, halfedge_id1, exterior_direction);
    }

    return exterior_halfedge(create_edge_connect(halfedge_id0, halfedge_id1), exterior_direction);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::close_enclosed_loop(const id_type hole_id, const id_type halfedge_id0, const id_type halfedge_id1, const vector_type& exterior_direction)
{
    const id_type face_id = m_mesh.halfedge(halfedge_id0).face_id();

    id_type halfedge_id = create_edge_connect(halfedge_id0, halfedge_id1);
    id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);
    id_type halfedge_id_interior = m_mesh.halfedge(halfedge_id_exterior).partner_id();

    m_mesh.hole(hole_id).set_halfedge_id(halfedge_id_exterior);

    // Create a face from the interior halfedges
    id_type face_id_interior = m_mesh.create_face(m_mesh.face(face_id).surface_id(), halfedge_id_interior, m_mesh.face(face_id).attributes());
    m_mesh.face(face_id_interior).assign_halfedges();

    // release_hole_halfedges, clear_hole_halfedges? ...
    for (auto& halfedge : m_mesh.face(face_id_interior).halfedges())
    {
        halfedge.set_hole_id(nullid);
    }

    m_mesh.face(face_id).invalidate();
    m_mesh.face(face_id_interior).invalidate();

#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << std::format("close_enclosed_loop f {} hole {} h0 {} h1 {} -> hole loop {}, face {} loop {}", id_string(face_id), id_string(hole_id), id_string(halfedge_id0), id_string(halfedge_id1), id_string(halfedge_id_exterior), id_string(face_id_interior), id_string(halfedge_id_interior)) << std::endl;
#endif
    return halfedge_id_exterior;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::uncouple(connection_trace_type& connection)
{
    for (const auto& [halfedge_id_a_exterior, _a, halfedge_id_b_exterior, _b] : connection) // just _'s ...
    {
        m_mesh.halfedge(halfedge_id_a_exterior).uncouple();
        m_mesh.halfedge(halfedge_id_b_exterior).uncouple();
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
const quetzal::geometry::PolygonMapping<typename M::vertex_attributes_type>& quetzal::brep::IntersectionSplitter<M>::face_mapping(const id_type face_id)
{
    auto i = m_face_mappings.find(face_id);
    if (i == m_face_mappings.end())
    {
        i = m_face_mappings.emplace(face_id, position_texcoord_mapping(m_mesh.face(face_id))).first;
    }

    return i->second;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::create_edge_internal(const id_type face_id, const segment_type& segment)
{
    auto [position0, position1] = segment.endpoints();

    auto normal = m_mesh.face(face_id).attributes().normal();

    const auto& mapping = face_mapping(face_id);
    auto texcoord0 = denoise01(mapping(position0));
    auto texcoord1 = denoise01(mapping(position1));

    typename M::vertex_attributes_type av0{position0, normal, texcoord0};
    typename M::vertex_attributes_type av1{position1, normal, texcoord1};

    size_t nh = m_mesh.halfedge_store_count();
    m_mesh.create_halfedge_vertex(nh + 1, nh + 1, nh + 1, face_id, av0);
    m_mesh.create_halfedge_vertex(nh, nh, nh, face_id, av1);

#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << "    " << std::format("create_edge_internal f {} segment {} created {} {}", id_string(face_id), segment, nh, nh + 1) << std::endl;
#endif
    return nh;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::create_edge_partial(const id_type halfedge_id0, const point_type& position1)
{
    id_type face_id = m_mesh.halfedge(halfedge_id0).face_id();
    auto normal = m_mesh.face(face_id).attributes().normal();

    // The spur goes in ahead of its anchor halfedge, so where an earlier spur has already left the loop
    // reaching this position more than once, the one it goes ahead of decides which corner of the face
    // it lands in. It belongs in the corner holding the direction it runs, a corner at an occurrence
    // running counterclockwise from the halfedge leaving it to the reverse of the one arriving
    id_type halfedge_id_anchor = halfedge_id0;
    auto position0 = vertex_position(m_mesh.halfedge(halfedge_id0));
    auto direction_spur = normalize(position1 - position0);

    for (const auto& halfedge : m_mesh.face(face_id).halfedges())
    {
        if (vertex_position(halfedge) == position0 && geometry::ccw_order(direction(halfedge), direction_spur, -direction(halfedge.prev()), normal))
        {
            halfedge_id_anchor = halfedge.id();
            break;
        }
    }

    const auto& mapping = face_mapping(face_id);
    auto texcoord1 = denoise01(mapping(position1));

    typename M::vertex_attributes_type av0 = m_mesh.halfedge(halfedge_id_anchor).vertex_attributes();
    typename M::vertex_attributes_type av1{position1, normal, texcoord1};

    size_t nh = m_mesh.halfedge_store_count();
    m_mesh.create_halfedge_vertex(nh + 1, nh + 1, m_mesh.halfedge(halfedge_id_anchor).prev_id(), face_id, av0);
    m_mesh.create_halfedge_vertex(nh, halfedge_id_anchor, nh, face_id, av1);

    m_mesh.halfedge(halfedge_id_anchor).prev().set_next_id(nh);
    m_mesh.halfedge(halfedge_id_anchor).set_prev_id(nh + 1);

    // Spliced into whichever loop the anchor is on, so the pair takes its hole id: nullid on a
    // perimeter, the hole's id on a hole, without needing to know which
    id_type hole_id = m_mesh.halfedge(halfedge_id_anchor).hole_id();
    m_mesh.halfedge(nh).set_hole_id(hole_id);
    m_mesh.halfedge(nh + 1).set_hole_id(hole_id);

    m_mesh.face(face_id).invalidate(); // The spur is now part of the loop halfedge_id0 is on

#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << std::format("create_edge_partial h {} (face {}) p {} created {} {} (both on face {})", id_string(halfedge_id0), id_string(face_id), position1, nh, nh + 1, id_string(face_id)) << std::endl;
#endif
    return nh;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::create_edge_connect(const id_type halfedge_id0, const id_type halfedge_id1, bool coupled)
{
#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << std::format("create_edge_connect {} {} {} {}", id_string(halfedge_id0), id_string(halfedge_id1), vertex_position(m_mesh.halfedge(halfedge_id0)), vertex_position(m_mesh.halfedge(halfedge_id1))) << std::endl;
#endif
    id_type face_id = m_mesh.halfedge(halfedge_id0).face_id();
    assert(m_mesh.halfedge(halfedge_id1).face_id() == face_id);

    // Which loop each end is on, established before the connection changes them
    id_type hole_halfedge_id0 = m_mesh.halfedge(halfedge_id0).hole_id();
    id_type hole_halfedge_id1 = m_mesh.halfedge(halfedge_id1).hole_id();

    // connect splices the new pair in as the predecessor of each end, so prev(halfedge_id1) is the one
    // running from the halfedge_id0 vertex to the halfedge_id1 vertex and prev(halfedge_id0) is its partner

    // Both ends on the perimeter divides the face, which split_face does whole: the connecting pair,
    // a face for the side halfedge_id0 does not keep, the holes moved across, and both invalidated
    if (hole_halfedge_id0 == nullid && hole_halfedge_id1 == nullid)
    {
        split_face(m_mesh, halfedge_id0, halfedge_id1, coupled);
#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << "    " << std::format("create_edge_connect h0 {} h1 {} split face {} created {} {}, new face {}", id_string(halfedge_id0), id_string(halfedge_id1), id_string(face_id), id_string(m_mesh.halfedge(halfedge_id0).prev_id()), id_string(m_mesh.halfedge(halfedge_id1).prev_id()), id_string(m_mesh.halfedge(halfedge_id1).face_id())) << std::endl;
#endif
        return m_mesh.halfedge(halfedge_id1).prev_id();
    }

    // A hole is involved, so the loops are relinked without any face changing hands and only the hole
    // records need bringing up to date

    // Ends on different loops makes them one, so one record ceases to describe anything. A hole joined
    // to the perimeter is absorbed by it, a hole joined to another hole becomes part of that one. The
    // record goes now, while its loop is still intact, since removing it walks the loop to clear the
    // hole id it set
    id_type hole_id_merged = hole_halfedge_id0;
    if (hole_halfedge_id0 != hole_halfedge_id1)
    {
        hole_id_merged = hole_halfedge_id0 == nullid || hole_halfedge_id1 == nullid ? nullid : hole_halfedge_id0;
        m_mesh.face(face_id).remove_hole(hole_halfedge_id0 == hole_id_merged ? hole_halfedge_id1 : hole_halfedge_id0);
    }

    connect(m_mesh, halfedge_id0, halfedge_id1, coupled);

    id_type halfedge_id_created0 = m_mesh.halfedge(halfedge_id0).prev_id();
    id_type halfedge_id_created1 = m_mesh.halfedge(halfedge_id1).prev_id();

    if (hole_halfedge_id0 != hole_halfedge_id1)
    {
        // One loop now, so the surviving hole id applies the whole way round, setting it on the side
        // whose hole was just removed along with the pair just created
        for (id_type i = halfedge_id_created0; ; )
        {
            m_mesh.halfedge(i).set_hole_id(hole_id_merged);
            i = m_mesh.halfedge(i).next_id();
            if (i == halfedge_id_created0)
            {
                break;
            }
        }
    }
    else
    {
        // Ends on one hole loop divides it rather than joining, leaving the pair on either side of the
        // division with the hole id that loop already had. Which side stays the hole and which
        // becomes a face depends on which way the trace runs, so close_enclosed_loop finishes it
        m_mesh.halfedge(halfedge_id_created0).set_hole_id(hole_halfedge_id0);
        m_mesh.halfedge(halfedge_id_created1).set_hole_id(hole_halfedge_id0);
    }

    m_mesh.face(face_id).invalidate(); // Either loop the connection touched may be the perimeter

#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << "    " << std::format("create_edge_connect h0 {} h1 {} created {} {} (both on face {})", id_string(halfedge_id0), id_string(halfedge_id1), id_string(halfedge_id_created0), id_string(halfedge_id_created1), id_string(face_id)) << std::endl;
#endif
    return halfedge_id_created1;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::insert_trace_intersection(const id_type halfedge_id_a_exterior, const id_type halfedge_id_a_interior, const id_type halfedge_id_b_exterior, const id_type halfedge_id_b_interior)
{
    m_connection_halfedge_ids.insert(halfedge_id_a_exterior);
    m_connection_halfedge_ids.insert(halfedge_id_a_interior);
    m_connection_halfedge_ids.insert(halfedge_id_b_exterior);
    m_connection_halfedge_ids.insert(halfedge_id_b_interior);

    m_connection_positions.insert(vertex_position(m_mesh.halfedge(halfedge_id_a_exterior)));
    m_connection_positions.insert(vertex_position(m_mesh.halfedge(halfedge_id_a_interior)));
    m_connection_positions.insert(vertex_position(m_mesh.halfedge(halfedge_id_b_exterior)));
    m_connection_positions.insert(vertex_position(m_mesh.halfedge(halfedge_id_b_interior)));
    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::exterior_halfedge(const id_type halfedge_id, const vector_type& exterior_direction) const
{
    return direction_compare(direction(m_mesh.halfedge(halfedge_id)), exterior_direction) > 0 ? halfedge_id : m_mesh.halfedge(halfedge_id).partner_id();
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::interior_partner(const id_type halfedge_id_exterior, const vector_type& exterior_direction) const
{
    id_type halfedge_id_interior = m_mesh.halfedge(halfedge_id_exterior).partner_id();
auto ve = vertex_position(m_mesh.halfedge(halfedge_id_exterior));
auto vi = vertex_position(m_mesh.halfedge(halfedge_id_interior));
auto ven = vertex_position(m_mesh.halfedge(halfedge_id_exterior).next());
auto vin = vertex_position(m_mesh.halfedge(halfedge_id_interior).next());
auto zzz = direction(m_mesh.halfedge(halfedge_id_exterior));
    assert(halfedge_id_exterior == m_mesh.halfedge(halfedge_id_interior).partner_id());
    assert(vertex_position(m_mesh.halfedge(halfedge_id_exterior).next()) == vertex_position(m_mesh.halfedge(halfedge_id_interior)));
    assert(vertex_position(m_mesh.halfedge(halfedge_id_interior).next()) == vertex_position(m_mesh.halfedge(halfedge_id_exterior)));
    assert(direction_compare(direction(m_mesh.halfedge(halfedge_id_exterior)), exterior_direction) > 0);
    assert(float_eq(direction(m_mesh.halfedge(halfedge_id_exterior)), exterior_direction));
    return halfedge_id_interior;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::find_halfedge(id_type face_id, const point_type& position)
{
    for (const auto& halfedge : m_mesh.face(face_id).halfedges_ext())
    {
        if (halfedge_contains(halfedge, position))
        {
            return halfedge.id();
        }
    }

    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::set_trace_points_halfedge(TraceSegments& segments, const bool side_a, const point_type& position, const id_type halfedge_id)
{
    id_type face_id = m_mesh.halfedge(halfedge_id).face_id();

    for (auto& segment : segments)
    {
        for (TracePoint* ptrace_point : {&segment.p0, &segment.p1})
        {
            SubmeshTracePointRef ref = {ptrace_point, side_a};
            if (ref.halfedge_id() == nullid && ref.face_id() == face_id && ref.position() == position)
            {
                ref.halfedge_id() = halfedge_id;
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
bool quetzal::brep::IntersectionSplitter<M>::farther(const point_type& position_a, const point_type& position_b, const vector_type& direction)
{
    return (direction_compare(position_a - position_b, direction) > 0);
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::align_segment_positions(TraceSegments& segments)
{
    auto point_less = [](const point_type& a, const point_type& b) -> bool
    {
        return float_priority_less(a, b);
    };

    struct PositionGroup
    {
        point_type m_position; // canonical value that every member of this group is set to
        size_t m_count = 0;
    };

    // Endpoint positions grouped by the tolerant comparator, so that separately computed values for the
    // same point land in one group and can be given a single canonical value. Downstream comparisons are
    // exact and depend on that
    std::map<point_type, PositionGroup, decltype(point_less)> position_groups(point_less);

    for (auto& segment : segments)
    {
        assert(segment.p0.face_id_a == segment.p1.face_id_a);
        assert(segment.p0.face_id_b == segment.p1.face_id_b);

        for (TracePoint* ptrace_point : {&segment.p0, &segment.p1})
        {
            auto i = position_groups.find(ptrace_point->position);
            if (i == position_groups.end())
            {
                i = position_groups.emplace(ptrace_point->position, PositionGroup{ptrace_point->position}).first;
            }

            ++i->second.m_count;

            for (id_type halfedge_id : {ptrace_point->halfedge_id_a, ptrace_point->halfedge_id_b})
            {
                // An existing vertex position takes precedence as the canonical value, aligning the trace to
                // the mesh rather than moving the mesh to the trace. Only an intersection coincident with the
                // halfedge's own vertex has one; one that lies within the halfedge has no vertex yet, and
                // gets one at the exact position when the edge is split during instantiation. Checking the
                // starting vertex is sufficient since halfedge_plane_intersection excludes the ending position
                if (halfedge_id != nullid && float_eq(vertex_position(m_mesh.halfedge(halfedge_id)), ptrace_point->position))
                {
                    i->second.m_position = vertex_position(m_mesh.halfedge(halfedge_id));
                }
            }
        }
    }

#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
std::cout << std::format("align_segment_positions: {} segments, {} distinct positions", segments.size(), position_groups.size()) << std::endl;
#endif

    for (const auto& position_group : position_groups)
    {
        if (!math::even(position_group.second.m_count))
        {
std::cout << std::format("align_segment_positions: position {:[, }: odd position count {}", position_group.second.m_position, position_group.second.m_count) << std::endl;
        }
        assert(math::even(position_group.second.m_count));
    }

    // Nothing is modified until the grouping has been validated
    for (auto& segment : segments)
    {
        for (TracePoint* ptrace_point : {&segment.p0, &segment.p1})
        {
            const point_type position = position_groups.find(ptrace_point->position)->second.m_position;

            for (id_type halfedge_id : {ptrace_point->halfedge_id_a, ptrace_point->halfedge_id_b})
            {
                if (halfedge_id != nullid && float_eq(vertex_position(m_mesh.halfedge(halfedge_id)), position))
                {
                    set_vertex_position(m_mesh.halfedge(halfedge_id).vertex(), position);
                }
            }

            ptrace_point->position = position;
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::validate_segments(const TraceSegments& segments)
{
    std::map<point_type, size_t> position_counts;

    for (const auto& segment : segments)
    {
        assert(segment.p0.face_id_a == segment.p1.face_id_a);
        assert(segment.p0.face_id_b == segment.p1.face_id_b);

        assert(segment.p0.halfedge_id_a != nullid);
        assert(segment.p0.halfedge_id_b != nullid);
        assert(segment.p1.halfedge_id_a != nullid);
        assert(segment.p1.halfedge_id_b != nullid);

        // A segment spans two distinct positions, so its ends stand on distinct halfedges on each side
        assert(segment.p0.position != segment.p1.position);
        assert(segment.p0.halfedge_id_a != segment.p1.halfedge_id_a);
        assert(segment.p0.halfedge_id_b != segment.p1.halfedge_id_b);

        assert(segment.p0.position == vertex_position(m_mesh.halfedge(segment.p0.halfedge_id_a)));
        assert(segment.p0.position == vertex_position(m_mesh.halfedge(segment.p0.halfedge_id_b)));
        assert(segment.p1.position == vertex_position(m_mesh.halfedge(segment.p1.halfedge_id_a)));
        assert(segment.p1.position == vertex_position(m_mesh.halfedge(segment.p1.halfedge_id_b)));
/*
        if (m_mesh.halfedge(segment.p0.halfedge_id_a).next_id() == segment.p1.halfedge_id_a)
        {
            assert(m_mesh.halfedge(segment.p1.halfedge_id_a).next_id() == segment.p0.halfedge_id_a);
            assert(segment.p1.position == vertex_position(m_mesh.halfedge(segment.p0.halfedge_id_a).next()));
            assert(segment.p1.position == vertex_position(m_mesh.halfedge(segment.p0.halfedge_id_b).next()));
        }

        if (m_mesh.halfedge(segment.p0.halfedge_id_b).next_id() == segment.p1.halfedge_id_b)
        {
            assert(m_mesh.halfedge(segment.p1.halfedge_id_b).next_id() == segment.p0.halfedge_id_b);
            assert(segment.p0.position == vertex_position(m_mesh.halfedge(segment.p1.halfedge_id_a).next()));
            assert(segment.p0.position == vertex_position(m_mesh.halfedge(segment.p1.halfedge_id_b).next()));
        }
*/
        ++position_counts[segment.p0.position];
        ++position_counts[segment.p1.position];
    }

    for (const auto& [position, count] : position_counts)
    {
        if (!math::even(count))
        {
std::cout << std::format("validate_segments: position {:[, }: odd position count {}", position, count) << std::endl;
        }
        assert(math::even(count));
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::validate_intersection_traces(const Traces& traces)
{
    size_t i = 0;
    for (const auto& trace : traces)
    {
        size_t j_prev = trace.size() - 1;;
        auto segment_prev = trace.back();
        size_t j = 0;
        for (const auto& segment : trace)
        {
            assert(segment.p0.face_id_a == segment.p1.face_id_a);
            assert(segment.p0.face_id_b == segment.p1.face_id_b);

            vector_type trace_direction = normalize(cross(m_mesh.face(segment.p0.face_id_a).attributes().normal(), m_mesh.face(segment.p0.face_id_b).attributes().normal()));
            vector_type segment_direction = normalize(segment.p1.position - segment.p0.position);
            value_type d = dot(segment_direction, trace_direction);
            if (!math::float_gt(d, value_type{0}))
            {
std::cout << std::format("validate_intersection_traces, direction: segment p0 {} -> p1 {} (face_id_a {}, face_id_b {}) has direction {} vs trace_direction {}, dot {}", segment.p0.position, segment.p1.position, id_string(segment.p0.face_id_a), id_string(segment.p0.face_id_b), segment_direction, trace_direction, d) << std::endl;
            }
            assert(math::float_gt(d, value_type{0}));

            const auto& p1_prev = segment_prev.p1.position;
            const auto& p0 = segment.p0.position;
            if (p1_prev != p0)
            {
std::cout << std::format("validate_intersection_traces, order: trace {} segment {} p1 {} != segment {} p0 {}", i, j_prev, p1_prev, j, p0) << std::endl;
            }
            assert(p1_prev == p0);

            segment_prev = segment;
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::validate_connections(const connection_traces_type& connections)
{
    for (const auto& connection : m_connections)
    {
        for (const auto& halfedge_ids : connection)
        {
            for (id_type halfedge_id : halfedge_ids)
            {
                assert(m_connection_halfedge_ids.contains(halfedge_id));
                assert(m_connection_positions.contains(vertex_position(m_mesh.halfedge(halfedge_id))));
            }
        }
    }

#if defined(QUETZAL_INTERSECTION_SPLITTER_TRACE_VERBOSE)
    dump_multitrace_vertices(m_mesh, connections); // TEMP diagnostic
#endif
assert(check_faces_halfedges_ext(m_mesh).ok()); // TEMP probe

    // Validate each border path through each of the connection traces
    validate_connection_traces(m_mesh, connections);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::dump_trace_comparison(const TraceSegments& segments, const connection_trace_type& connection_trace) const
{
    auto halfedge_string = [this](id_type halfedge_id) -> std::string
    {
        return std::format("h {} f {} {}", id_string(halfedge_id), id_string(m_mesh.halfedge(halfedge_id).face_id()), vertex_position(m_mesh.halfedge(halfedge_id)));
    };

    // How the two ends of one side of a segment stand relative to each other: an edge of the loop
    // already, the two halfedges of a spur, or two vertices with nothing between them
    auto side_string = [this](id_type halfedge_id0, id_type halfedge_id1) -> std::string
    {
        const auto& halfedge0 = m_mesh.halfedge(halfedge_id0);
        const auto& halfedge1 = m_mesh.halfedge(halfedge_id1);

        std::string relation = "vv";
        if (halfedge_id1 == halfedge0.next_id()) { relation = halfedge0.partner_id() == halfedge_id1 ? "SPUR" : "EDGE next"; }
        else if (halfedge_id1 == halfedge0.prev_id()) { relation = halfedge0.partner_id() == halfedge_id1 ? "SPUR" : "EDGE prev"; }

        return std::format("{:9} h0 {} f {} hole {} partner {} next {} prev {} | h1 {} f {} hole {} partner {} next {} prev {}", relation,
            id_string(halfedge_id0), id_string(halfedge0.face_id()), id_string(halfedge0.hole_id()), id_string(halfedge0.partner_id()), id_string(halfedge0.next_id()), id_string(halfedge0.prev_id()),
            id_string(halfedge_id1), id_string(halfedge1.face_id()), id_string(halfedge1.hole_id()), id_string(halfedge1.partner_id()), id_string(halfedge1.next_id()), id_string(halfedge1.prev_id()));
    };

    std::set<id_type> faces_dumped;
    std::cout << std::format("COMPARE trace of {} segments", segments.size()) << std::endl;

    for (size_t i = 0; i < segments.size(); ++i)
    {
        const TracePoint& point0 = segments[i].p0;
        const TracePoint& point1 = segments[i].p1;

        std::cout << std::format("  {:3} seg p0 {} (a f {} h {}, b f {} h {})", i, point0.position, id_string(point0.face_id_a), id_string(point0.halfedge_id_a), id_string(point0.face_id_b), id_string(point0.halfedge_id_b)) << std::endl;
        std::cout << std::format("      seg p1 {} (a f {} h {}, b f {} h {})", point1.position, id_string(point1.face_id_a), id_string(point1.halfedge_id_a), id_string(point1.face_id_b), id_string(point1.halfedge_id_b)) << std::endl;
        std::cout << std::format("      a  {}", side_string(point0.halfedge_id_a, point1.halfedge_id_a)) << std::endl;
        std::cout << std::format("      b  {}", side_string(point0.halfedge_id_b, point1.halfedge_id_b)) << std::endl;

        if (connection_trace.empty()) // TEMP diagnostic, the loop each side stands on, before any of it is instantiated
        {
            for (id_type halfedge_id : {point0.halfedge_id_a, point0.halfedge_id_b})
            {
                id_type face_id = m_mesh.halfedge(halfedge_id).face_id();
                if (!faces_dumped.insert(face_id).second)
                {
                    continue;
                }

                std::cout << std::format("      FACE {} loop", id_string(face_id)) << std::endl;
                for (const auto& halfedge : m_mesh.face(face_id).halfedges())
                {
                    std::cout << std::format("          h {:3} {}", halfedge.id(), vertex_position(halfedge)) << std::endl;
                }
            }
        }

        if (i < connection_trace.size())
        {
            const auto& [halfedge_id_a_exterior, halfedge_id_a_interior, halfedge_id_b_exterior, halfedge_id_b_interior] = connection_trace[i];
            std::cout << std::format("      con a ext {} | a int {}", halfedge_string(halfedge_id_a_exterior), halfedge_string(halfedge_id_a_interior)) << std::endl;
            std::cout << std::format("      con b ext {} | b int {}", halfedge_string(halfedge_id_b_exterior), halfedge_string(halfedge_id_b_interior)) << std::endl;
        }
    }

    return;
}

#endif // QUETZAL_BREP_INTERSECTION_SPLITTER_HPP
