#if !defined(QUETZAL_ROGUELIKE_DISCRETESIZE_HPP)
#define QUETZAL_ROGUELIKE_DISCRETESIZE_HPP
//------------------------------------------------------------------------------
// DiscreteSize.hpp
//------------------------------------------------------------------------------

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class DiscreteSize
    {
    public:

        DiscreteSize();
        DiscreteSize(int dx, int dy, int dz = 1);
        DiscreteSize(const DiscreteSize&) = default;
        ~DiscreteSize() = default;

        DiscreteSize& operator=(const DiscreteSize&) = default;

        int dx();
        int dy();
        int dz();

        void set(int dx, int dy, int dz = 1);

    private:

        int m_dx;
        int m_dy;
        int m_dz;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_DISCRETESIZE_HPP
