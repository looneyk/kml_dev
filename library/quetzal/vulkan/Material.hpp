#if !defined(QUETZAL_VULKAN_MATERIAL_HPP)
#define QUETZAL_VULKAN_MATERIAL_HPP

#include "Descriptors.hpp"
#include "Device.hpp"
#include "Effect.hpp"
#include "Texture.hpp"
#include <vulkan/vulkan.hpp>
#include <cstddef>
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
            const Effect& effect,
            const Texture& texture,
            DescriptorAllocator& descriptor_allocator,
            DescriptorLayoutCache& descriptor_layout_cache,
            size_t frame_count
        )
        {
            m_pipeline_layout = effect.pipeline_layout();
            m_pipeline = effect.pipeline();

            m_descriptor_sets.resize(frame_count);
            VkDescriptorImageInfo image_info = texture.descriptor();

            for (size_t i = 0; i < frame_count; ++i)
            {
                DescriptorBuilder::begin(&descriptor_allocator, &descriptor_layout_cache)
                    .bind(0, image_info, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .build(m_descriptor_sets[i]);
            }
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
