#if !defined(QUETZAL_ROGUELIKE_DISCRETEDirection_HPP)
#define QUETZAL_ROGUELIKE_DISCRETEDirection_HPP
//------------------------------------------------------------------------------
// DiscreteDirection.hpp
//------------------------------------------------------------------------------

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class DiscreteDirection
    {
    public:

        DiscreteDirection();
        DiscreteDirection(int x, int y, int z = 0);
        DiscreteDirection(const DiscreteDirection&) = default;
        ~DiscreteDirection() = default;

        DiscreteDirection& operator=(const DiscreteDirection&) = default;

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

#endif // QUETZAL_ROGUELIKE_DISCRETEDirection_HPP
