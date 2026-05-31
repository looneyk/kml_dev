#if !defined(QUETZAL_VULKAN_RESOURCEMANAGER_HPP)
#define QUETZAL_VULKAN_RESOURCEMANAGER_HPP
//------------------------------------------------------------------------------
// quetzal::vulkan
// ResourceManager.hpp
//------------------------------------------------------------------------------

#include "Device.hpp"
#include "PipelineLayout.hpp"
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
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_RESOURCEMANAGER_HPP
