#if !defined(QUETZAL_VULKAN_COMMANDPOOL_HPP)
#define QUETZAL_VULKAN_COMMANDPOOL_HPP

#include "Device.hpp"
#include "VulkanError.hpp"
#include "initializer.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    class CommandPool
    {
    public:

        CommandPool(const Device& device) :
            m_device(device),
            m_handle(VK_NULL_HANDLE)
        {
        }

        operator VkCommandPool() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

        void create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) // 0
        {
            VkCommandPoolCreateInfo ci = vk_command_pool_create_info(queueFamilyIndex, flags);
            VkResult vk_result = vkCreateCommandPool(m_device, &ci, nullptr, &m_handle);
            check_result(vk_result, "vkCreateCommandPool");
            return;
        }

        void destroy()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyCommandPool(m_device, m_handle, nullptr);
            return;
        }

    private:

        const Device& m_device;
        VkCommandPool m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_COMMANDPOOL_HPP
