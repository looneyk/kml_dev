#if !defined(QUETZAL_VULKAN_TEXTUREDATA_HPP)
#define QUETZAL_VULKAN_TEXTUREDATA_HPP
//------------------------------------------------------------------------------
// quetzal vulkan
// TextureData.hpp
//------------------------------------------------------------------------------

#include <vulkan/vulkan.h>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace quetzal::vulkan
{

    struct MipLevel
    {
        const void* data;
        size_t byte_size;
        uint32_t width;
        uint32_t height;
    };

    struct TextureData
    {
        VkFormat format;
        std::vector<MipLevel> mips;
    };

    inline bool is_compressed(VkFormat format)
    {
        return format >= VK_FORMAT_BC1_RGB_UNORM_BLOCK && format <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
    }

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_TEXTUREDATA_HPP
