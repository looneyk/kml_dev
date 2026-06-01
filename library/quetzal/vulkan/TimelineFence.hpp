#if !defined(QUETZAL_VULKAN_TIMELINEFENCE_HPP)
#define QUETZAL_VULKAN_TIMELINEFENCE_HPP

#include "Device.hpp"
#include <vulkan/vulkan.hpp>
#include <cstdint>

namespace quetzal::vulkan
{

    class TimelineFence
    {
    public:

        TimelineFence(const Device& device, uint64_t initial_value = 0);
        ~TimelineFence();

        TimelineFence(const TimelineFence&) = delete;
        TimelineFence(TimelineFence&& other) noexcept;

        operator VkFence() const
        {
            return m_handle;
        }

        VkFence handle() const { return m_handle; }

        uint64_t counter_value() const;
        void wait(uint64_t value, uint64_t timeout = UINT64_MAX) const;
        void reset();

    private:

        const Device& m_device;
        VkFence m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_TIMELINEFENCE_HPP
