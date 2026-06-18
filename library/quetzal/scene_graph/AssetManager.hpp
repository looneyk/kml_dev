#if !defined(QUETZAL_SCENE_GRAPH_ASSETMANAGER_HPP)
#define QUETZAL_SCENE_GRAPH_ASSETMANAGER_HPP
//------------------------------------------------------------------------------
// quetzal scene_graph
// AssetManager.hpp
//------------------------------------------------------------------------------

#include "MeshLoader.hpp"
#include "TextureManager.hpp"
#include "TinyobjReader.hpp"
#include "quetzal/vulkan/handles.hpp"
#include <filesystem>
#include <memory>
#include <string>

namespace quetzal::scene_graph
{

    class AssetManager
    {
    public:

        AssetManager(vulkan::IRenderer& renderer, std::filesystem::path assets_path) :
            m_assets_path(assets_path),
            m_mesh_loader(assets_path / "models"),
            m_texture_manager(renderer, assets_path / "textures")
        {
            m_mesh_loader.add_reader(std::make_unique<TinyobjReader>());
        }

        const std::filesystem::path& assets_path() const { return m_assets_path; }

        MeshData load_mesh(const std::filesystem::path& path)
        {
            return m_mesh_loader.load(path);
        }

        vulkan::TextureHandle load_texture(const std::string& id, const std::filesystem::path& path)
        {
            return m_texture_manager.load(id, path);
        }

    private:

        std::filesystem::path m_assets_path;
        MeshLoader m_mesh_loader;
        TextureManager m_texture_manager;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_ASSETMANAGER_HPP
