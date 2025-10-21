#if !defined(QUETZAL_VULKAN_UNIFORMBUFFERS_HPP)
#define QUETZAL_VULKAN_UNIFORMBUFFERS_HPP

#include "Buffer.hpp"
#include "Device.hpp"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <span>
#include <vector>
#include <cassert>

namespace quetzal::vulkan
{

    template<typename T>
    class UniformBuffers
    {
    public:

        UniformBuffers(const Device& device, size_t n = 0) :
            m_device(device),
            m_buffers(n, Buffer(m_device)),
            m_mapped(n)
        {
            create();
        }

        ~UniformBuffers()
        {
            destroy();
        }

        operator std::span<const Buffer>() const
        {
            return m_buffers;
        }

        const Buffer& operator[](size_t i)
        {
            assert(i < m_buffers.size());
            return m_buffers[i];
        }

        void set(size_t i, const T& t)
        {
            memcpy(m_mapped[i], &t, buffer_size);
            return;
        }

        void resize(size_t n)
        {
            m_buffers.resize(n, Buffer(m_device.allocator()));
            m_mapped.resize(n);
            create();
            return;
        }

        void destroy()
        {
            for (size_t i = 0; i < m_buffers.size(); ++i)
            {
                m_buffers[i].unmap();
                m_buffers[i].destroy();
            }

            m_buffers.resize(0, Buffer(m_device));
            m_mapped.resize(0);
            return;
        }

    private:

        void create()
        {
            assert(m_mapped.size() == m_buffers.size());

            for (size_t i = 0; i < m_buffers.size(); ++i)
            {
                m_buffers[i].create(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
                m_buffers[i].map(m_mapped[i]);
            }

            return;
        }

        const size_t buffer_size = sizeof (T);

        const Device& m_device;
        std::vector<Buffer> m_buffers;
        std::vector<void*> m_mapped;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_UNIFORMBUFFERS_HPP
