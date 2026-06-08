#if !defined(QUETZAL_VULKAN_MATERIALCACHE_HPP)
#define QUETZAL_VULKAN_MATERIALCACHE_HPP

#include "Descriptors.hpp"
#include "Device.hpp"
#include "Material.hpp"
#include "PipelineManager.hpp"
#include "ResourceManager.hpp"
#include "TextureCache.hpp"
#include "handles.hpp"
#include <vulkan/vulkan.hpp>
#include <span>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace quetzal::vulkan
{

    class MaterialCache
    {
    public:

        MaterialCache(const Device& device, DescriptorAllocator& descriptor_allocator, DescriptorLayoutCache& descriptor_layout_cache,
            ResourceManager& resource_manager, PipelineManager& pipeline_manager, TextureCache& texture_cache,
            std::span<const Buffer> uniform_buffers, VkFormat color_format, VkFormat depth_format,
            VkSampleCountFlagBits samples, std::span<VkPushConstantRange> push_constant_ranges) :
            m_device(device),
            m_descriptor_allocator(descriptor_allocator),
            m_descriptor_layout_cache(descriptor_layout_cache),
            m_resource_manager(resource_manager),
            m_pipeline_manager(pipeline_manager),
            m_texture_cache(texture_cache),
            m_uniform_buffers(uniform_buffers),
            m_color_format(color_format),
            m_depth_format(depth_format),
            m_samples(samples),
            m_push_constant_ranges(push_constant_ranges.begin(), push_constant_ranges.end()),
            m_cache()
        {
        }

        MaterialHandle create(const TextureHandle& texture, const std::string& vert_shader, const std::string& frag_shader,
            const VkVertexInputBindingDescription& binding_desc, const std::vector<VkVertexInputAttributeDescription>& attr_descs)
        {
            std::string layout_key = std::to_string(binding_desc.binding) + ":" + std::to_string(binding_desc.stride);
            for (const auto& ad : attr_descs)
                layout_key += "|" + std::to_string(ad.location) + ":" + std::to_string(ad.format) + ":" + std::to_string(ad.offset);
            std::string key = texture.id + "|" + vert_shader + "|" + frag_shader + "|" + layout_key;

            auto [i, inserted] = m_cache.try_emplace(key, m_device);
            if (inserted)
            {
                i->second.create(
                    m_uniform_buffers,
                    m_texture_cache.get(texture.id),
                    m_descriptor_allocator,
                    m_descriptor_layout_cache,
                    m_resource_manager,
                    m_pipeline_manager,
                    vert_shader,
                    frag_shader,
                    binding_desc,
                    attr_descs,
                    m_color_format,
                    m_depth_format,
                    m_samples,
                    m_push_constant_ranges
                );
            }

            return MaterialHandle{key};
        }

        template<typename V>
        MaterialHandle create(const TextureHandle& texture, const std::string& vert_shader, const std::string& frag_shader)
        {
            return create(texture, vert_shader, frag_shader, V::binding_description(), V::attribute_descriptions());
        }

        const Material& get(const MaterialHandle& handle) const
        {
            auto i = m_cache.find(handle.id);
            assert(i != m_cache.end());
            return i->second;
        }

        void clear()
        {
            for (auto& [key, material] : m_cache)
                material.destroy();
            m_cache.clear();
        }

    private:

        const Device& m_device;
        DescriptorAllocator& m_descriptor_allocator;
        DescriptorLayoutCache& m_descriptor_layout_cache;
        ResourceManager& m_resource_manager;
        PipelineManager& m_pipeline_manager;
        TextureCache& m_texture_cache;
        std::span<const Buffer> m_uniform_buffers;
        VkFormat m_color_format;
        VkFormat m_depth_format;
        VkSampleCountFlagBits m_samples;
        std::vector<VkPushConstantRange> m_push_constant_ranges;
        std::unordered_map<std::string, Material> m_cache;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_MATERIALCACHE_HPP
