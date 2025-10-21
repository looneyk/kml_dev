#if !defined(QUETZAL_VULKAN_COMMANDBUFFER_HPP)
#define QUETZAL_VULKAN_COMMANDBUFFER_HPP

#include "CommandPool.hpp"
#include "Device.hpp"
#include "Framebuffer.hpp"
#include "RenderPass.hpp"
#include <vulkan/vulkan.hpp>
#include <span>

namespace quetzal::vulkan
{

    class CommandBuffer
    {
    public:

        CommandBuffer(const Device& device, const CommandPool& command_pool);
        CommandBuffer(const CommandBuffer&) = default;
        CommandBuffer(CommandBuffer&& other) noexcept;
        ~CommandBuffer() = default;

        CommandBuffer& operator=(const CommandBuffer&) = delete;
        CommandBuffer& operator=(CommandBuffer&&) = delete;

        operator VkCommandBuffer() const;

        VkCommandBuffer handle() const;

        void create();
        void destroy();

        void begin(VkCommandBufferUsageFlags flags = 0);
        void end();

        void begin_render_pass(const RenderPass& render_pass, const Framebuffer& framebuffer, VkExtent2D extent, std::span<VkClearValue> clear_values, VkSubpassContents subpass_contents = VK_SUBPASS_CONTENTS_INLINE);
        void end_render_pass();

        void set_viewport(uint32_t first, std::span<VkViewport> viewports);
        void set_viewport(VkExtent2D extent);

        void set_scissor(uint32_t first, std::span<VkRect2D> scissors);
        void set_scissor(VkExtent2D extent, VkOffset2D offset = {0, 0});

    private:

        const Device& m_device;
        const CommandPool& m_command_pool;
        VkCommandBuffer m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_COMMANDBUFFER_HPP
