#if !defined(QUETZAL_VULKAN_TIMELINESEMAPHORE_HPP)
#define QUETZAL_VULKAN_TIMELINESEMAPHORE_HPP

#include "Device.hpp"
#include <vulkan/vulkan.hpp>
#include <cstdint>

namespace quetzal::vulkan
{

    class TimelineSemaphore
    {
    public:

        TimelineSemaphore(const Device& device, uint64_t initial_value = 0);
        ~TimelineSemaphore();

        TimelineSemaphore(const TimelineSemaphore&) = delete;
        TimelineSemaphore(TimelineSemaphore&& other) noexcept;

        operator VkSemaphore() const
        {
            return m_handle;
        }

        VkSemaphore handle() const { return m_handle; }

        uint64_t counter_value() const;
        void wait(uint64_t value, uint64_t timeout = UINT64_MAX) const;
        void signal(uint64_t value) const;

    private:

        const Device& m_device;
        VkSemaphore m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_TIMELINESEMAPHORE_HPP
