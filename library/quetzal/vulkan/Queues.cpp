//------------------------------------------------------------------------------
// quetzal::vulkan
// Queues.cpp
//------------------------------------------------------------------------------

#include "Queues.hpp"
#include "vulkan_util.hpp"
#include <stdexcept>
#include <cassert>

//------------------------------------------------------------------------------
quetzal::vulkan::Queues::Queues(const PhysicalDevice& physical_device, const LogicalDevice& logical_device, const Surface& surface) :
    m_physical_device(physical_device),
    m_logical_device(logical_device),
    m_queues()
{
    auto queue_family_properties = m_physical_device.queue_family_properties();
    m_queues.resize(queue_family_properties.size());

    for (uint32_t family_index = 0; family_index < queue_family_properties.size(); ++family_index)
    {
        const auto& family_properties = queue_family_properties[family_index];
        bool presentable = m_physical_device.queue_family_presentable(family_index, surface);

        for (uint32_t index = 0; index < family_properties.queueCount; ++index)
        {
            m_queues[family_index].emplace_back(m_logical_device, family_index, index, family_properties, presentable);
        }
    }
}

//------------------------------------------------------------------------------
const quetzal::vulkan::Queue& quetzal::vulkan::Queues::get(uint32_t family_index, uint32_t index)
{
    return m_queues[family_index][index];
}

//------------------------------------------------------------------------------
const quetzal::vulkan::Queue& quetzal::vulkan::Queues::get(VkQueueFlags required_flags, uint32_t index) const
{
    const auto& family_properties = m_physical_device.queue_family_properties();
    for (uint32_t family_index = 0U; family_index < family_properties.size(); ++family_index)
    {
        const auto& properties = family_properties[family_index];

        VkQueueFlags flags = properties.queueFlags;
        uint32_t count = properties.queueCount;

        if (((flags & required_flags) == required_flags) && index < count)
        {
            return m_queues[family_index][index];
        }
    }

    throw std::runtime_error("Queue not found");
}

//------------------------------------------------------------------------------
const quetzal::vulkan::Queue& quetzal::vulkan::Queues::get_presentable_queue(uint32_t index) const
{
    for (uint32_t family_index = 0U; family_index < m_queues.size(); ++family_index)
    {
        const Queue& queue = m_queues[family_index][0];
        if (queue.presentable() && index < queue.properties().queueCount)
        {
            return m_queues[family_index][index];
        }
    }

    throw std::runtime_error("Queue not found");
}

//------------------------------------------------------------------------------
const quetzal::vulkan::Queue& quetzal::vulkan::Queues::get_suitable_graphics_queue() const
{
    for (uint32_t family_index = 0U; family_index < m_queues.size(); ++family_index)
    {
        const Queue& queue = m_queues[family_index][0];
        if (queue.presentable() && 0 < queue.properties().queueCount)
        {
            return m_queues[family_index][0];
        }
    }

    return get(VK_QUEUE_GRAPHICS_BIT, 0);
}

//------------------------------------------------------------------------------
uint32_t quetzal::vulkan::Queues::family_queue_count(uint32_t family_index)
{
    const auto& family_properties = m_physical_device.queue_family_properties();
    return family_properties[family_index].queueCount;
}

//------------------------------------------------------------------------------
uint32_t quetzal::vulkan::Queues::family_index(VkQueueFlagBits flags)
{
    const auto& family_properties = m_physical_device.queue_family_properties();

    // Dedicated compute queue
    // Try to find a queue family that supports compute but not graphics
    if (flags & VK_QUEUE_COMPUTE_BIT)
    {
        for (uint32_t family_index = 0; family_index < to_uint32(family_properties.size()); ++family_index)
        {
            VkQueueFlags family_flags = family_properties[family_index].queueFlags;
            if ((family_flags & flags) && !(family_flags & VK_QUEUE_GRAPHICS_BIT))
            {
                return family_index;
            }
        }
    }

    // Dedicated transfer queue
    // Try to find a queue family that supports transfer but neither graphics nor compute
    if (flags & VK_QUEUE_TRANSFER_BIT)
    {
        for (uint32_t family_index = 0; family_index < to_uint32(family_properties.size()); ++family_index)
        {
            VkQueueFlags family_flags = family_properties[family_index].queueFlags;
            if ((family_flags & flags) && !(family_flags & VK_QUEUE_GRAPHICS_BIT) && !(family_flags & VK_QUEUE_COMPUTE_BIT))
            {
                return family_index;
            }
        }
    }

    // For other queue types or if no dedicated queue is present, return the first one that supports the requested flags
    for (uint32_t family_index = 0; family_index < to_uint32(family_properties.size()); ++family_index)
    {
        if (family_properties[family_index].queueFlags & flags)
        {
            return family_index;
        }
    }

    throw std::runtime_error("Error finding a queue family for the givern flags");
}
