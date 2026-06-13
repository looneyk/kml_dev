#if !defined(QUETZAL_SCENE_GRAPH_TINYOBJREADER_HPP)
#define QUETZAL_SCENE_GRAPH_TINYOBJREADER_HPP
//------------------------------------------------------------------------------
// quetzal scene_graph
// TinyobjReader.hpp
//------------------------------------------------------------------------------

#include "IMeshReader.hpp"

namespace quetzal::scene_graph
{

    class TinyobjReader : public IMeshReader
    {
    public:

        bool can_read(const std::filesystem::path& path) const override;
        MeshData read(const std::filesystem::path& path) const override;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_TINYOBJREADER_HPP
