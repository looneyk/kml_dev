#if !defined(QUETZAL_VULKAN_PIPELINECACHE_HPP)
#define QUETZAL_VULKAN_PIPELINECACHE_HPP

#include "Device.hpp"
#include "VulkanError.hpp"
#include <vulkan/vulkan.hpp>
#include <span>
#include <stdexcept>

namespace quetzal::vulkan
{

    class PipelineCache
    {
    public:

        PipelineCache(const Device& device) :
            m_device(device),
            m_handle(VK_NULL_HANDLE)
        {
//            create();
        }

        ~PipelineCache()
        {
            destroy();
        }

        operator VkPipelineCache() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

        // Not created as opposed to not containing any pipelines, but we may not have any way of knowing the latter
        // if created empty in constructor, get rid of this function ...
        bool empty()
        {
            return m_handle == VK_NULL_HANDLE;
        }

        // load ...
        void create(std::span<uint8_t> buffer = {})
        {
            VkPipelineCacheCreateInfo ci = vk_pipeline_cache_create_info(buffer);
            VkResult vk_result = vkCreatePipelineCache(m_device, &ci, nullptr, &m_handle);
            check_result(vk_result, "vkCreatePipelineCache");
            return;
        }

        void merge(std::span<uint8_t> buffer = {})
        {
            // ...
            buffer;
            return;
        }

        void merge(VkPipelineCache other)
        {
            VkResult vk_result = vkMergePipelineCaches(m_device, m_handle, 1, &other);
            check_result(vk_result, "vkMergePipelineCaches");
            return;
        }

        void destroy()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyPipelineCache(m_device, m_handle, nullptr);
            m_handle = VK_NULL_HANDLE;
            return;
        }

    private:

        const Device& m_device;
        VkPipelineCache m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_PIPELINECACHE_HPP
