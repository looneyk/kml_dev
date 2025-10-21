#if !defined(QUETZAL_VULKAN_SURFACE_HPP)
#define QUETZAL_VULKAN_SURFACE_HPP

#include <vulkan/vulkan.hpp>
#include "Instance.hpp"
#include "Window.hpp"

namespace quetzal::vulkan
{

    class Surface
    {
    public:

        Surface(const Instance& instance, const Window& window) :
            m_instance(instance),
            m_handle(window.create_surface(m_instance))
        {
        }

        ~Surface()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroySurfaceKHR(m_instance, m_handle, nullptr);
        }

        operator VkSurfaceKHR() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

    private:

        const Instance& m_instance;
        VkSurfaceKHR m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_SURFACE_HPP
