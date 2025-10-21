#if !defined(QUETZAL_VULKAN_FRAMEBUFFER_HPP)
#define QUETZAL_VULKAN_FRAMEBUFFER_HPP

#include "Device.hpp"
#include "ImageView.hpp"
#include "initializer.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    class Framebuffer
    {
    public:

        Framebuffer(const Device& device) :
            m_device(device),
            m_handle(VK_NULL_HANDLE)
        {
        }

        void create(VkExtent2D extent, std::span<VkImageView> attachments, VkRenderPass render_pass)
        {
            VkFramebufferCreateInfo ci = vk_framebuffer_create_info(extent, attachments, render_pass);
            VkResult result = vkCreateFramebuffer(m_device, &ci, nullptr, &m_handle);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error(std::format("Error vkCreateFramebuffer {}", result));
            }
        }

        void destroy()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyFramebuffer(m_device, m_handle, nullptr);
        }

        operator VkFramebuffer() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

    private:

        const Device& m_device;
        VkFramebuffer m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_FRAMEBUFFER_HPP
