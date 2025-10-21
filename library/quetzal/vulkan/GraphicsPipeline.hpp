#if !defined(QUETZAL_VULKAN_GRAPHICSPIPELINE_HPP)
#define QUETZAL_VULKAN_GRAPHICSPIPELINE_HPP

#include "Device.hpp"
#include "GraphicsPipelineCreateInfo.hpp"
#include "vulkan_util.hpp"
#include <vulkan/vulkan.hpp>
#include <format>
#include <span>
#include <stdexcept>
#include <cassert>

namespace quetzal::vulkan
{

    class GraphicsPipeline
    {
    public:

        GraphicsPipeline(const Device& device) :
            m_device(device),
            m_handle(),
            m_pipeline_layout()
        {
        }

        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline(GraphicsPipeline&& other) :
            m_device(other.m_device),
            m_handle(other.m_handle),
            m_pipeline_layout(other.m_pipeline_layout)
        {
            other.m_handle = VK_NULL_HANDLE;
        }

        ~GraphicsPipeline()
        {
            if (m_handle != VK_NULL_HANDLE)
            {
                destroy();
            }
        }

        GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(GraphicsPipeline&&) = delete;

        operator VkPipeline() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

        void create(VkPipelineCache pipeline_cache, const GraphicsPipelineCreateInfo& ci)
        {
            if (m_handle != VK_NULL_HANDLE)
            {
                destroy();
            }

            auto cis = to_span<GraphicsPipelineCreateInfo>(ci);
            VkResult result = vkCreateGraphicsPipelines(m_device, pipeline_cache, to_uint32(cis.size()), cis.data(), nullptr, &m_handle);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error(std::format("Error vkCreateGraphicsPipelines {}", result));
            }

            m_pipeline_layout = ci.pipeline_layout();
            return;
        }

        void destroy()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyPipeline(m_device, m_handle, nullptr);
            m_handle = VK_NULL_HANDLE;
            return;
        }

        VkPipelineLayout pipeline_layout() const
        {
            return m_pipeline_layout;
        }

    private:

        const Device& m_device;
        VkPipeline m_handle;
//        PipelineLayout& m_pipeline_layout;
        VkPipelineLayout m_pipeline_layout; // ...
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_GRAPHICSPIPELINE_HPP
