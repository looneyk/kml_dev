#if !defined(QUETZAL_VULKAN_PIPELINEMANAGER_HPP)
#define QUETZAL_VULKAN_PIPELINEMANAGER_HPP

#include "Device.hpp"
#include "GraphicsPipelines.hpp"
#include "PipelineCache.hpp"
#include <vulkan/vulkan.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <vector>

namespace quetzal::vulkan
{

    class PipelineManager
    {
    public:

        PipelineManager(const Device& device, const std::filesystem::path& rootpath) :
            m_device(device),
            m_rootpath(rootpath),
            m_pipeline_cache(device),
            m_pipelines()
        {
            m_pipeline_cache.create({});
        }

        ~PipelineManager()
        {
            //m_pipeline_cache.destroy();
        }

        void load(const std::filesystem::path& file)
        {
            auto filepath = pipeline_cache_filepath(m_rootpath, file);
            auto pipeline_cache = read_cache(filepath);
            if (!m_pipeline_cache)
            {
                // log load error ...
                return;
            }

            m_pipeline_cache.merge(pipeline_cache.value());
            pipeline_cache.value().destroy();
            return;
        }

        void store(const std::filesystem::path& file)
        {
            auto filepath = pipeline_cache_filepath(m_rootpath, file);
            write_cache(filepath);
            return;
        }

        const GraphicsPipeline& request_graphics_pipeline(const GraphicsPipelineCreateInfo& ci)
        {
            auto& pipeline = m_pipelines.emplace_back(m_device);
            pipeline.create(m_pipeline_cache, ci);
            return pipeline;
        }

        void request_graphics_pipelines(std::span<VkGraphicsPipelineCreateInfo> cis, GraphicsPipelines& graphics_pipelines)
        {
            graphics_pipelines.create(m_pipeline_cache, cis);
            return;
        }

    private:

        std::filesystem::path pipeline_cache_filepath(const std::filesystem::path& rootpath, const std::filesystem::path& file)
        {
            return rootpath / "pipelines" / file;
        }

        std::optional<PipelineCache> read_cache(const std::filesystem::path& filepath)
        {
            auto buffer = read_binary(filepath);
            if (!buffer)
            {
                return std::nullopt;
            }

            PipelineCache pipeline_cache(m_device);
            pipeline_cache.create(buffer.value());
            return pipeline_cache;
        }

        std::optional<std::vector<uint8_t>> read_binary(const std::filesystem::path& filepath)
        {
            std::basic_ifstream<uint8_t> ifs(filepath, std::ios_base::binary | std::ios_base::ate);
            if (!ifs)
            {
                // log file error ...
                return std::nullopt;
            }

            std::streamsize size = ifs.tellg();
            std::vector<uint8_t> buffer(size);

            ifs.seekg(0, std::ios::beg);
            ifs.read(buffer.data(), size);
            if (!ifs)
            {
                // log read error ...
                return std::nullopt;
            }

            return buffer;
        }

        void write_cache(const std::filesystem::path& filepath)
        {
            size_t size = 0;
            vkGetPipelineCacheData(m_device, m_pipeline_cache, &size, nullptr);

            std::vector<uint8_t> buffer(size);
            vkGetPipelineCacheData(m_device, m_pipeline_cache, &size, buffer.data());

            write_binary(filepath, buffer);
            return;
        }

        void write_binary(const std::filesystem::path& filepath, std::span<uint8_t> buffer)
        {
            std::basic_ofstream<uint8_t> ofs(filepath, std::ios_base::binary);
            if (!ofs)
            {
                // log file error ...
                return;
            }

            ofs.write(buffer.data(), buffer.size());
            if (!ofs)
            {
                // log write error ...
                return;
            }

            return;
        }

        const Device& m_device;
        std::filesystem::path m_rootpath;
        PipelineCache m_pipeline_cache;
        std::vector<GraphicsPipeline> m_pipelines;
//        std::unordered_map<size_t, Pipeline> m_pipelines;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_PIPELINEMANAGER_HPP
