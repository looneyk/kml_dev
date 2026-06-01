#if !defined(QUETZAL_VULKAN_SEMAPHORE_HPP)
#define QUETZAL_VULKAN_SEMAPHORE_HPP

#include "Device.hpp"
#include "VulkanError.hpp"
#include "initializer.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    class Semaphore
    {
    public:

        Semaphore(const Device& device) :
            m_device(device),
            m_handle(VK_NULL_HANDLE)
        {
            VkSemaphoreCreateInfo ci = vk_semaphore_create_info();
            VkResult vk_result = vkCreateSemaphore(m_device, &ci, nullptr, &m_handle);
            check_result(vk_result, "vkCreateSemaphore");
        }

        Semaphore(const Semaphore&) = delete;

        Semaphore(Semaphore&& other) noexcept :
            m_device(other.m_device),
            m_handle(other.m_handle)
        {
            other.m_handle = VK_NULL_HANDLE;
        }

        ~Semaphore()
        {
            if (m_handle != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_device, m_handle, nullptr);
            }
        }

        operator VkSemaphore() const
        {
            return m_handle;
        }

    private:

        const Device& m_device;
        VkSemaphore m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_SEMAPHORE_HPP
