#if !defined(QUETZAL_VULKAN_RENDERER_HPP)
#define QUETZAL_VULKAN_RENDERER_HPP

#include "Buffer.hpp"
#include "CommandBuffers.hpp"
#include "CommandPool.hpp"
#include "Context.hpp"
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
#include "VulkanError.hpp"
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

    namespace detail
    {

#ifdef NDEBUG
        const std::vector<const char*> validation_layers = {};
#else
        const std::vector<const char*> validation_layers =
        {
            "VK_LAYER_KHRONOS_validation"
        };
#endif

        const std::vector<const char*> device_extensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

    } // namespace detail

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
            m_context(config, window, detail::validation_layers),
            m_device(m_context, window, detail::device_extensions, physical_device_selector),
            m_resource_manager(m_device),
            m_pipeline_manager(m_device, "."),
            m_swap_frame_count(swap_frame_count_(config, 2)),
            m_swapchain(m_device, window),
            m_framebuffers(m_device),
            m_render_pass(m_device),
            m_descriptor_allocator(m_device),
            m_descriptor_layout_cache(m_device),
            m_descriptor_set_layouts(),
            m_descriptor_sets(),
            m_command_pool(m_device),
            m_model_cache(m_device, ".", m_command_pool),
            m_texture_cache(m_device, ".", m_command_pool, m_device.queues().get(VK_QUEUE_GRAPHICS_BIT, 0)),
            m_uniform_buffers(m_device, m_swap_frame_count),
            m_command_buffers(m_device, m_command_pool),
            m_image_available_semaphores(),
            m_render_finished_semaphores(),
            m_in_flight_fences(),
            m_current_frame(0),
            m_framebuffer_resized(false),
            m_nodes(),
            m_ubo()
        {
        }

        ~Renderer() = default;

        void create()
        {
            m_msaa_samples = m_device.physical_device().max_sample_count(); // clamped config? ...

            m_swapchain.create(m_device.window().extent(), to_uint32(m_swap_frame_count)); // no provision for handling change to m_swap_frame_count ...
            m_render_pass.create(m_swapchain.image_format(), m_swapchain.depth_format(), m_msaa_samples);
            m_framebuffers.create(m_swapchain.extent(), m_swapchain.image_format(), m_swapchain.depth_format(), m_swapchain.image_views(), m_msaa_samples, m_render_pass);

            m_command_pool.create(m_device.queues().get(VK_QUEUE_GRAPHICS_BIT, 0).family_index());
            m_command_buffers.create(m_swap_frame_count);
            create_sync_objects();
        }

        void destroy()
        {
            m_device.wait_idle();

            cleanup_swapchain();

            m_render_pass.destroy();

            m_descriptor_layout_cache.clear();
            m_descriptor_allocator.clear();

            m_texture_cache.clear();
            m_model_cache.clear();

            for (size_t i = 0; i < m_swap_frame_count; ++i)
            {
                vkDestroySemaphore(m_device, m_render_finished_semaphores[i], nullptr);
                vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
                vkDestroyFence(m_device, m_in_flight_fences[i], nullptr);
            }

            m_command_pool.destroy();
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

            GraphicsPipelineCreateInfo ci = {m_device, "shaders/vert_pct_pc.spv", "shaders/frag_pct.spv", V::binding_description(), V::attribute_descriptions(), pipeline_layout, m_render_pass, m_msaa_samples};
            VkPipeline pipeline = m_pipeline_manager.request_graphics_pipeline(ci);
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
            vkWaitForFences(m_device, 1, &m_in_flight_fences[m_current_frame], VK_TRUE, UINT64_MAX);

            auto [result, image_index] = m_swapchain.acquire_next_image(m_image_available_semaphores[m_current_frame]);
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                recreate_swapchain();
                return;
            }
            else if (result != VK_SUBOPTIMAL_KHR)
            {
                check_result(result, "vkAcquireNextImageKHR");
            }

            m_uniform_buffers.set(m_current_frame, m_ubo);

            CommandBuffer& command_buffer = m_command_buffers[m_current_frame];
            vkResetFences(m_device, 1, &m_in_flight_fences[m_current_frame]);
            vkResetCommandBuffer(command_buffer, /*VkCommandBufferResetFlagBits*/ 0);
            record_command_buffer(command_buffer, image_index);

            std::array<const VkSemaphore, 1> wait_semaphores = {m_image_available_semaphores[m_current_frame]};
            std::array<const VkPipelineStageFlags, 1> wait_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            std::array<const VkCommandBuffer, 1> command_buffers = {command_buffer};
            std::array<const VkSemaphore, 1> signal_semaphores = {m_render_finished_semaphores[m_current_frame]};
            VkSubmitInfo submit_info = vk_submit_info(wait_semaphores, wait_stages, command_buffers, signal_semaphores);

            const Queue& graphics_queue = m_device.queues().get(VK_QUEUE_GRAPHICS_BIT, 0);
            graphics_queue.submit(std::span(&submit_info, 1), m_in_flight_fences[m_current_frame]);

            std::array<const VkSwapchainKHR, 1> swapchains = {m_swapchain};
            VkPresentInfoKHR present_info = vk_present_info(signal_semaphores, swapchains, {&image_index, 1});
            const Queue& present_queue = m_device.queues().get_presentable_queue(0);
            result = present_queue.present(present_info);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized)
            {
                m_framebuffer_resized = false;
                recreate_swapchain();
            }
            else
            {
                check_result(result, "vkQueuePresentKHR");
            }

            m_current_frame = (m_current_frame + 1) % m_swap_frame_count;
        }

        VkExtent2D extent() const
        {
            return m_swapchain.extent();
        }

        void set_resized()
        {
            m_framebuffer_resized = true;
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
            m_framebuffers.create(m_swapchain.extent(), m_swapchain.image_format(), m_swapchain.depth_format(), m_swapchain.image_views(), m_msaa_samples, m_render_pass);
        }

        std::vector<std::tuple<VkDescriptorSet, VkDescriptorSetLayout>> descriptor_sets(std::span<const Buffer> uniform_buffers, const Texture& texture)
        {
            std::vector<std::tuple<VkDescriptorSet, VkDescriptorSetLayout>> descriptors(m_swap_frame_count);

            for (size_t i = 0; i < m_swap_frame_count; ++i)
            {
                create_descriptor_set(uniform_buffers[i], texture, get<0>(descriptors[i]), get<1>(descriptors[i]));
            }

            return descriptors;
        }

        void create_descriptor_set(const Buffer& buffer, const Texture& texture, VkDescriptorSet& descriptor_set, VkDescriptorSetLayout& descriptor_set_layout)
        {
            VkDescriptorBufferInfo buffer_info = buffer.descriptor();
            VkDescriptorImageInfo image_info = texture.descriptor();

            auto builder = DescriptorBuilder::begin(&m_descriptor_allocator, &m_descriptor_layout_cache);
            builder.bind(0, buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
            builder.bind(1, image_info, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
            builder.build(descriptor_set, descriptor_set_layout);
        }

        void record_command_buffer(CommandBuffer& command_buffer, uint32_t image_index)
        {
            command_buffer.begin();

            auto extent = m_swapchain.extent();

            std::array<VkClearValue, 2> clear_values{};
            clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clear_values[1].depthStencil = {1.0f, 0};

            command_buffer.begin_render_pass(m_render_pass, m_framebuffers[image_index], extent, clear_values);
            command_buffer.set_viewport(extent);
            command_buffer.set_scissor(extent);

            for (const auto& node : m_nodes)
            {
                auto& descriptor_sets = node.m_descriptor_sets;
                auto& pipeline_layout = node.m_pipeline_layout;
                auto& graphics_pipeline = node.m_graphics_pipeline;
                const auto& geometry = m_model_cache.get(node.m_model_id);
                PushConstants pcs;
                pcs.model_matrix = node.m_matrix;

                VkBuffer vertex_buffer = geometry.vertex_buffer();
                VkBuffer index_buffer = geometry.index_buffer();
                uint32_t start = 0;
                uint32_t count = to_uint32(geometry.index_count());

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[m_current_frame], 0, nullptr);
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

                VkBuffer vertex_buffers[] = {vertex_buffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
                vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pcs);
                vkCmdDrawIndexed(command_buffer, count, 1, start, 0, 0);
            }

            command_buffer.end_render_pass();
            command_buffer.end();
        }

        void create_sync_objects()
        {
            m_image_available_semaphores.resize(m_swap_frame_count);
            m_render_finished_semaphores.resize(m_swap_frame_count);
            m_in_flight_fences.resize(m_swap_frame_count);

            VkSemaphoreCreateInfo semaphore_ci = vk_semaphore_create_info();
            VkFenceCreateInfo fence_ci = vk_fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

            for (size_t i = 0; i < m_swap_frame_count; ++i)
            {
                if (vkCreateSemaphore(m_device, &semaphore_ci, nullptr, &m_image_available_semaphores[i]) != VK_SUCCESS ||
                    vkCreateSemaphore(m_device, &semaphore_ci, nullptr, &m_render_finished_semaphores[i]) != VK_SUCCESS ||
                    vkCreateFence(m_device, &fence_ci, nullptr, &m_in_flight_fences[i]) != VK_SUCCESS)
                {
                    throw std::runtime_error("Error failed to create synchronization objects");
                }
            }
        }

        static bool physical_device_selector(const PhysicalDevice& physical_device, const Surface& surface)
        {
            bool extensions_supported = physical_device.check_extension_support(detail::device_extensions);

            SurfaceSupport surface_support = physical_device.surface_support(surface);
            bool swapchain_adequate = !surface_support.formats.empty() && !surface_support.present_modes.empty();

            VkPhysicalDeviceFeatures supported_features;
            vkGetPhysicalDeviceFeatures(physical_device, &supported_features);

            return physical_device.queue_family_presentable(surface) && extensions_supported && swapchain_adequate && supported_features.samplerAnisotropy;
        }

        Context m_context;
        Device m_device;
        ResourceManager m_resource_manager;
        PipelineManager m_pipeline_manager;

        size_t m_swap_frame_count;
        VkSampleCountFlagBits m_msaa_samples;

        Swapchain m_swapchain;
        Framebuffers m_framebuffers;

        RenderPass m_render_pass;

        DescriptorAllocator m_descriptor_allocator;
        DescriptorLayoutCache m_descriptor_layout_cache;

        std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
        std::vector<std::vector<VkDescriptorSet>> m_descriptor_sets; // [model_id][swap_frame]

        CommandPool m_command_pool;

        ModelCache m_model_cache;
        TextureCache m_texture_cache;

        UniformBuffers<UniformBufferObject> m_uniform_buffers;

        std::vector<Node> m_nodes;

        CommandBuffers m_command_buffers;

        std::vector<VkSemaphore> m_image_available_semaphores;
        std::vector<VkSemaphore> m_render_finished_semaphores;
        std::vector<VkFence> m_in_flight_fences;
        uint32_t m_current_frame = 0;

        bool m_framebuffer_resized = false;

        UniformBufferObject m_ubo;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_RENDERER_HPP
