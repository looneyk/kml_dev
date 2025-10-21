#if !defined(QUETZAL_FLAGS_HPP)
#define QUETZAL_FLAGS_HPP
//------------------------------------------------------------------------------
// common
// Flags.hpp
//
// This is intended to be used as invisible metadata, in cases where its state has no impact on the constness of the class it is a part of and vice versa.
// Thus, all methods are const and the data is mutable so that it can be used even in the const context.
// Note that when setting deleted true, the state of the other flags is undefined should it subsequently revert to false other than through clear or reset.
//
//------------------------------------------------------------------------------

#include <iosfwd>
#include <cstdint>

namespace quetzal
{

    //--------------------------------------------------------------------------
    class Flags
    {
    public:

        Flags() = default;
        Flags(const Flags&) = default;
        Flags(Flags&&) = default;
        virtual ~Flags() = default;

        Flags& operator=(const Flags&) = default;
        Flags& operator=(Flags&&) = default;

        bool deleted() const;
        void set_deleted(bool b = true) const;

        bool visited() const;
        void set_visited(bool b = true) const;

        bool checked() const;
        void set_checked(bool b = true) const;

        bool marked() const;
        void set_marked(bool b = true) const;

        static constexpr int Uncompared = -2;

        bool compared() const;
        int comparison() const; // -1, 0, 1, Uncompared
        void set_comparison(int result = Uncompared) const; // -1, 0, 1: comparison result or Uncompared (or not Uncompared and add reset_compared for that case? ...)

        // Applies to entire state
        bool empty() const;
        void clear() const;
        virtual void clear_all() const; // Override in parent class for deep clear

        // Preserves deleted state
        bool unset() const; // If deleted, returns true and other flags are ignored, otherwise same as empty
        void reset() const;
        virtual void reset_all() const; // Override in parent class for deep reset

    private:

        mutable uint32_t m_bits{};
    };

    std::ostream& operator<<(std::ostream& os, const Flags& flags);

} // namespace quetzal

#endif // QUETZAL_FLAGS_HPP
