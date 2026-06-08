#if !defined(QUETZAL_VULKAN_MATERIAL_HPP)
#define QUETZAL_VULKAN_MATERIAL_HPP

#include "Buffer.hpp"
#include "Descriptors.hpp"
#include "Device.hpp"
#include "GraphicsPipelineCreateInfo.hpp"
#include "PipelineManager.hpp"
#include "ResourceManager.hpp"
#include "Texture.hpp"
#include "initializer.hpp"
#include <vulkan/vulkan.hpp>
#include <filesystem>
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
            m_descriptor_sets(),
            m_pipeline_layout(VK_NULL_HANDLE),
            m_pipeline(VK_NULL_HANDLE)
        {
        }

        void create(
            std::span<const Buffer> uniform_buffers,
            const Texture& texture,
            DescriptorAllocator& descriptor_allocator,
            DescriptorLayoutCache& descriptor_layout_cache,
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
            size_t n = uniform_buffers.size();
            m_descriptor_sets.resize(n);
            std::vector<VkDescriptorSetLayout> descriptor_set_layouts(n);

            VkDescriptorImageInfo image_info = texture.descriptor();

            for (size_t i = 0; i < n; ++i)
            {
                VkDescriptorBufferInfo buffer_info = uniform_buffers[i].descriptor();
                auto builder = DescriptorBuilder::begin(&descriptor_allocator, &descriptor_layout_cache);
                builder.bind(0, buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
                builder.bind(1, image_info, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
                builder.build(m_descriptor_sets[i], descriptor_set_layouts[i]);
            }

            const PipelineLayout& pipeline_layout = resource_manager.request_pipeline_layout(descriptor_set_layouts, push_constant_ranges);
            m_pipeline_layout = pipeline_layout;

            GraphicsPipelineCreateInfo ci = {m_device, vert_shader, frag_shader, binding_description, attribute_descriptions, pipeline_layout, color_format, depth_format, samples};
            m_pipeline = pipeline_manager.request_graphics_pipeline(ci);
        }

        void destroy()
        {
            m_descriptor_sets.clear();
            m_pipeline_layout = VK_NULL_HANDLE;
            m_pipeline = VK_NULL_HANDLE;
        }

        VkDescriptorSet descriptor_set(size_t frame) const { return m_descriptor_sets[frame]; }
        VkPipelineLayout pipeline_layout() const { return m_pipeline_layout; }
        VkPipeline pipeline() const { return m_pipeline; }

    private:

        const Device& m_device;
        std::vector<VkDescriptorSet> m_descriptor_sets;
        VkPipelineLayout m_pipeline_layout;
        VkPipeline m_pipeline;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_MATERIAL_HPP
