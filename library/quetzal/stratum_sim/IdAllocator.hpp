#if !defined(QUETZAL_STRATUM_SIM_IDALLOCATOR_HPP)
#define QUETZAL_STRATUM_SIM_IDALLOCATOR_HPP
//------------------------------------------------------------------------------
// stratum_sim
// IdAllocator.hpp
//------------------------------------------------------------------------------

#include "PersistentId.hpp"

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The source of a world's persistent ids
    class IdAllocator
    {
    public:

        virtual ~IdAllocator() = default;

        virtual PersistentId next() = 0;
    };

    //--------------------------------------------------------------------------
    // Ids from a count. A world is saved with an id from next, and restored with an allocator constructed from it
    class CountingIdAllocator final : public IdAllocator
    {
    public:

        explicit CountingIdAllocator(PersistentId first);

        PersistentId next() override;

    private:

        PersistentId m_next;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_IDALLOCATOR_HPP
