#if !defined(QUETZAL_VULKAN_RESOURCEMANAGER_HPP)
#define QUETZAL_VULKAN_RESOURCEMANAGER_HPP
//------------------------------------------------------------------------------
// quetzal::vulkan
// ResourceManager.hpp
//------------------------------------------------------------------------------

#include "Device.hpp"
#include "PipelineLayout.hpp"
#include "RenderPass.hpp"
#include "ResourceCache.hpp"
#include <vulkan/vulkan.hpp>
#include <span>

namespace quetzal::vulkan
{

    class ResourceRecorder;

    //--------------------------------------------------------------------------
    class ResourceManager
    {
    public:

        ResourceManager(const Device& device);
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager(ResourceManager&&) = delete;
        ~ResourceManager() = default;

        ResourceManager& operator=(const ResourceManager&) = delete;
        ResourceManager& operator=(ResourceManager&&) = delete;

        PipelineLayout& request_pipeline_layout(std::span<VkDescriptorSetLayout> set_layouts, std::span<VkPushConstantRange> push_constant_ranges);
//        RenderPass& request_render_pass(std::span<VkAttachmentDescription> attachments, std::span<LoadStoreInfo> load_store_infos, std::span<VkSubpassDescription> subpasses);
        RenderPass& request_render_pass(std::span<VkAttachmentDescription> attachments, std::span<VkSubpassDescription> subpasses, std::span<VkSubpassDependency> dependencies);
        // ...

        void clear();

        // not yet ...
        std::vector<uint8_t> serialize();
        void load(std::span<uint8_t> data);

    private:

        //template<typename T, typename... A>
        //T& request(ResourceCache<T>& cache, A&... args);

        const Device& m_device;
        ResourceRecorder* m_precorder;
        // replayer ...

//        std::filesystem::path m_rootpath;
        ResourceCache<PipelineLayout> m_pipeline_layouts;
        ResourceCache<RenderPass> m_render_passes;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_RESOURCEMANAGER_HPP
