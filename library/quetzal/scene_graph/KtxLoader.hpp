#if !defined(QUETZAL_SCENE_GRAPH_KTXLOADER_HPP)
#define QUETZAL_SCENE_GRAPH_KTXLOADER_HPP
//------------------------------------------------------------------------------
// quetzal scene_graph
// KtxLoader.hpp
//------------------------------------------------------------------------------

#include "quetzal/vulkan/TextureData.hpp"
#include <filesystem>

namespace quetzal::scene_graph
{

    struct KtxTextureData
    {
        vulkan::TextureData td;

        ~KtxTextureData();
        KtxTextureData(KtxTextureData&&) noexcept;
        KtxTextureData& operator=(KtxTextureData&&) noexcept;
        KtxTextureData(const KtxTextureData&) = delete;
        KtxTextureData& operator=(const KtxTextureData&) = delete;

    private:

        KtxTextureData() = default;
        void* m_handle = nullptr;

        friend KtxTextureData load_ktx2(const std::filesystem::path&);
    };

    KtxTextureData load_ktx2(const std::filesystem::path& path);

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_KTXLOADER_HPP
