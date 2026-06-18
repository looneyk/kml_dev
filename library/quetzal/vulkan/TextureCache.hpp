#if !defined(QUETZAL_VULKAN_TEXTURECACHE_HPP)
#define QUETZAL_VULKAN_TEXTURECACHE_HPP

#include "CommandPool.hpp"
#include "Device.hpp"
#include "Queue.hpp"
#include "Texture.hpp"
#include "TextureData.hpp"
#include "handles.hpp"
#include <unordered_map>
#include <string>

namespace quetzal::vulkan
{

    class TextureCache
    {
    public:

        TextureCache(const Device& device, const CommandPool& command_pool, const Queue& queue) :
            m_device(device),
            m_command_pool(command_pool),
            m_queue(queue),
            m_cache()
        {
        }

        ~TextureCache() = default;

        TextureHandle upload(const std::string& id, const TextureData& td)
        {
            auto [i, b] = m_cache.try_emplace(id, m_device);
            if (b)
            {
                i->second.create(td, m_command_pool, m_queue);
            }
            return TextureHandle{id};
        }

        const Texture& get(const std::string& id) const
        {
            auto i = m_cache.find(id);
            assert(i != m_cache.end());
            return i->second;
        }

        void clear()
        {
            for (auto& i : m_cache)
            {
                i.second.destroy();
            }

            m_cache.clear();
            return;
        }

    private:

        const Device& m_device;
        const CommandPool& m_command_pool;
        const Queue& m_queue;
        std::unordered_map<std::string, Texture> m_cache;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_TEXTURECACHE_HPP
