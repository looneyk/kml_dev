#if !defined(QUETZAL_VULKAN_MATERIAL_HPP)
#define QUETZAL_VULKAN_MATERIAL_HPP
//------------------------------------------------------------------------------
// quetzal
// Material.hpp
//------------------------------------------------------------------------------

#include "Device.hpp"
#include "GraphicsPipelineCreateInfo.hpp"
#include "PipelineManager.hpp"
#include "ResourceManager.hpp"
#include <vulkan/vulkan.hpp>
#include <span>
#include <string>
#include <vector>

namespace quetzal::vulkan
{

    class Material
    {
    public:

        Material(const Device& device) :
            m_device(device),
            m_pipeline_layout(VK_NULL_HANDLE),
            m_pipeline(VK_NULL_HANDLE)
        {
        }

        void create(
            VkDescriptorSetLayout set0_layout,
            VkDescriptorSetLayout set1_layout,
            ResourceManager& resource_manager,
            PipelineManager& pipeline_manager,
            const std::string& vert_shader,
            const std::string& frag_shader,
            const VkVertexInputBindingDescription& binding_description,
            std::span<const VkVertexInputAttributeDescription> attribute_descriptions,
            VkFormat color_format,
            VkFormat depth_format,
            VkSampleCountFlagBits samples,
            std::span<VkPushConstantRange> push_constant_ranges
        )
        {
            std::array<VkDescriptorSetLayout, 2> set_layouts = {set0_layout, set1_layout};
            const PipelineLayout& pipeline_layout = resource_manager.request_pipeline_layout(set_layouts, push_constant_ranges);
            m_pipeline_layout = pipeline_layout;

            GraphicsPipelineCreateInfo ci = {m_device, vert_shader, frag_shader, binding_description, attribute_descriptions, pipeline_layout, color_format, depth_format, samples};
            m_pipeline = pipeline_manager.request_graphics_pipeline(ci);
        }

        void destroy()
        {
            m_pipeline_layout = VK_NULL_HANDLE;
            m_pipeline = VK_NULL_HANDLE;
        }

        VkPipelineLayout pipeline_layout() const { return m_pipeline_layout; }
        VkPipeline pipeline() const { return m_pipeline; }

    private:

        const Device& m_device;
        VkPipelineLayout m_pipeline_layout;
        VkPipeline m_pipeline;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_MATERIAL_HPP
