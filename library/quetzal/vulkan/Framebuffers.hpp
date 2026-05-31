#if !defined(QUETZAL_VULKAN_FRAMEBUFFERS_HPP)
#define QUETZAL_VULKAN_FRAMEBUFFERS_HPP

#include "Device.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include <vulkan/vulkan.hpp>
#include <array>

namespace quetzal::vulkan
{

    class Framebuffers
    {
    public:

        Framebuffers(const Device& device) :
            m_device(device),
            m_colorImage(m_device),
            m_colorImageView(m_device),
            m_depthImage(m_device),
            m_depthImageView(m_device)
        {
        }

        VkImage color_image() const { return m_colorImage; }
        VkImageView color_image_view() const { return m_colorImageView; }
        VkImage depth_image() const { return m_depthImage; }
        VkImageView depth_image_view() const { return m_depthImageView; }

        void create(VkExtent2D extent, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits samples)
        {
            m_colorImage.create(extent, colorFormat, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, samples, 1, VK_IMAGE_TILING_OPTIMAL, 0);
            m_colorImageView.create(m_colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

            m_depthImage.create(extent, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, samples, 1, VK_IMAGE_TILING_OPTIMAL, 0);
            m_depthImageView.create(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        }

        void destroy()
        {
            m_depthImageView.destroy();
            m_depthImage.destroy();

            m_colorImageView.destroy();
            m_colorImage.destroy();
        }

    private:

        const Device& m_device;
        Image m_colorImage;
        ImageView m_colorImageView;
        Image m_depthImage;
        ImageView m_depthImageView;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_FRAMEBUFFERS_HPP
