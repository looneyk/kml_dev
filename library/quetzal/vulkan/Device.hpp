#if !defined(QUETZAL_VULKAN_DEVICE_HPP)
#define QUETZAL_VULKAN_DEVICE_HPP

#include "Allocator.hpp"
#include "Instance.hpp"
#include "LogicalDevice.hpp"
#include "PhysicalDevice.hpp"
#include "PhysicalDevices.hpp"
#include "Queue.hpp"
#include "Queues.hpp"
#include "Surface.hpp"
#include "Window.hpp"
#include "vulkan_util.hpp"
#include "quetzal/common/Config.hpp"
#include <vulkan/vulkan.hpp>
#include <cassert>

namespace quetzal::vulkan
{

    class Device
    {
    public:

        Device(const Config& config, const Window& window, const std::vector<const char*>& deviceExtensions, const std::vector<const char*>& validationLayers, PhysicalDevice::selector_type selector) :
            m_window(window),
            m_instance(config, required_extensions(m_window, !validationLayers.empty()), validationLayers),
            m_surface(m_instance, m_window),
            m_physicalDevices(m_instance),
            m_physicalDevice(m_physicalDevices, m_surface, selector),
            m_logicalDevice(m_physicalDevice, deviceExtensions),
            m_allocator(m_instance, m_physicalDevice, m_logicalDevice),
            m_queues(m_physicalDevice, m_logicalDevice, m_surface)
        {
        }

        ~Device() = default;

        operator VkDevice() const
        {
            return m_logicalDevice;
        }

        const Window& window() const
        {
            return m_window;
        }

        const Instance& instance() const
        {
            return m_instance;
        }

        const Surface& surface() const
        {
            return m_surface;
        }

        const PhysicalDevice& physical_device() const
        {
            return m_physicalDevice;
        }

        const LogicalDevice& logicalDevice() const // ? ...
        {
            return m_logicalDevice;
        }

        const Allocator& allocator() const
        {
            return m_allocator;
        }

        const Queues& queues() const
        {
            return m_queues;
        }

        void wait_idle() const
        {
            vkDeviceWaitIdle(m_logicalDevice);
            return;
        }

    private:

        std::vector<const char*> required_extensions(const Window& window, bool validation)
        {
            auto extensions = window.required_extensions();

            if (validation)
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            return extensions;
        }

        const Window& m_window;
        Instance m_instance;
        Surface m_surface;
        PhysicalDevices m_physicalDevices;
        PhysicalDevice m_physicalDevice;
        LogicalDevice m_logicalDevice;
        Allocator m_allocator;
        Queues m_queues;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_DEVICE_HPP
