#if !defined(QUETZAL_VULKAN_WINDOW_HPP)
#define QUETZAL_VULKAN_WINDOW_HPP
//------------------------------------------------------------------------------
// quetzal vulkan
// Window.hpp
//------------------------------------------------------------------------------

#include <vulkan/vulkan.hpp> // must preceed GLFW include in game/Window.hpp
#include "quetzal/common/Config.hpp"
#include "quetzal/game/Window.hpp"

namespace quetzal::vulkan
{

    class Window : public game::Window
    {
    public:

        using position_type = game::Window::position_type;
        using size_type = game::Window::size_type;
        using cursor_position_type = game::Window::cursor_position_type;

        explicit Window(const Config& config);
        Window(const Window&) = delete;
        Window(Window&&) = delete;

        virtual ~Window() = default;

        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&) = delete;

        VkExtent2D extent() const;

        VkSurfaceKHR create_surface(VkInstance instance) const; // Instance parameter, Surface return value? ...

        std::vector<const char*> required_extensions() const;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_WINDOW_HPP
