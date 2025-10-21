#if !defined(QUETZAL_VULKAN_PIPELINELAYOUT_HPP)
#define QUETZAL_VULKAN_PIPELINELAYOUT_HPP
//------------------------------------------------------------------------------
// quetzal::vulkan
// PipelineLayout.hpp
//------------------------------------------------------------------------------

#include "Device.hpp"
#include <vulkan/vulkan.hpp>
#include <span>

namespace quetzal::vulkan
{

    //--------------------------------------------------------------------------
    class PipelineLayout
    {
    public:

        PipelineLayout(const Device& device);
        PipelineLayout(const PipelineLayout&) = delete;
        PipelineLayout(PipelineLayout&& other);
        ~PipelineLayout();

        PipelineLayout& operator=(const PipelineLayout&) = delete;
        PipelineLayout& operator=(PipelineLayout&&) = delete;

        operator VkPipelineLayout() const;
        VkPipelineLayout handle() const;

        void create(std::span<VkDescriptorSetLayout> set_layouts, std::span<VkPushConstantRange> push_constant_ranges);
        void destroy();

    private:

        const Device& m_device;
        VkPipelineLayout m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_PIPELINELAYOUT_HPP
