#if !defined(QUETZAL_VULKAN_QUEUE_HPP)
#define QUETZAL_VULKAN_QUEUE_HPP
//------------------------------------------------------------------------------
// quetzal::vulkan
// Queue.hpp
//------------------------------------------------------------------------------

#include "LogicalDevice.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    //--------------------------------------------------------------------------
    class Queue
    {
    public:

        Queue(const LogicalDevice& logical_device, uint32_t family_index, uint32_t index, const VkQueueFamilyProperties& properties, bool presentable);
        Queue(const Queue&) = delete;
        Queue(Queue&& other);
        ~Queue() = default;

        Queue& operator=(const Queue&) = delete;
        Queue& operator=(Queue&&) = delete;

        operator VkQueue() const;

        uint32_t family_index() const;
        uint32_t index() const;

        const VkQueueFamilyProperties& properties() const;
        bool presentable() const;

        void submit(std::span<VkSubmitInfo> submitInfo, VkFence fence) const;
        void submit2(std::span<const VkSubmitInfo2> submit_infos, VkFence fence) const;
        // submit commandbuffer ...
        VkResult present(const VkPresentInfoKHR& present_info) const;
        void wait_idle() const;

    private:

        VkQueue m_handle;
        uint32_t m_family_index;
        uint32_t m_index;
        VkQueueFamilyProperties m_properties;
        bool m_presentable;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_QUEUE_HPP
