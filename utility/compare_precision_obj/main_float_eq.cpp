
#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/geometry/Attributes.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/model/obj_io.hpp"
#include <filesystem>
#include <iostream>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    constexpr int ulp_compare = 10000000;

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        cerr << "Error: Requires exactly two obj file path arguments, " << (argc - 1) << " provided" << endl;
        return -1;
    }

    filesystem::path path_lhs(argv[1]);
    if (!filesystem::exists(path_lhs))
    {
        cerr << "Error: First obj file path does not exist." << endl;
        return -1;
    }

    if (path_lhs.extension() != ".obj")
    {
        cerr << "Error: First file path is not an obj file." << endl;
        return -1;
    }

    filesystem::path path_rhs(argv[2]);
    if (!filesystem::exists(path_rhs))
    {
        cerr << "Error: Second obj file path does not exist." << endl;
        return -1;
    }

    if (path_rhs.extension() != ".obj")
    {
        cerr << "Error: Second file path is not an obj file." << endl;
        return -1;
    }

    mesh_type mesh_lhs;
    model::read_obj(mesh_lhs, path_lhs);

    mesh_type mesh_rhs;
    model::read_obj(mesh_rhs, path_rhs);

    size_t nv_lhs = mesh_lhs.vertex_store_count();
    size_t nv_rhs = mesh_rhs.vertex_store_count();

    if (nv_lhs != nv_rhs)
    {
        cout << "Vertex count mismatch:" << endl;
        cout << path_lhs << "\t" << nv_lhs << endl;
        cout << path_rhs << "\t" << nv_rhs << endl;
        return 1;
    }

    size_t n = 0;

    for (const auto& [i, v_lhs, v_rhs] : ranges::views::zip(std::views::iota(0), mesh_lhs.vertex_store(), mesh_rhs.vertex_store()))
    {
        const auto& av_lhs = v_lhs.attributes();
        const auto& av_rhs = v_rhs.attributes();

        if (float_eq(av_lhs, av_rhs, ulp_compare))
        {
            continue;
        }

        cout << "Vertex " << i << " attribute mismatch:" << endl;
        cout << av_lhs << endl;
        cout << av_rhs << endl;

        ++n;
    }

    if (n > 0)
    {
        cout << n << " mismatches" << endl;
    }

    return n > 0;
}
