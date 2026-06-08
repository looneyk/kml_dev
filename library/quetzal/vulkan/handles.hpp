#if !defined(QUETZAL_VULKAN_HANDLES_HPP)
#define QUETZAL_VULKAN_HANDLES_HPP

#include <string>

namespace quetzal::vulkan
{

    struct GeometryHandle
    {
        std::string id;

        bool operator==(const GeometryHandle&) const = default;
    };

    struct MaterialHandle
    {
        std::string id;

        bool operator==(const MaterialHandle&) const = default;
    };

    struct TextureHandle
    {
        std::string id;

        bool operator==(const TextureHandle&) const = default;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_HANDLES_HPP
