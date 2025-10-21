#if !defined(QUETZAL_VULKAN_DESCRIPTORPOOL_HPP)
#define QUETZAL_VULKAN_DESCRIPTORPOOL_HPP

#include "Device.hpp"
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
            VkResult result = vkCreateDescriptorPool(m_device, &ci, nullptr, &m_handle);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error(std::format("Error vkCreateDescriptorPool {}", result));
            }
        }

        void destroy()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyDescriptorPool(m_device, m_handle, nullptr);
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
