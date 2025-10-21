#if !defined(QUETZAL_ROGUELIKE_ROOM_HPP)
#define QUETZAL_ROGUELIKE_ROOM_HPP
//------------------------------------------------------------------------------
// Room.hpp
//------------------------------------------------------------------------------

namespace quetzal
{

    //--------------------------------------------------------------------------
    class Room
    {
    public:

        Room();
        Room(const Room&) = default;
        virtual ~Room();

        Room& operator=(const Room&) = default;

        

    private:
    };

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_ROOM_HPP
