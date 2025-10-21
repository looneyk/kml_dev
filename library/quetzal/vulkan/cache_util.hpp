#if !defined(QUETZAL_VULKAN_CACHE_UTIL_HPP)
#define QUETZAL_VULKAN_CACHE_UTIL_HPP

#include "PipelineLayout.hpp"
__pragma(warning(push, 0))
#include <glm/gtx/hash.hpp>
__pragma(warning(pop, 0))
#include <span>

namespace quetzal::vulkan
{

    template<typename T>
    inline void hash_combine(size_t& seed, const T& value)
    {
        std::hash<T> hasher;
        glm::detail::hash_combine(seed, hasher(value));
    }

} // namespace quetzal::vulkan

namespace std
{

    template<>
    struct hash<VkAttachmentDescription>
    {
        size_t operator()(const VkAttachmentDescription& attachment_description) const
        {
            size_t result = 0;
            quetzal::vulkan::hash_combine(result, attachment_description.flags);
            quetzal::vulkan::hash_combine(result, attachment_description.format);
            quetzal::vulkan::hash_combine(result, attachment_description.samples);
            quetzal::vulkan::hash_combine(result, attachment_description.loadOp);
            quetzal::vulkan::hash_combine(result, attachment_description.storeOp);
            quetzal::vulkan::hash_combine(result, attachment_description.stencilLoadOp);
            quetzal::vulkan::hash_combine(result, attachment_description.stencilStoreOp);
            quetzal::vulkan::hash_combine(result, attachment_description.initialLayout);
            quetzal::vulkan::hash_combine(result, attachment_description.finalLayout);
            return result;
        }
    };

    template<>
    struct hash<VkPushConstantRange>
    {
        size_t operator()(const VkPushConstantRange& push_constant_range) const
        {
            size_t result = 0;
            quetzal::vulkan::hash_combine(result, push_constant_range.stageFlags);
            quetzal::vulkan::hash_combine(result, push_constant_range.offset);
            quetzal::vulkan::hash_combine(result, push_constant_range.size);
            return result;
        }
    };

    template<>
    struct hash<VkSubpassDescription>
    {
        size_t operator()(const VkSubpassDescription& subpass_description) const
        {
            size_t result = 0;
            quetzal::vulkan::hash_combine(result, subpass_description.flags);
            quetzal::vulkan::hash_combine(result, subpass_description.pipelineBindPoint);
            quetzal::vulkan::hash_combine(result, subpass_description.inputAttachmentCount);
            quetzal::vulkan::hash_combine(result, subpass_description.pInputAttachments);
            quetzal::vulkan::hash_combine(result, subpass_description.colorAttachmentCount);
            quetzal::vulkan::hash_combine(result, subpass_description.pColorAttachments);
            quetzal::vulkan::hash_combine(result, subpass_description.pResolveAttachments);
            quetzal::vulkan::hash_combine(result, subpass_description.pDepthStencilAttachment);
            quetzal::vulkan::hash_combine(result, subpass_description.preserveAttachmentCount);
            quetzal::vulkan::hash_combine(result, subpass_description.pPreserveAttachments);
            return result;
        }
    };

    template<>
    struct hash<VkSubpassDependency>
    {
        size_t operator()(const VkSubpassDependency& subpass_dependency) const
        {
            size_t result = 0;
            quetzal::vulkan::hash_combine(result, subpass_dependency.srcSubpass);
            quetzal::vulkan::hash_combine(result, subpass_dependency.dstSubpass);
            quetzal::vulkan::hash_combine(result, subpass_dependency.srcStageMask);
            quetzal::vulkan::hash_combine(result, subpass_dependency.dstStageMask);
            quetzal::vulkan::hash_combine(result, subpass_dependency.srcAccessMask);
            quetzal::vulkan::hash_combine(result, subpass_dependency.dstAccessMask);
            quetzal::vulkan::hash_combine(result, subpass_dependency.dependencyFlags);
            return result;
        }
    };

/*
    template<>
    struct hash<VkDescriptorSetLayout>
    {
        size_t operator()(const VkDescriptorSetLayout &descriptor_set_layout) const
        {
            std::size_t result = 0;
            quetzal::vulkan::hash_combine(result, descriptor_set_layout.handle());
            return result;
        }
    };

    template<>
    struct hash<quetzal::vulkan::PipelineLayout>
    {
        size_t operator()(const quetzal::vulkan::PipelineLayout& pipeline_layout) const
        {
            std::size_t result = 0;
            quetzal::vulkan::hash_combine(result, pipeline_layout.handle());
            return result;
        }
    };
*/
} // namespace std

namespace quetzal::vulkan
{

    template<typename T>
    inline void hash_param(size_t& seed, const T& arg)
    {
        hash_combine(seed, arg);
    }

    template<typename T, typename... A>
    inline void hash_param(size_t& seed, const T& arg0, const A&... args)
    {
        hash_param(seed, arg0);
        hash_param(seed, args...);
    }
/* why not? ...
    template<typename T>
    inline void hash_param(size_t& seed, std::span<T> resources)
    {
        for (auto& resource : resources)
        {
            hash_combine(seed, resource);
        }
    }
*/
    template<>
    inline void hash_param<std::span<VkDescriptorSetLayout>>(size_t& seed, const std::span<VkDescriptorSetLayout>& resources)
    {
        for (auto& resource : resources)
        {
            hash_combine(seed, resource);
        }
    }

    template<>
    inline void hash_param<std::span<VkPushConstantRange>>(size_t& seed, const std::span<VkPushConstantRange>& resources)
    {
        for (auto& resource : resources)
        {
            hash_combine(seed, resource);
        }
    }

    template<>
    inline void hash_param<std::span<VkAttachmentDescription>>(size_t& seed, const std::span<VkAttachmentDescription>& resources)
    {
        for (auto& resource : resources)
        {
            hash_combine(seed, resource);
        }
    }

    template<>
    inline void hash_param<std::span<VkSubpassDescription>>(size_t& seed, const std::span<VkSubpassDescription>& resources)
    {
        for (auto& resource : resources)
        {
            hash_combine(seed, resource);
        }
    }

    template<>
    inline void hash_param<std::span<VkSubpassDependency>>(size_t& seed, const std::span<VkSubpassDependency>& resources)
    {
        for (auto& resource : resources)
        {
            hash_combine(seed, resource);
        }
    }

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_CACHE_UTIL_HPP
