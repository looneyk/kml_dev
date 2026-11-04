# IntersectionSplitter --- Reference

`quetzal::brep::IntersectionSplitter` (`library/quetzal/brep/IntersectionSplitter.hpp`) finds where two submeshes of a
mesh intersect and splits both along the intersection, producing the border loops that the CSG operations in
`brep/mesh_operations.hpp` connect. `model/mesh_operations` is its test.

## The mesh

`quetzal::brep::Mesh<Traits>` is a half-edge boundary representation with separate stores for halfedges, vertices,
faces, holes, surfaces, and submeshes. Every element has a unique `quetzal::id_type`, with `nullid` as the null value.
Ids are stable for the life of the mesh: elements are marked deleted rather than removed, and `pack()` renumbers them
through a `Mappings` remap.

- **A halfedge** has `next_id`, `prev_id`, `partner_id`, `vertex_id`, `face_id`, and `hole_id`. It starts at its own
  vertex and ends at the vertex of `next`, a half-open interval. `border()` is true when `partner_id` is `nullid`.
  `hole_id` is `nullid` on a face's perimeter and the hole's id on a hole loop.
- **A vertex** contains the attributes of a mesh vertex: position, normal, and texcoord. Vertices and halfedges
  correspond one to one, so a "single vertex" means every halfedge vertex at one position.
- **A face** is a perimeter loop reached from `m_halfedge_id` through `next`, plus `m_hole_ids`. `halfedges()` walks the
  perimeter, `halfedges_ext()` the perimeter and then every hole, and `holes()` the holes. `halfedge_ids()` caches the
  perimeter's halfedges in order, and `invalidate()` discards that cache and the cached bounds, so it is called whenever
  a loop is relinked. Faces are wound counterclockwise about their normal.
- **A hole** is a halfedge loop within a face, with its `face_id` and a `halfedge_id` on its loop.
  `assign_halfedges()` sets the hole's id on every halfedge of the loop, and `release_halfedges()` clears them.
  `Face::create_hole`, `remove_hole`, and `insert_hole` attach and detach holes; `remove_hole` releases the loop while it
  is still intact, since it walks the loop to clear the ids.
- **`Elements<M, T, I>`** is the virtual container behind `halfedges()`, `holes()`, `faces()`, and the others, providing
  iterators over each element type. Each view has a mesh pointer, an id, and static function objects for size, first,
  last, end, forward, reverse, and element access, so a change to a mesh pointer or an owning id updates every view
  through `set_mesh`, `set_id`, and `check_mesh`.
- **A vertex's halfedges** are those that start at it, one for each incident face. `Vertex::halfedges()` iterates them,
  forward by `partner(prev(h))` and in reverse by `next(partner(h))`, each stopping at a border. `mesh_util.hpp` has the
  single steps as `next_vertex_halfedge_id` and `prev_vertex_halfedge_id`.

## Mesh operations

`mesh_util.hpp` has the operations the splitter uses:

- **`split_halfedge` and `split_edge`** insert a vertex partway along an edge and invalidate the affected faces.
- **`connect(mesh, a, b, coupled)`** splices a partnered pair between two halfedges of one face and creates no face. The
  pair becomes `prev(a)` and `prev(b)`: `prev(a)` starts at b's vertex and is in a's face, and `prev(b)` starts at a's
  vertex and is in b's face.
- **`split_face(mesh, a, b, coupled)`** keeps the original face for `a`, assigns the run from `b` up to `a` to a new face,
  calls `connect`, invalidates both faces, and moves each hole to the side that contains it with `partition_holes`. It
  asserts that the two halfedges are on the same face and not adjacent.
- **`Mesh::delete_face`** deletes the perimeter halfedges, then detaches each hole with `remove_hole` before deleting its
  halfedges.

## The pipeline

`connections(submesh_id_a, submesh_id_b)` returns a `connection_traces_type` and runs:

1. `find_intersections`, which tests every face pair whose bounds overlap with `find_face_intersections`. Each pair's
   intersection is computed in isolation, with no continuation and no mutation, and coplanar pairs are not handled. The
   result is a flat list of trace segments.
2. `align_segment_positions`, which moves near-coincident positions onto one position for each location.
3. `split_trace_segments`, which splits edges so that every trace point is at a vertex, with `update_trace_points`
   correcting the halfedge ids after each split.
4. `create_isolated_intersection_point_edges` (`## Isolated intersection points`).
5. `validate_segments`.
6. `order_intersections`, which links the segments into closed traces (`## Ordering`).
7. `validate_intersection_traces`.
8. `instantiate_connection_traces`, which runs `instantiate_trace` for each trace (`## Instantiation`).
9. `validate_connections`.

The CSG entry points then use the border functions in `brep/border.hpp`: `connect_border_fragments`,
`delete_unconnected_border_fragments`, and `set_border_fragment_submesh`.

## Trace structures

- **`TracePoint`** has a `position` and, for each submesh, a face id and a halfedge id: `face_id_a` and `halfedge_id_a`,
  `face_id_b` and `halfedge_id_b`. A `nullid` halfedge id means the point is inside the face rather than on its boundary,
  a floating point.
- **`TraceSegment`** has `p0` and `p1`. A segment is the intersection of exactly one face pair, so `p0.face_id_a ==
  p1.face_id_a`, and likewise for b. A segment's two endpoints are always resolved together, never mixed with a
  neighboring segment's point at the same position, which may differ about the face it exits.
- **`TraceSegments`** is one trace, and **`Traces`** is every trace.
- **`connection_borders_type`** (`brep/border.hpp`) is a `std::array<id_type, 4>` of `{A exterior, A interior, B
  exterior, B interior}` halfedge ids. A `connection_trace_type` has one for each segment, and a
  `connection_traces_type` has one connection trace for each trace.

A halfedge id stays valid, but a halfedge's `face_id` changes when `split_face` reassigns one of its two runs. A trace
point refers to one of the halfedges that start at its position, which is unambiguous when it is set; a later split can
leave several halfedges at that position, in different faces.

## Isolated intersection points

`create_isolated_intersection_point_edges` runs before ordering, while the faces are unsplit, so that texcoords are
interpolated from the original face. For each segment with at least one floating endpoint:

- **Both ends floating:** `create_edge_internal` builds a free-standing loop of two halfedges, each the other's `next`,
  `prev`, and `partner`, and `Face::create_hole` registers it. The segment's trace points refer to the two halfedges.
- **One end floating:** `create_edge_partial` splices a spur, a partnered pair inserted immediately before the anchor
  halfedge: `prev(anchor) -> nh -> nh+1 -> anchor`, where `nh` starts at the anchor's vertex and `nh+1` at a new vertex
  at the floating position. The pair has the anchor's `hole_id`, so it is on the anchor's loop, and the floating end's
  trace point refers to the tip halfedge.

A spur anchored at an endpoint of an existing interior edge is spliced between that edge's two halves, since the loop
arrives on one half and leaves on the other. After spurs at both ends, the halves are still partners but no longer
adjacent.

A new vertex's texcoords come from `face_mapping(face_id)`, which builds a `PolygonMapping` of the face the first time
it is requested and keeps it, so the mapping describes the face as found.

## Ordering

`order_intersections` links the segments into closed traces from segment data alone, after every segment is found.
`index_positions` groups every segment end by position, and `successor_position` chooses the continuation at a
position. Since a segment is one face pair, leaving through an edge of one submesh moves to the adjacent face there
while the face of the other submesh stays the same, so the continuation is the end at that position that shares at
least one face id with the arriving point. Both faces change only where an original vertex meets a vertex.

## Instantiation

`instantiate_trace` walks the ordered segments. For each, `trace_direction` is `normalize(cross(normal_a, normal_b))`;
side a uses `-trace_direction` as its exterior direction and side b `+trace_direction`. Each side is instantiated
separately, `interior_partner` returns the opposite halfedge of each, and the four are added as one
`connection_borders_type`. After the loop, `uncouple` separates the exterior halfedges so that the result is a pair of
border loops.

`instantiate_segment_side(halfedge_id0, halfedge_id1, exterior_direction)` returns the halfedge along one side of one
segment in `exterior_direction`:

- **The segment is already an edge:** `exterior_halfedge` returns that edge's halfedge.
- **Both ends are on the same hole loop:** `close_enclosed_loop` divides the loop; one side becomes a new face, and the
  hole is repointed at the other side, which faces the perimeter.
- **Otherwise:** `create_edge_connect` makes the connection.

`create_edge_connect(halfedge_id0, halfedge_id1, coupled)` dispatches on the two ends' hole ids:

- **Both `nullid`,** both on the perimeter: `split_face` makes the edge and divides the face.
- **A hole involved:** the hole record that the splice invalidates is removed first, `connect` is called, and then the
  surviving hole id is applied around the merged loop, or, where one hole loop was divided, both sides keep the hole id
  for the caller to finish.

It returns `prev(halfedge_id1)`, the halfedge from the vertex of `halfedge_id0` to the vertex of `halfedge_id1`.
`exterior_halfedge(h, direction)` chooses `h` or its partner by comparing its direction with the exterior direction,
and `interior_partner` returns the other one, with asserts on their end positions.

## Validation

- **`validate_segments`** checks that endpoints are distinct, halfedge ids are not null, and each trace point's position
  matches its halfedges' vertex positions.
- **`validate_intersection_traces`** checks each segment's orientation against the trace direction and that consecutive
  endpoints meet.
- **`validate_connections`** walks the connection traces.
- **`brep/validation.hpp`** has whole-mesh checks, among them `check_face`, which checks hole loop closure, hole and face
  agreement, disjoint perimeter and holes, and the `halfedge_ext_count` total.

## The test

`model/mesh_operations/main.cpp` selects its geometry models with `REGRESSION_TESTS`, each enabled by its own `#define`,
for point and face, vertex and face, vertex and edge, vertex and vertex, edge and face, edge and edge, box and
cylinder, box and torus, disjoint, and inclusion intersections. Each model is a lambda building two submeshes, run
through `test_model_operations` with a list of `TestOperation`s: `noop`, identity, union, intersection, difference, and
symmetric difference, each in `_ab` and `_ba` order, with `_clipped` and `_separated` operations that continue from the
previous mesh. Only the CSG operations run the splitter, and `_ab` and `_ba` differ only in submesh order, so a model
has two distinct traces. The output `.obj` files go to `out/` and are compared with `reference/`.

## Terms

- **Trace:** one closed loop of intersection between the two submeshes. A model may have several.
- **Trace segment, or segment:** one element of a trace, the intersection of exactly one face pair.
- **Spur:** the dangling two-halfedge edge that `create_edge_partial` builds from a loop vertex to an interior point,
  with zero area, both halves on the same loop, and one halfedge at its tip vertex.
- **Exterior and interior:** the two sides of a connection, by trace direction, not the inside and outside of a solid.
