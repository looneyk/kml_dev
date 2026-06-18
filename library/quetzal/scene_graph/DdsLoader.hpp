#if !defined(QUETZAL_SCENE_GRAPH_DDSLOADER_HPP)
#define QUETZAL_SCENE_GRAPH_DDSLOADER_HPP
//------------------------------------------------------------------------------
// quetzal scene_graph
// DdsLoader.hpp
//------------------------------------------------------------------------------

#include "quetzal/vulkan/TextureData.hpp"
#include <cstddef>
#include <cstdint>

namespace quetzal::scene_graph
{

    // Returns TextureData with MipLevel::data pointers into file_data.
    // file_data must remain valid for the duration of the subsequent upload_texture call.
    vulkan::TextureData parse_dds(const uint8_t* file_data, size_t file_size);

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_DDSLOADER_HPP
