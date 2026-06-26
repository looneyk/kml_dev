#if !defined(QUETZAL_VULKAN_MATERIALINSTANCE_HPP)
#define QUETZAL_VULKAN_MATERIALINSTANCE_HPP
//------------------------------------------------------------------------------
// quetzal
// MaterialInstance.hpp
//------------------------------------------------------------------------------

#include "Descriptors.hpp"
#include "Device.hpp"
#include "Material.hpp"
#include "Texture.hpp"
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <cstddef>
#include <variant>
#include <vector>

namespace quetzal::vulkan
{

    class MaterialInstance
    {
    public:

        MaterialInstance(const Device& device) :
            m_device(device),
            m_data(ColorData{}),
            m_pipeline_layout(VK_NULL_HANDLE),
            m_pipeline(VK_NULL_HANDLE)
        {
        }

        void create(
            const Material& material,
            const glm::vec4& color
        )
        {
            m_pipeline_layout = material.pipeline_layout();
            m_pipeline = material.pipeline();
            m_data = ColorData{color};
        }

        void create(
            const Material& material,
            const Texture& texture,
            DescriptorAllocator& descriptor_allocator,
            DescriptorLayoutCache& descriptor_layout_cache,
            size_t frame_count
        )
        {
            m_pipeline_layout = material.pipeline_layout();
            m_pipeline = material.pipeline();

            TextureData td;
            td.descriptor_sets.resize(frame_count);
            VkDescriptorImageInfo image_info = texture.descriptor();

            for (size_t i = 0; i < frame_count; ++i)
            {
                DescriptorBuilder::begin(&descriptor_allocator, &descriptor_layout_cache)
                    .bind(0, image_info, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .build(td.descriptor_sets[i]);
            }

            m_data = std::move(td);
        }

        void destroy()
        {
            m_data = ColorData{};
            m_pipeline_layout = VK_NULL_HANDLE;
            m_pipeline = VK_NULL_HANDLE;
        }

        void update(const glm::vec4& color)
        {
            std::visit([&](auto& data) { data.color = color; }, m_data);
        }

        void bind(VkCommandBuffer command_buffer, VkDescriptorSet scene_set, size_t frame) const
        {
            std::visit([&](const auto& data) { bind_(command_buffer, scene_set, frame, data); }, m_data);
        }

        VkPipelineLayout pipeline_layout() const { return m_pipeline_layout; }
        VkPipeline pipeline() const { return m_pipeline; }

    private:

        static constexpr uint32_t material_push_offset_ = 64;

        struct ColorData
        {
            glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        };

        struct TextureData
        {
            std::vector<VkDescriptorSet> descriptor_sets;
            glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        };

        using MaterialData = std::variant<ColorData, TextureData>;

        void bind_(VkCommandBuffer command_buffer, VkDescriptorSet scene_set, size_t, const ColorData& data) const
        {
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &scene_set, 0, nullptr);
            vkCmdPushConstants(command_buffer, m_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, material_push_offset_, sizeof(glm::vec4), &data.color);
        }

        void bind_(VkCommandBuffer command_buffer, VkDescriptorSet scene_set, size_t frame, const TextureData& data) const
        {
            VkDescriptorSet sets[] = {scene_set, data.descriptor_sets[frame]};
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 2, sets, 0, nullptr);
            vkCmdPushConstants(command_buffer, m_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, material_push_offset_, sizeof(glm::vec4), &data.color);
        }

        const Device& m_device;
        MaterialData m_data;
        VkPipelineLayout m_pipeline_layout;
        VkPipeline m_pipeline;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_MATERIALINSTANCE_HPP
