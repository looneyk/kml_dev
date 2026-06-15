#if !defined(QUETZAL_SCENE_GRAPH_GLTFLOADER_HPP)
#define QUETZAL_SCENE_GRAPH_GLTFLOADER_HPP
//------------------------------------------------------------------------------
// quetzal scene_graph
// GltfLoader.hpp
//------------------------------------------------------------------------------

#include "AssetManager.hpp"
#include "Scene.hpp"
#include "quetzal/vulkan/IRenderer.hpp"
#include <filesystem>
#include <string>

namespace quetzal::scene_graph
{

    void load_gltf(Scene& scene,
                   Node& parent,
                   vulkan::IRenderer& renderer,
                   AssetManager& assets,
                   const std::filesystem::path& path,
                   const std::string& vert_spv = "shaders/vert_pnt.spv",
                   const std::string& frag_spv = "shaders/frag_pnt.spv");

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_GLTFLOADER_HPP
