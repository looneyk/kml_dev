#if !defined(QUETZAL_ROGUELIKE_DISCRETEPOSITION_HPP)
#define QUETZAL_ROGUELIKE_DISCRETEPOSITION_HPP
//------------------------------------------------------------------------------
// DiscretePosition.hpp
//------------------------------------------------------------------------------

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class DiscretePosition
    {
    public:

        DiscretePosition();
        DiscretePosition(int x, int y, int z = 0);
        DiscretePosition(const DiscretePosition&) = default;
        ~DiscretePosition() = default;

        DiscretePosition& operator=(const DiscretePosition&) = default;

        int x();
        int y();
        int z();

        void set(int x, int y); // z unchanged
        void set(int x, int y, int z);

    private:

        int m_x;
        int m_y;
        int m_z;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_DISCRETEPOSITION_HPP
