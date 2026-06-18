
#include "TextureManager.hpp"
#include "DdsLoader.hpp"
#include "KtxLoader.hpp"
#include "quetzal/vulkan/TextureData.hpp"
#include <stb_image.h>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace quetzal::scene_graph
{

TextureManager::TextureManager(vulkan::IRenderer& renderer, std::filesystem::path base_path, std::filesystem::path fallback_path) :
    m_renderer(renderer),
    m_base_path(std::move(base_path)),
    m_fallback_path(std::move(fallback_path)),
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
    if (!std::filesystem::exists(full_path) && !m_fallback_path.empty())
        full_path = m_fallback_path / path;

    vulkan::TextureHandle handle;

    auto ext = full_path.extension();

    if (ext == ".ktx" || ext == ".ktx2")
    {
        KtxTextureData ktx = load_ktx2(full_path);
        handle = m_renderer.upload_texture(id, ktx.td);
    }
    else if (ext == ".dds")
    {
        std::ifstream file(full_path, std::ios::binary | std::ios::ate);
        if (!file)
            throw std::runtime_error("Failed to open texture: " + full_path.string());
        auto file_size = static_cast<size_t>(file.tellg());
        std::vector<uint8_t> file_data(file_size);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(file_data.data()), static_cast<std::streamsize>(file_size));

        vulkan::TextureData td = parse_dds(file_data.data(), file_data.size());
        handle = m_renderer.upload_texture(id, td);
    }
    else
    {
        int width, height, channels;
        stbi_uc* pixels = stbi_load(full_path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels)
            throw std::runtime_error("Failed to load texture: " + full_path.string());

        vulkan::TextureData td{VK_FORMAT_R8G8B8A8_SRGB, {{pixels, static_cast<size_t>(width) * height * 4, static_cast<uint32_t>(width), static_cast<uint32_t>(height)}}};
        handle = m_renderer.upload_texture(id, td);
        stbi_image_free(pixels);
    }

    m_cache.emplace(id, handle);
    return handle;
}

} // namespace quetzal::scene_graph
