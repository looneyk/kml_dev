#if !defined(QUETZAL_VULKAN_SAMPLER_HPP)
#define QUETZAL_VULKAN_SAMPLER_HPP

#include "Device.hpp"
#include "VulkanError.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    class Sampler
    {
    public:

        Sampler(const Device& device) :
            m_device(device),
            m_handle(VK_NULL_HANDLE)
        {
        }

        void create()
        {
            VkPhysicalDeviceProperties properties = m_device.physical_device().properties();
            VkSamplerCreateInfo ci = vk_sampler_create_info(properties);
            VkResult vk_result = vkCreateSampler(m_device, &ci, nullptr, &m_handle);
            check_result(vk_result, "vkCreateSampler");
            return;
        }

        void destroy()
        {
            if (m_handle != VK_NULL_HANDLE)
            {
                vkDestroySampler(m_device, m_handle, nullptr);
                m_handle = VK_NULL_HANDLE;
            }
            return;
        }

        operator VkSampler() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

    private:

        const Device& m_device;
        VkSampler m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_SAMPLER_HPP
