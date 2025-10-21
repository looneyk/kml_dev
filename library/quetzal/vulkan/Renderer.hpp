#if !defined(QUETZAL_VULKAN_RENDERER_HPP)
#define QUETZAL_VULKAN_RENDERER_HPP

#include "Buffer.hpp"
#include "CommandBuffers.hpp"
#include "CommandPool.hpp"
#include "Descriptors.hpp"
#include "Device.hpp"
#include "Framebuffers.hpp"
#include "GraphicsPipeline.hpp"
#include "GraphicsPipelineCreateInfo.hpp"
#include "Model.hpp"
#include "ModelCache.hpp"
#include "Node.hpp"
#include "PhysicalDevice.hpp"
#include "PipelineLayout.hpp"
#include "PipelineManager.hpp"
#include "RenderPass.hpp"
#include "ResourceManager.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "Texture.hpp"
#include "TextureCache.hpp"
#include "UniformBuffers.hpp"
#include "Vertex.hpp"
#include "Window.hpp"
#include "initializer.hpp"
#include "quetzal/common/Config.hpp"
#include "quetzal/game/types.hpp"
#include <vulkan/vulkan.hpp>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <vector>
#include <cstdint>
#include <cstdlib>

namespace quetzal::vulkan
{

#ifdef NDEBUG
    const std::vector<const char*> validationLayers = {};
#else
    const std::vector<const char*> validationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };
#endif

    const std::vector<const char*> deviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct UniformBufferObject
    {
        alignas(16) game::matrix_type view_matrix;
        alignas(16) game::matrix_type proj_matrix;
    };

    struct PushConstants
    {
        alignas(16) game::matrix_type model_matrix;
    };

    class Renderer
    {
    public:

        Renderer(const Config& config, const Window& window) :
            m_device(config, window, deviceExtensions, validationLayers, physical_device_selector),
            m_resource_manager(m_device),
            m_pipelineManager(m_device, "."),
            m_swap_frame_count(swap_frame_count_(config, 2)),
            m_swapchain(m_device, window),
            m_framebuffers(m_device),
            m_renderPass(m_device),
            m_descriptorAllocator(m_device),
            m_descriptorLayoutCache(m_device),
            m_descriptorSetLayouts(),
            m_descriptorSets(),
            m_commandPool(m_device),
            m_model_cache(m_device, ".", m_commandPool),
            m_texture_cache(m_device, ".", m_commandPool, m_device.queues().get(VK_QUEUE_GRAPHICS_BIT, 0)),
            m_uniform_buffers(m_device, m_swap_frame_count),
            m_commandBuffers(m_device, m_commandPool),
            m_imageAvailableSemaphores(),
            m_renderFinishedSemaphores(),
            m_inFlightFences(),
            m_currentFrame(0),
            m_framebufferResized(false),
            m_nodes(),
            m_ubo()
        {
        }

        ~Renderer() = default;

        void create()
        {
            m_msaaSamples = m_device.physical_device().max_sample_count(); // clamped config? ...

            m_swapchain.create(m_device.window().extent(), to_uint32(m_swap_frame_count)); // no provision for handling change to m_swap_frame_count ...
            m_renderPass.create(m_swapchain.image_format(), m_swapchain.depth_format(), m_msaaSamples);
            m_framebuffers.create(m_swapchain.extent(), m_swapchain.image_format(), m_swapchain.depth_format(), m_swapchain.image_views(), m_msaaSamples, m_renderPass);

            m_commandPool.create(m_device.queues().get(VK_QUEUE_GRAPHICS_BIT, 0).family_index());
            m_commandBuffers.create(m_swap_frame_count);
            createSyncObjects();
        }

        void destroy()
        {
            m_device.wait_idle();

            cleanup_swapchain();

            m_renderPass.destroy();

            m_descriptorLayoutCache.clear();
            m_descriptorAllocator.clear();

            m_texture_cache.clear();
            m_model_cache.clear();

            for (size_t i = 0; i < m_swap_frame_count; ++i)
            {
                vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
            }

            m_commandPool.destroy();
        }

        size_t swap_frame_count() const
        {
            return m_swap_frame_count;
        }

        void clear()
        {
            m_nodes.clear();
            return;
        }

        // really should set entire new scene, not by object ...
        // swap new in between frames ...
        template<typename V>
        void insert(const std::string& id, const std::filesystem::path& filepath_model, const std::filesystem::path& filepath_texture, const game::matrix_type& matrix)
        {
            m_model_cache.load<V>(filepath_model.string(), filepath_model);
            auto texture = m_texture_cache.load(filepath_texture.string(), filepath_texture);
            // create ...
            // for each swap frame
            // n? descriptor set layout
            // n descriptor set
            // pipeline layout
            // pipeline
            std::vector<std::tuple<VkDescriptorSet, VkDescriptorSetLayout>> descriptors = descriptor_sets(m_uniform_buffers, texture);
            assert(descriptors.size() == m_swap_frame_count);

            std::vector<VkDescriptorSetLayout> descriptor_set_layouts(m_swap_frame_count);
            for (const auto& e : std::views::zip(descriptors, descriptor_set_layouts))
            {
                get<1>(e) = get<1>(get<0>(e));
            }

            std::vector<VkDescriptorSet> descriptor_sets(m_swap_frame_count);
            for (const auto& e : std::views::zip(descriptors, descriptor_sets))
            {
                get<1>(e) = get<0>(get<0>(e));
            }

            VkPushConstantRange push_constant_range = vk_push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants));
            const auto& pipeline_layout = m_resource_manager.request_pipeline_layout(descriptor_set_layouts, {&push_constant_range, 1});

            GraphicsPipelineCreateInfo ci = {m_device, "shaders/vert_pct_pc.spv", "shaders/frag_pct.spv", V::binding_description(), V::attribute_descriptions(), pipeline_layout, m_renderPass, m_msaaSamples};
            VkPipeline pipeline = m_pipelineManager.request_graphics_pipeline(ci);
            m_nodes.emplace_back(id, filepath_model.string(), filepath_texture.string(), matrix, descriptor_sets, pipeline_layout, pipeline); // the rest ...
            return;
        }

        void update_viewpoint(const game::matrix_type& view_matrix, const game::matrix_type& proj_matrix)
        {
            m_ubo.view_matrix = view_matrix;
            m_ubo.proj_matrix = proj_matrix;
        }

        void update_model_instance(const std::string& id, const game::matrix_type& matrix)
        {
            size_t i = stoul(id); // map ...
            m_nodes[i].m_matrix *= matrix;
        }

        void draw()
        {
            vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

            auto [result, image_index] = m_swapchain.acquire_next_image(m_imageAvailableSemaphores[m_currentFrame]);
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                recreate_swapchain();
                return;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            {
                throw std::runtime_error(std::format("Error vkAcquireNextImageKHR {}", result));
            }

            m_uniform_buffers.set(m_currentFrame, m_ubo);

            CommandBuffer& command_buffer = m_commandBuffers[m_currentFrame];
            vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
            vkResetCommandBuffer(command_buffer, /*VkCommandBufferResetFlagBits*/ 0);
            recordCommandBuffer(command_buffer, image_index);

            std::array<const VkSemaphore, 1> waitSemaphores = {m_imageAvailableSemaphores[m_currentFrame]};
            std::array<const VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            std::array<const VkCommandBuffer, 1> command_buffers = {command_buffer};
            std::array<const VkSemaphore, 1> signalSemaphores = {m_renderFinishedSemaphores[m_currentFrame]};
            VkSubmitInfo submit_info = vk_submit_info(waitSemaphores, waitStages, command_buffers, signalSemaphores);

            const Queue& graphics_queue = m_device.queues().get(VK_QUEUE_GRAPHICS_BIT, 0);
            graphics_queue.submit(std::span(&submit_info, 1), m_inFlightFences[m_currentFrame]);

            std::array<const VkSwapchainKHR, 1> swapchains = {m_swapchain};
            VkPresentInfoKHR present_info = vk_present_info(signalSemaphores, swapchains, {&image_index, 1});
            const Queue& present_queue = m_device.queues().get_presentable_queue(0);
            result = present_queue.present(present_info);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
            {
                m_framebufferResized = false;
                recreate_swapchain();
            }
            else if (result != VK_SUCCESS)
            {
                throw std::runtime_error(std::format("Error vkQueuePresentKHR {}", result));
            }

            m_currentFrame = (m_currentFrame + 1) % m_swap_frame_count;
        }

        VkExtent2D extent() const
        {
            return m_swapchain.extent();
        }

        void set_resized()
        {
            m_framebufferResized = true;
        }

    private:

        size_t swap_frame_count_(const Config& config, size_t n_default = 2)
        {
            size_t n = static_cast<size_t>(::strtoul(config["swap_frame_count"].c_str(), nullptr, 10));
            return n > 0 ? n : n_default;
        }

        void cleanup_swapchain()
        {
            m_framebuffers.destroy();
            m_swapchain.destroy();
        }

        void recreate_swapchain()
        {
            VkExtent2D extent{};
            do
            {
                extent = m_device.window().extent();
                m_device.window().wait_events();
            } while (extent.width == 0 || extent.height == 0);

            m_device.wait_idle();
            cleanup_swapchain();

            m_swapchain.create(extent, to_uint32(m_swap_frame_count));
            m_framebuffers.create(m_swapchain.extent(), m_swapchain.image_format(), m_swapchain.depth_format(), m_swapchain.image_views(), m_msaaSamples, m_renderPass);
        }

        std::vector<std::tuple<VkDescriptorSet, VkDescriptorSetLayout>> descriptor_sets(std::span<const Buffer> uniform_buffers, const Texture& texture)
        {
            std::vector<std::tuple<VkDescriptorSet, VkDescriptorSetLayout>> descriptors(m_swap_frame_count);

            for (size_t i = 0; i < m_swap_frame_count; ++i)
            {
                createDescriptorSet(uniform_buffers[i], texture, get<0>(descriptors[i]), get<1>(descriptors[i]));
            }

            return descriptors;
        }

        void createDescriptorSet(const Buffer& buffer, const Texture& texture, VkDescriptorSet& descriptorSet, VkDescriptorSetLayout& descriptorSetLayout)
        {
            VkDescriptorBufferInfo buffer_info = buffer.descriptor();
            VkDescriptorImageInfo image_info = texture.descriptor();

            auto builder = DescriptorBuilder::begin(&m_descriptorAllocator, &m_descriptorLayoutCache);
            builder.bind(0, buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
            builder.bind(1, image_info, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
            builder.build(descriptorSet, descriptorSetLayout);
        }

        void recordCommandBuffer(CommandBuffer& commandBuffer, uint32_t imageIndex)
        {
            commandBuffer.begin();

            auto extent = m_swapchain.extent();

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValues[1].depthStencil = {1.0f, 0};

            commandBuffer.begin_render_pass(m_renderPass, m_framebuffers[imageIndex], extent, clearValues);
            commandBuffer.set_viewport(extent);
            commandBuffer.set_scissor(extent);

            for (const auto& node : m_nodes)
            {
                auto& descriptorSets = node.m_descriptor_sets;
                auto& pipeline_layout = node.m_pipeline_layout;
                auto& graphics_pipeline = node.m_graphics_pipeline;
                const auto& geometry = m_model_cache.get(node.m_model_id);
                PushConstants pcs;
                pcs.model_matrix = node.m_matrix;

                VkBuffer vertexBuffer = geometry.vertex_buffer();
                VkBuffer indexBuffer = geometry.index_buffer();
                uint32_t start = 0;
                uint32_t count = to_uint32(geometry.index_count());

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptorSets[m_currentFrame], 0, nullptr);
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

                VkBuffer vertexBuffers[] = {vertexBuffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdPushConstants(commandBuffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pcs);
                vkCmdDrawIndexed(commandBuffer, count, 1, start, 0, 0);
            }

            commandBuffer.end_render_pass();
            commandBuffer.end();
        }

        void createSyncObjects()
        {
            m_imageAvailableSemaphores.resize(m_swap_frame_count);
            m_renderFinishedSemaphores.resize(m_swap_frame_count);
            m_inFlightFences.resize(m_swap_frame_count);

            VkSemaphoreCreateInfo semaphore_ci = vk_semaphore_create_info();
            VkFenceCreateInfo fence_ci = vk_fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

            for (size_t i = 0; i < m_swap_frame_count; ++i)
            {
                if (vkCreateSemaphore(m_device, &semaphore_ci, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                    vkCreateSemaphore(m_device, &semaphore_ci, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
                    vkCreateFence(m_device, &fence_ci, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
                {
                    throw std::runtime_error("Error failed to create synchronization objects");
                }
            }
        }

        static bool physical_device_selector(const PhysicalDevice& physical_device, const Surface& surface)
        {
            bool extensionsSupported = physical_device.check_extension_support(deviceExtensions);

            SurfaceSupport surface_support = physical_device.surface_support(surface);
            bool swapchainAdequate = !surface_support.formats.empty() && !surface_support.present_modes.empty();

            VkPhysicalDeviceFeatures supportedFeatures;
            vkGetPhysicalDeviceFeatures(physical_device, &supportedFeatures);

            return physical_device.queue_family_presentable(surface) && extensionsSupported && swapchainAdequate && supportedFeatures.samplerAnisotropy;
        }

        Device m_device;
        ResourceManager m_resource_manager;
        PipelineManager m_pipelineManager;

        size_t m_swap_frame_count;
        VkSampleCountFlagBits m_msaaSamples;

        Swapchain m_swapchain;
        Framebuffers m_framebuffers;

        RenderPass m_renderPass;

        DescriptorAllocator m_descriptorAllocator;
        DescriptorLayoutCache m_descriptorLayoutCache;

        std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
        std::vector<std::vector<VkDescriptorSet>> m_descriptorSets; // [model_id][swap_frame]

        CommandPool m_commandPool;

        ModelCache m_model_cache;
        TextureCache m_texture_cache;

        UniformBuffers<UniformBufferObject> m_uniform_buffers;

        std::vector<Node> m_nodes;

        CommandBuffers m_commandBuffers;

        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_inFlightFences;
        uint32_t m_currentFrame = 0;

        bool m_framebufferResized = false;

        UniformBufferObject m_ubo;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_RENDERER_HPP
