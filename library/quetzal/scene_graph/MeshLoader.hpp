#if !defined(QUETZAL_SCENE_GRAPH_MESHLOADER_HPP)
#define QUETZAL_SCENE_GRAPH_MESHLOADER_HPP
//------------------------------------------------------------------------------
// quetzal scene_graph
// MeshLoader.hpp
//------------------------------------------------------------------------------

#include "IMeshReader.hpp"
#include "MeshData.hpp"
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <vector>

namespace quetzal::scene_graph
{

    class MeshLoader
    {
    public:

        MeshLoader(std::filesystem::path base_path, std::filesystem::path fallback_path = {}) :
            m_base_path(std::move(base_path)),
            m_fallback_path(std::move(fallback_path)),
            m_readers()
        {
        }

        void add_reader(std::unique_ptr<IMeshReader> reader)
        {
            m_readers.push_back(std::move(reader));
        }

        MeshData load(const std::filesystem::path& path) const
        {
            for (const auto& base : {m_base_path, m_fallback_path})
            {
                if (base.empty()) continue;
                std::filesystem::path full_path = base / path;
                if (!std::filesystem::exists(full_path)) continue;
                for (const auto& reader : m_readers)
                {
                    if (reader->can_read(full_path))
                        return reader->read(full_path);
                }
            }
            throw std::runtime_error("No reader found for: " + (m_base_path / path).string());
        }

    private:

        std::filesystem::path m_base_path;
        std::filesystem::path m_fallback_path;
        std::vector<std::unique_ptr<IMeshReader>> m_readers;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_MESHLOADER_HPP
