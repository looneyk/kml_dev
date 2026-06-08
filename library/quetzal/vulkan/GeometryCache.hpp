#if !defined(QUETZAL_VULKAN_GEOMETRYCACHE_HPP)
#define QUETZAL_VULKAN_GEOMETRYCACHE_HPP

#include "CommandPool.hpp"
#include "Device.hpp"
#include "Geometry.hpp"
#include "handles.hpp"
#include <map>
#include <span>
#include <string>

namespace quetzal::vulkan
{

    class GeometryCache
    {
    public:

        GeometryCache(const Device& device, const CommandPool& command_pool) :
            m_device(device),
            m_command_pool(command_pool),
            m_cache()
        {
        }

        ~GeometryCache() = default;

        GeometryHandle upload(const std::string& id, Geometry::buffer_type vertex_data, std::span<const Geometry::index_type> indices)
        {
            auto [i, b] = m_cache.try_emplace(id, m_device, m_command_pool);
            if (b)
            {
                i->second.upload(vertex_data, indices);
            }
            return GeometryHandle{id};
        }

        template<typename V>
        GeometryHandle upload(const std::string& id, std::span<const V> vertices, std::span<const Geometry::index_type> indices)
        {
            return upload(id, {reinterpret_cast<const uint8_t*>(vertices.data()), vertices.size_bytes()}, indices);
        }

        const Geometry& get(const GeometryHandle& handle) const
        {
            auto i = m_cache.find(handle.id);
            assert(i != m_cache.end());
            return i->second;
        }

        void clear()
        {
            for (auto& i : m_cache)
            {
                i.second.clear();
            }

            m_cache.clear();
            return;
        }

    private:

        const Device& m_device;
        const CommandPool& m_command_pool;
        std::map<std::string, Geometry> m_cache;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_GEOMETRYCACHE_HPP
