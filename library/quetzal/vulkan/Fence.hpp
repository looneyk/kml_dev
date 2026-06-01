#if !defined(QUETZAL_VULKAN_FENCE_HPP)
#define QUETZAL_VULKAN_FENCE_HPP

#include "Device.hpp"
#include "VulkanError.hpp"
#include "initializer.hpp"
#include <vulkan/vulkan.hpp>
#include <cstdint>

namespace quetzal::vulkan
{

    class Fence
    {
    public:

        Fence(const Device& device, bool signaled = true) :
            m_device(device),
            m_handle(VK_NULL_HANDLE)
        {
            VkFenceCreateInfo ci = vk_fence_create_info(signaled ? VK_FENCE_CREATE_SIGNALED_BIT : VkFenceCreateFlagBits(0));
            VkResult vk_result = vkCreateFence(m_device, &ci, nullptr, &m_handle);
            check_result(vk_result, "vkCreateFence");
        }

        Fence(const Fence&) = delete;

        Fence(Fence&& other) noexcept :
            m_device(other.m_device),
            m_handle(other.m_handle)
        {
            other.m_handle = VK_NULL_HANDLE;
        }

        ~Fence()
        {
            if (m_handle != VK_NULL_HANDLE)
            {
                vkDestroyFence(m_device, m_handle, nullptr);
            }
        }

        operator VkFence() const
        {
            return m_handle;
        }

        void wait(uint64_t timeout = UINT64_MAX) const
        {
            vkWaitForFences(m_device, 1, &m_handle, VK_TRUE, timeout);
        }

        void reset()
        {
            vkResetFences(m_device, 1, &m_handle);
        }

    private:

        const Device& m_device;
        VkFence m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_FENCE_HPP
