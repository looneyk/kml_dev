#if !defined(QUETZAL_BREP_MESH_OPERATIONS_HPP)
#define QUETZAL_BREP_MESH_OPERATIONS_HPP
//------------------------------------------------------------------------------
// brep
// mesh_operations.hpp
//------------------------------------------------------------------------------

#include "IntersectionSplitter.hpp"
#include "Mesh.hpp"
#include "id.hpp"
#include "border.hpp"
#include "geometry.hpp"
#include "inversion.hpp"
#include <functional>
#include <print>
#include <set>
#include <vector>

namespace quetzal::brep
{

    // input: border halfedge ids in order {A exterior, A interior, B exterior, B interior}
    template<typename Traits>
    using connect_function_type = std::function<std::vector<id_type> (Mesh<Traits>&, id_type, id_type, id_type, id_type)>;

    // input: submesh_id_a, submesh_id_b
    template<typename Traits>
    using disjoint_function_type = std::function<std::vector<id_type> (Mesh<Traits>&, id_type, id_type)>;

    // input: submesh_id_a, submesh_id_b
    // submesh A contains submesh B
    template<typename Traits>
    using inclusion_function_type = std::function<std::vector<id_type> (Mesh<Traits>&, id_type, id_type)>;

    // C = A || B
    // The original submeshes will no longer exist after this operation
    template<typename Traits>
    std::vector<id_type> mesh_union(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, const std::string& name);

    // C = A && B
    // The origonal submeshes will no longer exist after this operation
    template<typename Traits>
    std::vector<id_type> mesh_intersection(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, const std::string& name);

    // A = A && !B
    // There is no ambiguity, so submesh names remain the same
    template<typename Traits>
    std::vector<id_type> mesh_difference(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, const std::string& name = {});

    // A = A && !B || B = B && !A
    // There is no ambiguity, so submesh names remain the same
    template<typename Traits>
    std::vector<id_type> mesh_symmetric_difference(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, const std::string& name = {});

    // A = A || B = B
    // There is no ambiguity, so submesh names remain the same
    // Essentially a noop where the generated borders are simply reconnected so they can be examined for debugging
    template<typename Traits>
    std::vector<id_type> mesh_identity(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, const std::string& name = {});

    // Create a new or use an existing submesh with name for the result
    // Surface names in the result will typically consist of some from each original submesh
    // Returns submesh id of the result
    template<typename Traits>
    std::vector<id_type> mesh_operation(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, connect_function_type<Traits> connect, disjoint_function_type<Traits> disjoint, inclusion_function_type<Traits> inclusion, const std::string& name);

    //--------------------------------------------------------------------------
    template<typename M>
    void validate_border_id(const M& mesh, id_type id, size_t n_max = 10)
    {
        assert(mesh.halfedge(id).border());
        assert(next_border_halfedge_id(mesh, prev_border_halfedge_id(mesh, id)) == id);
        assert(prev_border_halfedge_id(mesh, next_border_halfedge_id(mesh, id)) == id);
        assert(vertex_position(mesh.halfedge(id).next()) == vertex_position(mesh.halfedge(next_border_halfedge_id(mesh, id))));

        size_t n = 0;
        std::set<id_type> ids{id};
        auto position0 = vertex_position(mesh.halfedge(id));

        for (id_type halfedge_id = mesh.halfedge(id).prev_id(); !mesh.halfedge(halfedge_id).border(); halfedge_id = mesh.halfedge(halfedge_id).partner().prev_id())
        {
            assert(n++ < n_max);
            assert(!ids.contains(halfedge_id));
            ids.insert(id);

            assert(halfedge_id != id);
            assert(!mesh.halfedge(halfedge_id).border());
            auto position = vertex_position(mesh.halfedge(halfedge_id).partner());
            assert(position == position0);
        }

        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void validate_connection_bidirectional(const M& mesh, const std::vector<id_type>& seq_next, const std::vector<id_type>& seq_prev)
    {
        size_t seq_count = seq_next.size();
        assert(seq_prev.size() == seq_count);

        for (size_t i_next = 0; i_next < seq_count; ++i_next)
        {
            id_type i_prev = (seq_count - i_next) % seq_count;
            assert(seq_next[i_next] == seq_prev[i_prev]);
            assert(vertex_position(mesh.halfedge(seq_next[i_next])) == vertex_position(mesh.halfedge(seq_prev[i_prev])));
        }

        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void validate_connection_positions(const M& mesh, const std::vector<id_type>& seq_ext, const std::vector<id_type>& seq_int, int offset)
    {
        size_t seq_count = seq_ext.size();
        assert(seq_int.size() == seq_count);

        for (size_t i_ext = 0; i_ext < seq_count; ++i_ext)
        {
            id_type i_int = (seq_count + offset - i_ext) % seq_count;
            std::cout << std::format("        ext {} {}  int {} {}  {:20}  {:20}", i_ext, id_string(seq_ext[i_ext]), i_int, id_string(seq_int[i_int]), vertex_position(mesh.halfedge(seq_ext[i_ext])), vertex_position(mesh.halfedge(seq_int[i_int]))) << std::endl;
            assert(vertex_position(mesh.halfedge(seq_ext[i_ext])) == vertex_position(mesh.halfedge(seq_int[i_int])));
//            assert(float_eq(vertex_position(mesh.halfedge(seq_ext[i_ext])), vertex_position(mesh.halfedge(seq_int[i_int]))));
        }

        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void validate_connection(const M& mesh, const typename IntersectionSplitter<M>::connection_type& connection)
    {
        auto [id_a_ext, id_a_int, id_b_ext, id_b_int] = connection;
        std::vector<id_type> seq_next_id_a_ext;
        std::vector<id_type> seq_next_id_a_int;
        std::vector<id_type> seq_next_id_b_ext;
        std::vector<id_type> seq_next_id_b_int;

std::cout << "    next_border_halfedge_id" << std::endl;
        do
        {
            seq_next_id_a_ext.push_back(id_a_ext);
            seq_next_id_a_int.push_back(id_a_int);
            seq_next_id_b_ext.push_back(id_b_ext);
            seq_next_id_b_int.push_back(id_b_int);
            auto pos_a_ext = vertex_position(mesh.halfedge(id_a_ext));
            auto pos_a_int = vertex_position(mesh.halfedge(id_a_int));
            auto pos_b_ext = vertex_position(mesh.halfedge(id_b_ext));
            auto pos_b_int = vertex_position(mesh.halfedge(id_b_int));
std::cout << std::format("    a_ext {}  a_int {}  b_ext {}  b_int {}  {:20}  {:20}  {:20}  {:20}", id_string(id_a_ext), id_string(id_a_int), id_string(id_b_ext), id_string(id_b_int), pos_a_ext, pos_a_int, pos_b_ext, pos_b_int) << std::endl;
            validate_border_id(mesh, id_a_ext);
            validate_border_id(mesh, id_a_int);
            validate_border_id(mesh, id_b_ext);
            validate_border_id(mesh, id_b_int);
            id_a_ext = next_border_halfedge_id(mesh, id_a_ext);
            id_a_int = next_border_halfedge_id(mesh, id_a_int);
            id_b_ext = next_border_halfedge_id(mesh, id_b_ext);
            id_b_int = next_border_halfedge_id(mesh, id_b_int);
        } while (id_a_ext != connection[0]);
        assert(id_a_int == connection[1]);
        assert(id_b_ext == connection[2]);
        assert(id_b_int == connection[3]);
 
        std::vector<id_type> seq_prev_id_a_ext;
        std::vector<id_type> seq_prev_id_a_int;
        std::vector<id_type> seq_prev_id_b_ext;
        std::vector<id_type> seq_prev_id_b_int;

std::cout << "    prev_border_halfedge_id" << std::endl;
        do
        {
            seq_prev_id_a_ext.push_back(id_a_ext);
            seq_prev_id_a_int.push_back(id_a_int);
            seq_prev_id_b_ext.push_back(id_b_ext);
            seq_prev_id_b_int.push_back(id_b_int);
            auto pos_a_ext = vertex_position(mesh.halfedge(id_a_ext));
            auto pos_a_int = vertex_position(mesh.halfedge(id_a_int));
            auto pos_b_ext = vertex_position(mesh.halfedge(id_b_ext));
            auto pos_b_int = vertex_position(mesh.halfedge(id_b_int));
std::cout << std::format("    a_ext {}  a_int {}  b_ext {}  b_int {}  {:20}  {:20}  {:20}  {:20}", id_string(id_a_ext), id_string(id_a_int), id_string(id_b_ext), id_string(id_b_int), pos_a_ext, pos_a_int, pos_b_ext, pos_b_int) << std::endl;
            validate_border_id(mesh, id_a_ext);
            validate_border_id(mesh, id_a_int);
            validate_border_id(mesh, id_b_ext);
            validate_border_id(mesh, id_b_int);
            id_a_ext = prev_border_halfedge_id(mesh, id_a_ext);
            id_a_int = prev_border_halfedge_id(mesh, id_a_int);
            id_b_ext = prev_border_halfedge_id(mesh, id_b_ext);
            id_b_int = prev_border_halfedge_id(mesh, id_b_int);
        } while (id_a_ext != connection[0]);
        assert(id_a_int == connection[1]);
        assert(id_b_ext == connection[2]);
        assert(id_b_int == connection[3]);

        // These checks assume that ext and int are partners or in partner orientation and that a and b are coincident

        validate_connection_bidirectional(mesh, seq_prev_id_a_ext, seq_next_id_a_ext);
        validate_connection_bidirectional(mesh, seq_prev_id_a_int, seq_next_id_a_int);
        validate_connection_bidirectional(mesh, seq_prev_id_b_ext, seq_next_id_b_ext);
        validate_connection_bidirectional(mesh, seq_prev_id_b_int, seq_next_id_b_int);

        validate_connection_positions(mesh, seq_next_id_a_ext, seq_next_id_a_int, 1);
        validate_connection_positions(mesh, seq_prev_id_b_ext, seq_prev_id_b_int, -1);

        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void validate_connections(const M& mesh, const typename IntersectionSplitter<M>::connections_type& connections)
    {
size_t i = 0;
        for (const auto& connection : connections)
        {
std::cout << std::format("connection {}", i++) << std::endl;;
            validate_connection(mesh, connection);
        }
        return;
    }

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::id_type> quetzal::brep::mesh_union(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, const std::string& name)
{
    connect_function_type<Traits> connect = [&](Mesh<Traits>& mesh, id_type halfedge_id_a_exterior, id_type halfedge_id_a_interior, id_type halfedge_id_b_exterior, id_type halfedge_id_b_interior) -> std::vector<id_type>
    {
        connect_border_fragments(mesh, halfedge_id_a_exterior, halfedge_id_b_exterior);
        delete_border_fragment(mesh, halfedge_id_a_interior);
        delete_border_fragment(mesh, halfedge_id_b_interior);
        return {mesh.submesh_id(name)};
    };

    disjoint_function_type<Traits> disjoint = [&](Mesh<Traits>&, id_type, id_type) -> std::vector<id_type>
    {
        return {submesh_id_a, submesh_id_b};
    };

    inclusion_function_type<Traits> inclusion = [&](Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b) -> std::vector<id_type>
    {
        mesh.delete_submesh(submesh_id_b);
        return {submesh_id_a};
    };

    // Renaming the resulting submeshes is done here
    return mesh_operation(mesh, submesh_id_a, submesh_id_b, connect, disjoint, inclusion, name);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::id_type> quetzal::brep::mesh_intersection(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, const std::string& name)
{
    connect_function_type<Traits> connect = [&](Mesh<Traits>& mesh, id_type halfedge_id_a_exterior, id_type halfedge_id_a_interior, id_type halfedge_id_b_exterior, id_type halfedge_id_b_interior) -> std::vector<id_type>
    {
        connect_border_fragments(mesh, halfedge_id_a_interior, halfedge_id_b_interior);
        delete_border_fragment(mesh, halfedge_id_a_exterior);
        delete_border_fragment(mesh, halfedge_id_b_exterior);
        return {mesh.submesh_id(name)};
    };

    disjoint_function_type<Traits> disjoint = [&](Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b) -> std::vector<id_type>
    {
        mesh.delete_submesh(submesh_id_a);
        mesh.delete_submesh(submesh_id_b);
        return {};
    };

    inclusion_function_type<Traits> inclusion = [&](Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b) -> std::vector<id_type>
    {
        mesh.delete_submesh(submesh_id_a);
        return {submesh_id_b};
    };

    // Renaming the resulting submeshes is done here
    return mesh_operation(mesh, submesh_id_a, submesh_id_b, connect, disjoint, inclusion, name);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::id_type> quetzal::brep::mesh_difference(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, const std::string& name)
{
    connect_function_type<Traits> connect = [&](Mesh<Traits>& mesh, id_type halfedge_id_a_exterior, id_type halfedge_id_a_interior, id_type halfedge_id_b_exterior, id_type halfedge_id_b_interior) -> std::vector<id_type>
    {
        BorderFragment<Traits> border_fragment(mesh, halfedge_id_b_interior);
//std::cout << std::format("mesh_difference connect border_fragment {} marked {}", halfedge_id_b_interior, border_fragment.marked()) << std::endl;
        if (!border_fragment.marked())
        {
            border_fragment.invert();
            border_fragment.set_submesh_id(submesh_id_a);
        }

        connect_border_fragments(mesh, halfedge_id_a_exterior, halfedge_id_b_interior);
        delete_border_fragment(mesh, halfedge_id_a_interior);
        delete_border_fragment(mesh, halfedge_id_b_exterior);
        return {submesh_id_a};
    };

    disjoint_function_type<Traits> disjoint = [&](Mesh<Traits>& mesh, id_type, id_type submesh_id_b) -> std::vector<id_type>
    {
        mesh.delete_submesh(submesh_id_b);
        return {submesh_id_a};
    };

    inclusion_function_type<Traits> inclusion = [&](Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b) -> std::vector<id_type>
    {
        invert(mesh.submesh(submesh_id_b));
        set_border_fragment_submesh(mesh, mesh.submesh(submesh_id_b).faces().front().halfedge_id(), submesh_id_a);
        return {submesh_id_a};
    };

    return mesh_operation(mesh, submesh_id_a, submesh_id_b, connect, disjoint, inclusion, name);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::id_type> quetzal::brep::mesh_symmetric_difference(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, const std::string& name)
{
    connect_function_type<Traits> connect = [&](Mesh<Traits>& mesh, id_type halfedge_id_a_exterior, id_type halfedge_id_a_interior, id_type halfedge_id_b_exterior, id_type halfedge_id_b_interior) -> std::vector<id_type>
    {
        BorderFragment<Traits> border_fragment_a(mesh, halfedge_id_b_interior);
//std::cout << std::format("mesh_symmetric_difference connect border_fragment_a {} marked {}", halfedge_id_b_interior, border_fragment_a.marked()) << std::endl;
        if (!border_fragment_a.marked())
        {
            border_fragment_a.invert();
            border_fragment_a.set_submesh_id(submesh_id_a);
        }

        BorderFragment<Traits> border_fragment_b(mesh, halfedge_id_a_interior);
//std::cout << std::format("mesh_symmetric_difference connect border_fragment_b {} marked {}", halfedge_id_a_interior, border_fragment_b.marked()) << std::endl;
        if (!border_fragment_b.marked())
        {
            border_fragment_b.invert();
            border_fragment_b.set_submesh_id(submesh_id_b);
        }

        connect_border_fragments(mesh, halfedge_id_a_exterior, halfedge_id_b_interior);
        connect_border_fragments(mesh, halfedge_id_b_exterior, halfedge_id_a_interior);
        return {submesh_id_a, submesh_id_b};
    };

    disjoint_function_type<Traits> disjoint = [&](Mesh<Traits>&, id_type, id_type) -> std::vector<id_type>
    {
        return {submesh_id_a, submesh_id_b};
    };

    inclusion_function_type<Traits> inclusion = [&](Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b) -> std::vector<id_type>
    {
        invert(mesh.submesh(submesh_id_b));
        set_border_fragment_submesh(mesh, mesh.submesh(submesh_id_b).faces().front().halfedge_id(), submesh_id_a);
        return {submesh_id_a};
    };

    return mesh_operation(mesh, submesh_id_a, submesh_id_b, connect, disjoint, inclusion, name);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::id_type> quetzal::brep::mesh_identity(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, const std::string& name)
{
    connect_function_type<Traits> connect = [&](Mesh<Traits>& mesh, id_type halfedge_id_a_exterior, id_type halfedge_id_a_interior, id_type halfedge_id_b_exterior, id_type halfedge_id_b_interior) -> std::vector<id_type>
    {
 std::cout << std::format("connect {} {} {} {}", id_string(halfedge_id_a_exterior), id_string(halfedge_id_a_interior), id_string(halfedge_id_b_exterior), id_string(halfedge_id_b_interior)) << std::endl;
        connect_border_fragments(mesh, halfedge_id_a_exterior, halfedge_id_a_interior);
        connect_border_fragments(mesh, halfedge_id_b_exterior, halfedge_id_b_interior);
        return {submesh_id_a, submesh_id_b};
    };

    disjoint_function_type<Traits> disjoint = [&](Mesh<Traits>&, id_type, id_type) -> std::vector<id_type>
    {
        return {submesh_id_a, submesh_id_b};
    };

    inclusion_function_type<Traits> inclusion = [&](Mesh<Traits>&, id_type submesh_id_a, id_type submesh_id_b) -> std::vector<id_type>
    {
        return {submesh_id_a, submesh_id_b};
    };

    return mesh_operation(mesh, submesh_id_a, submesh_id_b, connect, disjoint, inclusion, name);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::id_type> quetzal::brep::mesh_operation(Mesh<Traits>& mesh, id_type submesh_id_a, id_type submesh_id_b, connect_function_type<Traits> connect, disjoint_function_type<Traits> disjoint, inclusion_function_type<Traits> inclusion, const std::string& name)
{
//    if (!name.empty())
    {
        std::string prefix;
        auto surface_rename = [&](const std::string& surface_name) -> std::string { return std::format("{}_{}", prefix, surface_name); };

        prefix = mesh.submesh(submesh_id_a).name();
        mesh.rename_submesh_surfaces(submesh_id_a, surface_rename);

        prefix = mesh.submesh(submesh_id_b).name();
        mesh.rename_submesh_surfaces(submesh_id_b, surface_rename);
    }

    IntersectionSplitter intersection_splitter(mesh);
    auto connections = intersection_splitter.connections(submesh_id_a, submesh_id_b);
    validate_connections(mesh, connections);

    std::vector<id_type> submesh_ids;

    // Resolve connections
    if (connections.empty())
    {
        // No intersection borders, so only a single vertex position needs to be checked

        if (solid_contains(mesh.submesh(submesh_id_a), mesh.submesh(submesh_id_b).faces().front().halfedge().vertex_attributes().position()))
        {
            submesh_ids = inclusion(mesh, submesh_id_a, submesh_id_b);
        }
        else if (solid_contains(mesh.submesh(submesh_id_b), mesh.submesh(submesh_id_a).faces().front().halfedge().vertex_attributes().position()))
        {
            submesh_ids = inclusion(mesh, submesh_id_b, submesh_id_a);
        }
        else
        {
            submesh_ids = disjoint(mesh, submesh_id_a, submesh_id_b);
        }
    }
    else
    {
        mesh.reset_all();

        for (const auto& connection : connections)
        {
            auto [halfedge_id_a_exterior, halfedge_id_a_interior, halfedge_id_b_exterior, halfedge_id_b_interior] = connection;
            auto connected_submesh_ids = connect(mesh, halfedge_id_a_exterior, halfedge_id_a_interior, halfedge_id_b_exterior, halfedge_id_b_interior);
            submesh_ids.append_range(connected_submesh_ids);
        }
    }

    if (!name.empty())
    {
        id_type submesh_id = mesh.submesh_id(name);

        if (!mesh.submesh_store()[submesh_id_a].deleted() && submesh_id_a != submesh_id)
        {
            mesh.rename_submesh(submesh_id_a, name);
        }

        if (!mesh.submesh_store()[submesh_id_b].deleted() && submesh_id_b != submesh_id)
        {
            mesh.rename_submesh(submesh_id_b, name);
        }
    }

    return submesh_ids;
}

#endif // QUETZAL_BREP_MESH_OPERATIONS_HPP
