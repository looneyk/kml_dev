#if !defined(QUETZAL_VULKAN_SAMPLER_HPP)
#define QUETZAL_VULKAN_SAMPLER_HPP

#include "Device.hpp"
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
            VkResult result = vkCreateSampler(m_device, &ci, nullptr, &m_handle);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error(std::format("Error vkCreateSampler {}", result));
            }
        }

        void destroy()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroySampler(m_device, m_handle, nullptr);
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
