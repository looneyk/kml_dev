
#if !defined(QUETZAL_SCENE_GRAPH_SHADERMANAGER_HPP)
#define QUETZAL_SCENE_GRAPH_SHADERMANAGER_HPP

#include <filesystem>
#include <string>

namespace quetzal::scene_graph
{

    class ShaderManager
    {
    public:

        ShaderManager(std::filesystem::path shaders_path) :
            m_shaders_path(std::move(shaders_path))
        {
        }

        ShaderManager(const ShaderManager&) = delete;
        ShaderManager(ShaderManager&&) = delete;
        ~ShaderManager() = default;

        ShaderManager& operator=(const ShaderManager&) = delete;
        ShaderManager& operator=(ShaderManager&&) = delete;

        std::filesystem::path vert_path(const std::string& name) const
        {
            return m_shaders_path / (name + ".vert.spv");
        }

        std::filesystem::path frag_path(const std::string& name) const
        {
            return m_shaders_path / (name + ".frag.spv");
        }

    private:

        std::filesystem::path m_shaders_path;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_SHADERMANAGER_HPP
