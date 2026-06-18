
#include "DdsLoader.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace quetzal::scene_graph
{

namespace
{

constexpr uint32_t DDS_MAGIC    = 0x20534444;

constexpr uint32_t FOURCC_DXT1  = 0x31545844;
constexpr uint32_t FOURCC_DXT3  = 0x33545844;
constexpr uint32_t FOURCC_DXT5  = 0x35545844;
constexpr uint32_t FOURCC_ATI1  = 0x31495441;
constexpr uint32_t FOURCC_ATI2  = 0x32495441;
constexpr uint32_t FOURCC_BC4U  = 0x55344342;
constexpr uint32_t FOURCC_BC4S  = 0x53344342;
constexpr uint32_t FOURCC_BC5U  = 0x55354342;
constexpr uint32_t FOURCC_BC5S  = 0x53354342;
constexpr uint32_t FOURCC_DX10  = 0x30315844;

struct DdsPixelFormat
{
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t rgbBitCount;
    uint32_t rBitMask;
    uint32_t gBitMask;
    uint32_t bBitMask;
    uint32_t aBitMask;
};

struct DdsHeader
{
    uint32_t       size;
    uint32_t       flags;
    uint32_t       height;
    uint32_t       width;
    uint32_t       pitchOrLinearSize;
    uint32_t       depth;
    uint32_t       mipMapCount;
    uint32_t       reserved1[11];
    DdsPixelFormat ddspf;
    uint32_t       caps;
    uint32_t       caps2;
    uint32_t       caps3;
    uint32_t       caps4;
    uint32_t       reserved2;
};

struct DdsHeaderDxt10
{
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};

static_assert(sizeof(DdsPixelFormat) == 32);
static_assert(sizeof(DdsHeader)      == 124);
static_assert(sizeof(DdsHeaderDxt10) == 20);

VkFormat dxgi_to_vk(uint32_t dxgi)
{
    switch (dxgi)
    {
    case 28:  return VK_FORMAT_R8G8B8A8_UNORM;
    case 29:  return VK_FORMAT_R8G8B8A8_SRGB;
    case 71:  return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case 72:  return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case 74:  return VK_FORMAT_BC2_UNORM_BLOCK;
    case 75:  return VK_FORMAT_BC2_SRGB_BLOCK;
    case 77:  return VK_FORMAT_BC3_UNORM_BLOCK;
    case 78:  return VK_FORMAT_BC3_SRGB_BLOCK;
    case 80:  return VK_FORMAT_BC4_UNORM_BLOCK;
    case 81:  return VK_FORMAT_BC4_SNORM_BLOCK;
    case 83:  return VK_FORMAT_BC5_UNORM_BLOCK;
    case 84:  return VK_FORMAT_BC5_SNORM_BLOCK;
    case 87:  return VK_FORMAT_B8G8R8A8_UNORM;
    case 91:  return VK_FORMAT_B8G8R8A8_SRGB;
    case 95:  return VK_FORMAT_BC6H_UFLOAT_BLOCK;
    case 96:  return VK_FORMAT_BC6H_SFLOAT_BLOCK;
    case 98:  return VK_FORMAT_BC7_UNORM_BLOCK;
    case 99:  return VK_FORMAT_BC7_SRGB_BLOCK;
    default:  return VK_FORMAT_UNDEFINED;
    }
}

uint32_t block_bytes(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
        return 8;
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return 16;
    default:
        return 0;
    }
}

size_t mip_byte_size(VkFormat format, uint32_t width, uint32_t height)
{
    uint32_t bb = block_bytes(format);
    if (bb > 0)
    {
        return static_cast<size_t>(std::max(1u, (width + 3) / 4))
             * std::max(1u, (height + 3) / 4)
             * bb;
    }
    return static_cast<size_t>(width) * height * 4;
}

} // namespace

vulkan::TextureData parse_dds(const uint8_t* file_data, size_t file_size)
{
    if (file_size < 4 + sizeof(DdsHeader))
        throw std::runtime_error("DDS file too small");

    uint32_t magic;
    std::memcpy(&magic, file_data, 4);
    if (magic != DDS_MAGIC)
        throw std::runtime_error("Not a DDS file");

    const auto* header = reinterpret_cast<const DdsHeader*>(file_data + 4);
    if (header->size != 124)
        throw std::runtime_error("Invalid DDS header");

    uint32_t width     = header->width;
    uint32_t height    = header->height;
    uint32_t mip_count = std::max(1u, header->mipMapCount);

    size_t   data_offset = 4 + sizeof(DdsHeader);
    VkFormat format      = VK_FORMAT_UNDEFINED;

    if (header->ddspf.fourCC == FOURCC_DX10)
    {
        if (file_size < data_offset + sizeof(DdsHeaderDxt10))
            throw std::runtime_error("DDS DX10 header truncated");
        const auto* dx10 = reinterpret_cast<const DdsHeaderDxt10*>(file_data + data_offset);
        data_offset += sizeof(DdsHeaderDxt10);
        format = dxgi_to_vk(dx10->dxgiFormat);
    }
    else
    {
        switch (header->ddspf.fourCC)
        {
        case FOURCC_DXT1:               format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK; break;
        case FOURCC_DXT3:               format = VK_FORMAT_BC2_UNORM_BLOCK;      break;
        case FOURCC_DXT5:               format = VK_FORMAT_BC3_UNORM_BLOCK;      break;
        case FOURCC_ATI1: case FOURCC_BC4U: format = VK_FORMAT_BC4_UNORM_BLOCK; break;
        case FOURCC_BC4S:               format = VK_FORMAT_BC4_SNORM_BLOCK;      break;
        case FOURCC_ATI2: case FOURCC_BC5U: format = VK_FORMAT_BC5_UNORM_BLOCK; break;
        case FOURCC_BC5S:               format = VK_FORMAT_BC5_SNORM_BLOCK;      break;
        default:
            throw std::runtime_error("Unsupported DDS FourCC");
        }
    }

    if (format == VK_FORMAT_UNDEFINED)
        throw std::runtime_error("Unsupported DXGI format in DDS file");

    vulkan::TextureData td;
    td.format = format;
    td.mips.reserve(mip_count);

    const uint8_t* mip_ptr    = file_data + data_offset;
    uint32_t       mip_width  = width;
    uint32_t       mip_height = height;

    for (uint32_t i = 0; i < mip_count; ++i)
    {
        size_t byte_size = mip_byte_size(format, mip_width, mip_height);
        if (mip_ptr + byte_size > file_data + file_size)
            throw std::runtime_error("DDS mip data truncated");

        td.mips.push_back({mip_ptr, byte_size, mip_width, mip_height});
        mip_ptr    += byte_size;
        mip_width   = std::max(1u, mip_width  / 2);
        mip_height  = std::max(1u, mip_height / 2);
    }

    return td;
}

} // namespace quetzal::scene_graph
