#if !defined(QUETZAL_VULKAN_TEXTURE_HPP)
#define QUETZAL_VULKAN_TEXTURE_HPP

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "CommandPool.hpp"
#include "Device.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "Queue.hpp"
#include "Sampler.hpp"
#include "SingleSubmitCommands.hpp"
#include "StagingBuffer.hpp"
#include "TextureData.hpp"
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <vector>

namespace quetzal::vulkan
{

    class Texture
    {
    public:

        Texture(const Device& device) :
            m_device(device),
            m_textureImage(m_device),
            m_textureImageView(m_device),
            m_textureSampler(m_device),
            m_mipLevels(1)
        {
        }

        ~Texture()
        {
            destroy();
        }

        void create(const TextureData& td, const CommandPool& commandPool, const Queue& queue)
        {
            const MipLevel& base = td.mips[0];

            if (!is_compressed(td.format) && td.mips.size() == 1)
            {
                m_mipLevels = to_uint32(std::floor(std::log2(std::max(base.width, base.height)))) + 1;

                StagingBuffer stagingBuffer(m_device, static_cast<VkDeviceSize>(base.byte_size), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
                stagingBuffer.copy(base.data, base.byte_size);

                VkExtent2D extent{base.width, base.height};
                m_textureImage.create(extent, td.format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, m_mipLevels, VK_IMAGE_TILING_OPTIMAL, 0);

                transitionImageLayout(m_textureImage, td.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels, queue, commandPool);
                copyBufferToImage(stagingBuffer, m_textureImage, base.width, base.height, queue, commandPool);
                generateMipmaps(m_textureImage, td.format, extent, m_mipLevels, queue, commandPool);
            }
            else
            {
                m_mipLevels = static_cast<uint32_t>(td.mips.size());

                VkDeviceSize totalSize = 0;
                for (const auto& mip : td.mips)
                    totalSize += static_cast<VkDeviceSize>(mip.byte_size);

                std::vector<uint8_t> staging_data;
                staging_data.reserve(static_cast<size_t>(totalSize));
                for (const auto& mip : td.mips)
                {
                    const auto* bytes = static_cast<const uint8_t*>(mip.data);
                    staging_data.insert(staging_data.end(), bytes, bytes + mip.byte_size);
                }

                StagingBuffer stagingBuffer(m_device, totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
                stagingBuffer.copy(staging_data.data(), staging_data.size());

                VkExtent2D extent{base.width, base.height};
                m_textureImage.create(extent, td.format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, m_mipLevels, VK_IMAGE_TILING_OPTIMAL, 0);

                transitionImageLayout(m_textureImage, td.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels, queue, commandPool);
                copyBufferToImageMips(stagingBuffer, td.mips, queue, commandPool);
                transitionImageLayout(m_textureImage, td.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels, queue, commandPool);
            }

            m_textureImageView.create(m_textureImage, td.format, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels);
            m_textureSampler.create();
        }

        void destroy()
        {
            m_textureSampler.destroy();
            m_textureImageView.destroy();
            m_textureImage.destroy();
        }

        VkDescriptorImageInfo descriptor() const
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_textureImageView;
            imageInfo.sampler = m_textureSampler;
            return imageInfo;
        }

    private:

        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, const Queue& queue, const CommandPool& commandPool)
        {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {width, height, 1};

            SingleSubmitCommands commands(m_device, commandPool, queue);
            vkCmdCopyBufferToImage(commands, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            commands.submit();
            return;
        }

        void copyBufferToImageMips(VkBuffer buffer, const std::vector<MipLevel>& mips, const Queue& queue, const CommandPool& commandPool)
        {
            std::vector<VkBufferImageCopy> regions;
            regions.reserve(mips.size());

            VkDeviceSize offset = 0;
            for (uint32_t i = 0; i < static_cast<uint32_t>(mips.size()); ++i)
            {
                VkBufferImageCopy region{};
                region.bufferOffset = offset;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = i;
                region.imageSubresource.baseArrayLayer = 0;
                region.imageSubresource.layerCount = 1;
                region.imageOffset = {0, 0, 0};
                region.imageExtent = {mips[i].width, mips[i].height, 1};
                regions.push_back(region);
                offset += static_cast<VkDeviceSize>(mips[i].byte_size);
            }

            SingleSubmitCommands commands(m_device, commandPool, queue);
            vkCmdCopyBufferToImage(commands, buffer, m_textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(regions.size()), regions.data());
            commands.submit();
        }

        void generateMipmaps(VkImage image, VkFormat imageFormat, VkExtent2D extent, uint32_t mipLevels, const Queue& queue, const CommandPool& commandPool)
        {
            VkFormatProperties formatProperties = m_device.physical_device().format_properties(imageFormat);
            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            {
                throw std::runtime_error("texture image format does not support linear blitting!");
            }

            SingleSubmitCommands commands(m_device, commandPool, queue);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            int32_t mipWidth = extent.width;
            int32_t mipHeight = extent.height;

            for (uint32_t i = 1; i < mipLevels; ++i)
            {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                VkImageBlit blit{};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(commands, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = mipLevels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            commands.submit();
            return;
        }

        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, const Queue& queue, const CommandPool& commandPool)
        {
            format;

            SingleSubmitCommands commands(m_device, commandPool, queue);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = mipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else
            {
                throw std::invalid_argument("Unsupported layout transition!");
            }

            vkCmdPipelineBarrier(commands, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            commands.submit();
        }

        const Device& m_device;
        Image m_textureImage;
        ImageView m_textureImageView;
        Sampler m_textureSampler;
        uint32_t m_mipLevels;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_TEXTURE_HPP
