#if !defined(QUETZAL_SCENE_GRAPH_TEXTUREMANAGER_HPP)
#define QUETZAL_SCENE_GRAPH_TEXTUREMANAGER_HPP
//------------------------------------------------------------------------------
// quetzal scene_graph
// TextureManager.hpp
//------------------------------------------------------------------------------

#include "quetzal/vulkan/IRenderer.hpp"
#include "quetzal/vulkan/handles.hpp"
#include <filesystem>
#include <string>
#include <unordered_map>

namespace quetzal::scene_graph
{

    class TextureManager
    {
    public:

        TextureManager(vulkan::IRenderer& renderer, std::filesystem::path base_path, std::filesystem::path fallback_path = {});

        vulkan::TextureHandle load(const std::string& id, const std::filesystem::path& path);

    private:

        vulkan::IRenderer& m_renderer;
        std::filesystem::path m_base_path;
        std::filesystem::path m_fallback_path;
        std::unordered_map<std::string, vulkan::TextureHandle> m_cache;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_TEXTUREMANAGER_HPP
