#if !defined(QUETZAL_VULKAN_NODE_HPP)
#define QUETZAL_VULKAN_NODE_HPP

//#include "DescriptorSets.hpp"
#include "GraphicsPipeline.hpp"
#include "GraphicsPipelineCreateInfo.hpp"
#include "quetzal/game/types.hpp"
#include <string>

namespace quetzal::vulkan
{

    class Node
    {
    public:

        Node(const std::string id, const std::string model_id, const std::string texture_id, const game::matrix_type& matrix,
//            const DescriptorSets& descriptor_sets, const GraphicsPipeline& graphics_pipeline
const std::vector<VkDescriptorSet>& descriptor_sets, const PipelineLayout& pipeline_layout, VkPipeline graphics_pipeline
) :
    m_id(id),
            m_model_id(model_id),
            m_texture_id(texture_id),
            m_matrix(matrix),
            m_descriptor_sets(descriptor_sets),
            m_pipeline_layout(pipeline_layout),
            //m_pipeline_ci(),
            m_graphics_pipeline(graphics_pipeline)
        {
        }

        std::string m_id;
        std::string m_model_id;
        std::string m_texture_id;
        game::matrix_type m_matrix;
        //DescriptorSetLayout& m_descriptor_set_layout;
//        const DescriptorSets& m_descriptor_sets;
        std::vector<VkDescriptorSet> m_descriptor_sets;
        const PipelineLayout& m_pipeline_layout;
//        GraphicsPipelineCreateInfo m_pipeline_ci;
//        const GraphicsPipeline& m_graphics_pipeline;
        VkPipeline m_graphics_pipeline;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_NODE_HPP
