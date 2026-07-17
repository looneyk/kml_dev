#if !defined(QUETZAL_VULKAN_RESOURCECACHE_HPP)
#define QUETZAL_VULKAN_RESOURCECACHE_HPP
//------------------------------------------------------------------------------
// quetzal::vulkan
// ResourceCache.hpp
//------------------------------------------------------------------------------

#include "Device.hpp"
#include "cache_util.hpp"
#include <vulkan/vulkan.hpp>
#include <format>
#include <mutex>
#include <type_traits>
#include <unordered_map>

namespace quetzal::vulkan
{

    class ResourceRecorder;

    //--------------------------------------------------------------------------
    template<typename T>
    class ResourceCache
    {
    public:

        ResourceCache(const Device& device);
        ResourceCache(const ResourceCache&) = delete;
        ResourceCache(ResourceCache&&) = delete;
        ~ResourceCache() = default;

        ResourceCache& operator=(const ResourceCache&) = delete;
        ResourceCache& operator=(ResourceCache&&) = delete;

        template<typename... A>
        T& request(A&... args);

        void clear();

    private:

        template<typename... A>
        T& request_resource(ResourceRecorder* recorder, A&... args);

        const Device& m_device;
        std::unordered_map<std::size_t, T> m_cache;
        std::mutex m_mutex;
    };

} // namespace quetzal::vulkan

//------------------------------------------------------------------------------
template<typename T>
quetzal::vulkan::ResourceCache<T>::ResourceCache(const Device& device) :
    m_device(device),
    m_cache(),
    m_mutex()
{
}

//------------------------------------------------------------------------------
template<typename T>
template<typename... A>
T& quetzal::vulkan::ResourceCache<T>::request(A&... args)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return request_resource(nullptr, args...);
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::vulkan::ResourceCache<T>::clear()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_cache.clear();
    return;
}

//------------------------------------------------------------------------------
template<typename T>
template<typename... A>
T& quetzal::vulkan::ResourceCache<T>::request_resource(ResourceRecorder* precorder, A&... args)
{
    std::size_t key{0};
    hash_param(key, args...);

    auto i = m_cache.find(key);
    if (i != m_cache.end())
    {
        return i->second;
    }

    // If we do not have it already, create and cache it
    auto type = typeid(T).name();
    size_t n = m_cache.size();
    //log.debug("Building #{}, cache object type {}", n, type);

    try
    {
        T resource(m_device);
        resource.create(args...);

        auto [j, b] = m_cache.emplace(key, std::move(resource));
        if (!b)
        {
            throw std::runtime_error{std::format("Insertion error at #{}, cache object type {}", n, type)};
        }

        if (precorder)
        {
            //RecordHelper<T, A...> record_helper;
            //size_t index = record_helper.record(*precorder, args...);
            //record_helper.index(*precorder, index, j->second);
        }

        return j->second;
    }
    catch (const std::exception& e)
    {
        e;
        //log.error("Creation error for #{}, cache object type {}", n, type);
        throw;
    }
}

#endif // QUETZAL_VULKAN_RESOURCECACHE_HPP
