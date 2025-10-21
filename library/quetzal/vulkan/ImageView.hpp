#if !defined(QUETZAL_VULKAN_IMAGEVIEW_HPP)
#define QUETZAL_VULKAN_IMAGEVIEW_HPP

#include "Device.hpp"
#include "Image.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    class ImageView
    {
    public:

        ImageView(const Device& device) :
            m_device(device),
            m_handle(VK_NULL_HANDLE)
        {
        }

        void create(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
        {
            VkImageViewCreateInfo ci = vk_image_view_create_info(image, format, aspectFlags, mipLevels);
            VkResult result = vkCreateImageView(m_device, &ci, nullptr, &m_handle);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error(std::format("Error vkCreateImageView {}", result));
            }
        }

        void destroy()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyImageView(m_device, m_handle, nullptr);
        }

        operator VkImageView() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

    private:

        const Device& m_device;
        VkImageView m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_IMAGEVIEW_HPP
