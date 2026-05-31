//------------------------------------------------------------------------------
// quetzal::vulkan
// Swapchain.cpp
//------------------------------------------------------------------------------

#include "Swapchain.hpp"
#include "VulkanError.hpp"
#include "vulkan_util.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <assert.h>

namespace
{

    //--------------------------------------------------------------------------
    bool validate_format_feature(VkImageUsageFlagBits image_usage_flags, VkFormatFeatureFlags format_feature_flags)
    {
        return image_usage_flags != VK_IMAGE_USAGE_STORAGE_BIT || (format_feature_flags & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
    }

    //--------------------------------------------------------------------------
    VkCompositeAlphaFlagBitsKHR select_composite_alpha(VkCompositeAlphaFlagBitsKHR composite_alpha, const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (composite_alpha & capabilities.supportedCompositeAlpha)
        {
            return composite_alpha;
        }

        static const std::vector<VkCompositeAlphaFlagBitsKHR> composite_alpha_flag_bits =
        {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        };

        for (VkCompositeAlphaFlagBitsKHR composite_alpha_flag : composite_alpha_flag_bits)
        {
            if (composite_alpha_flag & capabilities.supportedCompositeAlpha)
            {
                // log ...
                return composite_alpha_flag;
            }
        }

        throw std::runtime_error("No compatible composite alpha found.");
    }

    //--------------------------------------------------------------------------
    VkExtent2D select_extent(VkExtent2D extent, const VkSurfaceCapabilitiesKHR& capabilities)
    {
        bool invalid_requested_extent = extent.width < 1 || extent.height < 1;
        bool invalid_current_extent = capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max();
        if (invalid_requested_extent && invalid_current_extent)
        {
            throw std::runtime_error("No extent available.");
        }

        if (invalid_requested_extent)
        {
            // log ...
            extent = capabilities.currentExtent;
        }

        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return extent;
    }

    //--------------------------------------------------------------------------
    uint32_t select_image_array_layers(uint32_t image_array_layers, const VkSurfaceCapabilitiesKHR& capabilities)
    {
        return std::clamp(image_array_layers, 1u, capabilities.maxImageArrayLayers);
    }

    //--------------------------------------------------------------------------
    uint32_t select_image_count(uint32_t image_count, const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.maxImageCount == 0)
        {
            return std::max(image_count, capabilities.minImageCount);
        }

        return std::clamp(image_count, capabilities.minImageCount, capabilities.maxImageCount);
    }

    //--------------------------------------------------------------------------
    std::set<VkImageUsageFlagBits> select_image_usage(const std::set<VkImageUsageFlagBits>& image_usage_flags, const VkSurfaceCapabilitiesKHR& capabilities, VkFormatFeatureFlags format_feature_flags)
    {
        std::set<VkImageUsageFlagBits> validated_image_usage_flags;
        for (auto flag : image_usage_flags)
        {
            if ((flag & capabilities.supportedUsageFlags) && validate_format_feature(flag, format_feature_flags))
            {
                validated_image_usage_flags.insert(flag);
            }
            else
            {
                // log requested but not supported ...
            }
        }

        if (validated_image_usage_flags.empty())
        {
            // Pick the first supported format from list of defaults
            static const std::vector<VkImageUsageFlagBits> image_usage_flag_bits =
            {
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_IMAGE_USAGE_STORAGE_BIT,
                VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT
            };

            for (VkImageUsageFlagBits image_usage_flag : image_usage_flag_bits)
            {
                if ((image_usage_flag & capabilities.supportedUsageFlags) && validate_format_feature(image_usage_flag, format_feature_flags))
                {
                    validated_image_usage_flags.insert(image_usage_flag);
                    break;
                }
            }
        }

        if (validated_image_usage_flags.empty())
        {
            throw std::runtime_error("No compatible image usage found.");
        }

        std::string usage_list;
        for (VkImageUsageFlagBits image_usage_flag : validated_image_usage_flags)
        {
            usage_list += std::to_string(image_usage_flag) + " ";
        }
        // log usage list ...

        return validated_image_usage_flags;
    }

    //--------------------------------------------------------------------------
    VkPresentModeKHR select_present_mode(VkPresentModeKHR requested_present_mode, const std::vector<VkPresentModeKHR>& available_present_modes, const std::vector<VkPresentModeKHR>& present_mode_priority_list)
    {
        auto i = std::find(available_present_modes.begin(), available_present_modes.end(), requested_present_mode);
        if (i != available_present_modes.end())
        {
            // log "(Swapchain) Present mode selected: {}", to_string(requested_present_mode);
            return *i;
        }

        VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (auto& present_mode : present_mode_priority_list)
        {
            if (std::find(available_present_modes.begin(), available_present_modes.end(), present_mode) != available_present_modes.end())
            {
                selected_present_mode = present_mode;
                break;
            }
        }

        // log "(Swapchain) Present mode '{}' not supported. Selecting '{}'.", to_string(requested_present_mode), to_string(selected_present_mode);
        return selected_present_mode;
    }

    //--------------------------------------------------------------------------
    VkSurfaceFormatKHR select_surface_format(const VkSurfaceFormatKHR requested_surface_format, const std::vector<VkSurfaceFormatKHR>& available_surface_formats, const std::vector<VkSurfaceFormatKHR>& surface_format_priority_list)
    {
        // Try to find the requested surface format in the supported surface formats
        auto i = std::find_if(available_surface_formats.begin(), available_surface_formats.end(),
            [&requested_surface_format](const VkSurfaceFormatKHR& surface) {
                return surface.format == requested_surface_format.format && surface.colorSpace == requested_surface_format.colorSpace;
            });

        if (i != available_surface_formats.end())
        {
            // log "(Swapchain) Surface format selected: {}", std::to_string(requested_surface_format)
            return *i;
        }

        for (auto& surface_format : surface_format_priority_list)
        {
            i = std::find_if(available_surface_formats.begin(), available_surface_formats.end(),
                [&surface_format](const VkSurfaceFormatKHR& surface) {
                    return surface.format == surface_format.format && surface.colorSpace == surface_format.colorSpace;
                });
            if (i != available_surface_formats.end())
            {
                // log "(Swapchain) Surface format ({}) not supported. Selecting ({}).", std::to_string(requested_surface_format), std::to_string(*i)
                return *i;
            }
        }

        // log "(Swapchain) Surface format ({}) not supported. Selecting ({}).", to_string(requested_surface_format), to_string(available_surface_formats.front()
        return available_surface_formats.front();
    }

    //--------------------------------------------------------------------------
    VkSurfaceTransformFlagBitsKHR select_transform(VkSurfaceTransformFlagBitsKHR transform, const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (transform & capabilities.supportedTransforms)
        {
            return transform;
        }

        // log ...
        return capabilities.currentTransform;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::vulkan::Swapchain::Swapchain(const Device& device, const Window& window) :
    m_device(device),
    m_window(window),
    m_handle(VK_NULL_HANDLE),
    m_image_format{},
    m_image_count(2),
    m_extent{window.extent()},
    m_images(),
    m_imageViews()
{
}

//------------------------------------------------------------------------------
quetzal::vulkan::Swapchain::operator VkSwapchainKHR() const
{
    assert(m_handle != VK_NULL_HANDLE);
    return m_handle;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::Swapchain::create(VkExtent2D extent, uint32_t image_count)
{
    SurfaceSupport surface_support = m_device.physical_device().surface_support(m_device.surface());

    m_extent = select_extent(extent, surface_support.capabilities);
    m_image_count = select_image_count(image_count, surface_support.capabilities);
    VkSurfaceFormatKHR surface_format = select_surface_format(surface_format_priority_list[0], surface_support.formats, surface_format_priority_list);
    m_image_format = surface_format.format;

    VkSwapchainCreateInfoKHR ci = vk_swapchain_create_info(m_device.surface(), image_count, extent, surface_format);
    const Queue& graphics_queue = m_device.queues().get(VK_QUEUE_GRAPHICS_BIT, 0);
    const Queue& present_queue = m_device.queues().get_presentable_queue(0);
    uint32_t queueFamilyIndices[] = {present_queue.family_index(), graphics_queue.family_index()};
    if (present_queue.family_index() != graphics_queue.family_index())
    {
        ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    ci.preTransform = select_transform(surface_support.capabilities.currentTransform, surface_support.capabilities);
    ci.compositeAlpha = select_composite_alpha(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, surface_support.capabilities);
    ci.presentMode = select_present_mode(present_mode_priority_list[0], surface_support.present_modes, present_mode_priority_list);
    ci.clipped = VK_TRUE;

    VkResult vk_result = vkCreateSwapchainKHR(m_device, &ci, nullptr, &m_handle);
    check_result(vk_result, "vkCreateSwapchainKHR");

    uint32_t count;
    vkGetSwapchainImagesKHR(m_device, m_handle, &count, nullptr);
    assert(count == m_image_count);
    m_images.resize(count);
    vkGetSwapchainImagesKHR(m_device, m_handle, &count, m_images.data());

    create_image_views();
}

//------------------------------------------------------------------------------
void quetzal::vulkan::Swapchain::destroy()
{
    for (auto imageView : m_imageViews)
    {
        imageView.destroy();
    }

    assert(m_handle != VK_NULL_HANDLE);
    vkDestroySwapchainKHR(m_device, m_handle, nullptr);
}

//------------------------------------------------------------------------------
void quetzal::vulkan::Swapchain::recreate()
{
    VkExtent2D extent{};
    do
    {
        extent = m_window.extent();
        m_window.wait_events();
    } while (extent.width == 0 || extent.height == 0);

    m_device.wait_idle();

    destroy();
    create(extent, m_image_count);
}

//------------------------------------------------------------------------------
VkExtent2D quetzal::vulkan::Swapchain::extent() const
{
    return m_extent;
}

//------------------------------------------------------------------------------
VkFormat quetzal::vulkan::Swapchain::image_format() const
{
    return m_image_format;
}

//------------------------------------------------------------------------------
VkFormat quetzal::vulkan::Swapchain::depth_format() const
{
    return m_device.physical_device().find_depth_format();
}

//------------------------------------------------------------------------------
const std::vector<VkImage>& quetzal::vulkan::Swapchain::images() const
{
    return m_images;
}

//------------------------------------------------------------------------------
const std::vector<quetzal::vulkan::ImageView>& quetzal::vulkan::Swapchain::image_views() const
{
    return m_imageViews;
}

//------------------------------------------------------------------------------
std::tuple<VkResult, uint32_t> quetzal::vulkan::Swapchain::acquire_next_image(VkSemaphore semaphore, VkFence fence)
{
    uint32_t image_index{};
    VkResult vk_result = vkAcquireNextImageKHR(m_device, m_handle, std::numeric_limits<uint64_t>::max(), semaphore, fence, &image_index);
    return {vk_result, image_index};
}

//------------------------------------------------------------------------------
void quetzal::vulkan::Swapchain::create_image_views()
{
    m_imageViews.resize(m_images.size(), ImageView(m_device));
    for (uint32_t i = 0; i < m_images.size(); ++i)
    {
        m_imageViews[i].create(m_images[i], m_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}
