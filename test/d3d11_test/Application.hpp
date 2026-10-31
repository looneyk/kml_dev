#if !defined(QUETZAL_CUBES_APPLICATION_HPP)
#define QUETZAL_CUBES_APPLICATION_HPP
//------------------------------------------------------------------------------
// d3d11_test
// Application.hpp
//------------------------------------------------------------------------------

#include "quetzal/direct3d11/Application.hpp"
#include "quetzal/direct3d11/Camera.hpp"
#include "quetzal/direct3d11/Statistics.hpp"
#include "Scene.hpp"

namespace quetzal
{

namespace cubes
{

    //--------------------------------------------------------------------------
    class Application : public quetzal::direct3d11::Application
    {
    public:

        Application();
        Application(const Application&) = delete;
        ~Application() = default;

        Application& operator=(const Application&) = delete;

    protected:

        virtual void update(float dt);
        virtual void render();

        virtual void on_size(size_type size);
        virtual void on_key_down(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::input::key::Code code);
        virtual void on_mouse_down(Timestamp ts, quetzal::input::key::Modifiers modifiers, position_type position, quetzal::input::key::Code code);
        virtual void on_mouse_up(Timestamp ts, quetzal::input::key::Modifiers modifiers, position_type position, quetzal::input::key::Code code);
        virtual void on_mouse_move(Timestamp ts, quetzal::input::key::Modifiers modifiers, position_type position);

    private:

        void check_input(float dt);

        void init_projection();

        Scene m_scene;

        direct3d11::Camera m_camera; // main camera, up vector, part of direct3d11::Application ...
        DirectX::XMVECTOR m_up; // World up vector
        float m_speed; // In camera view direction

        bool m_bOrthographic;
        float m_scale; // for ortho projection ...

        position_type m_posMouse;

        Timestamp m_tFrame;
        Timestamp m_t;

        quetzal::direct3d11::Statistics m_statistics;
    };

} // namespace cubes

} // namespace quetzal

#endif // QUETZAL_CUBES_APPLICATION_HPP
