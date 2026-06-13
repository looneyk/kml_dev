#if !defined(QUETZAL_VULKAN_MATERIALCACHE_HPP)
#define QUETZAL_VULKAN_MATERIALCACHE_HPP

#include "Descriptors.hpp"
#include "Device.hpp"
#include "EffectCache.hpp"
#include "Material.hpp"
#include "TextureCache.hpp"
#include "handles.hpp"
#include <cassert>
#include <cstddef>
#include <string>
#include <unordered_map>

namespace quetzal::vulkan
{

    class MaterialCache
    {
    public:

        MaterialCache(const Device& device, DescriptorAllocator& descriptor_allocator,
            DescriptorLayoutCache& descriptor_layout_cache, EffectCache& effect_cache,
            TextureCache& texture_cache, size_t frame_count) :
            m_device(device),
            m_descriptor_allocator(descriptor_allocator),
            m_descriptor_layout_cache(descriptor_layout_cache),
            m_effect_cache(effect_cache),
            m_texture_cache(texture_cache),
            m_frame_count(frame_count),
            m_cache()
        {
        }

        MaterialHandle create(const EffectHandle& effect, const TextureHandle& texture)
        {
            std::string key = effect.id + "|" + texture.id;

            auto [i, inserted] = m_cache.try_emplace(key, m_device);
            if (inserted)
            {
                i->second.create(
                    m_effect_cache.get(effect),
                    m_texture_cache.get(texture.id),
                    m_descriptor_allocator,
                    m_descriptor_layout_cache,
                    m_frame_count
                );
            }

            return MaterialHandle{key};
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
        EffectCache& m_effect_cache;
        TextureCache& m_texture_cache;
        size_t m_frame_count;
        std::unordered_map<std::string, Material> m_cache;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_MATERIALCACHE_HPP
