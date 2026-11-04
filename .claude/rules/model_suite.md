---
paths:
  - "model/**"
  - "library/quetzal/{brep,model,geometry,triangulation,wavefront_obj,svg,xml,pdf}/**"
  - "utility/**"
---

# Model suite

These rules apply to the procedurally generated 3D models, the boundary-representation libraries they are built with,
and `model/mesh_operations`, the test of the CSG feature. `doc/intersection_splitter.md` describes `IntersectionSplitter`.

## Working method

- **Derive decisions from the structures.** The half-edge structures have unambiguous linkage (`next`, `prev`, `partner`,
  face and hole ids, the virtual containers) and direction (trace direction, halfedge direction, exterior and interior),
  and what a decision point needs is almost always derivable from them. Read the actual values, the ids, faces,
  positions, directions, and loop contents, and derive the rule from what they show. A tie-break between two candidate
  faces or halfedges is decided by a geometric or topological quantity. A fix that works only for the model in front of
  you is evidence that the model of the problem is wrong, not an edge case to special-case.
- **A finding mentions the model and the test** every time: "In box_box_vertex_face's identity_ab ...".

## Design

- **`IntersectionSplitter` finds every face-pair intersection segment first and then links them into traces.** Walking
  one trace from a seed pair, computing each segment as it is reached, was implemented and abandoned because its special
  cases accumulated. The walk remains legitimate only as an optimization once trace topology is trustworthy, never as the
  way to fix correctness. Linking uses mesh connectivity: the successor of segment `(a, b)` exiting halfedge `h` is the
  pair `(a, partner(h).face_id())`, from the `halfedge_id_a` and `halfedge_id_b` of each `TracePoint`, not a match of
  endpoint positions.
- **Coplanar intersections are handled in the main splitter,** in line, as branching traces, not as a preprocessing
  pass. The open piece is the stitching rule.
- **`library/quetzal/spatial_search` is a testbed,** with no API, and is scheduled after the `IntersectionSplitter`
  redesign. Until then `find_intersections` is a plain O(n^2) face-pair sweep with bounds rejection.
- **`CoplanarIntersectionSplitter` never worked** and is not a reference.
- **`quetzal::FileValidator` and `quetzal::model::mesh_diff` stay separate.** FileValidator is the byte comparison that
  decides pass or fail, and mesh_diff is run on a failing pair to explain how it differs. Joining them would make
  `common` depend on `model`, which already depends on `common`, and FileValidator is file-type agnostic, with pdf, stl,
  mtl, and gcode output beside obj. Their strictness differs by design: reordered faces are different to `file_equal`
  and equivalent to mesh_diff. An integration, if one is ever built, is an injected `std::function<bool(path, path)>`
  comparator, supplied by `utility/verify_model_output`.

## Terms

- **`spur`** is a dangling two-halfedge edge from a loop vertex to an interior point, built by `create_edge_partial`.
- **`gap`** is an established term.
- **`section`** is provisional: an isolated piece of a fragment that a border bounds, meeting the others only at a
  vertex.

## Running

- **A failed `assert` in a Debug model executable reaches stderr as UTF-16,** so tools that assume UTF-8 show nothing or
  report "Binary file matches". Capture with `1> file 2>&1` and read the file with `Get-Content`, where the text comes
  back spaced out but readable. A capture that is genuinely empty is not an assert: exit code 3 with no assert text is
  an abort from something else, such as an out-of-range `std::vector` `operator[]`, and is treated as a hard crash.
