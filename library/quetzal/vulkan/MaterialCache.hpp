#if !defined(QUETZAL_VULKAN_MATERIALCACHE_HPP)
#define QUETZAL_VULKAN_MATERIALCACHE_HPP
//------------------------------------------------------------------------------
// quetzal
// MaterialCache.hpp
//------------------------------------------------------------------------------

#include "Device.hpp"
#include "Material.hpp"
#include "PipelineManager.hpp"
#include "ResourceManager.hpp"
#include "handles.hpp"
#include <vulkan/vulkan.hpp>
#include <cassert>
#include <format>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace quetzal::vulkan
{

    class MaterialCache
    {
    public:

        MaterialCache(const Device& device, VkDescriptorSetLayout set0_layout, VkDescriptorSetLayout set1_layout,
            ResourceManager& resource_manager, PipelineManager& pipeline_manager,
            VkFormat color_format, VkFormat depth_format, VkSampleCountFlagBits samples,
            std::span<VkPushConstantRange> push_constant_ranges) :
            m_device(device),
            m_set0_layout(set0_layout),
            m_set1_layout(set1_layout),
            m_resource_manager(resource_manager),
            m_pipeline_manager(pipeline_manager),
            m_color_format(color_format),
            m_depth_format(depth_format),
            m_samples(samples),
            m_push_constant_ranges(push_constant_ranges.begin(), push_constant_ranges.end()),
            m_cache()
        {
        }

        MaterialHandle create(const std::string& vert_shader, const std::string& frag_shader,
            const VkVertexInputBindingDescription& binding_desc, const std::vector<VkVertexInputAttributeDescription>& attr_descs)
        {
            std::string layout_key = std::to_string(binding_desc.binding) + ":" + std::to_string(binding_desc.stride);
            for (const auto& ad : attr_descs)
                layout_key += std::format("|{}:{}:{}", ad.location, ad.format, ad.offset);
            std::string key = std::format("{}|{}|{}", vert_shader, frag_shader, layout_key);

            auto [i, inserted] = m_cache.try_emplace(key, m_device);
            if (inserted)
            {
                i->second.create(
                    m_set0_layout,
                    m_set1_layout,
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
        MaterialHandle create(const std::string& vert_shader, const std::string& frag_shader)
        {
            return create(vert_shader, frag_shader, V::binding_description(), V::attribute_descriptions());
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
        VkDescriptorSetLayout m_set0_layout;
        VkDescriptorSetLayout m_set1_layout;
        ResourceManager& m_resource_manager;
        PipelineManager& m_pipeline_manager;
        VkFormat m_color_format;
        VkFormat m_depth_format;
        VkSampleCountFlagBits m_samples;
        std::vector<VkPushConstantRange> m_push_constant_ranges;
        std::unordered_map<std::string, Material> m_cache;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_MATERIALCACHE_HPP
