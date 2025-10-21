#if !defined(QUETZAL_VULKAN_COMPUTEPIPELINECREATEINFO_HPP)
#define QUETZAL_VULKAN_COMPUTEPIPELINECREATEINFO_HPP

#include "Device.hpp"
#include "PipelineLayout.hpp"
#include "ShaderModule.hpp"
#include <vulkan/vulkan.hpp>
#include <string>

namespace quetzal::vulkan
{

    class ComputePipelineCreateInfo
    {
    public:

        using vk_type = VkComputePipelineCreateInfo;

        ComputePipelineCreateInfo(const Device& device, const std::string& shader_filepath, const PipelineLayout& pipelineLayout) :
            m_shader_module(device, shader_filepath),
            m_shader_stage(),
            m_pipeline_layout(pipelineLayout),
            m_ci()
        {
            m_shaderStage = m_shader_module.shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT);

            m_ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            m_ci.flags = 0;
            m_ci.stages = m_shaderStage;
            m_ci.layout = pipelineLayout;
            m_ci.basePipelineHandle = VK_NULL_HANDLE;
            m_ci.basePipelineIndex = 0;
        }

        operator VkComputePipelineCreateInfo() const
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

        ShaderModule m_shader_module;
        VkPipelineShaderStageCreateInfo m_shader_stage;
        const PipelineLayout& m_pipeline_layout;
        VkComputePipelineCreateInfo m_ci;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_COMPUTEPIPELINECREATEINFO_HPP
