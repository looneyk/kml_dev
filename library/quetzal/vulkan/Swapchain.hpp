#if !defined(QUETZAL_VULKAN_SWAPCHAIN_HPP)
#define QUETZAL_VULKAN_SWAPCHAIN_HPP
//------------------------------------------------------------------------------
// quetzal::vulkan
// Swapchain.hpp
//------------------------------------------------------------------------------

#include "Device.hpp"
//#include "Image.hpp"
#include "ImageView.hpp"
#include "Window.hpp"
#include <vulkan/vulkan.hpp>
#include <tuple>

namespace quetzal::vulkan
{

    class Swapchain
    {
    public:

        Swapchain(const Device& device, const Window& window);
        Swapchain(const Swapchain&) = delete;
        Swapchain(Swapchain&&) = delete;
        ~Swapchain() = default;

        Swapchain& operator=(const Swapchain&) = delete;
        Swapchain& operator=(Swapchain&&) = delete;

        operator VkSwapchainKHR() const;

        void create(VkExtent2D extent, uint32_t image_count);
        void destroy();
        void recreate();

        VkExtent2D extent() const;
        VkFormat image_format() const;
        VkFormat depth_format() const;

        const std::vector<VkImage>& images() const;
        const std::vector<ImageView>& image_views() const;

        std::tuple<VkResult, uint32_t> acquire_next_image(VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE);

    private:

        void create_image_views();

        const Device& m_device;
        const Window& m_window;
        VkSwapchainKHR m_handle;

        VkExtent2D m_extent;
        VkFormat m_image_format;
        uint32_t m_image_count;

        std::vector<VkImage> m_images; // Not Image, managed by swapchain
        std::vector<ImageView> m_imageViews;

        std::vector<VkPresentModeKHR> present_mode_priority_list = {
            VK_PRESENT_MODE_FIFO_KHR,
            VK_PRESENT_MODE_MAILBOX_KHR
        };

        std::vector<VkSurfaceFormatKHR> surface_format_priority_list = {
            {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
            {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}};
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_SWAPCHAIN_HPP
