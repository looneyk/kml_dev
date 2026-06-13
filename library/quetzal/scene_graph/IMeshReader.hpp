#if !defined(QUETZAL_SCENE_GRAPH_IMESHREADER_HPP)
#define QUETZAL_SCENE_GRAPH_IMESHREADER_HPP
//------------------------------------------------------------------------------
// quetzal scene_graph
// IMeshReader.hpp
//------------------------------------------------------------------------------

#include "MeshData.hpp"
#include <filesystem>

namespace quetzal::scene_graph
{

    class IMeshReader
    {
    public:

        virtual ~IMeshReader() = default;

        virtual bool can_read(const std::filesystem::path& path) const = 0;
        virtual MeshData read(const std::filesystem::path& path) const = 0;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_IMESHREADER_HPP
