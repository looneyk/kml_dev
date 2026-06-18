#if !defined(QUETZAL_SPATIAL_SEARCH_POOLEDALLOCATOR_HPP)
#define QUETZAL_SPATIAL_SEARCH_POOLEDALLOCATOR_HPP

#include <stdexcept>
#include <cstdlib>

namespace quetzal::spatial_search
{

    // Pooled storage allocator
    // The following routines allow for the efficient allocation of storage in small chunks from a specified pool. Rather than allowing each structure
    // to be freed individually, an entire pool of storage is freed at once.
    // This method has two advantages over just using malloc() and free(). First, it is far more efficient for allocating small objects, as there is
    // no overhead for remembering all the information needed to free each object or consolidating fragmented memory. Second, the decision about
    // how long to keep an object is made at the time of allocation, and there is no need to track down all the objects to free them.
    const size_t WORDSIZE = 16;
    const size_t BLOCKSIZE = 8192;

    //--------------------------------------------------------------------------
    class PooledAllocator
    {
        // We maintain memory alignment to word boundaries by requiring that all allocations be in multiples of the machine wordsize.
        // Size of machine word in bytes. Must be power of 2.
        // Minimum number of bytes requested at a time from the system. Must be multiple of WORDSIZE.

        using size_type = size_t;

        size_type remaining; // Number of bytes left in current block of storage.
        void* base;     // Pointer to base of current block of storage.
        void* loc;      // Current location in block to next allocate memory.

        void internal_init()
        {
            remaining = 0;
            base = nullptr;
            usedMemory = 0;
            wastedMemory = 0;
        }

    public:

        size_type usedMemory;
        size_type wastedMemory;

        PooledAllocator() { internal_init(); }
        ~PooledAllocator() { free_all(); }

        // Frees all allocated memory chunks
        void free_all()
        {
            while (base != nullptr)
            {
                void* prev = *(static_cast<void**>(base)); // Get pointer to prev block.
                ::free(base);
                base = prev;
            }
            internal_init();
        }

        // Returns a pointer to a piece of new memory of the given size in bytes allocated from the pool.
        void* malloc(const size_t req_size)
        {
            // Round size up to a multiple of wordsize (only works for WORDSIZE that is a power of 2, by masking last bits of incremented size to zero)
            const size_type size = (req_size + (WORDSIZE - 1)) & ~(WORDSIZE - 1);

            // Check whether a new block must be allocated. Note that the first word of a block is reserved for a pointer to the previous block.
            if (size > remaining)
            {
                wastedMemory += remaining;

                // Allocate new storage.
                const size_type blocksize = (size + sizeof(void*) + (WORDSIZE - 1) > BLOCKSIZE)
                        ? size + sizeof(void*) + (WORDSIZE - 1)
                        : BLOCKSIZE;

                // use the standard C malloc to allocate memory
                void* m = ::malloc(blocksize);
                if (!m)
                {
                    fprintf(stderr, "Failed to allocate memory.\n");
                    throw std::bad_alloc();
                }

                // Fill first word of new block with pointer to previous block.
                static_cast<void**>(m)[0] = base;
                base = m;

                size_type shift = 0;
                // int size_t = (WORDSIZE - ( (((size_t)m) + sizeof(void*)) & (WORDSIZE-1))) & (WORDSIZE-1);
                remaining = blocksize - sizeof(void*) - shift;
                loc = (static_cast<char*>(m) + sizeof(void*) + shift);
            }

            void* rloc = loc;
            loc = static_cast<char*>(loc) + size;
            remaining -= size;
            usedMemory += size;
            return rloc;
        }

        // Allocates (using this pool) a generic type T.
        // count = number of instances to allocate.
        // Returns: pointer (of type T*) to memory buffer
        template <typename T>
        T* allocate(const size_t count = 1)
        {
            T* mem = static_cast<T*>(this->malloc(sizeof(T) * count));
            return mem;
        }
    };

    //--------------------------------------------------------------------------
    // Allocates (using C's malloc) a generic type T.
    // count: number of instances to allocate.
    // Returns: pointer (of type T*) to memory buffer
    template <typename T>
    T* allocate(size_t count = 1)
    {
        T* mem = static_cast<T*>(::malloc(sizeof(T) * count));
        return mem;
    }

} // namespace quetzal::spatial_search

#endif // QUETZAL_SPATIAL_SEARCH_POOLEDALLOCATOR_HPP
