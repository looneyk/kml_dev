#if !defined(QUETZAL_VULKAN_VULKAN_UTIL_HPP)
#define QUETZAL_VULKAN_VULKAN_UTIL_HPP

#include <format>
#include <cstdint>

namespace quetzal::vulkan
{

    //--------------------------------------------------------------------------
    template<typename T>
    uint32_t to_uint32(T t)
    {
        return static_cast<uint32_t>(t);
    }

    //--------------------------------------------------------------------------
    template<typename T>
    std::span<const typename T::vk_type> to_span(const T& t)
    {
        return std::span{t.data(), 1};
    }

    //--------------------------------------------------------------------------
    template <typename E> requires std::is_enum_v<E>
    struct std::formatter<E> : std::formatter<std::underlying_type_t<E>>
    {
        auto format(const E& e, format_context& ctx) const
        {
            return std::formatter<std::underlying_type_t<E>>::format(static_cast<std::underlying_type_t<E>>(e), ctx);
        }
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_VULKAN_UTIL_HPP
