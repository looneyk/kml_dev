//------------------------------------------------------------------------------
// quetzal vulkan
// Window.cpp
//------------------------------------------------------------------------------

#include "Window.hpp"
#include "VulkanError.hpp"
#include "vulkan_util.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>

//------------------------------------------------------------------------------
quetzal::vulkan::Window::Window(const Config& config) :
    game::Window(config)
{
    create_windowed(config["title"], {}, { stoi(config["width"]), stoi(config["height"]) });
    // create(mode(config["mode"]), config["title"], {}, {stoi(config["width"]), stoi(config["height"])}); // and other parameters ...

    if (config["cursor"] == "disabled")
    {
        glfwSetInputMode(m_pwindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

//------------------------------------------------------------------------------
VkExtent2D quetzal::vulkan::Window::extent() const
{
    auto size = framebuffer_size();
    return {uint32_t(size.x), uint32_t(size.y)};
}

//------------------------------------------------------------------------------
VkSurfaceKHR quetzal::vulkan::Window::create_surface(VkInstance instance) const
{
    VkSurfaceKHR handle = VK_NULL_HANDLE;

    VkResult vk_result = glfwCreateWindowSurface(instance, m_pwindow, nullptr, &handle);
    check_result(vk_result, "glfwCreateWindowSurface");

    return handle;
}

//------------------------------------------------------------------------------
std::vector<const char*> quetzal::vulkan::Window::required_extensions() const
{
    uint32_t count = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    return { glfwExtensions, glfwExtensions + count };
}
