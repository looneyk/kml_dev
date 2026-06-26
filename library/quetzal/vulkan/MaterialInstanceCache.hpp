#if !defined(QUETZAL_VULKAN_MATERIALINSTANCECACHE_HPP)
#define QUETZAL_VULKAN_MATERIALINSTANCECACHE_HPP
//------------------------------------------------------------------------------
// quetzal
// MaterialInstanceCache.hpp
//------------------------------------------------------------------------------

#include "Descriptors.hpp"
#include "Device.hpp"
#include "IRenderer.hpp"
#include "MaterialCache.hpp"
#include "MaterialInstance.hpp"
#include "TextureCache.hpp"
#include "handles.hpp"
#include <cassert>
#include <cstddef>
#include <string>
#include <unordered_map>

namespace quetzal::vulkan
{

    class MaterialInstanceCache
    {
    public:

        MaterialInstanceCache(const Device& device, DescriptorAllocator& descriptor_allocator,
            DescriptorLayoutCache& descriptor_layout_cache, MaterialCache& material_cache,
            TextureCache& texture_cache, size_t frame_count) :
            m_device(device),
            m_descriptor_allocator(descriptor_allocator),
            m_descriptor_layout_cache(descriptor_layout_cache),
            m_material_cache(material_cache),
            m_texture_cache(texture_cache),
            m_frame_count(frame_count),
            m_next_id(0),
            m_cache()
        {
        }

        MaterialInstanceHandle create(const MaterialHandle& material, const MaterialParameters& params)
        {
            std::string key = "mi_" + std::to_string(m_next_id++);
            auto [i, inserted] = m_cache.try_emplace(key, m_device);
            assert(inserted);
            std::visit([&](const auto& p) { create_(i->second, material, p); }, params);
            return MaterialInstanceHandle{key};
        }

        MaterialInstance& get(const MaterialInstanceHandle& handle)
        {
            auto i = m_cache.find(handle.id);
            assert(i != m_cache.end());
            return i->second;
        }

        const MaterialInstance& get(const MaterialInstanceHandle& handle) const
        {
            auto i = m_cache.find(handle.id);
            assert(i != m_cache.end());
            return i->second;
        }

        void update(const MaterialInstanceHandle& handle, const MaterialParameters& params)
        {
            MaterialInstance& instance = get(handle);
            std::visit([&](const auto& p) { update_(instance, p); }, params);
        }

        void clear()
        {
            for (auto& [key, instance] : m_cache)
                instance.destroy();
            m_cache.clear();
        }

    private:

        void create_(MaterialInstance& instance, const MaterialHandle& material, const ColorParameters& p)
        {
            instance.create(m_material_cache.get(material), p.color);
        }

        void create_(MaterialInstance& instance, const MaterialHandle& material, const TextureParameters& p)
        {
            instance.create(
                m_material_cache.get(material),
                m_texture_cache.get(p.texture.id),
                m_descriptor_allocator,
                m_descriptor_layout_cache,
                m_frame_count
            );
        }

        static void update_(MaterialInstance& instance, const ColorParameters& p)
        {
            instance.update(p.color);
        }

        static void update_(MaterialInstance&, const TextureParameters&)
        {
        }

        const Device& m_device;
        DescriptorAllocator& m_descriptor_allocator;
        DescriptorLayoutCache& m_descriptor_layout_cache;
        MaterialCache& m_material_cache;
        TextureCache& m_texture_cache;
        size_t m_frame_count;
        size_t m_next_id;
        std::unordered_map<std::string, MaterialInstance> m_cache;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_MATERIALINSTANCECACHE_HPP
