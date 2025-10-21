
//#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/floating_point.hpp"
#include "obj_io.hpp"
#include <filesystem>
#include <iostream>
#include <ranges>

using namespace std;
using namespace quetzal;

namespace
{

    struct Mesh
    {
        using value_type = float;
        using vector_traits = quetzal::math::HomogeneousTraits<value_type, 3>;
        using texcoord_traits = quetzal::math::HomogeneousTraits<value_type, 2>;
        using point_type = quetzal::geometry::Point<vector_traits>;
        using vector_type = point_type::vector_type;
        using texcoord_type = quetzal::geometry::Point<texcoord_traits>;

        std::vector<point_type> positions;
        std::vector<vector_type> normals;
        std::vector<texcoord_type> texcoords;
    };

    using mesh_type = Mesh;

    constexpr int ulp_compare = 100000;

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
        cout << "Skipping " << path_lhs << ", file path is not an obj file." << endl;
        return 0;
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
    obj_io::read_obj(mesh_lhs, path_lhs);

    mesh_type mesh_rhs;
    obj_io::read_obj(mesh_rhs, path_rhs);

    size_t nv_lhs = mesh_lhs.positions.size();
    size_t nv_rhs = mesh_rhs.positions.size();
    if (nv_lhs != nv_rhs)
    {
        cout << "Vertex position count mismatch:" << endl;
        cout << path_lhs << "\t" << nv_lhs << endl;
        cout << path_rhs << "\t" << nv_rhs << endl;
    }

    size_t nvn_lhs = mesh_lhs.normals.size();
    size_t nvn_rhs = mesh_rhs.normals.size();
    if (nvn_lhs != nvn_rhs)
    {
        cout << "Vertex normal count mismatch:" << endl;
        cout << path_lhs << "\t" << nvn_lhs << endl;
        cout << path_rhs << "\t" << nvn_rhs << endl;
    }

    size_t nvt_lhs = mesh_lhs.texcoords.size();
    size_t nvt_rhs = mesh_rhs.texcoords.size();
    if (nvt_lhs != nvt_rhs)
    {
        cout << "Vertex texcoord count mismatch:" << endl;
        cout << path_lhs << "\t" << nvt_lhs << endl;
        cout << path_rhs << "\t" << nvt_rhs << endl;
    }

    size_t nv = 0;
    size_t nvn = 0;
    size_t nvt = 0;

    for (const auto& [i, v_lhs, v_rhs] : ranges::views::zip(std::views::iota(0), mesh_lhs.positions, mesh_rhs.positions))
    {
        if (float_eq(v_lhs, v_rhs, ulp_compare))
        {
            continue;
        }

        cout << "Vertex position " << i << " mismatch:" << endl;
        cout << v_lhs << endl;
        cout << v_rhs << endl;
    }

    for (const auto& [i, vn_lhs, vn_rhs] : ranges::views::zip(std::views::iota(0), mesh_lhs.normals, mesh_rhs.normals))
    {
        if (float_eq(vn_lhs, vn_rhs, ulp_compare))
        {
            continue;
        }

        cout << "Vertex normal " << i << " mismatch:" << endl;
        cout << vn_lhs << endl;
        cout << vn_rhs << endl;
    }

    for (const auto& [i, vt_lhs, vt_rhs] : ranges::views::zip(std::views::iota(0), mesh_lhs.texcoords, mesh_rhs.texcoords))
    {
        if (float_eq(vt_lhs, vt_rhs, ulp_compare))
        {
            continue;
        }

        cout << "Vertex texcoord " << i << " mismatch:" << endl;
        cout << vt_lhs << endl;
        cout << vt_rhs << endl;
    }

    if (nv > 0)
    {
        cout << nv << " position mismatches" << endl;
    }

    if (nvn > 0)
    {
        cout << nvn << " normal mismatches" << endl;
    }

    if (nvt > 0)
    {
        cout << nvt << " texcoord mismatches" << endl;
    }

    return nv > 0 || nvn > 0 || nvt > 0;
}
