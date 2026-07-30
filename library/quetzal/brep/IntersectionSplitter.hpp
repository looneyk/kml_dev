#if !defined(QUETZAL_BREP_INTERSECTION_SPLITTER_HPP)
#define QUETZAL_BREP_INTERSECTION_SPLITTER_HPP
//------------------------------------------------------------------------------
// brep
// IntersectionSplitter.hpp
//------------------------------------------------------------------------------

#include "Mesh.hpp"
#include "Submesh.hpp"
#include "Halfedge.hpp"
#include "id.hpp"
#include "AnnotatedIntersection.hpp"
#include "border.hpp"
#include "geometry.hpp"
#include "mesh_util.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
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

        // find_face_intersections_linear returns 0..2 of these
        // These are used to construct the actual connection traces

        //----------------------------------------------------------------------
        struct HalfedgeIntersection
        {
            id_type m_halfedge_id_a = nullid;
            id_type m_halfedge_id_b = nullid;
            point_type m_position;
        };

        //----------------------------------------------------------------------
        struct Floating
        {
            // id0, id1; continuous partner chains, each is the other's end, id0 and id1 will either both be valid ids or both nullid
            id_type id0 = nullid;
            id_type id1 = nullid;
            void clear() { id0 = nullid; id1 = nullid; }
            bool valid() const { return  id0 == nullid && id1 == nullid || id0 != nullid && id1 != nullid; }
            operator bool() const { assert(valid()); return id0 != nullid; }
        };

        // An intersection point along a trace between face_a and face_b.
        // The initial set references the id's of the original read-only meshes.
        // As phase two goes on, these id's will be updated to reflect changes in the meshes due to instantiation of missing trace elements.
        // Halfedge id's may be nullid here when the intersection occurs in the interior as opposed to the boundary of the face (floating).

        //----------------------------------------------------------------------
        struct TracePoint
        {
            point_type position;
            id_type face_id_a = nullid;
            id_type halfedge_id_a = nullid;
            id_type face_id_b = nullid;
            id_type halfedge_id_b = nullid;
        };

        // The segment of intersection between two faces, before connecting them into connection traces.
        //----------------------------------------------------------------------
        struct TraceSegment
        {
            TracePoint p0;
            TracePoint p1;
        };

        // trace[i].p1 and trace[i + 1 mod size].p0 share a position but may legitimately differ in
        // face_id_a/face_id_b on whichever side transitions at that joint -- each segment keeps its
        // own self-consistent p0/p1 pair (as discovered) rather than collapsing shared joints into a
        // single point, since the "arriving" and "departing" segments can disagree on the exiting face
        using TraceSegments = std::vector<TraceSegment>;
        using Traces = std::vector<TraceSegments>;

        // Reference to one side of one TracePoint, used to bring the trace up to date as instantiation
        // modifies the mesh
        //----------------------------------------------------------------------
        struct TracePointRef
        {
            TracePoint* m_ptrace_point = nullptr;
            bool m_side_a = false;

            bool operator==(const TracePointRef&) const = default;

            const point_type& position() const { return m_ptrace_point->position; }
            id_type& face_id() const { return m_side_a ? m_ptrace_point->face_id_a : m_ptrace_point->face_id_b; }
            id_type& halfedge_id() const { return m_side_a ? m_ptrace_point->halfedge_id_a : m_ptrace_point->halfedge_id_b; }
        };

        // Note: Trace... types above are short for/represent intersection traces ...
        // connection_ types are short for/represent connection traces ...

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
        // Returns nullopt or an AnnotatedSegment where the latter can be:
        //   a virtual segment, two points on separate halfedges (id != nullid) or floating (id == nullid)
        //   a an existing halfedge (id1 == nullid, id0 next vertex position is implied)
        // Endpoints are ordered by trace_direction
        // Does not support non-convex faces ...
        std::optional<AnnotatedSegment> face_plane_intersection(const face_type& face, const plane_type& plane, const vector_type& trace_direction);

        // Find the face intersection with the plane of the opposing face
        // Supports non-convex faces
        // Returns both segment and isolated halfedge vertex intersections
        // Returns
        //   nullopt: the face is in the plane (entire face intersection, no individual intersections)
        //   empty vector: no intersection
        //   otherwise: a sequence of annotated intersections (point/segment, halfedge_id) in trace_direction order
        std::optional<annotated_intersections_type> face_plane_intersection2(const face_type& face, const plane_type& plane, const vector_type& trace_direction);

        // temp translation layer ...
        std::optional<AnnotatedSegment> face_plane_intersection3(const face_type& face, const plane_type& plane, const vector_type& trace_direction);

        intersection_type halfedge_plane_intersection(const halfedge_type& halfedge, const plane_type& plane);

        // Connects a set of face/face intersection segments into closed-loop intersection traces
        // Matches shared endpoint positions. Primary rule: two segment-ends at
        // the same position are consecutive if they share face_id_a or face_id_b (pure id/position
        // matching). Fallback: a genuine multi-way tie (3+ independent curves converging on one point) is
        // disambiguated geometrically, once, statically, against the pristine vertex fan
        Traces order_intersections(const TraceSegments& segments);

        // Instantiates all intersection traces, producing connection traces
        connection_traces_type instantiate_connections(Traces& traces);

        // Instantiates an intersection trace producing a single connection trace
        // Connects the ordered trace segments, adding and splitting faces and halfedges as required
        connection_trace_type instantiate_trace(TraceSegments& trace);

        void uncouple(connection_trace_type& connection);

        // Reference to one endpoint of one segment, used internally by order_intersections
        struct SegmentEndpointRef
        {
            size_t segment_index;
            int endpoint_index; // 0 = p0, 1 = p1
        };

        // Disambiguates a genuine tie among candidates that all share a face id with trace_point at
        // the same position -- only reached when 3+ independent curves converge on one point
        SegmentEndpointRef disambiguate_connect_candidates(const TracePoint& trace_point, const TracePoint& trace_point_prev, const TraceSegments& segments, const std::vector<SegmentEndpointRef>& candidates);

        // Returns the first intersection of two non-convex polygons
        // The intersection starts at the common vertex at position and extends through the polygon interior to the closest intersection
        intersection_type polygon_intersection(const polygon_type& polygon_a, const polygon_type& polygon_b, const point_type& position);

        // Returns exterior, impending border halfedge id, tracer halfedge id
        // tracer is nullid unless the edge endpoint is floating or there has been a split and we know that we can continue with the partner face (probably not the or...)
        // Segment endpoints should already be denoised
        // instantiated_halfedge_this_side is this side's slot of m_instantiated_halfedge_a/b -- every
        // split_edge done here also splits the ADJACENT face's edge for free (its partner), which is a
        // perfectly valid, permanent anchor at this exact position on that neighboring face; recording
        // it immediately means a later point that resolves onto that neighboring face (e.g. the trace's
        // own closing point, arriving via whatever face happens to border this one) finds it directly
        std::array<id_type, 2> create_intersection_edge(id_type face_id, id_type halfedge_id0, id_type halfedge_id1, const segment_type& segment, const vector_type& exterior_direction, Floating& floating, std::vector<std::pair<point_type, id_type>>& instantiated_halfedge_this_side);

        // Returns exterior, impending border halfedge id, tracer halfedge id
        // tracer is nullid unless the edge endpoint is floating or there has been a split and we know that we can continue with the partner face (probably not the or...)
        // Segment endpoints should already be denoised
        // connection_halfedge_id0 is this side's halfedge at the trace's own start, used to close the trace back onto it by identity
        // is_last_segment tells us this call is instantiating the trace's own closing segment (i == trace.size() - 1), known structurally
        // by the caller from the already-stitched, fixed-length trace -- not something to be inferred from a position comparison here
        // instantiated_halfedge_this_side: see the other overload's comment -- same reconciliation applies here
        std::array<id_type, 2> create_intersection_edge(id_type halfedge_id0, id_type halfedge_id1, const segment_type& segment, const vector_type& exterior_direction, Floating& floating, id_type connection_halfedge_id0, bool is_last_segment, std::vector<std::pair<point_type, id_type>>& instantiated_halfedge_this_side);

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

        // Halfedges are half open intervals: a halfedge covers its starting position, but not its ending
        // position, which belongs to the next halfedge
        bool covers(id_type halfedge_id, const point_type& position) const;

        // Populates m_trace_point_refs for every trace, once, before any of them are instantiated. Splitting
        // an edge while instantiating one trace can invalidate a point belonging to any of the others, so
        // they all have to be indexed for the whole of instantiation, not one at a time
        void index_trace_points(Traces& traces);

        // Splits an edge, keeping the trace current: a trace point referencing either halfedge of the edge
        // stays with it while it still covers the point, and otherwise moves to that halfedge's new successor
        void split_trace_edge(id_type halfedge_id, const point_type& position);

        // Repoints one side of one trace point at another halfedge, keeping m_trace_point_refs in step
        void set_trace_point_halfedge(const TracePointRef& ref, id_type halfedge_id);

        // Both endpoints of a segment have to stand on the same current face for its edge to be created.
        // Where an earlier segment's edge has divided the face, the positions at which it met the boundary
        // belong to both fragments, so one endpoint can be left on the wrong one. Only the endpoint that the
        // other fragment can also account for is the one out of place, which makes the choice unambiguous
        void align_segment_faces(TracePoint& trace_point0, TracePoint& trace_point1, bool side_a, id_type& face_id0, id_type& halfedge_id0, id_type& face_id1, id_type& halfedge_id1);

        // position_a is farther along than position_b in direction
        // position_a and position_b should both fall on a line with the given direction or else the result will be ambiguous
        bool farther(const point_type& position_a, const point_type& position_b, const vector_type& direction);

        // Records that face_id_new was spun off from face_id_kept (or from face_id_kept's own
        // origin, if face_id_kept was itself already a descendant of an earlier split) during instantiation
        void register_face_split(id_type face_id_kept, id_type face_id_new);

        // Given an original (Phase 1 discovery time) face id and a position on it, returns the
        // current (possibly already split by an earlier trace's instantiation) face id that now
        // contains that position
        id_type resolve_face(id_type face_id_original, const point_type& position);

        // Full resolution of one side of one TracePoint to its current face id and halfedge id (or
        // nullid for floating), during instantiation. If face_id_hint is given (the current face the
        // OTHER endpoint of this same segment just resolved to), checks m_instantiated_halfedge_a/b for
        // an entry already on that exact face first -- create_intersection_edge needs both endpoints on
        // the same current face, so a hint match is preferred over resolve_face's independent guess,
        // which can only pick some current descendant of the point's ORIGINAL face and has no way to
        // know it needs to agree with where the other endpoint stands. Falls back to a hint-less cache
        // check (position may already have been touched by an earlier trace, or an earlier point of
        // this same trace, on whatever face resolve_face guesses), then resolve_face + find_halfedge
        std::array<id_type, 2> resolve_point(const TracePoint& trace_point, bool side_a);

        // For every vertex position in intersection trace segments, for each vertex at that position,
        // set the position to the same "canonical" position (the value of the first position encountered there)
        void align_segment_positions(TraceSegments& segments);

        // Validates that each intersection trace segment is oriented in trace_direction and that consecutive segment endpoints match
        // replaces the two above ...
        void validate_intersection_traces(const Traces& traces);

        void validate_connections(const connection_traces_type& connections);

        M& m_mesh;
        // Only one of these will ever be valid at a given time
        Floating m_floating_a;
        Floating m_floating_b;

        std::set<id_type> m_connection_halfedge_ids; // All of the halfedge ids that are part of any continuous connection (not just the start points) so that they are not mistaken for new intersections (for functionality)
        // Subset of m_connection_halfedge_ids belonging to the intersection trace currently being instantiated.
        // Currently write-only: populated by insert_trace_intersection but its only reader, claimed_by_earlier_connection,
        // belongs to the retired live-search algorithm (commented out below) and is never called from the active pipeline
        std::set<point_type> m_connection_positions; // All of the vertex positons that are part of any continuous connection (not just the start points) so that they are not mistaken for new intersections (for functionality)
        connection_traces_type m_connections;

        // halfedge id -> the trace point sides referencing it, for the trace currently being instantiated.
        // Traces can run to many thousands of segments, so the points affected by a split are found by
        // lookup rather than by scanning the whole trace
        std::unordered_map<id_type, std::vector<TracePointRef>> m_trace_point_refs;

        // Cleared once per connections() call; populated during instantiation (Phase 2)
        std::unordered_map<id_type, id_type> m_face_origin; // current face id -> its original (Phase 1) ancestor id, for any face already split during instantiation
        std::unordered_map<id_type, std::vector<id_type>> m_face_descendants; // original face id -> every id spun off from it so far during instantiation
        // position -> a live current halfedge there, already instantiated by an earlier trace; a plain
        // vector scanned with float_eq rather than a std::map keyed on exact position, since the same
        // point computed via two different face pairs can differ in the last bit or two
        std::vector<std::pair<point_type, id_type>> m_instantiated_halfedge_a; // submesh A
        std::vector<std::pair<point_type, id_type>> m_instantiated_halfedge_b; // submesh B

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
    m_floating_a{},
    m_floating_b{},
    m_connection_halfedge_ids(),
    m_connection_positions(),
    m_connections(),
    m_face_origin(),
    m_face_descendants(),
    m_instantiated_halfedge_a(),
    m_instantiated_halfedge_b(),
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
for (id_type face_id : m_mesh.submesh(submesh_id_a).face_ids()) { std::cout << std::format("    submesh_a face {}", id_string(face_id)) << std::endl; }
for (id_type face_id : m_mesh.submesh(submesh_id_b).face_ids()) { std::cout << std::format("    submesh_b face {}", id_string(face_id)) << std::endl; }
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

    align_segment_positions(segments);
//std::cout << "segments" << '\n' << segments << '\n';
    auto traces = order_intersections(segments);
//std::cout << "traces" << '\n' << traces << '\n';
    validate_intersection_traces(traces);

    // Instantiate each intersection trace, splitting and adding halfedges and faces as required
    // Uncouple the trace halfedges to produce the final connection traces of border halfedges
    m_connections = instantiate_connections(traces);
    validate_connections(m_connections);

    return m_connections;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::clear()
{
    m_floating_a.clear();
    m_floating_b.clear();
    m_connection_halfedge_ids.clear();
    m_connection_positions.clear();
    m_connections.clear();
    m_trace_point_refs.clear();
    m_face_origin.clear();
    m_face_descendants.clear();
    m_instantiated_halfedge_a.clear();
    m_instantiated_halfedge_b.clear();
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
std::cout << std::format("find_face_intersections_linear fa {}, fb {}", face_id_a, face_id_b) << std::endl;

    const auto& face_a = m_mesh.face(face_id_a);
    const auto& face_b = m_mesh.face(face_id_b);

    const auto plane_a = to_plane(face_a);
    const auto plane_b = to_plane(face_b);

    auto ix_a = face_plane_intersection(face_a, plane_b, trace_direction);
std::cout << std::format("ix_a {}", ix_a) << std::endl;
auto ix_a3 = face_plane_intersection3(face_a, plane_b, trace_direction);
std::cout << std::format("ix_a3 {}", ix_a3) << std::endl;
assert(ix_a == ix_a3);
    if (!ix_a)
    {
std::cout << "face_plane_intersection a returned nullopt" << std::endl;
        return {};
    }

    id_type id_a0 = ix_a->m_halfedge_id0;
    id_type id_a1 = ix_a->m_halfedge_id1;
    segment_type segment_a = *ix_a;
std::cout << std::format("face_plane_intersection a returned a0 {}, a1 {}, segment {}", id_string(id_a0), id_string(id_a1), segment_a) << std::endl;

    auto ix_b = face_plane_intersection(face_b, plane_a, trace_direction);
std::cout << std::format("ix_b {}", ix_b) << std::endl;
auto ix_b3 = face_plane_intersection3(face_b, plane_a, trace_direction);
std::cout << std::format("ix_b3 {}", ix_b3) << std::endl;
assert(ix_b == ix_b3);
    if (!ix_b)
    {
std::cout << "face_plane_intersection b returned nullopt" << std::endl;
        return {};
    }

    id_type id_b0 = ix_b->m_halfedge_id0;
    id_type id_b1 = ix_b->m_halfedge_id1;
    segment_type segment_b = *ix_b;
std::cout << std::format("face_plane_intersection b returned b0 {}, b1 {}, segment {}", id_string(id_b0), id_string(id_b1), segment_b) << std::endl;

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
std::optional<typename quetzal::brep::IntersectionSplitter<M>::AnnotatedSegment> quetzal::brep::IntersectionSplitter<M>::face_plane_intersection(const face_type& face, const plane_type& plane, const vector_type& trace_direction)
{
    using geometry::Locus;
std::cout << std::format("face_plane_intersection first face {} {}, plane {}, trace_direction {}", face.id(), face.attributes().normal(), plane, trace_direction) << std::endl;

    auto connection_contains = [this](const point_type& position)
    {
        for (const auto& connection_position : m_connection_positions)
        {
            if (float_eq(connection_position, position)) // Reject any portion of an earlier connection
            {
                return true;
            }
        }
        return false;
    };

//test_halfege_face_iteration(face);

    std::vector<std::tuple<id_type, intersection_type>> intersections;
    for (const auto& halfedge : face.halfedges())
//    for (const auto& halfedge : face.halfedges_ext())
    {
        if (m_connection_halfedge_ids.contains(halfedge.id())) // Reject any portion of an earlier connection
//if (connection_contains(vertex_position(halfedge)) || connection_contains(vertex_position(halfedge.next()))) // Reject any portion of an earlier connection
        {
//std::cout << std::format("face_plane_intersection rejecting halfedge {}", halfedge.id()) << std::endl;
            assert(m_connections.size() > 0); // This should never happen on the first connection
            continue;
        }

        auto ix = halfedge_plane_intersection(halfedge, plane);
        if (ix.empty())
        {
            continue;
        }
        else if (ix.locus() == Locus::Segment) // Intersection is an edge of the face
        {
            assert(intersections.empty() || get<0>(intersections[0]) == halfedge.next_id());
            assert(float_eq(ix.segment(), to_segment(halfedge)));
            if (direction_compare(ix.segment().direction(), trace_direction) <= 0)
            {
                return std::nullopt;
            }

            return {{to_segment(halfedge), halfedge.id(), halfedge.next_id()}};
        }
        else if (ix.locus() == Locus::Point)
        {
            if (m_connection_positions.contains(ix.point())) // Reject any portion of an earlier connection
//            if (connection_contains(ix.point())) // Reject any portion of an earlier connection
            {
//std::cout << std::format("face_plane_intersection rejecting position ix {}", ix.point()) << std::endl;
                assert(m_connections.size() > 0); // This should never happen on the first connection
                continue;
            }

            intersections.push_back({halfedge.id(), ix});
        }
    }

    if (intersections.size() == 2)
    {
        auto [id0, ix0] = intersections[0];
        auto [id1, ix1] = intersections[1];
        assert(ix0.locus() == Locus::Point && ix1.locus() == Locus::Point);
        auto segment = segment_type(ix0.point(), ix1.point());
        if (direction_compare(segment.direction(), trace_direction) < 0)
        {
            std::swap(id0, id1);
            segment.reverse();
        }
        return {{segment, id0, id1}};
    }

    if (intersections.size() == 1)
    {
        // This is the edge case where the plane grazes a single vertex, not yet supported ...
        // This would either be a floating point intersection or no interseetion (or a floating point that is later discarded) ...
        // (there is a similar edge or polyline only connection which should be treated similarly)
        // For now, discard it here, no intersection ...
        // This assertion is failing, we have no test cases that should ever reach this point ...
        // Why are we getting here at all? ...
//        assert(false);
        return std::nullopt;
    }

    assert(intersections.empty());
    return std::nullopt;
}

//------------------------------------------------------------------------------
template<typename M>
std::optional<typename quetzal::brep::IntersectionSplitter<M>::annotated_intersections_type> quetzal::brep::IntersectionSplitter<M>::face_plane_intersection2(const face_type& face, const plane_type& plane, const vector_type& trace_direction)
{
std::cout << std::format("face_plane_intersection2 face {} {}, plane {}, trace_direction {}", face.id(), face.attributes().normal(), plane, trace_direction) << std::endl;
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
std::cout << std::format("face_plane_intersection2 halfedge_plane_intersection {}", ix) << std::endl;
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

std::cout << std::format("face_plane_intersection2 sorted {}", ixs) << std::endl;
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
std::optional<typename quetzal::brep::IntersectionSplitter<M>::AnnotatedSegment> quetzal::brep::IntersectionSplitter<M>::face_plane_intersection3(const face_type& face, const plane_type& plane, const vector_type& trace_direction)
{
    using geometry::Locus;

    auto result = face_plane_intersection2(face, plane, trace_direction);
std::cout << std::format("face_plane_intersection2 {}", result ? std::format("{}", std::span<const AnnotatedIntersection>(*result)) : std::string("nullopt")) << std::endl;
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
typename quetzal::brep::IntersectionSplitter<M>::Traces quetzal::brep::IntersectionSplitter<M>::order_intersections(const TraceSegments& segments)
{
    std::vector<bool> used(segments.size(), false);
    Traces traces;

    auto find_candidates = [&](const point_type& position, size_t exclude_segment_index) -> std::vector<SegmentEndpointRef>
    {
        std::vector<SegmentEndpointRef> candidates;
        for (size_t i = 0; i < segments.size(); ++i)
        {
            if (used[i] || i == exclude_segment_index)
            {
                continue;
            }
            if (segments[i].p0.position == position)
            {
                candidates.push_back({i, 0});
            }
            // since segments are in trace_direction order, this should never happen ...
            if (segments[i].p1.position == position)
            {
                candidates.push_back({i, 1});
            }
        }
        return candidates;
    };

    for (size_t start_index = 0; start_index < segments.size(); ++start_index)
    {
        if (used[start_index])
        {
            continue;
        }

        TraceSegments trace;
        point_type start_position = segments[start_index].p0.position;
        size_t current_segment_index = start_index;
        int current_endpoint_index = 0;

        for (;;)
        {
            used[current_segment_index] = true;
            const TraceSegment& segment = segments[current_segment_index];
            const TracePoint& current_point = current_endpoint_index == 0 ? segment.p0 : segment.p1;
            const TracePoint& far_point = current_endpoint_index == 0 ? segment.p1 : segment.p0;
            // Oriented so .p0 == current_point, .p1 == far_point in traversal order, keeping each
            // point's own face_id_a/face_id_b intact rather than collapsing to a single shared point
            trace.push_back(current_endpoint_index == 0 ? segment : TraceSegment{segment.p1, segment.p0});
std::cout << std::format("DEBUG order_intersections trace[{}] <- segment {} reversed {} (p0 {} face_id_a {} face_id_b {}, p1 {} face_id_a {} face_id_b {})", trace.size() - 1, current_segment_index, current_endpoint_index != 0, segment.p0.position, id_string(segment.p0.face_id_a), id_string(segment.p0.face_id_b), segment.p1.position, id_string(segment.p1.face_id_a), id_string(segment.p1.face_id_b)) << std::endl;

            if (far_point.position == start_position)
            {
                break; // Loop closes back to trace[0] via this last segment
            }

            auto candidates = find_candidates(far_point.position, current_segment_index);
            assert(!candidates.empty()); // A trace can't dead-end -- every non-start point has a continuation
std::cout << std::format("DEBUG order_intersections far_point pos {} face_id_a {} face_id_b {}, {} candidates", far_point.position, id_string(far_point.face_id_a), id_string(far_point.face_id_b), candidates.size()) << std::endl;
for (const auto& c : candidates) { const TracePoint& cp = c.endpoint_index == 0 ? segments[c.segment_index].p0 : segments[c.segment_index].p1; std::cout << std::format("    candidate seg {} end {} face_id_a {} face_id_b {}", c.segment_index, c.endpoint_index, id_string(cp.face_id_a), id_string(cp.face_id_b)) << std::endl; }

            std::vector<SegmentEndpointRef> matching;
            for (const auto& ref : candidates)
            {
                const TracePoint& candidate_point = ref.endpoint_index == 0 ? segments[ref.segment_index].p0 : segments[ref.segment_index].p1;
                if (candidate_point.face_id_a == far_point.face_id_a || candidate_point.face_id_b == far_point.face_id_b)
                {
                    matching.push_back(ref);
                }
            }

            SegmentEndpointRef chosen;
            if (matching.size() == 1)
            {
                chosen = matching.front();
            }
            else if (matching.size() > 1)
            {
                chosen = disambiguate_connect_candidates(far_point, current_point, segments, matching);
            }
            else if (candidates.size() == 1)
            {
                chosen = candidates.front(); // No shared face id, but only one candidate physically present -- use it leniently
            }
            else
            {
                assert(false); // Genuinely stuck: multiple candidates, none sharing a face id
                chosen = candidates.front();
            }

            current_segment_index = chosen.segment_index;
            current_endpoint_index = chosen.endpoint_index;
        }

        traces.push_back(trace);
    }

    return traces;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::connection_traces_type quetzal::brep::IntersectionSplitter<M>::instantiate_connections(Traces& traces)
{
    index_trace_points(traces);

    connection_traces_type connections;
    for (auto& trace : traces)
    {
        connections.push_back(instantiate_trace(trace));
    }

    return connections;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::connection_trace_type quetzal::brep::IntersectionSplitter<M>::instantiate_trace(TraceSegments& trace)
{
    assert(trace.size() >= 2);
    assert(!m_floating_a);
    assert(!m_floating_b);

    connection_trace_type connection;
    const point_type connection_position0 = trace[0].p0.position;
    id_type connection_halfedge_id_a0 = nullid;
    id_type connection_halfedge_id_b0 = nullid;

    const size_t n = trace.size();
    for (size_t i = 0; i < n; ++i)
    {
        // Each segment is self-consistent (both its own points share its own face_id_a/face_id_b, as
        // discovered), so both endpoints are always resolved from the SAME segment -- never mixed with
        // the neighboring segment's point at the same position, which may disagree on the exiting face
        TracePoint& point0 = trace[i].p0;
        TracePoint& point1 = trace[i].p1;
        segment_type segment{point0.position, point1.position};

        id_type face_id_a0;
        id_type face_id_b0;
        id_type halfedge_id_a0;
        id_type halfedge_id_b0;

        std::tie(face_id_a0, halfedge_id_a0) = resolve_point(point0, true);
        std::tie(face_id_b0, halfedge_id_b0) = resolve_point(point0, false);

        if (i > 0)
        {
            // The "next" overload of create_intersection_edge requires h0 to already be an exact vertex
            // match. resolve_point's fallback (find_halfedge) only guarantees a containing edge, not an
            // exact match -- a vertex fan search was tried instead (halfedge_at_vertex_on_face) but that
            // relies on vertex IDENTITY, which doesn't hold here: split_edge gives each side of a coupled
            // edge its OWN new vertex id (see mesh_util.hpp), so a vertex on this face's side of a split
            // has no incident halfedge on the adjacent face even though they sit at the same position.
            // Position-based lookup (resolve_point/find_halfedge) works regardless of vertex identity;
            // just split it ourselves if it's not already exact, mirroring what the function already
            // does internally for h1
            if (halfedge_id_a0 != nullid && vertex_position(m_mesh.halfedge(halfedge_id_a0)) != point0.position)
            {
                assert(to_segment(m_mesh.halfedge(halfedge_id_a0)).contains(point0.position));
                split_trace_edge(halfedge_id_a0, point0.position);
                halfedge_id_a0 = m_mesh.halfedge(halfedge_id_a0).next_id();
                // The partner side of this split lands on the adjacent face for free -- record it (see
                // the matching reconciliation inside create_intersection_edge)
                m_instantiated_halfedge_a.emplace_back(point0.position, m_mesh.halfedge(halfedge_id_a0).partner_id());
            }
            if (halfedge_id_b0 != nullid && vertex_position(m_mesh.halfedge(halfedge_id_b0)) != point0.position)
            {
                assert(to_segment(m_mesh.halfedge(halfedge_id_b0)).contains(point0.position));
                split_trace_edge(halfedge_id_b0, point0.position);
                halfedge_id_b0 = m_mesh.halfedge(halfedge_id_b0).next_id();
                m_instantiated_halfedge_b.emplace_back(point0.position, m_mesh.halfedge(halfedge_id_b0).partner_id());
            }
        }

        auto [face_id_a1, halfedge_id_a1] = resolve_point(point1, true);
        auto [face_id_b1, halfedge_id_b1] = resolve_point(point1, false);

        align_segment_faces(point0, point1, true, face_id_a0, halfedge_id_a0, face_id_a1, halfedge_id_a1);
        align_segment_faces(point0, point1, false, face_id_b0, halfedge_id_b0, face_id_b1, halfedge_id_b1);

std::cout << std::format("DEBUG instantiate_trace i {} n {} face_id_a0 {} h_a0 {} face_id_a1 {} h_a1 {} face_id_b0 {} h_b0 {} face_id_b1 {} h_b1 {}", i, n, id_string(face_id_a0), id_string(halfedge_id_a0), id_string(face_id_a1), id_string(halfedge_id_a1), id_string(face_id_b0), id_string(halfedge_id_b0), id_string(face_id_b1), id_string(halfedge_id_b1)) << std::endl;
        vector_type trace_direction = normalize(cross(m_mesh.face(face_id_a0).attributes().normal(), m_mesh.face(face_id_b0).attributes().normal()));
std::cout << std::format("DEBUG trace_direction i {} normal_a {} normal_b {} trace_direction {}", i, m_mesh.face(face_id_a0).attributes().normal(), m_mesh.face(face_id_b0).attributes().normal(), trace_direction) << std::endl;

        id_type halfedge_id_a_exterior;
        id_type halfedge_id_a_tracer;
        id_type halfedge_id_b_exterior;
        id_type halfedge_id_b_tracer;

        if (i == 0)
        {
            std::tie(halfedge_id_a_exterior, halfedge_id_a_tracer) = create_intersection_edge(face_id_a0, halfedge_id_a0, halfedge_id_a1, segment, -trace_direction, m_floating_a, m_instantiated_halfedge_a);
            std::tie(halfedge_id_b_exterior, halfedge_id_b_tracer) = create_intersection_edge(face_id_b0, halfedge_id_b0, halfedge_id_b1, segment, trace_direction, m_floating_b, m_instantiated_halfedge_b);
        }
        else
        {
            bool is_last_segment = i == n - 1;
            std::tie(halfedge_id_a_exterior, halfedge_id_a_tracer) = create_intersection_edge(halfedge_id_a0, halfedge_id_a1, segment, -trace_direction, m_floating_a, connection_halfedge_id_a0, is_last_segment, m_instantiated_halfedge_a);
            std::tie(halfedge_id_b_exterior, halfedge_id_b_tracer) = create_intersection_edge(halfedge_id_b0, halfedge_id_b1, segment, trace_direction, m_floating_b, connection_halfedge_id_b0, is_last_segment, m_instantiated_halfedge_b);
        }

        id_type halfedge_id_a_interior = interior_partner(halfedge_id_a_exterior, -trace_direction);
        id_type halfedge_id_b_interior = interior_partner(halfedge_id_b_exterior, trace_direction);

        insert_trace_intersection(halfedge_id_a_exterior, halfedge_id_a_interior, halfedge_id_b_exterior, halfedge_id_b_interior);
std::cout << std::format("DEBUG i {} inserted a_ext {} face {} pos {} (v {}), a_int {} face {} pos {} (v {}), b_ext {} face {} pos {} (v {}), b_int {} face {} pos {} (v {})", i, id_string(halfedge_id_a_exterior), id_string(m_mesh.halfedge(halfedge_id_a_exterior).face_id()), vertex_position(m_mesh.halfedge(halfedge_id_a_exterior)), id_string(m_mesh.halfedge(halfedge_id_a_exterior).vertex_id()), id_string(halfedge_id_a_interior), id_string(m_mesh.halfedge(halfedge_id_a_interior).face_id()), vertex_position(m_mesh.halfedge(halfedge_id_a_interior)), id_string(m_mesh.halfedge(halfedge_id_a_interior).vertex_id()), id_string(halfedge_id_b_exterior), id_string(m_mesh.halfedge(halfedge_id_b_exterior).face_id()), vertex_position(m_mesh.halfedge(halfedge_id_b_exterior)), id_string(m_mesh.halfedge(halfedge_id_b_exterior).vertex_id()), id_string(halfedge_id_b_interior), id_string(m_mesh.halfedge(halfedge_id_b_interior).face_id()), vertex_position(m_mesh.halfedge(halfedge_id_b_interior)), id_string(m_mesh.halfedge(halfedge_id_b_interior).vertex_id())) << std::endl;

        connection.push_back({halfedge_id_a_exterior, halfedge_id_a_interior, halfedge_id_b_exterior, halfedge_id_b_interior});


        if (i == 0)
        {
            connection_halfedge_id_a0 = vertex_position(m_mesh.halfedge(halfedge_id_a_exterior)) == connection_position0 ? halfedge_id_a_exterior : halfedge_id_a_interior;
            connection_halfedge_id_b0 = vertex_position(m_mesh.halfedge(halfedge_id_b_exterior)) == connection_position0 ? halfedge_id_b_exterior : halfedge_id_b_interior;
        }
    }

    uncouple(connection);
    return connection;
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
typename quetzal::brep::IntersectionSplitter<M>::SegmentEndpointRef quetzal::brep::IntersectionSplitter<M>::disambiguate_connect_candidates(const TracePoint& trace_point, const TracePoint& trace_point_prev, const TraceSegments& segments, const std::vector<SegmentEndpointRef>& candidates)
{
    using geometry::Locus;

    vector_type incoming_direction = normalize(trace_point.position - trace_point_prev.position);

    for (const auto& candidate : candidates)
    {
        const TraceSegment& segment = segments[candidate.segment_index];
        const TracePoint& candidate_point = candidate.endpoint_index == 0 ? segment.p0 : segment.p1;
        const TracePoint& candidate_far_point = candidate.endpoint_index == 0 ? segment.p1 : segment.p0;

        bool shares_a = candidate_point.face_id_a == trace_point.face_id_a;
        id_type face_id_from = shares_a ? trace_point.face_id_b : trace_point.face_id_a;
        id_type face_id_to = shares_a ? candidate_point.face_id_b : candidate_point.face_id_a;

        if (face_id_from == nullid || face_id_to == nullid)
        {
            continue; // Floating on the transitioning side -- no face boundary to test at this vertex
        }

        const auto& face_from = m_mesh.face(face_id_from);
        const auto& face_to = m_mesh.face(face_id_to);
        if (!intersects(to_polygon(face_to), to_plane(face_from)))
        {
            continue; // Quick rejection
        }

        auto ix = polygon_intersection(to_polygon(face_from), to_polygon(face_to), trace_point.position);
        if (ix.empty() || ix.locus() != Locus::Segment)
        {
            continue;
        }

        vector_type outgoing_direction = normalize(candidate_far_point.position - trace_point.position);
        if (!math::float_gt(dot(outgoing_direction, incoming_direction), value_type{0}))
        {
            continue; // Doubles back toward where the trace came from -- not a valid continuation
        }

        return candidate;
    }

    assert(false); // No candidate passed the geometric continuation check -- a genuinely unresolved tie
    return candidates.front();
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::IntersectionSplitter<M>::intersection_type quetzal::brep::IntersectionSplitter<M>::polygon_intersection(const polygon_type& polygon_a, const polygon_type& polygon_b, const point_type& position0)
{
    using geometry::Locus;
std::cout << std::format("polygon_intersection position0 {}", position0) << std::endl;
//std::cout << "\n(original)" << std::endl;
//    auto ixx = intersection(polygon_a, polygon_b);
//std::cout << std::format("intersection pp returned ix {}", ixx) << std::endl;
//    return ixx;

    assert(polygon_a.plane().contains(position0));
    assert(polygon_b.plane().contains(position0));
/* why not? ...
    bool b = false;
    for (const auto& position : polygon_a.vertices())
    {
        if (float_eq(position, position0))
        {
            assert(position == position0);
            b = true;
            break;
        }
    }
    assert(b);
    b = false;
    for (const auto& position : polygon_b.vertices())
    {
        if (float_eq(position, position0))
        {
            assert(position == position0);
            b = true;
            break;
        }
    }
    assert(b);
*/
//std::cout << "\n(new)" << std::endl;
    auto ixs = intersection_non_convex(polygon_a, polygon_b);
std::cout << std::format("intersection_non_convex pp returned n {}", ixs.size()) << std::endl;
//for (const auto ix : ixs) std::cout << std::format("    ix {}", ix) << std::endl;
    if (ixs.empty())
    {
        return {};
    }
/*
    for (const auto& ix : ixs)
    {
        if (ix.locus() == Locus::Segment)
        {
            if (float_eq(ix.segment().endpoint(1), position0))
            {
                assert(ix.segment().endpoint(1) == position0);
                ix.segment().reverse();
            }
        }
    }
std::cout << std::format("    oriented n {}", ixs.size()) << std::endl;
for (const auto ix : ixs) std::cout << std::format("    ix {}", ix) << std::endl;
*/
    // All intersection positions fall on the line of intersection of the two planes

    // Arbitrary direction, all intersection points should be in that direction or opposite it
    // If it turns out that there can be intersections in both directions, then we'll need something more sophisticated ...
    auto direction = normalize(cross(polygon_a.plane().normal(), polygon_b.plane().normal()));
    std::vector<value_type> direction_indicators;

    value_type d_closest = std::numeric_limits<value_type>::max();
    std::optional<point_type> position_closest;
    bool found_position0 = false;

    for (const auto& ix : ixs)
    {
        if (ix.locus() == Locus::Point)
        {
            auto position = ix.point();
            if (float_eq(position, position0))
            {
                // assert == ...
                found_position0 = true;
                continue;
            }

            direction_indicators.push_back(dot(position - position0, direction));

            auto d = (position - position0).norm_squared();
            if (math::float_lt(d, d_closest))
            {
                d_closest = d;
                position_closest = position;
            }
        }
        else if (ix.locus() == Locus::Segment)
        {
            for (int i = 0; i < 2; ++i)
            {
                auto position = ix.segment().endpoint(i);
                if (float_eq(position, position0))
                {
                    found_position0 = true;
                    continue;
                }

                direction_indicators.push_back(dot(position - position0, direction));

                auto d = (position - position0).norm_squared();
                if (math::float_lt(d, d_closest))
                {
                    d_closest = d;
                    position_closest = position;
                }
            }
        }
        else if (ix.locus() == Locus::Polygon) // coplanar polygons
        {
            assert(false); // not yet supported ...
        }
    }

    // Check that position0 was in the intersection set
    assert(found_position0);

    if (!position_closest)
    {
        return position0;
    }

    // Check that all intersection points are in the same direction from position0
    size_t n_lt0 = 0;
    size_t n_gt0 = 0;
    for (auto d : direction_indicators)
    {
        if (math::float_lt(d, value_type(0))) ++n_lt0;
        else if (math::float_gt(d, value_type(0))) ++n_gt0;
    }
    assert(n_lt0 == 0 || n_gt0 == 0);

    auto segment = segment_type(position0, *position_closest);

    // Check that the resulting segment is inside the polygons
    // Because the segment is constructed from the closest point, there can be no intervening interior/exterior transitions
    auto midpoint = segment.point(value_type(0.5));
    assert(polygon_a.contains(midpoint));
    assert(polygon_b.contains(midpoint));

    return segment;
}

//------------------------------------------------------------------------------
template<typename M>
std::array<quetzal::id_type, 2> quetzal::brep::IntersectionSplitter<M>::create_intersection_edge(const id_type face_id, id_type halfedge_id0, id_type halfedge_id1, const segment_type& segment, const vector_type& exterior_direction, Floating& floating, std::vector<std::pair<point_type, id_type>>& instantiated_halfedge_this_side)
{
std::cout << std::format("create_intersection_edge first f {}, h0 {}, h1 {}, segment {}, exterior_direction {}", id_string(face_id), id_string(halfedge_id0), id_string(halfedge_id1), segment, exterior_direction) << std::endl;
    assert(halfedge_id0 == nullid || m_mesh.halfedge(halfedge_id0).face_id() == face_id);
    assert(halfedge_id1 == nullid || m_mesh.halfedge(halfedge_id1).face_id() == face_id);
    assert(halfedge_id0 == nullid || to_segment(m_mesh.halfedge(halfedge_id0)).contains(segment.endpoint(0)));
    assert(halfedge_id1 == nullid || to_segment(m_mesh.halfedge(halfedge_id1)).contains(segment.endpoint(1)));
    assert(halfedge_id0 == nullid || ne_or_exact(vertex_position(m_mesh.halfedge(halfedge_id0)), segment.endpoint(0)));
    assert(halfedge_id1 == nullid || ne_or_exact(vertex_position(m_mesh.halfedge(halfedge_id1)), segment.endpoint(1)));

    // Both intersections floating, within face
    if (halfedge_id0 == nullid && halfedge_id1 == nullid)
    {
std::cout << "create_intersection_edge first floating" << std::endl;
        id_type halfedge_id = create_edge_internal(face_id, segment);
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);

        // This is the only place where floating gets initialized
        assert(!floating);
        id_type partner_id = m_mesh.halfedge(halfedge_id).partner_id();
        floating = {halfedge_id, partner_id};
//std::cout << std::format("create_intersection_edge first floating h {} {}", id_string(halfedge_id), to_segment(m_mesh.halfedge(halfedge_id))) << std::endl;
//std::cout << std::format("create_intersection_edge first floating p {} {}", id_string(partner_id), to_segment(m_mesh.halfedge(partner_id))) << std::endl;
//std::cout << std::format("create_intersection_edge first floating e {} {}", id_string(halfedge_id_exterior), to_segment(m_mesh.halfedge(halfedge_id_exterior))) << std::endl;
        m_mesh.face(face_id).create_hole(halfedge_id);
//        id_type halfedge_id_tracer = partner_id; // correct, there is no other halfedge at this vertex ...
//        return {halfedge_id_exterior, halfedge_id_tracer};
        return {halfedge_id_exterior, nullid};
    }

    // Intersections along edges require a split so that each then reference vertices instead

    id_type halfedge_id_tracer = nullid;

    if (halfedge_id0 != nullid)
    {
        if (segment.endpoint(0) != vertex_position(m_mesh.halfedge(halfedge_id0)))
        {
std::cout << std::format("create_intersection_edge first split_edge_0 h {} {}, pos {}", halfedge_id0, to_segment(m_mesh.halfedge(halfedge_id0)), segment.endpoint(0)) << std::endl;
            split_trace_edge(halfedge_id0, segment.endpoint(0));
auto hn = m_mesh.halfedge(halfedge_id0).next();
auto hp = m_mesh.halfedge(halfedge_id0).partner();
std::cout << std::format("    created next {} face {} {}, partner {} face {} {}", hn.id(), id_string(hn.face_id()), to_segment(hn), hp.id(), id_string(hp.face_id()), to_segment(hp)) << std::endl;
            // hp sits on the adjacent face, at this exact position, for free -- record it now so a
            // later point resolving onto that adjacent face finds it directly instead of needing a
            // bridge back to whichever fragment hn's own side ends up on after further splits
            instantiated_halfedge_this_side.emplace_back(vertex_position(hp), hp.id());
            halfedge_id0 = m_mesh.halfedge(halfedge_id0).next_id();
        }
    }

    if (halfedge_id1 != nullid)
    {
        if (segment.endpoint(1) != vertex_position(m_mesh.halfedge(halfedge_id1)))
        {
            assert(to_segment(m_mesh.halfedge(halfedge_id1)).contains(segment.endpoint(1)));
std::cout << std::format("create_intersection_edge first split_edge_1 h {}, segment {}, pos {}", halfedge_id1, to_segment(m_mesh.halfedge(halfedge_id1)), segment.endpoint(1)) << std::endl;
            split_trace_edge(halfedge_id1, segment.endpoint(1));
auto hn = m_mesh.halfedge(halfedge_id1).next();
auto hp = m_mesh.halfedge(halfedge_id1).partner();
std::cout << std::format("    created next {} face {} {}, partner {} face {} {}", hn.id(), id_string(hn.face_id()), to_segment(hn), hp.id(), id_string(hp.face_id()), to_segment(hp)) << std::endl;
//            halfedge_id_tracer = m_mesh.halfedge(halfedge_id1).partner_id(); // this is the possible fold back edge case ...
            // hp sits on the adjacent face, at this exact position, for free -- record it now (see the
            // matching comment on the split_edge_0 branch above)
            instantiated_halfedge_this_side.emplace_back(vertex_position(hp), hp.id());
            halfedge_id1 = m_mesh.halfedge(halfedge_id1).next_id();
        }
    }

    // Handle the case where one of the intersection endpoints is within the face

    // Initial intersection point is floating, within face
    if (halfedge_id0 == nullid)
    {
std::cout << "create_intersection_edge first initial floating" << std::endl;
        id_type halfedge_id = create_edge_partial(halfedge_id1, segment.endpoint(0));
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);
        return {halfedge_id_exterior, halfedge_id_tracer};
    }

    assert(segment.endpoint(0) == vertex_position(m_mesh.halfedge(halfedge_id0)));

    // Terminal intersection point is floating, within face
    if (halfedge_id1 == nullid)
    {
std::cout << "create_intersection_edge first terminal floating" << std::endl;
        id_type halfedge_id = create_edge_partial(halfedge_id0, segment.endpoint(1));
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);
//        halfedge_id_tracer = m_mesh.halfedge(halfedge_id).partner_id(); // correct, there is no other halfedge at this vertex ...
        return {halfedge_id_exterior, halfedge_id_tracer};
    }

    assert(segment.endpoint(1) == vertex_position(m_mesh.halfedge(halfedge_id1)));

    // We now have a vertex to vertex intersection

    id_type halfedge_id_exterior = nullid;

    if (halfedge_id1 == m_mesh.halfedge(halfedge_id0).next_id()) // Intersection is this edge
    {
std::cout << "create_intersection_edge first vv intersection this edge" << std::endl;
        halfedge_id_exterior = exterior_halfedge(halfedge_id0, exterior_direction);
    }
    else if (halfedge_id1 == m_mesh.halfedge(halfedge_id0).prev_id()) // Intersection is the previous edge
    {
std::cout << "create_intersection_edge first vv intersection prev edge" << std::endl;
        halfedge_id_exterior = exterior_halfedge(m_mesh.halfedge(halfedge_id0).prev_id(), exterior_direction);
    }
    else // Intersection requires a vertex to vertex connection
    {
        split_face(m_mesh, halfedge_id0, halfedge_id1);
        register_face_split(face_id, m_mesh.halfedge(halfedge_id1).face_id());
std::cout << std::format("create_intersection_edge first split_face vv kept face {} (h0 {}), new face {} (h1 {}) created h0p {} face {}, h1p {} face {}", face_id, halfedge_id0, id_string(m_mesh.halfedge(halfedge_id1).face_id()), halfedge_id1, id_string(m_mesh.halfedge(halfedge_id0).prev_id()), id_string(m_mesh.halfedge(m_mesh.halfedge(halfedge_id0).prev_id()).face_id()), id_string(m_mesh.halfedge(halfedge_id1).prev_id()), id_string(m_mesh.halfedge(m_mesh.halfedge(halfedge_id1).prev_id()).face_id())) << std::endl;
        halfedge_id_exterior = exterior_halfedge(m_mesh.halfedge(halfedge_id1).prev_id(), exterior_direction);
    }

    return {halfedge_id_exterior, halfedge_id_tracer};
}

//------------------------------------------------------------------------------
template<typename M>
std::array<quetzal::id_type, 2> quetzal::brep::IntersectionSplitter<M>::create_intersection_edge(const id_type halfedge_id0, id_type halfedge_id1, const segment_type& segment, const vector_type& exterior_direction, Floating& floating, id_type connection_halfedge_id0, bool is_last_segment, std::vector<std::pair<point_type, id_type>>& instantiated_halfedge_this_side)
{
//std::cout << std::format("create_intersection_edge next assertion hid1 {}, position1 {}, endpoint1 {}", halfedge_id1, halfedge_id1 != nullid ? vertex_position(m_mesh.halfedge(halfedge_id1)) : point_type{}, segment.endpoint(1)) << std::endl;
    assert(halfedge_id0 != nullid);
    assert(halfedge_id1 == nullid || to_segment(m_mesh.halfedge(halfedge_id1)).contains(segment.endpoint(1)));
    assert(vertex_position(m_mesh.halfedge(halfedge_id0)) == segment.endpoint(0));
    assert(halfedge_id1 == nullid || ne_or_exact(vertex_position(m_mesh.halfedge(halfedge_id1)), segment.endpoint(1)));

    // Split face according to the type of intersection

    const id_type face_id = m_mesh.halfedge(halfedge_id0).face_id();
if (halfedge_id1 != nullid && m_mesh.halfedge(halfedge_id1).face_id() != face_id) std::cout << std::format("DEBUG mismatch: h0 {} face {}, h1 {} face {}, h1 pos {}, segment {}", id_string(halfedge_id0), id_string(face_id), id_string(halfedge_id1), id_string(m_mesh.halfedge(halfedge_id1).face_id()), vertex_position(m_mesh.halfedge(halfedge_id1)), segment) << std::endl;
    assert(halfedge_id1 == nullid || m_mesh.halfedge(halfedge_id1).face_id() == face_id);
std::cout << std::format("create_intersection_edge next f {}, h0 {}, h1 {}, segment {}, exterior_direction {}", id_string(face_id), id_string(halfedge_id0), id_string(halfedge_id1), segment, exterior_direction) << std::endl;

    id_type halfedge_id_tracer = nullid;

    // Terminal intersection point is floating, within face
    if (halfedge_id1 == nullid)
    {
        // The trace has arrived back at its own starting position via a second, independent floating
        // point rather than revisiting the first one -- close back to the trace's own initial halfedge
        // by identity (is_last_segment, known structurally from the trace's already-fixed length, not
        // a position guess) instead of leaving it orphaned while a second, unrelated floating stub
        // gets created alongside it
        if (is_last_segment && connection_halfedge_id0 != halfedge_id0)
        {
std::cout << "create_intersection_edge next closing back to the trace's initial halfedge" << std::endl;
            id_type halfedge_id = create_edge_connect(halfedge_id0, connection_halfedge_id0);
            id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);

            // Splicing this edge in divides face_id's loop into two cycles; face_id keeps one,
            // the other (the far side of the connecting edge) needs to become its own face.
            // face_id's own halfedge_id may have been pointing into the cycle that just moved,
            // so it needs to be re-pointed at a halfedge known to still be on its side.
            id_type face_halfedge_id = m_mesh.halfedge(halfedge_id_exterior).partner_id();
            id_type face_id1 = m_mesh.create_face(m_mesh.face(face_id).surface_id(), face_halfedge_id, m_mesh.face(face_id).attributes());
            register_face_split(face_id, face_id1);
            for (auto& halfedge : m_mesh.face(face_id1).halfedges())
            {
                halfedge.set_face_id(face_id1);
            }

            m_mesh.face(face_id).set_halfedge_id(halfedge_id_exterior);

            return {halfedge_id_exterior, nullid};
        }

std::cout << "create_intersection_edge next floating" << std::endl;
        id_type halfedge_id = create_edge_partial(halfedge_id0, segment.endpoint(1));
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);
        id_type partner_id = m_mesh.halfedge(halfedge_id).partner_id();

        // This is the only place where floating chain gets extended
        if (halfedge_id0 == floating.id1)
        {
            floating.id1 = partner_id;
        }

//        halfedge_id_tracer = partner_id; // correct, there is no other halfedge at this vertex ...
        return {halfedge_id_exterior, halfedge_id_tracer};
    }

    if (halfedge_id1 == floating.id0)
    {
        // Close off the floating section
        // Add a hole with an uncoupled face
std::cout << std::format("create_intersection_edge next create_edge_connect floating f {} h0 {} h1 {} -> {} {}", face_id, id_string(halfedge_id0), id_string(halfedge_id1), m_mesh.halfedge(halfedge_id0).prev_id(), m_mesh.halfedge(halfedge_id1).prev_id()) << std::endl;
        id_type halfedge_id = create_edge_connect(halfedge_id0, halfedge_id1);
assert(vertex_position(m_mesh.halfedge(halfedge_id).partner()) == vertex_position(m_mesh.halfedge(halfedge_id).next()));
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);
std::cout << std::format("create_intersection_edge next partial h {} e {} {} {}", id_string(halfedge_id), id_string(halfedge_id_exterior), vertex_position(m_mesh.halfedge(halfedge_id)), vertex_position(m_mesh.halfedge(halfedge_id_exterior))) << std::endl;

        id_type face_halfedge_id = m_mesh.halfedge(halfedge_id_exterior).partner_id();
        id_type hole_halfedge_id = halfedge_id_exterior;

        id_type face_id1 = m_mesh.create_face(m_mesh.face(face_id).surface_id(), face_halfedge_id, m_mesh.face(face_id).attributes());
        register_face_split(face_id, face_id1);
std::cout << std::format("DEBUG hole-extract: face_id {} face_id1 {} face_halfedge_id {} hole_halfedge_id {}", id_string(face_id), id_string(face_id1), id_string(face_halfedge_id), id_string(hole_halfedge_id)) << std::endl;
        for (auto& halfedge : m_mesh.face(face_id1).halfedges())
        {
std::cout << std::format("    relabeling {} (was face {}) to face {}, pos {}", halfedge.id(), id_string(halfedge.face_id()), id_string(face_id1), vertex_position(halfedge)) << std::endl;
            halfedge.set_face_id(face_id1);
        }

        m_mesh.face(face_id).create_hole(hole_halfedge_id);
        m_mesh.face(face_id).remove_hole(floating.id0);
        floating.clear();
        return {halfedge_id_exterior, nullid};
    }

    // Intersection along the edge requires a split
    if (segment.endpoint(1) != vertex_position(m_mesh.halfedge(halfedge_id1)))
    {
        assert(to_segment(m_mesh.halfedge(halfedge_id1)).contains(segment.endpoint(1)));
std::cout << std::format("create_intersection_edge next split_edge_1 h {} {}, pos {}", halfedge_id1, to_segment(m_mesh.halfedge(halfedge_id1)), segment.endpoint(1)) << std::endl;
        split_trace_edge(halfedge_id1, segment.endpoint(1));
auto hn = m_mesh.halfedge(halfedge_id1).next();
auto hp = m_mesh.halfedge(halfedge_id1).partner();
std::cout << std::format("    created next {} face {} {}, partner {} face {} {}", hn.id(), id_string(hn.face_id()), to_segment(hn), hp.id(), id_string(hp.face_id()), to_segment(hp)) << std::endl;
//        halfedge_id_tracer = m_mesh.halfedge(halfedge_id1).partner_id();  // probably ok even if the intersection trace folds back into the same face (vertex position is correct) ...
//assert(vertex_position(m_mesh.halfedge(halfedge_id_tracer)) == segment.endpoint(1));
        // hp sits on the adjacent face, at this exact position, for free -- record it now (see the
        // matching comment on the "first" overload's split_edge branches)
        instantiated_halfedge_this_side.emplace_back(vertex_position(hp), hp.id());
        halfedge_id1 = m_mesh.halfedge(halfedge_id1).next_id();
    }

    if (halfedge_id0 == floating.id1)
    {
        // Add the floating section to the face

        id_type halfedge_id = create_edge_connect(halfedge_id0, halfedge_id1);
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);

        m_mesh.face(face_id).remove_hole(floating.id0);
        floating.clear();
        return {halfedge_id_exterior, nullid};
    }

    // We now have a vertex to vertex intersection

    id_type halfedge_id_exterior = nullid;

    if (halfedge_id1 == m_mesh.halfedge(halfedge_id0).next_id()) // Intersection is this edge
    {
std::cout << "create_intersection_edge next vv intersection this edge" << std::endl;
        halfedge_id_exterior = exterior_halfedge(halfedge_id0, exterior_direction);
    }
    else if (halfedge_id1 == m_mesh.halfedge(halfedge_id0).prev_id()) // Intersection is the previous edge
    {
std::cout << "create_intersection_edge next vv intersection prev edge" << std::endl;
        halfedge_id_exterior = exterior_halfedge(m_mesh.halfedge(halfedge_id0).prev_id(), exterior_direction);
    }
    else // Intersection requires a vertex to vertex connection
    {
std::cout << std::format("create_intersection_edge next split_face vv f {}, h0 {}, h1 {}", face_id, halfedge_id0, halfedge_id1) << std::endl;
//validate(m_mesh);
//dump_faces(m_mesh);
        split_face(m_mesh, halfedge_id0, halfedge_id1);
        register_face_split(face_id, m_mesh.halfedge(halfedge_id1).face_id());
std::cout << std::format("create_intersection_edge next split_face vv kept face {} (h0 {}), new face {} (h1 {}) created h0p {} face {}, h1p {} face {}", face_id, halfedge_id0, id_string(m_mesh.halfedge(halfedge_id1).face_id()), halfedge_id1, id_string(m_mesh.halfedge(halfedge_id0).prev_id()), id_string(m_mesh.halfedge(m_mesh.halfedge(halfedge_id0).prev_id()).face_id()), id_string(m_mesh.halfedge(halfedge_id1).prev_id()), id_string(m_mesh.halfedge(m_mesh.halfedge(halfedge_id1).prev_id()).face_id())) << std::endl;
        halfedge_id_exterior = exterior_halfedge(m_mesh.halfedge(halfedge_id1).prev_id(), exterior_direction);
    }

    return {halfedge_id_exterior, halfedge_id_tracer};
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::create_edge_internal(const id_type face_id, const segment_type& segment)
{
    auto [position0, position1] = segment.endpoints();

    auto normal = m_mesh.face(face_id).attributes().normal();

    auto mapping = position_texcoord_mapping(m_mesh.face(face_id));
    auto texcoord0 = denoise01(mapping(position0));
    auto texcoord1 = denoise01(mapping(position1));

    typename M::vertex_attributes_type av0{position0, normal, texcoord0};
    typename M::vertex_attributes_type av1{position1, normal, texcoord1};

    size_t nh = m_mesh.halfedge_store_count();
    m_mesh.create_halfedge_vertex(nh + 1, nh + 1, nh + 1, face_id, av0);
    m_mesh.create_halfedge_vertex(nh, nh, nh, face_id, av1);

std::cout << "    " << std::format("create_edge_internal f {} segment {} created {} {}", id_string(face_id), segment, nh, nh + 1) << std::endl;
    return nh;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::create_edge_partial(const id_type halfedge_id0, const point_type& position1)
{
    id_type face_id = m_mesh.halfedge(halfedge_id0).face_id();
    auto normal = m_mesh.face(face_id).attributes().normal();

    auto mapping = position_texcoord_mapping(m_mesh.face(face_id));
    auto texcoord1 = denoise01(mapping(position1));

    typename M::vertex_attributes_type av0 = m_mesh.halfedge(halfedge_id0).vertex_attributes();
    typename M::vertex_attributes_type av1{position1, normal, texcoord1};

    size_t nh = m_mesh.halfedge_store_count();
    m_mesh.create_halfedge_vertex(nh + 1, nh + 1, m_mesh.halfedge(halfedge_id0).prev_id(), face_id, av0);
    m_mesh.create_halfedge_vertex(nh, halfedge_id0, nh, face_id, av1);

    m_mesh.halfedge(halfedge_id0).prev().set_next_id(nh);
    m_mesh.halfedge(halfedge_id0).set_prev_id(nh + 1);

std::cout << std::format("create_edge_partial h {} (face {}) p {} created {} {} (both on face {})", id_string(halfedge_id0), id_string(face_id), position1, nh, nh + 1, id_string(face_id)) << std::endl;
    return nh;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::create_edge_connect(const id_type halfedge_id0, const id_type halfedge_id1, bool coupled)
{
std::cout << std::format("create_edge_connect {} {} {} {}", id_string(halfedge_id0), id_string(halfedge_id1), vertex_position(m_mesh.halfedge(halfedge_id0)), vertex_position(m_mesh.halfedge(halfedge_id1))) << std::endl;
    id_type face_id = m_mesh.halfedge(halfedge_id0).face_id();
    assert(m_mesh.halfedge(halfedge_id1).face_id() == face_id);

    size_t nh = m_mesh.halfedge_store_count();

    id_type partner_id0 = coupled ? nh + 1 : nullid;
    id_type partner_id1 = coupled ? nh : nullid;

    const auto av0 = m_mesh.halfedge(halfedge_id0).vertex_attributes();
    const auto av1 = m_mesh.halfedge(halfedge_id1).vertex_attributes();

    m_mesh.create_halfedge_vertex(partner_id0, halfedge_id1, m_mesh.halfedge(halfedge_id0).prev_id(), face_id, av0);
    m_mesh.create_halfedge_vertex(partner_id1, halfedge_id0, m_mesh.halfedge(halfedge_id1).prev_id(), face_id, av1);

    m_mesh.halfedge(halfedge_id0).prev().set_next_id(nh);
    m_mesh.halfedge(halfedge_id0).set_prev_id(nh + 1);

    m_mesh.halfedge(halfedge_id1).prev().set_next_id(nh + 1);
    m_mesh.halfedge(halfedge_id1).set_prev_id(nh);

std::cout << "    " << std::format("create_edge_connect h0 {} h1 {} created {} {} (both on face {})", id_string(halfedge_id0), id_string(halfedge_id1), nh, nh + 1, id_string(face_id)) << std::endl;
    return nh;
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

    // Record both the ext and int halfedge on each side, since a later trace's point may need either
    // one's face at this position
    m_instantiated_halfedge_a.emplace_back(vertex_position(m_mesh.halfedge(halfedge_id_a_exterior)), halfedge_id_a_exterior);
    m_instantiated_halfedge_a.emplace_back(vertex_position(m_mesh.halfedge(halfedge_id_a_interior)), halfedge_id_a_interior);
    m_instantiated_halfedge_b.emplace_back(vertex_position(m_mesh.halfedge(halfedge_id_b_exterior)), halfedge_id_b_exterior);
    m_instantiated_halfedge_b.emplace_back(vertex_position(m_mesh.halfedge(halfedge_id_b_interior)), halfedge_id_b_interior);
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
    for (const auto& halfedge : m_mesh.face(face_id).halfedges())
    {
        if (covers(halfedge.id(), position))
        {
            return halfedge.id();
        }
    }

    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
bool quetzal::brep::IntersectionSplitter<M>::covers(const id_type halfedge_id, const point_type& position) const
{
    const auto segment = to_segment(m_mesh.halfedge(halfedge_id));
    return segment.contains(position) && segment.endpoint(1) != position;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::index_trace_points(Traces& traces)
{
    m_trace_point_refs.clear();

    for (auto& trace : traces)
    {
        for (auto& segment : trace)
        {
            for (TracePoint* ptrace_point : {&segment.p0, &segment.p1})
            {
                for (bool side_a : {true, false})
                {
                    TracePointRef ref = {ptrace_point, side_a};
                    if (ref.halfedge_id() != nullid)
                    {
                        m_trace_point_refs[ref.halfedge_id()].push_back(ref);
                    }
                }
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::split_trace_edge(const id_type halfedge_id, const point_type& position)
{
    // The split rewires partner relationships, so both halfedges of the edge are captured beforehand. Each
    // is shortened in place and gains the remainder of its own side as its new successor, so one rule
    // covers both
    const std::array<id_type, 2> halfedge_ids = {halfedge_id, m_mesh.halfedge(halfedge_id).partner_id()};

    split_edge(m_mesh, halfedge_id, position);

    for (id_type id : halfedge_ids)
    {
        if (id == nullid)
        {
            continue;
        }

        auto i = m_trace_point_refs.find(id);
        if (i == m_trace_point_refs.end())
        {
            continue;
        }

        const id_type id_next = m_mesh.halfedge(id).next_id();
        std::vector<TracePointRef> refs_moved;

        std::erase_if(i->second, [this, id, &refs_moved](const TracePointRef& ref) -> bool
        {
            if (covers(id, ref.position()))
            {
                return false;
            }

            refs_moved.push_back(ref);
            return true;
        });

        // Reindexed after the erase so that rehashing cannot invalidate the bucket being erased from.
        // face_id is left alone: it names the original face the point was discovered on, which is what
        // identifies a trace continuing across the same face, and the current face is always available
        // from the halfedge itself
        for (const auto& ref : refs_moved)
        {
            ref.halfedge_id() = id_next;
            m_trace_point_refs[id_next].push_back(ref);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::set_trace_point_halfedge(const TracePointRef& ref, const id_type halfedge_id)
{
    if (ref.halfedge_id() == halfedge_id)
    {
        return;
    }

    if (ref.halfedge_id() != nullid)
    {
        auto i = m_trace_point_refs.find(ref.halfedge_id());
        if (i != m_trace_point_refs.end())
        {
            std::erase(i->second, ref);
        }
    }

    ref.halfedge_id() = halfedge_id;

    if (halfedge_id != nullid)
    {
        m_trace_point_refs[halfedge_id].push_back(ref);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::align_segment_faces(TracePoint& trace_point0, TracePoint& trace_point1, const bool side_a, id_type& face_id0, id_type& halfedge_id0, id_type& face_id1, id_type& halfedge_id1)
{
    if (face_id0 == face_id1 || halfedge_id0 == nullid || halfedge_id1 == nullid)
    {
        return;
    }

    id_type halfedge_id0_aligned = find_halfedge(face_id1, trace_point0.position);
    id_type halfedge_id1_aligned = find_halfedge(face_id0, trace_point1.position);

    // Exactly one endpoint stands on both fragments, or the segment does not lie on either of them
    assert(halfedge_id0_aligned != nullid || halfedge_id1_aligned != nullid);
    assert(halfedge_id0_aligned == nullid || halfedge_id1_aligned == nullid);

    if (halfedge_id0_aligned != nullid)
    {
        set_trace_point_halfedge({&trace_point0, side_a}, halfedge_id0_aligned);
        halfedge_id0 = halfedge_id0_aligned;
        face_id0 = face_id1;
    }
    else
    {
        set_trace_point_halfedge({&trace_point1, side_a}, halfedge_id1_aligned);
        halfedge_id1 = halfedge_id1_aligned;
        face_id1 = face_id0;
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
void quetzal::brep::IntersectionSplitter<M>::register_face_split(const id_type face_id_kept, const id_type face_id_new)
{
    id_type face_id_original = m_face_origin.contains(face_id_kept) ? m_face_origin.at(face_id_kept) : face_id_kept;
    m_face_origin[face_id_new] = face_id_original;
    m_face_descendants[face_id_original].push_back(face_id_new);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::resolve_face(const id_type face_id_original, const point_type& position)
{
    if (!m_face_descendants.contains(face_id_original))
    {
        return face_id_original; // Never split by any earlier trace instantiated this run
    }

    if (to_polygon(m_mesh.face(face_id_original)).contains(position))
    {
        return face_id_original;
    }

    for (id_type face_id_candidate : m_face_descendants.at(face_id_original))
    {
        if (to_polygon(m_mesh.face(face_id_candidate)).contains(position))
        {
            return face_id_candidate;
        }
    }

    assert(false); // The manifold-partition invariant guarantees exactly one match
    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
std::array<quetzal::id_type, 2> quetzal::brep::IntersectionSplitter<M>::resolve_point(const TracePoint& trace_point, const bool side_a)
{
    id_type halfedge_id = side_a ? trace_point.halfedge_id_a : trace_point.halfedge_id_b;

    // A point on the boundary keeps a current halfedge, so its face follows from that halfedge exactly.
    // Deriving the face from the position instead cannot distinguish the fragments of a split face at a
    // position on their shared boundary, where both contain it
    if (halfedge_id != nullid)
    {
        return {m_mesh.halfedge(halfedge_id).face_id(), halfedge_id};
    }

    // Floating: no halfedge of its own, but the position is strictly within the face, so which fragment of
    // a split contains it is unambiguous
    id_type face_id = resolve_face(side_a ? trace_point.face_id_a : trace_point.face_id_b, trace_point.position);

    // An earlier segment may already have materialized a real halfedge at this position
    const auto& instantiated_halfedges = side_a ? m_instantiated_halfedge_a : m_instantiated_halfedge_b;
    for (const auto& [position, id] : instantiated_halfedges)
    {
        if (position == trace_point.position && m_mesh.halfedge(id).face_id() == face_id)
        {
            return {face_id, id};
        }
    }

    return {face_id, nullid};
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

std::cout << std::format("align_segment_positions: {} segments, {} distinct positions", segments.size(), position_groups.size()) << std::endl;

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

    // Validate each border path through each of the connection traces
    validate_connection_traces(m_mesh, connections);
    return;
}

#endif // QUETZAL_BREP_INTERSECTION_SPLITTER_HPP
