#if !defined(QUETZAL_HEX_LEVEL_WALL_HPP)
#define QUETZAL_HEX_LEVEL_WALL_HPP
//------------------------------------------------------------------------------
// Wall.hpp
//------------------------------------------------------------------------------

#include "Position.hpp"
#include "Model.hpp"
#include <memory>

namespace quetzal
{

namespace hex_level
{

    class Cell;

    //--------------------------------------------------------------------------
    class Wall
    {
    public:

        Wall();
        Wall(const Wall&) = default;
        ~Wall() = default;

        Wall& operator=(const Wall&) = default;

        Cell* cell() const;
        Position position() const;
        Position normal() const; // ...
        const std::shared_ptr<Model> model() const;

        void set_cell(Cell* pcell);
        void set_position(float x, float y, float z);
        void set_normal(float x, float y, float z);
        void set_model(std::shared_ptr<Model> pmodel);

    private:

        Cell* m_pcell;
        Position m_position;
        Position m_normal; // ...
        std::shared_ptr<Model> m_pmodel;
    };

} // namespace hex_level

} // namespace quetzal

#endif // QUETZAL_HEX_LEVEL_WALL_HPP
