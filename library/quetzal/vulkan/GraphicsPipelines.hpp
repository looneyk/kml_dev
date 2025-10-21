#if !defined(QUETZAL_VULKAN_GRAPHICSPIPELINES_HPP)
#define QUETZAL_VULKAN_GRAPHICSPIPELINES_HPP

#include "Device.hpp"
#include <vulkan/vulkan.hpp>
#include <span>
#include <stdexcept>
#include <vector>

namespace quetzal::vulkan
{

    class GraphicsPipelines
    {
    public:

        GraphicsPipelines(const Device& device) :
            m_device(device),
            m_handles()
        {
        }

        VkPipeline operator[](size_t i)
        {
            assert(i < m_handles.size());
            return m_handles[i];
        }

        size_t size() const
        {
            return m_handles.size();
        }

        const std::vector<VkPipeline>& data() const
        {
            return m_handles;
        }

        void create(VkPipelineCache pipelineCache, std::span<const VkGraphicsPipelineCreateInfo> createInfos)
        {
            destroy();

            m_handles.resize(createInfos.size());
            VkResult result = vkCreateGraphicsPipelines(m_device, pipelineCache, to_uint32(createInfos.size()), createInfos.data(), nullptr, m_handles.data());
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error(std::format("Error vkCreateGraphicsPipelines {}", result));
            }

            return;
        }

        void destroy()
        {
            for (const auto& handle : m_handles)
            {
                assert(handle != VK_NULL_HANDLE);
                vkDestroyPipeline(m_device, handle, nullptr);
            }

            m_handles.resize(0);
            return;
        }

    private:

        const Device& m_device;
        std::vector<VkPipeline> m_handles;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_GRAPHICSPIPELINES_HPP
