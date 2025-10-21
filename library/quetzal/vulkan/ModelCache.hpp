#if !defined(QUETZAL_VULKAN_MODELCACHE_HPP)
#define QUETZAL_VULKAN_MODELCACHE_HPP

#include "CommandPool.hpp"
#include "Device.hpp"
#include "Model.hpp"
#include <filesystem>
#include <map>
#include <string>

#include "Vertex.hpp"

namespace quetzal::vulkan
{

    class ModelCache
    {
    public:

        ModelCache(const Device& device, const std::filesystem::path& rootpath, const CommandPool& command_pool) :
            m_device(device),
            m_rootpath(rootpath),
            m_command_pool(command_pool),
            m_cache()
        {
        }

        ~ModelCache() = default;

        template<typename V>
        const Model& load(const std::string& id, const std::filesystem::path& file)
        {
            auto filepath = model_filepath(m_rootpath, file);

            auto [i, b] = m_cache.try_emplace(id, m_device, m_command_pool);
            if (b)
            {
                i->second.load<V>(filepath);
            }
            return i->second;
        }

        const Model& get(const std::string& id)
        {
            auto i = m_cache.find(id);
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

        std::filesystem::path model_filepath(const std::filesystem::path& rootpath, const std::filesystem::path& file)
        {
            return rootpath / "models" / file;
        }

        const Device& m_device;
        std::filesystem::path m_rootpath;
        const CommandPool& m_command_pool;
        std::map<std::string, Model> m_cache;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_MODELCACHE_HPP
