#if !defined(QUETZAL_VULKAN_DESCRIPTORPOOL_HPP)
#define QUETZAL_VULKAN_DESCRIPTORPOOL_HPP

#include "Device.hpp"
#include "VulkanError.hpp"
#include "initializer.hpp"
#include <vulkan/vulkan.hpp>
#include <span>

namespace quetzal::vulkan
{

    class DescriptorPool
    {
    public:

        DescriptorPool(const Device& device) :
            m_device(device),
            m_handle(VK_NULL_HANDLE)
        {
        }

        operator VkDescriptorPool() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

        void create(std::span<VkDescriptorPoolSize> sizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags = 0)
        {
            VkDescriptorPoolCreateInfo ci = descriptor_pool_create_info(sizes, maxSets, flags);
            VkResult vk_result = vkCreateDescriptorPool(m_device, &ci, nullptr, &m_handle);
            check_result(vk_result, "vkCreateDescriptorPool");
            return;
        }

        void destroy()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyDescriptorPool(m_device, m_handle, nullptr);
            return;
        }

        void reset()
        {
            vkResetDescriptorPool(m_device, m_handle, 0);
        }

    private:

        const Device& m_device;
        VkDescriptorPool m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_DESCRIPTORPOOL_HPP
