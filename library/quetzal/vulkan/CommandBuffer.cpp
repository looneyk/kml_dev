
#include "CommandBuffer.hpp"
#include "vulkan_util.hpp"
#include <stdexcept>

//------------------------------------------------------------------------------
quetzal::vulkan::CommandBuffer::CommandBuffer(const Device& device, const CommandPool& command_pool) :
    m_device(device),
    m_command_pool(command_pool),
    m_handle(VK_NULL_HANDLE)
{
}

//------------------------------------------------------------------------------
quetzal::vulkan::CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept :
    m_device(other.m_device),
    m_command_pool(other.m_command_pool),
    m_handle(other.m_handle)
{
    other.m_handle = VK_NULL_HANDLE;
}

//------------------------------------------------------------------------------
quetzal::vulkan::CommandBuffer::operator VkCommandBuffer() const
{
    assert(m_handle != VK_NULL_HANDLE);
    return m_handle;
}

//------------------------------------------------------------------------------
VkCommandBuffer quetzal::vulkan::CommandBuffer::handle() const
{
    assert(m_handle != VK_NULL_HANDLE);
    return m_handle;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::CommandBuffer::create()
{
    VkCommandBufferAllocateInfo allocate_info = vk_command_buffer_allocate_info(m_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VkResult result = vkAllocateCommandBuffers(m_device, &allocate_info, &m_handle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(std::format("Error vkAllocateCommandBuffers {}", result));
    }
}

//------------------------------------------------------------------------------
void quetzal::vulkan::CommandBuffer::destroy()
{
    if (m_handle != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(m_device, m_command_pool, 1, &m_handle);
        m_handle = VK_NULL_HANDLE;
    }
}

//------------------------------------------------------------------------------
void quetzal::vulkan::CommandBuffer::begin(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo begin_info = vk_command_buffer_begin_info(flags);
    VkResult result = vkBeginCommandBuffer(m_handle, &begin_info);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(std::format("Error vkBeginCommandBuffer {}", result));
    }
    return;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::CommandBuffer::end()
{
    VkResult result = vkEndCommandBuffer(m_handle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(std::format("Error vkEndCommandBuffer {}", result));
    }
    return;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::CommandBuffer::begin_render_pass(const RenderPass& render_pass, const Framebuffer& framebuffer, VkExtent2D extent, std::span<VkClearValue> clear_values, VkSubpassContents subpass_contents)
{
    VkRenderPassBeginInfo begin_info = vk_render_pass_begin_info(render_pass, framebuffer, extent, clear_values);
    vkCmdBeginRenderPass(m_handle, &begin_info, subpass_contents);
    return;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::CommandBuffer::end_render_pass()
{
    vkCmdEndRenderPass(m_handle);
    return;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::CommandBuffer::set_viewport(uint32_t first, std::span<VkViewport> viewports)
{
    vkCmdSetViewport(m_handle, first, to_uint32(viewports.size()), viewports.data());
    return;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::CommandBuffer::set_viewport(VkExtent2D extent)
{
    VkViewport viewport{0.0f, 0.0f, float(extent.width), float(extent.height), 0.0f, 1.0f};
    set_viewport(0, {&viewport, 1});
    return;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::CommandBuffer::set_scissor(uint32_t first, std::span<VkRect2D> scissors)
{
    vkCmdSetScissor(m_handle, first, to_uint32(scissors.size()), scissors.data());
    return;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::CommandBuffer::set_scissor(VkExtent2D extent, VkOffset2D offset)
{
    VkRect2D rect{offset, extent};
    set_scissor(0, {&rect, 1});
    return;
}
