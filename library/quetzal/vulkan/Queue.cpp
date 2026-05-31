//------------------------------------------------------------------------------
// quetzal::vulkan
// Queue.cpp
//------------------------------------------------------------------------------

#include "Queue.hpp"
#include "VulkanError.hpp"
#include "vulkan_util.hpp"

//------------------------------------------------------------------------------
quetzal::vulkan::Queue::Queue(const LogicalDevice& logical_device, uint32_t family_index, uint32_t index, const VkQueueFamilyProperties& properties, bool presentable) :
    m_handle(VK_NULL_HANDLE),
    m_family_index(family_index),
    m_index(index),
    m_properties(properties),
    m_presentable(presentable)
{
    vkGetDeviceQueue(logical_device, m_family_index, m_index, &m_handle);
}

//------------------------------------------------------------------------------
quetzal::vulkan::Queue::Queue(Queue&& other) :
    m_handle(other.m_handle),
    m_family_index(other.m_family_index),
    m_index(other.m_index),
    m_properties(other.m_properties),
    m_presentable(other.m_presentable)
{
}

//------------------------------------------------------------------------------
quetzal::vulkan::Queue::operator VkQueue() const
{
    assert(m_handle != VK_NULL_HANDLE);
    return m_handle;
}

//------------------------------------------------------------------------------
uint32_t quetzal::vulkan::Queue::family_index() const
{
    return m_family_index;
}

//------------------------------------------------------------------------------
uint32_t quetzal::vulkan::Queue::index() const
{
    return m_index;
}

//------------------------------------------------------------------------------
const VkQueueFamilyProperties& quetzal::vulkan::Queue::properties() const
{
    return m_properties;
}

//------------------------------------------------------------------------------
bool quetzal::vulkan::Queue::presentable() const
{
    return m_presentable;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::Queue::submit(std::span<VkSubmitInfo> submit_infos, VkFence fence) const
{
    VkResult vk_result = vkQueueSubmit(m_handle, to_uint32(submit_infos.size()), submit_infos.data(), fence);
    check_result(vk_result, "vkQueueSubmit");
    return;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::Queue::submit2(std::span<const VkSubmitInfo2> submit_infos, VkFence fence) const
{
    VkResult vk_result = vkQueueSubmit2(m_handle, to_uint32(submit_infos.size()), submit_infos.data(), fence);
    check_result(vk_result, "vkQueueSubmit2");
    return;
}

//------------------------------------------------------------------------------
VkResult quetzal::vulkan::Queue::present(const VkPresentInfoKHR& present_info) const
{
    if (!presentable())
    {
        return VK_ERROR_INCOMPATIBLE_DISPLAY_KHR;
    }

    return vkQueuePresentKHR(m_handle, &present_info);
}

//------------------------------------------------------------------------------
void quetzal::vulkan::Queue::wait_idle() const
{
    vkQueueWaitIdle(m_handle);
    return;
}
