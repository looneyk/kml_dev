
#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4244)
#include <stb_image.h>
#pragma warning(pop)

#include "TextureManager.hpp"
#include <stdexcept>

namespace quetzal::scene_graph
{

TextureManager::TextureManager(vulkan::IRenderer& renderer, std::filesystem::path base_path) :
    m_renderer(renderer),
    m_base_path(std::move(base_path)),
    m_cache()
{
}

vulkan::TextureHandle TextureManager::load(const std::string& id, const std::filesystem::path& path)
{
    auto it = m_cache.find(id);
    if (it != m_cache.end())
    {
        return it->second;
    }

    std::filesystem::path full_path = m_base_path / path;

    int width, height, channels;
    stbi_uc* pixels = stbi_load(full_path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        throw std::runtime_error("Failed to load texture: " + full_path.string());
    }

    vulkan::TextureHandle handle = m_renderer.upload_texture(id, pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    stbi_image_free(pixels);

    m_cache.emplace(id, handle);
    return handle;
}

} // namespace quetzal::scene_graph
