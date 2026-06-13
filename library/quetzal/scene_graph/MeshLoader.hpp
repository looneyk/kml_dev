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

        MeshLoader(std::filesystem::path base_path) :
            m_base_path(std::move(base_path)),
            m_readers()
        {
        }

        void add_reader(std::unique_ptr<IMeshReader> reader)
        {
            m_readers.push_back(std::move(reader));
        }

        MeshData load(const std::filesystem::path& path) const
        {
            std::filesystem::path full_path = m_base_path / path;
            for (const auto& reader : m_readers)
            {
                if (reader->can_read(full_path))
                {
                    return reader->read(full_path);
                }
            }
            throw std::runtime_error("No reader found for: " + full_path.string());
        }

    private:

        std::filesystem::path m_base_path;
        std::vector<std::unique_ptr<IMeshReader>> m_readers;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_MESHLOADER_HPP
