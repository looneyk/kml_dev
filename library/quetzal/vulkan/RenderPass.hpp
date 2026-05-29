#if !defined(QUETZAL_VULKAN_RENDERPASS_HPP)
#define QUETZAL_VULKAN_RENDERPASS_HPP

#include "Device.hpp"
#include "VulkanError.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    class RenderPass
    {
    public:

        RenderPass(const Device& device) :
            m_device(device),
            m_handle(VK_NULL_HANDLE)
        {
        }

        operator VkRenderPass() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

        void create(std::span<VkAttachmentDescription> attachments, std::span<VkSubpassDescription> subpasses, std::span<VkSubpassDependency> dependencies)
        {
            VkRenderPassCreateInfo ci = vk_render_pass_create_info(attachments, subpasses, dependencies);
            VkResult vk_result = vkCreateRenderPass(m_device, &ci, nullptr, &m_handle);
            check_result(vk_result, "vkCreateRenderPass");
            return;
        }

        void create(VkFormat color_format, VkFormat depth_format, VkSampleCountFlagBits samples)
        {
            std::vector<VkAttachmentDescription> attachments =
            {
                vk_attachment_description(color_format, samples,               VK_ATTACHMENT_LOAD_OP_CLEAR,     VK_ATTACHMENT_STORE_OP_STORE,     VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
                vk_attachment_description(depth_format, samples,               VK_ATTACHMENT_LOAD_OP_CLEAR,     VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
                vk_attachment_description(color_format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,     VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR),
            };

            VkAttachmentReference color_attachment_ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            VkAttachmentReference depth_attachment_ref{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            VkAttachmentReference resolve_attachment_ref{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

            std::vector<VkSubpassDescription> subpasses =
            {
                vk_subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS, {}, {&color_attachment_ref, 1}, resolve_attachment_ref, depth_attachment_ref, {})
            };

            std::vector<VkSubpassDependency> dependencies =
            {
                vk_subpass_dependency(VK_SUBPASS_EXTERNAL, 0,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
            };

            create(attachments, subpasses, dependencies);
            return;
        }

        void destroy()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyRenderPass(m_device, m_handle, nullptr);
            return;
        }

    private:

        const Device& m_device;
        VkRenderPass m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_RENDERPASS_HPP
