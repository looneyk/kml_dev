
#include "KtxLoader.hpp"
#include <ktx.h>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace quetzal::scene_graph
{

KtxTextureData::~KtxTextureData()
{
    if (m_handle)
        ktxTexture_Destroy(static_cast<ktxTexture*>(m_handle));
}

KtxTextureData::KtxTextureData(KtxTextureData&& other) noexcept :
    td(std::move(other.td)),
    m_handle(other.m_handle)
{
    other.m_handle = nullptr;
}

KtxTextureData& KtxTextureData::operator=(KtxTextureData&& other) noexcept
{
    if (this != &other)
    {
        if (m_handle)
            ktxTexture_Destroy(static_cast<ktxTexture*>(m_handle));
        td = std::move(other.td);
        m_handle = other.m_handle;
        other.m_handle = nullptr;
    }
    return *this;
}

KtxTextureData load_ktx2(const std::filesystem::path& path)
{
    ktxTexture2* texture = nullptr;
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(
        path.string().c_str(),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &texture);
    if (result != KTX_SUCCESS)
        throw std::runtime_error("Failed to load KTX2 texture: " + path.string());

    if (ktxTexture2_NeedsTranscoding(texture))
    {
        result = ktxTexture2_TranscodeBasis(texture, KTX_TTF_BC7_RGBA, 0);
        if (result != KTX_SUCCESS)
        {
            ktxTexture_Destroy(ktxTexture(texture));
            throw std::runtime_error("Failed to transcode KTX2 texture: " + path.string());
        }
    }

    KtxTextureData ktd;
    ktd.m_handle = texture;
    ktd.td.format = static_cast<VkFormat>(texture->vkFormat);

    uint32_t num_levels  = texture->numLevels;
    uint32_t base_width  = texture->baseWidth;
    uint32_t base_height = texture->baseHeight;

    ktd.td.mips.reserve(num_levels);

    for (uint32_t level = 0; level < num_levels; ++level)
    {
        ktx_size_t offset = 0;
        result = ktxTexture_GetImageOffset(ktxTexture(texture), level, 0, 0, &offset);
        if (result != KTX_SUCCESS)
        {
            ktxTexture_Destroy(ktxTexture(texture));
            ktd.m_handle = nullptr;
            throw std::runtime_error("Failed to get KTX2 mip offset: " + path.string());
        }

        const uint8_t* data      = ktxTexture_GetData(ktxTexture(texture)) + offset;
        ktx_size_t     byte_size = ktxTexture_GetImageSize(ktxTexture(texture), level);
        uint32_t       width     = std::max(1u, base_width  >> level);
        uint32_t       height    = std::max(1u, base_height >> level);

        ktd.td.mips.push_back({data, static_cast<size_t>(byte_size), width, height});
    }

    return ktd;
}

} // namespace quetzal::scene_graph
