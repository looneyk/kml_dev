#if !defined(QUETZAL_DEBUG_ASSERT_HPP)
#define QUETZAL_DEBUG_ASSERT_HPP
//------------------------------------------------------------------------------
// debug_assert.hpp
//------------------------------------------------------------------------------

#define debug_assert(x) if (!(x)) {cout << "assertion failed: " << #x << " " << __FILE__ << " " << __LINE__ << endl;}

#endif // QUETZAL_DEBUG_ASSERT_HPP
