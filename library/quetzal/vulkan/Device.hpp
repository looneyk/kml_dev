#if !defined(QUETZAL_VULKAN_DEVICE_HPP)
#define QUETZAL_VULKAN_DEVICE_HPP

#include "Allocator.hpp"
#include "Context.hpp"
#include "LogicalDevice.hpp"
#include "PhysicalDevice.hpp"
#include "Queue.hpp"
#include "Queues.hpp"
#include "Surface.hpp"
#include "Window.hpp"
#include <vulkan/vulkan.hpp>
#include <cassert>

namespace quetzal::vulkan
{

    class Device
    {
    public:

        Device(const Context& context, const Window& window, const std::vector<const char*>& extensions, PhysicalDevice::selector_type selector) :
            m_window(window),
            m_surface(context.instance(), m_window),
            m_physical_device(context.physical_devices(), m_surface, selector),
            m_logical_device(m_physical_device, extensions),
            m_allocator(context.instance(), m_physical_device, m_logical_device),
            m_queues(m_physical_device, m_logical_device, m_surface)
        {
        }

        ~Device() = default;

        operator VkDevice() const
        {
            return m_logical_device;
        }

        const Window& window() const
        {
            return m_window;
        }

        const Surface& surface() const
        {
            return m_surface;
        }

        const PhysicalDevice& physical_device() const
        {
            return m_physical_device;
        }

        const LogicalDevice& logical_device() const
        {
            return m_logical_device;
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
            vkDeviceWaitIdle(m_logical_device);
            return;
        }

    private:

        const Window& m_window;
        Surface m_surface;
        PhysicalDevice m_physical_device;
        LogicalDevice m_logical_device;
        Allocator m_allocator;
        Queues m_queues;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_DEVICE_HPP
