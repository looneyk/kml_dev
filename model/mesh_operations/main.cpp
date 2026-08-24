//------------------------------------------------------------------------------
// mesh_operations
// main.cpp
//------------------------------------------------------------------------------

#define REGRESSION_TESTS
#define DEBUG_VALIDATE_DUMP
#define DEBUG_VISUALIZE

#if defined(REGRESSION_TESTS)
#undef DEBUG_VALIDATE_DUMP
#undef DEBUG_VISUALIZE
#endif

#if defined(DEBUG_VALIDATE_DUMP)
constexpr bool debug_validate_dump = true;
#else
constexpr bool debug_validate_dump = false;
#endif

#if defined(DEBUG_VISUALIZE)
constexpr bool debug_visualize = true;
#else
constexpr bool debug_visualize = false;
#endif

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/Submesh.hpp"
#include "quetzal/brep/mesh_operations.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/common/file_util.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/primitives.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <cassert>

#include "quetzal/brep/validation.hpp"
#include "quetzal/brep/intersect.hpp" // just here for testing at the moment ...
#include "quetzal/geometry/geometry_util.hpp" // just here for testing at the moment ...

using namespace std;
using namespace quetzal;

namespace g = quetzal::geometry;

// triangulation occurs after operation, but run tests with both pre-triangulated and post-triangulated models ...

// point vertex, interior edge point intersects vertex
// point edge, interior edge point intersects interior other edge
// point face, interior edge point intersects interior other face (BOX_BOX_POINT_FACE, BOX_CYLINDER (entirely))
// vertex vertex, vertex intersects vertex
// vertex edge, vertex intersects interior of edge
// vertex face, vertex intersects interior of face
// edge edge, edges colinear and overlapping
// edge in edge, edge is completely within edge
// edge face, edge in face plane and interior intersects perimeter
// edge in face, edge is completely within face (BOX_BOX_EDGE_FACE)
// face face, face in face plane and interior intersects perimeter (later)
// face in face, face is completely within face (later)
// multiple intersection traces (BOX_CYLINDER_2)

// these should match the test cases above ...
#if defined(REGRESSION_TESTS)
#define BOX
// Edge interior points intersect at face interior points
#define BOX_BOX_POINT_FACE
// Face intersections include vertices
#define BOX_BOX_VERTEX_FACE
// Vertices intersect edges
#define BOX_BOX_VERTEX_EDGE
// Intersection at a vertex of each
#define BOX_BOX_VERTEX_VERTEX_1
// Double intersection at a vertex of each
#define BOX_BOX_VERTEX_VERTEX_2
// Triple intersection at a vertex of each
#define BOX_BOX_VERTEX_VERTEX_3
// Triple intersection at two opposite vertices of each (and edge-edge intersections)
//#define BOX_BOX_VERTEX_VERTEX_4
// Edges entirely within a face
#define BOX_BOX_EDGE_FACE
// Edges intersect at interior points
#define BOX_BOX_EDGE_EDGE_1
// Edges overlap
#define BOX_BOX_EDGE_EDGE_2
// Entire points intersection within a face
#define BOX_CYLINDER_1
// Entire points intersection within a face, and tests multiple intersection traces in the same original faces
#define BOX_CYLINDER_2
// Entire points intersection within a face
#define BOX_TORUS_1
// Entire points intersection within a face, and tests multiple intersection traces in the same original faces
#define BOX_TORUS_2
// Both entire points intersection within a face and regular intersection, and tests multiple intersection traces in the same original faces
#define BOX_TORUS_3
// Regular intersection
#define BOX_TORUS_4
#define DISJOINT
#define INCLUSION
#else
// FULL indicates that the intersection polygons are completely within a face, single path traces, no need the new trace scheme ...
// EXT indicates a single face only intersection, no interior intersection, 2A - 2 adjacent edges, 2B - 2 non-adjacent edges
// IN indicates that the entire intersection is in the face(s) and interior of one of the submeshes (excepting IN_EXT_0)
// IN_EXT_0 0 edges, 0 edges, 1 face flush, opposite face extends out the other side of the submesh, 2 traces, no branches
#define BOX_BOX_FACE_FACE_FULL_EXT_0
#define BOX_BOX_FACE_FACE_FULL_EXT_1
#define BOX_BOX_FACE_FACE_FULL_EXT_2A
#define BOX_BOX_FACE_FACE_FULL_EXT_2B
#define BOX_BOX_FACE_FACE_FULL_EXT_3
#define BOX_BOX_FACE_FACE_FULL_EXT_ALL
// Interior intersection, faces have 0 shared edges
#define BOX_BOX_FACE_FACE_FULL_IN_0
// Interior intersection, faces have 1 shared edge
#define BOX_BOX_FACE_FACE_FULL_IN_1
// Interior intersection, faces have 2 shared non-adjacent edges (2 adjacent edges is corner)
#define BOX_BOX_FACE_FACE_FULL_IN_2
// Interior intersection, includes a single vertex
#define BOX_BOX_FACE_FACE_FULL_IN_CORNER
// Interior intersection, includes two adjacent vertices (two non-adjacent is half or full)
#define BOX_BOX_FACE_FACE_FULL_IN_EDGE
// Interior intersection, no shared edges, but bounded by polygons within two non-adjacent faces
#define BOX_BOX_FACE_FACE_FULL_IN_COLUMN
// Interior intersection, sandwiched between two non-adjacent faces
#define BOX_BOX_FACE_FACE_FULL_IN_SLICE
// Interior intersection, at least one full face intersection and at least one non-adjacent face that does not intersect
#define BOX_BOX_FACE_FACE_FULL_IN_HALF
// Interior intersection of two identical submeshes
#define BOX_BOX_FACE_FACE_FULL_IN_FULL
// Interior intersection, 0 shared edges, one face intersection, non-adcent face extends outside the submesh
#define BOX_BOX_FACE_FACE_FULL_IN_EXT_0
// partial face face intersections that require new trace scheme ...
//#define BOX_BOX_FACE_FACE_1
//#define BOX_BOX_FACE_FACE_2
#endif

// add tests for single point intersections and that only splits are being done ...

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using point_type = mesh_type::point_type;
    using vector_type = point_type::vector_type;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "mesh_operations";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    template<typename V>
    void test_ccw_order(const V& ref, const V& a, const V& b, const V& normal)
    {
        assert(quetzal::geometry::ccw_order(ref, a, b, normal));
        assert(quetzal::geometry::ccw_order(ref, b, a, -normal));
        assert(quetzal::geometry::ccw_order(-ref, b, a, normal));
        assert(quetzal::geometry::ccw_order(-ref, -a, -b, normal));
        assert(quetzal::geometry::ccw_order(ref, a, -b, -normal));
        assert(quetzal::geometry::ccw_order(ref, -a, b, -normal));
        assert(quetzal::geometry::ccw_order(ref, b, -a, normal));
        assert(quetzal::geometry::ccw_order(ref, -b, a, normal));
        return;
    }

    //--------------------------------------------------------------------------
    void test_ccw_order()
    {
        vector_type a(1.0f, 0.0f, 0.0f);
        vector_type b(0.0f, 1.0f, 0.0f);
        vector_type ref(-1.0f, -1.0f, 0.0f);
        vector_type normal(0.0f, 0.0f, 1.0f);
        test_ccw_order(ref, a, b, normal);

        return;
    }

    using create_model_type = function<vector<id_type>(mesh_type&)>;
    using operation_type = function<vector<id_type>(mesh_type&, const string&, span<id_type>)>;

    //--------------------------------------------------------------------------
    vector<id_type> no_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        mesh;
        name;
        vector<id_type> ids;
        switch (model_ids.size())
        {
            case 1:
                ids = {model_ids[0]};
                break;

            case 2:
                ids = {model_ids[0], model_ids[1]};
                break;

            default:
                assert(false);
        }
        return ids;
    };

    //--------------------------------------------------------------------------
    vector<id_type> identity_ab_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        name;
        assert(model_ids.size() == 2);
        return brep::mesh_identity(mesh, model_ids[0], model_ids[1]);
    };

    //--------------------------------------------------------------------------
    vector<id_type> identity_ba_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        name;
        assert(model_ids.size() == 2);
        return brep::mesh_identity(mesh, model_ids[1], model_ids[0]);
    };

    //--------------------------------------------------------------------------
    vector<id_type> union_ab_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        assert(model_ids.size() == 2);
        return brep::mesh_union(mesh, model_ids[0], model_ids[1], name);
    };

    //--------------------------------------------------------------------------
    vector<id_type> union_ba_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        assert(model_ids.size() == 2);
        return brep::mesh_union(mesh, model_ids[1], model_ids[0], name);
    };

    //--------------------------------------------------------------------------
    vector<id_type> intersection_ab_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        assert(model_ids.size() == 2);
        return brep::mesh_intersection(mesh, model_ids[0], model_ids[1], name);
    };

    //--------------------------------------------------------------------------
    vector<id_type> intersection_ba_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        assert(model_ids.size() == 2);
        return brep::mesh_intersection(mesh, model_ids[1], model_ids[0], name);
    };

    //--------------------------------------------------------------------------
    vector<id_type> difference_ab_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        name;
        assert(model_ids.size() == 2);
        return brep::mesh_difference(mesh, model_ids[0], model_ids[1], name);
    };

    //--------------------------------------------------------------------------
    vector<id_type> difference_ba_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        name;
        assert(model_ids.size() == 2);
        return brep::mesh_difference(mesh, model_ids[1], model_ids[0], name);
    };

    //--------------------------------------------------------------------------
    vector<id_type> symmetric_difference_ab_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        name;
        assert(model_ids.size() == 2);
        return brep::mesh_symmetric_difference(mesh, model_ids[0], model_ids[1], name);
    };

    //--------------------------------------------------------------------------
    vector<id_type> symmetric_difference_ba_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        name;
        assert(model_ids.size() == 2);
        return brep::mesh_symmetric_difference(mesh, model_ids[1], model_ids[0], name);
    };

    // pass this in, these two should take parameters, then a lambda with the parameter value captured would be used in test_operations ...
    auto axis_xy = normalize(vector_type(1.0f, -1.0f, 0.0f));
//    point_type clip_point{0.0f, 0.0f, 0.0f}; // get this working ...
    point_type clip_point{0.1f, -0.1f, 0.0f};
    g::HalfSpace<vector_traits> clip_plane{{clip_point, axis_xy}, axis_xy};

    //--------------------------------------------------------------------------
    vector<id_type> clip_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        name;
        model_ids;
        brep::clip(mesh, clip_plane, "slice");
//        brep::clip(mesh.submesh(name), clip_plane, "slice");
        brep::triangulate(mesh);
        return {}; // the result is ultimately ignored ...
    };

    vector_type separation(20.0f, 0.0f, 0.0f);

    //--------------------------------------------------------------------------
    vector<id_type> separate_operation(mesh_type& mesh, const string& name, span<id_type> model_ids)
    {
        name;
        model_ids;
        assert(mesh.submesh_count() == 2);
        model::transform_positions(mesh.submesh(1), g::translation(separation));
        return {}; // the result is ultimately ignored ...
    };

    //--------------------------------------------------------------------------
    // Run full test
    vector<id_type> test_model_operation(mesh_type& mesh, const string& file_name, const string& model_name, create_model_type create_model, operation_type operation)
    {
        cout << file_name << endl;
        auto model_ids = create_model(mesh);
        model_ids = operation(mesh, model_name, model_ids);
        brep::triangulate(mesh);
        model::write_obj(mesh, to_filename(file_name));
        return model_ids;
    }

    //--------------------------------------------------------------------------
    // Run test that reuses the previous meshes
    vector<id_type> test_model_operation(mesh_type& mesh, const string& file_name, operation_type operation)
    {
        cout << file_name << endl;
        auto m = mesh;
        operation(m, file_name, {});
        brep::triangulate(m);
        model::write_obj(m, to_filename(file_name));
        return {};
    }

    struct TestOperation
    {
        string file_name;
        string model_name; // name applied to resulting mesh
        operation_type operation;
        bool create_model_flag = true; // would prefer something other than a flag for reusing mesh ...
    };

    using test_operations_type = vector<TestOperation>;

    //--------------------------------------------------------------------------
    void test_model_operations(mesh_type& mesh, const string& geometry_name, create_model_type create_model, test_operations_type test_operations)
    {
        for (const auto& test_operation : test_operations)
        {
            auto& [file_name, model_name, operation, create_model_flag] = test_operation;
            assert(create_model_flag || model_name.empty());
            auto full_file_name = format("{}_{}", geometry_name, file_name);
            if (create_model_flag)
            {
                auto full_model_name = model_name.empty() ? "" : format("{}_{}", geometry_name, model_name);
                test_model_operation(mesh, full_file_name, full_model_name, create_model, operation);
            }
            else
            {
                test_model_operation(mesh, full_file_name, operation);
            }
        }
        return;
    }

    //--------------------------------------------------------------------------
    void face_face_test(mesh_type& mesh, const string& geometry_name, const vector_type& box_a_size, const vector_type& box_a_offset, const vector_type& box_b_size, const vector_type& box_b_offset, test_operations_type test_operations)
    {
        auto create_model = [&](mesh_type& mesh) -> vector<id_type>
        {
            string name_a = "box_a";
            string name_b = "box_b";
            mesh.clear();
            id_type submesh_id_a = model::create_box(mesh, name_a, box_a_size);
            model::transform_positions(mesh.submesh(name_a), g::translation(box_a_offset));
            id_type submesh_id_b = model::create_box(mesh, name_b, box_b_size);
            model::transform_positions(mesh.submesh(name_b), g::translation(box_b_offset));
            return {submesh_id_a, submesh_id_b};
        };

        test_model_operations(mesh, geometry_name, create_model, test_operations);
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    set_output_file_lf_endings();

    FileValidator validator(dir_out, dir_ref);

    mesh_type mesh;

    test_ccw_order();
//    return 0; // ...

    // Comprehensive set of test operations for use in most test cases
    test_operations_type test_operations =
    {
        {"noop", "noop", no_operation},
        {"identity_ab", "identity_ab", identity_ab_operation},
        {"identity_ab_separated", "", separate_operation, false},
        {"identity_ba", "identity_ba", identity_ba_operation},
        {"union_ab", "union_ab", union_ab_operation},
        {"union_ab_clipped", "", clip_operation, false},
        {"union_ba", "union_ba", union_ba_operation},
        {"union_ba_clipped", "", clip_operation, false},
        {"intersection_ab", "intersection_ab", intersection_ab_operation},
        {"intersection_ba", "intersection_ba", intersection_ba_operation},
        {"difference_ab", "", difference_ab_operation},
        {"difference_ba", "", difference_ba_operation},
        {"symmetric_difference_ab", "", symmetric_difference_ab_operation},
        {"symmetric_difference_ab_clipped", "", clip_operation, false},
        {"symmetric_difference_ab_separated", "", separate_operation, false},
        {"symmetric_difference_ba", "", symmetric_difference_ba_operation},
        {"symmetric_difference_ba_clipped", "", clip_operation, false},
        {"symmetric_difference_ba_separated", "", separate_operation, false},
     };

#if defined(BOX)
    auto box_model = [](mesh_type& mesh) -> vector<id_type>
    {
        string name = "box";
        mesh.clear();
        id_type submesh_id = model::create_box(mesh, name, 20.0f, 20.0f, 20.0f);
        return {submesh_id};
    };

    test_model_operation(mesh, "box", "box", box_model, no_operation);
#endif // BOX

auto zzz = brep::intersection_coplanar(mesh.face(0), mesh.face(1)); // test ...

#if defined(BOX_BOX_POINT_FACE)
    auto box_box_point_face_model = [](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "box_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_box(mesh, name_b, 20.0f, 20.0f, 20.0f);
        model::transform_positions(mesh.submesh(name_b), g::translation(10.0f, 10.0f, 10.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_point_face", box_box_point_face_model, test_operations);
#endif // BOX_BOX_POINT_FACE

#if defined(BOX_BOX_VERTEX_FACE)
    auto box_box_vertex_face_model = [&](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "box_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_box(mesh, name_b, 10.0f, 10.0f, 10.0f);
        auto from = normalize(vector_type(1.0f, -1.0f, -1.0f));
        auto to = vector_type(1.0f, 0.0f, 0.0f);
        auto v = cross(from, to);
        auto axis = normalize(v);
        auto angle = asin(v.norm());
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis_unit(axis, angle) * g::translation(0.0f, 0.0f, 10.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_vertex_face", box_box_vertex_face_model, test_operations);
#endif // BOX_BOX_VERTEX_FACE

#if defined(BOX_BOX_VERTEX_EDGE)
    auto box_box_vertex_edge_model = [&](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "box_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_box(mesh, name_b, 10.0f, 10.0f, 10.0f);
        auto from = normalize(vector_type(1.0f, -1.0f, -1.0f));
        auto to = vector_type(1.0f, 0.0f, 0.0f);
        auto v = cross(from, to);
        auto axis = normalize(v);
        auto angle = asin(v.norm());
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis_unit(axis, angle) * g::translation(0.0f, 10.0f, 10.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_vertex_edge", box_box_vertex_edge_model, test_operations);
#endif // BOX_BOX_VERTEX_EDGE

#if defined(BOX_BOX_VERTEX_VERTEX_1)
    auto box_box_vertex_vertex_1_model = [&](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "box_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_box(mesh, name_b, 10.0f, 10.0f, 10.0f);
        auto from = normalize(vector_type(1.0f, -1.0f, -1.0f));
        auto to = vector_type(1.0f, 0.0f, 0.0f);
        auto v = cross(from, to);
        auto axis = normalize(v);
        auto angle = asin(v.norm());
        model::transform(mesh.submesh(submesh_id_b), g::translation(5.0f, -5.0f, -5.0f) * g::rotation_axis_unit(axis, angle) * g::translation(-10.0f, 10.0f, 10.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_vertex_vertex_1", box_box_vertex_vertex_1_model, test_operations);
#endif // BOX_BOX_VERTEX_VERTEX_1

#if defined(BOX_BOX_VERTEX_VERTEX_2)
    auto box_box_vertex_vertex_2_model = [&](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "pyramid_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_cylinder_edges(mesh, name_b, 4, 1, 13.0f, 0.0f, -13.0f, 0.0f);
        auto from = vector_type::z_direction();
        auto to = normalize(vector_type(1.0f, 1.0f, 1.5f));
        auto v = cross(from, to);
        auto axis = normalize(v);
        auto angle = asin(v.norm());
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis_unit(axis, angle) * g::translation(10.0f, 10.0f, 10.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_vertex_vertex_2", box_box_vertex_vertex_2_model, test_operations);
#endif // BOX_BOX_VERTEX_VERTEX_2

#if defined(BOX_BOX_VERTEX_VERTEX_3)
    auto box_box_vertex_vertex_3_model = [&](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "box_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_box(mesh, name_b, 10.0f, 10.0f, 10.0f);
        auto axis = normalize(vector_type(1.0f, 1.0f, 1.0f));
        auto angle = math::PiThird<value_type>;
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis_unit(axis, angle) * g::translation(5.0f, 5.0f, 5.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_vertex_vertex_3", box_box_vertex_vertex_3_model, test_operations);
#endif // BOX_BOX_VERTEX_VERTEX_3

#if defined(BOX_BOX_VERTEX_VERTEX_4)
    auto box_box_vertex_vertex_4_model = [&](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "box_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_box(mesh, name_b, 20.0f, 20.0f, 20.0f);
        auto axis = normalize(vector_type(1.0f, 1.0f, 1.0f));
        auto angle = math::PiThird<value_type>;
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis_unit(axis, angle));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_vertex_vertex_4", box_box_vertex_vertex_4_model, test_operations);
#endif // BOX_BOX_VERTEX_VERTEX_4

#if defined(BOX_BOX_EDGE_FACE)
    auto box_box_edge_face_model = [&](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "box_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_box(mesh, name_b, 10.0f, 10.0f, 10.0f);
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis(vector_type::y_direction(), -math::PiFourth<value_type>) * g::translation(0.0f, 0.0f, 10.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_edge_face", box_box_edge_face_model, test_operations);
#endif // BOX_BOX_EDGE_FACE

#if defined(BOX_BOX_EDGE_EDGE_1)
    auto box_box_edge_edge_1_model = [&](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "box_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_box(mesh, name_b, 10.0f, 10.0f, 10.0f);
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis(vector_type::x_direction(), math::PiFourth<value_type>) * g::translation(10.0f, 5.0f * (1.0f - sqrt(2.0f)), 10.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_edge_edge_1", box_box_edge_edge_1_model, test_operations);
#endif // BOX_BOX_EDGE_EDGE_1

#if defined(BOX_BOX_EDGE_EDGE_2)
    auto box_box_edge_edge_2_model = [&](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "box_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_box(mesh, name_b, 10.0f, 10.0f, 10.0f);
// fix so that edges overlap in line ...
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis(vector_type::x_direction(), math::PiFourth<value_type>) * g::translation(10.0f, 5.0f + 5.0f * (1.0f - sqrt(2.0f)), 10.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_edge_edge_2", box_box_edge_edge_2_model, test_operations);
#endif // BOX_BOX_EDGE_EDGE_2

#if defined(BOX_BOX_FACE_FACE_FULL_EXT_0)
    face_face_test(mesh, "box_box_face_face_full_ext_0", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 10.0f, 10.0f}, {0.0f, 0.0f, 15.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_EXT_0

#if defined(BOX_BOX_FACE_FACE_FULL_EXT_1)
    face_face_test(mesh, "box_box_face_face_full_ext_1", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 10.0f, 10.0f}, {5.0f, 0.0f, 15.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_EXT_1

#if defined(BOX_BOX_FACE_FACE_FULL_EXT_2A)
    face_face_test(mesh, "box_box_face_face_full_ext_2a", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 10.0f, 10.0f}, {5.0f, 5.0f, 15.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_EXT_2A

#if defined(BOX_BOX_FACE_FACE_FULL_EXT_2B)
    face_face_test(mesh, "box_box_face_face_full_ext_2b", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 20.0f, 10.0f}, {0.0f, 0.0f, 15.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_EXT_2B

#if defined(BOX_BOX_FACE_FACE_FULL_EXT_3)
    face_face_test(mesh, "box_box_face_face_full_ext_3", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 20.0f, 10.0f}, {5.0f, 0.0f, 15.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_EXT_3

#if defined(BOX_BOX_FACE_FACE_FULL_EXT_ALL)
    face_face_test(mesh, "box_box_face_face_full_ext_all", {20.0f, 20.0f, 20.0f}, {}, {20.0f, 20.0f, 10.0f}, {0.0f, 0.0f, 15.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_EXT_ALL

#if defined(BOX_BOX_FACE_FACE_FULL_IN_0)
    face_face_test(mesh, "box_box_face_face_full_in_0", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 10.0f, 10.0f}, {0.0f, 0.0f, 5.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_IN_0

#if defined(BOX_BOX_FACE_FACE_FULL_IN_1)
    face_face_test(mesh, "box_box_face_face_full_in_1", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 10.0f, 10.0f}, {5.0f, 0.0f, 5.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_IN_1

#if defined(BOX_BOX_FACE_FACE_FULL_IN_2)
    face_face_test(mesh, "box_box_face_face_full_in_2", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 10.0f, 10.0f}, {5.0f, 5.0f, 5.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_IN_2

#if defined(BOX_BOX_FACE_FACE_FULL_IN_CORNER)
    face_face_test(mesh, "box_box_face_face_full_in_corner", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 10.0f, 10.0f}, {5.0f, 5.0f, 5.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_IN_CORNER

#if defined(BOX_BOX_FACE_FACE_FULL_IN_EDGE)
    face_face_test(mesh, "box_box_face_face_full_in_edge", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 10.0f, 20.0f}, {5.0f, 5.0f, 0.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_IN_EDGE

#if defined(BOX_BOX_FACE_FACE_FULL_IN_COLUMN)
    face_face_test(mesh, "box_box_face_face_full_in_column", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 10.0f, 20.0f}, {0.0f, 0.0f, 0.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_IN_COLUMN

#if defined(BOX_BOX_FACE_FACE_FULL_IN_SLICE)
    face_face_test(mesh, "box_box_face_face_full_in_slice", {20.0f, 20.0f, 20.0f}, {}, {10.0f, 20.0f, 10.0f}, {0.0f, 0.0f, 5.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_IN_SLICE

#if defined(BOX_BOX_FACE_FACE_FULL_IN_HALF)
    face_face_test(mesh, "box_box_face_face_full_in_half", {20.0f, 20.0f, 20.0f}, {}, {20.0f, 20.0f, 10.0f}, {0.0f, 0.0f, 5.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_IN_HALF

#if defined(BOX_BOX_FACE_FACE_FULL_IN_FULL)
    face_face_test(mesh, "box_box_face_face_full_in_full", {20.0f, 20.0f, 20.0f}, {}, {20.0f, 20.0f, 20.0f}, {0.0f, 0.0f, 0.0f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_IN_FULL

#if defined(BOX_BOX_FACE_FACE_FULL_IN_EXT_0)
    face_face_test(mesh, "box_box_face_face_full_in_ext_0", {20.0f, 20.0f, 20.0f}, {}, {20.0f, 20.0f, 25.0f}, {0.0f, 0.0f, -2.5f}, test_operations);
#endif // BOX_BOX_FACE_FACE_FULL_IN_EXT_0

#if defined(BOX_BOX_FACE_FACE_1)
    auto box_box_face_face_1_model = [&](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "box_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_box(mesh, name_b, 10.0f, 10.0f, 10.0f);
        model::transform_positions(mesh.submesh(name_b), g::translation(0.0f, 0.0f, 5.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_face_face_1", box_box_face_face_1_model, test_operations);
#endif // BOX_BOX_FACE_FACE_1

#if defined(BOX_BOX_FACE_FACE_2)
    auto box_box_face_face_2_model = [&](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "box_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_box(mesh, name_b, 20.0f, 20.0f, 10.0f);
        model::transform_positions(mesh.submesh(name_b), g::translation(10.0f, 10.0f, 5.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_box_face_face_2", box_box_face_face_2_model, test_operations);
#endif // BOX_BOX_FACE_FACE_2

#if defined(BOX_CYLINDER_1)
    auto box_cylinder1_model = [](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "cylinder_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_cylinder(mesh, name_b, 6, 1, 5.0f, 5.0f, 0.0f, 20.0f);
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_cylinder1", box_cylinder1_model, test_operations);
#endif // BOX_CYLINDER_1

#if defined(BOX_CYLINDER_2)
    auto box_cylinder2_model = [](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "cylinder_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_cylinder(mesh, name_b, 6, 1, 5.0f, 5.0f, -15.0f, 20.0f);
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_cylinder2", box_cylinder2_model, test_operations);
#endif // BOX_CYLINDER_2

#if defined(BOX_TORUS_1)
    auto box_torus1_model = [](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "torus_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 5.0f);
        model::transform(mesh.submesh(submesh_id_a), g::translation(10.0f, 0.0f, 0.0f));
        id_type submesh_id_b = model::create_torus(mesh, name_b, 24, 8, 10.0f, 2.0f);
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis(vector_type::x_direction(), -math::PiHalf<value_type>));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_torus1", box_torus1_model, test_operations);
#endif // BOX_TORUS_1

#if defined(BOX_TORUS_2)
    auto box_torus2_model = [](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "torus_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 30.0f, 30.0f, 5.0f);
        id_type submesh_id_b = model::create_torus(mesh, name_b, 24, 8, 10.0f, 2.0f);
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis(vector_type::x_direction(), -math::PiHalf<value_type>));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_torus2", box_torus2_model, test_operations);
#endif // BOX_TORUS_2

#if defined(BOX_TORUS_3)
    auto box_torus3_model = [](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "torus_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 32.0f, 32.0f, 7.8f); // 8
        model::transform(mesh.submesh(submesh_id_a), g::translation(5.1f, 0.0f, 0.0f)); // 5
        id_type submesh_id_b = model::create_torus(mesh, name_b, 24, 8, 10.0f, 2.0f);
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis(vector_type::x_direction(), -math::PiHalf<value_type>));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_torus3", box_torus3_model, test_operations);
#endif // BOX_TORUS_3

#if defined(BOX_TORUS_4)
    auto box_torus4_model = [](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "box_a";
        string name_b = "torus_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 15.0f, 15.0f, 3.0f);
        model::transform(mesh.submesh(submesh_id_a), g::translation(-2.75f, 0.0f, 0.0f));
        id_type submesh_id_b = model::create_torus(mesh, name_b, 6, 4, 10.0f, 2.0f);
        model::transform(mesh.submesh(submesh_id_b), g::rotation_axis(vector_type::x_direction(), -math::PiHalf<value_type>));
        return {submesh_id_a, submesh_id_b};
    };

    test_model_operations(mesh, "box_torus4", box_torus4_model, test_operations);
#endif // BOX_TORUS_4

#if defined(DISJOINT)
    auto cube_octahedron_disjoint_model = [](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "cube_a";
        string name_b = "octahedron_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_octahedron(mesh, name_b, 9.0f);
        model::transform_positions(mesh.submesh(name_b), g::translation(20.0f, 0.0f, 0.0f));
        return {submesh_id_a, submesh_id_b};
    };

    test_operations_type disjoint_test_operations =
    {
        {"none", "none", no_operation},
        {"identity_ab", "identity_ab", identity_ab_operation},
        {"identity_ba", "identity_ba", identity_ba_operation},
        {"union_ab", "union_ab", union_ab_operation},
        {"union_ba", "union_ba", union_ba_operation},
        {"intersection_ab", "intersection_ab", intersection_ab_operation},
        {"intersection_ba", "intersection_ba", intersection_ba_operation},
        {"difference_ab", "", difference_ab_operation},
        {"difference_ba", "", difference_ba_operation},
        {"symmetric_difference_ab", "", symmetric_difference_ab_operation},
        {"symmetric_difference_ba", "", symmetric_difference_ba_operation},
    };

    test_model_operations(mesh, "cube_octahedron_disjoint", cube_octahedron_disjoint_model, disjoint_test_operations);
#endif // DISJOINT

#if defined(INCLUSION)
    auto cube_octahedron_inclusion_model = [](mesh_type& mesh) -> vector<id_type>
    {
        string name_a = "cube_a";
        string name_b = "octahedron_b";
        mesh.clear();
        id_type submesh_id_a = model::create_box(mesh, name_a, 20.0f, 20.0f, 20.0f);
        id_type submesh_id_b = model::create_octahedron(mesh, name_b, 9.0f);
        return {submesh_id_a, submesh_id_b};
    };

    test_operations_type inclusion_test_operations =
    {
        {"none", "none", no_operation},
        {"identity_ab", "identity_ab", identity_ab_operation},
        {"identity_ba", "identity_ba", identity_ba_operation},
        {"union_ab", "union_ab", union_ab_operation},
        {"union_ba", "union_ba", union_ba_operation},
        {"intersection_ab", "intersection_ab", intersection_ab_operation},
        {"intersection_ba", "intersection_ba", intersection_ba_operation},
        {"difference_ab", "", difference_ab_operation},
        {"difference_ab_clipped", "", clip_operation, false},
        {"difference_ba", "", difference_ba_operation},
        {"difference_ba_clipped", "", clip_operation, false},
        {"symmetric_difference_ab", "", symmetric_difference_ab_operation},
        {"symmetric_difference_ab_clipped", "", clip_operation, false},
        {"symmetric_difference_ba", "", symmetric_difference_ba_operation},
        {"symmetric_difference_ba_clipped", "", clip_operation, false},
    };

    test_model_operations(mesh, "cube_octahedron_inclusion", cube_octahedron_inclusion_model, inclusion_test_operations);
#endif // INCLUSION

/*
    name = "sphere_icosahedron";
    name_a = "sphere_a";
    name_b = "icosahedron_b";
    cout << name << endl;
    mesh.clear();
    submesh_id_a = model::create_sphere(mesh, name_a, 12, 6, 10.0f);
    submesh_id_b = model::create_icosahedron(mesh, name_b, 7.5f);
    model::transform_positions(mesh.submesh(name_a), g::translation(10.0f, 0.0f, 0.0f));
    model::transform_positions(mesh.submesh(name_b), g::translation(-10.0f, 0.0f, 0.0f));
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);
*/
    validator.report_results();
    return 0;
}
