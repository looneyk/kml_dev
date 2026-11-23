//------------------------------------------------------------------------------
// stratum_sim
// IdAllocator.cpp
//------------------------------------------------------------------------------

#include "IdAllocator.hpp"

//------------------------------------------------------------------------------
quetzal::stratum_sim::CountingIdAllocator::CountingIdAllocator(PersistentId first) :
    m_next(first)
{
}

//------------------------------------------------------------------------------
quetzal::stratum_core::PersistentId quetzal::stratum_sim::CountingIdAllocator::next()
{
    return PersistentId{m_next.id++};
}
