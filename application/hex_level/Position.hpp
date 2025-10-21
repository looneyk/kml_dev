#if !defined(QUETZAL_HEX_LEVEL_POSITION_HPP)
#define QUETZAL_HEX_LEVEL_POSITION_HPP
//------------------------------------------------------------------------------
// Position.hpp
//------------------------------------------------------------------------------

namespace quetzal
{

namespace hex_level
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
        Position& operator+=(const Position& other);

        float x() const;
        float y() const;
        float z() const;

        void set(float x, float y, float z);

    private:

        float m_x;
        float m_y;
        float m_z;
    };

    Position operator+(const Position& lhs, const Position& rhs);

} // namespace hex_level

} // namespace quetzal

#endif // QUETZAL_HEX_LEVEL_POSITION_HPP
