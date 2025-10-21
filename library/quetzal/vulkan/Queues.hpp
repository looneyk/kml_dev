#if !defined(QUETZAL_VULKAN_QUEUES_HPP)
#define QUETZAL_VULKAN_QUEUES_HPP
//------------------------------------------------------------------------------
// quetzal::vulkan
// Queues.hpp
//------------------------------------------------------------------------------

#include "LogicalDevice.hpp"
#include "PhysicalDevice.hpp"
#include "Queue.hpp"
#include "Surface.hpp"
#include <vulkan/vulkan.hpp>
#include <stdexcept>
#include <cassert>

namespace quetzal::vulkan
{

    //--------------------------------------------------------------------------
    class Queues
    {
    public:

        Queues(const PhysicalDevice& physical_device, const LogicalDevice& logical_device, const Surface& surface);
        Queues(const Queues&) = delete;
        Queues(Queues&&) = delete;
        ~Queues() = default;

        Queues& operator=(const Queues&) = delete;
        Queues& operator=(Queues&&) = delete;

        const Queue& get(uint32_t family_index, uint32_t index);
        const Queue& get(VkQueueFlags required_flags, uint32_t index) const;

        const Queue& get_presentable_queue(uint32_t index) const;
        const Queue& get_suitable_graphics_queue() const;

        uint32_t family_queue_count(uint32_t family_index);
        uint32_t family_index(VkQueueFlagBits flags);

    private:

        const PhysicalDevice& m_physical_device;
        const LogicalDevice& m_logical_device;
        std::vector<std::vector<Queue>> m_queues; // [family_index][index]
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_QUEUES_HPP
