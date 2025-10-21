#if !defined(QUETZAL_VULKAN_FRAMEBUFFERS_HPP)
#define QUETZAL_VULKAN_FRAMEBUFFERS_HPP

#include "Device.hpp"
#include "Framebuffer.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include <vulkan/vulkan.hpp>
#include <array>
#include <vector>

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
            m_depthImageView(m_device),
            m_framebuffers()
        {
        }

        const Framebuffer& operator[](size_t i)
        {
            assert(i < m_framebuffers.size());
            return m_framebuffers[i];
        }

        void create(VkExtent2D extent, VkFormat colorFormat, VkFormat depthFormat, std::span<const ImageView> imageViews, VkSampleCountFlagBits samples, VkRenderPass renderPass)
        {
            m_colorImage.create(extent, colorFormat, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, samples, 1, VK_IMAGE_TILING_OPTIMAL, 0);
            m_colorImageView.create(m_colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

            m_depthImage.create(extent, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, samples, 1, VK_IMAGE_TILING_OPTIMAL, 0);
            m_depthImageView.create(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

            auto count = imageViews.size();
            m_framebuffers.resize(count, Framebuffer(m_device));
            for (size_t i = 0; i < count; ++i)
            {
                std::array<VkImageView, 3> attachments = {
                    m_colorImageView,
                    m_depthImageView,
                    imageViews[i],
                };

                m_framebuffers[i].create(extent, attachments, renderPass);
            }
        }

        void destroy()
        {
            m_depthImageView.destroy();
            m_depthImage.destroy();

            m_colorImageView.destroy();
            m_colorImage.destroy();

            for (auto framebuffer : m_framebuffers)
            {
                framebuffer.destroy();
            }
        }

    private:

        const Device& m_device;
        Image m_colorImage;
        ImageView m_colorImageView;
        Image m_depthImage;
        ImageView m_depthImageView;
        std::vector<Framebuffer> m_framebuffers;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_FRAMEBUFFERS_HPP
