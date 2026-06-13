#if !defined(QUETZAL_SCENE_GRAPH_MESHDATA_HPP)
#define QUETZAL_SCENE_GRAPH_MESHDATA_HPP
//------------------------------------------------------------------------------
// quetzal scene_graph
// MeshData.hpp
//------------------------------------------------------------------------------

#include <array>
#include <cstdint>
#include <vector>

namespace quetzal::scene_graph
{

    struct MeshData
    {
        std::vector<std::array<float, 3>> positions;
        std::vector<std::array<float, 3>> normals;
        std::vector<std::array<float, 3>> colors;
        std::vector<std::array<float, 2>> texcoords;
        std::vector<uint32_t> indices;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_MESHDATA_HPP
