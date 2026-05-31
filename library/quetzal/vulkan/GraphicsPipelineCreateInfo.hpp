#if !defined(QUETZAL_VULKAN_GRAPHICSPIPELINECREATEINFO_HPP)
#define QUETZAL_VULKAN_GRAPHICSPIPELINECREATEINFO_HPP

#include "Device.hpp"
#include "PipelineLayout.hpp"
#include "ShaderModule.hpp"
#include "initializer.hpp"
#include "vulkan_util.hpp"
#include <vulkan/vulkan.hpp>
#include <array>
#include <span>
#include <string>
#include <vector>

namespace quetzal::vulkan
{

    class GraphicsPipelineCreateInfo
    {
    public:

        using vk_type = VkGraphicsPipelineCreateInfo;

        GraphicsPipelineCreateInfo(const Device& device, const std::string& vert_shader_filepath, const std::string& frag_shader_filepath,
                const VkVertexInputBindingDescription& binding_description, std::span<const VkVertexInputAttributeDescription> attribute_descriptions,
                const PipelineLayout& pipeline_layout, VkFormat color_format, VkFormat depth_format, VkSampleCountFlagBits sample_count) :
            m_vert_shader_module(device, vert_shader_filepath),
            m_frag_shader_module(device, frag_shader_filepath),
            m_shader_stages(),
            m_binding_description(binding_description),
            m_attribute_descriptions(std::from_range, attribute_descriptions),
            m_color_blend_attachment_state(),
            m_dynamic_states(),
            m_vertex_input_state_ci(),
            m_input_assembly_state_ci(),
            m_viewport_state_ci(),
            m_rasterization_state_ci(),
            m_multisampling_state_ci(),
            m_depth_stencil_state_ci(),
            m_color_blend_state_ci(),
            m_dynamic_state_ci(),
            m_pipeline_layout(pipeline_layout),
            m_color_format(),
            m_pipeline_rendering_ci(),
            m_ci()
        {
            auto vert_shader_stage_ci = vk_pipeline_shader_stage_create_info(m_vert_shader_module, VK_SHADER_STAGE_VERTEX_BIT);
            auto frag_shader_stage_ci = vk_pipeline_shader_stage_create_info(m_frag_shader_module, VK_SHADER_STAGE_FRAGMENT_BIT);
            m_shader_stages = {vert_shader_stage_ci, frag_shader_stage_ci};

            m_vertex_input_state_ci = vk_pipeline_vertex_input_state_create_info({&m_binding_description, 1}, m_attribute_descriptions);
            m_input_assembly_state_ci = vk_pipeline_input_assembly_state_create_info();
            m_viewport_state_ci = vk_pipeline_viewport_state_create_info();
            m_rasterization_state_ci = vk_pipeline_rasterization_state_create_info();
            m_multisampling_state_ci = vk_pipeline_multisample_state_create_info(sample_count);
            m_depth_stencil_state_ci = vk_pipeline_depth_stencil_state_create_info();

            m_color_blend_attachment_state = vk_pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
            m_color_blend_state_ci = vk_pipeline_color_blend_state_create_info({&m_color_blend_attachment_state, 1});

            m_dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            m_dynamic_state_ci = vk_pipeline_dynamic_state_create_info(m_dynamic_states);

            m_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            m_ci.flags = 0;
            m_ci.stageCount = to_uint32(m_shader_stages.size());
            m_ci.pStages = m_shader_stages.data();
            m_ci.pVertexInputState = &m_vertex_input_state_ci;
            m_ci.pInputAssemblyState = &m_input_assembly_state_ci;
            m_ci.pTessellationState = nullptr;
            m_ci.pViewportState = &m_viewport_state_ci;
            m_ci.pRasterizationState = &m_rasterization_state_ci;
            m_ci.pMultisampleState = &m_multisampling_state_ci;
            m_ci.pDepthStencilState = &m_depth_stencil_state_ci;
            m_ci.pColorBlendState = &m_color_blend_state_ci;
            m_ci.pDynamicState = &m_dynamic_state_ci;
            m_color_format = color_format;
            m_pipeline_rendering_ci = vk_pipeline_rendering_create_info({&m_color_format, 1}, depth_format);

            m_ci.pNext = &m_pipeline_rendering_ci;
            m_ci.layout = pipeline_layout;
            m_ci.renderPass = VK_NULL_HANDLE;
            m_ci.subpass = 0;
            m_ci.basePipelineHandle = VK_NULL_HANDLE;
            m_ci.basePipelineIndex = 0;
        }

        operator VkGraphicsPipelineCreateInfo() const
        {
            return m_ci;
        }

        size_t size() const
        {
            return 1;
        }

        const vk_type* data() const
        {
            return &m_ci;
        }

        const PipelineLayout& pipeline_layout() const
        {
            return m_pipeline_layout;
        }

        ShaderModule m_vert_shader_module;
        ShaderModule m_frag_shader_module;
        std::array<VkPipelineShaderStageCreateInfo, 2> m_shader_stages;
        VkVertexInputBindingDescription m_binding_description;
        std::vector<VkVertexInputAttributeDescription> m_attribute_descriptions;
        VkPipelineColorBlendAttachmentState m_color_blend_attachment_state;
        std::array<VkDynamicState, 2> m_dynamic_states;

        VkPipelineVertexInputStateCreateInfo m_vertex_input_state_ci;
        VkPipelineInputAssemblyStateCreateInfo m_input_assembly_state_ci;
        VkPipelineViewportStateCreateInfo m_viewport_state_ci;
        VkPipelineRasterizationStateCreateInfo m_rasterization_state_ci;
        VkPipelineMultisampleStateCreateInfo m_multisampling_state_ci;
        VkPipelineDepthStencilStateCreateInfo m_depth_stencil_state_ci;
        VkPipelineColorBlendStateCreateInfo m_color_blend_state_ci;
        VkPipelineDynamicStateCreateInfo m_dynamic_state_ci;

        const PipelineLayout& m_pipeline_layout;

        VkFormat m_color_format;
        VkPipelineRenderingCreateInfo m_pipeline_rendering_ci;
        VkGraphicsPipelineCreateInfo m_ci;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_GRAPHICSPIPELINECREATEINFO_HPP
