#if !defined(QUETZAL_ROGUELIKE_POSITION_HPP)
#define QUETZAL_ROGUELIKE_POSITION_HPP
//------------------------------------------------------------------------------
// Position.hpp
//------------------------------------------------------------------------------

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class Position
    {
    public:

        Position();
        Position(float x, float y, float z);
        Position(const Position&) = default;
        ~Position() = default;

        Position& operator=(const Position&) = default;

        float x();
        float y();
        float z();

        void set(float x, float y, float z);

    private:

        float m_x;
        float m_y;
        float m_z;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_POSITION_HPP
