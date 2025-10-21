#if !defined(QUETZAL_LIGHTMATRIX_APPLICATION_HPP)
#define QUETZAL_LIGHTMATRIX_APPLICATION_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Copyright (c) 2010 Keith Michael Looney. All Rights Reserved.
// Application.hpp
//------------------------------------------------------------------------------

#include "quetzal/direct3d11/Application.hpp"
#include "quetzal/direct3d11/Camera.hpp"
#include "quetzal/direct3d11/Statistics.hpp"
#include "Controller.hpp"
#include "Scene.hpp"

namespace quetzal::lightmatrix
{

    //----------------------------------------------------------------------
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
        virtual void on_mouse_move(Timestamp ts, quetzal::input::key::Modifiers modifiers, position_type position);

    private:

        void check_input(float dt);

        void init_projection();

        void set_pattern(int nPattern);

        Frame m_frame;
        Scene m_scene;
        Controller m_controller;

        direct3d11::Camera m_camera;
        DirectX::XMVECTOR m_up; // World up vector
        float m_speed;
        float m_tDissolve;
		
        quetzal::direct3d11::Statistics m_statistics;
    };

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_APPLICATION_HPP
