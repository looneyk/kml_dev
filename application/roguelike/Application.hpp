#if !defined(QUETZAL_ROGUELIKE_APPLICATION_HPP)
#define QUETZAL_ROGUELIKE_APPLICATION_HPP
//------------------------------------------------------------------------------
// roguelike
// Application.hpp
//------------------------------------------------------------------------------

#include "quetzal/direct3d11/Application.hpp"
#include "quetzal/direct3d11/Statistics.hpp"
#include <DirectXMath.h>

namespace quetzal
{

namespace roguelike
{

    //----------------------------------------------------------------------
    class Application : public quetzal::direct3d11::Application
    {
    public:

        Application(HINSTANCE hinstance, const std::string& caption);
        Application(const Application&) = delete;
        ~Application() = default;

        Application& operator=(const Application&) = delete;

    protected:

        virtual void update(float dt);
        virtual void render();

        virtual void on_size(quetzal::direct3d11::Window::size_type size);
        virtual void on_key_down(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::input::key::Code code);
        virtual void on_mouse_down(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::direct3d11::Window::pos_type position, input::key::Code code);
        virtual void on_mouse_up(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::direct3d11::Window::pos_type position, input::key::Code code);
        virtual void on_mouse_move(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::direct3d11::Window::pos_type position);

    private:

        void load_level(...);

        void init_geometry();
        void init_view();
        void init_projection();

        void check_input(float dt);

        void draw_level(const Level& level);
        void draw_cell(const Cell& cell, float x, float y, float z);
        void draw_wall(const Wall& wall, ...); // wall instance with wall reference, transformation matrix, ...

        com_ptr<ID3D11VertexShader> m_pVertexShader;
        com_ptr<ID3D11PixelShader> m_pPixelShader;

        com_ptr<ID3D11InputLayout> m_pInputLayout;
		
        com_ptr<ID3D11Buffer> m_pBufferVertex;
        com_ptr<ID3D11Buffer> m_pBufferIndex;
        com_ptr<ID3D11Buffer> m_pBufferConstant;

        com_ptr<ID3D11ShaderResourceView> m_pShaderResourceView;
        com_ptr<ID3D11SamplerState> m_pSamplerState;

        DirectX::XMMATRIX m_m4World;

        direct3d11::Camera m_camera;
        DirectX::XMVECTOR m_up; // World up vector
        float m_fov;
        float m_speed;
        direct3d11::Window::pos_type m_posMouse;

        Timestamp m_tFrame;
        Timestamp m_t;
		
        quetzal::direct3d11::Statistics m_statistics;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_APPLICATION_HPP
