#if !defined(QUETZAL_VULKAN_CONTEXT_HPP)
#define QUETZAL_VULKAN_CONTEXT_HPP

#include "DebugMessenger.hpp"
#include "Instance.hpp"
#include "PhysicalDevices.hpp"
#include "Window.hpp"
#include "quetzal/common/Config.hpp"
#include <vulkan/vulkan.hpp>
#include <vector>

namespace quetzal::vulkan
{

    class Context
    {
    public:

        Context(const Config& config, const Window& window, const std::vector<const char*>& layers) :
            m_instance(config, required_extensions(window, !layers.empty()), layers),
            m_debug_messenger(m_instance, !layers.empty()),
            m_physical_devices(m_instance)
        {
        }

        const Instance& instance() const
        {
            return m_instance;
        }

        const PhysicalDevices& physical_devices() const
        {
            return m_physical_devices;
        }

    private:

        static std::vector<const char*> required_extensions(const Window& window, bool validation)
        {
            auto extensions = window.required_extensions();
            if (validation)
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            return extensions;
        }

        Instance m_instance;
        DebugMessenger m_debug_messenger;
        PhysicalDevices m_physical_devices;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_CONTEXT_HPP
