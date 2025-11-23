#if !defined(QUETZAL_DIRECT3D11_BOX_APPLICATION_HPP)
#define QUETZAL_DIRECT3D11_BOX_APPLICATION_HPP
//------------------------------------------------------------------------------
// Application.hpp
//------------------------------------------------------------------------------

#include "quetzal/direct3d11/Application.hpp"
#include <DirectXMath.h>
#include <dxgi.h>

namespace quetzal
{

namespace direct3d11_box
{

    //--------------------------------------------------------------------------
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
        virtual void on_mouse_down(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::direct3d11::Window::pos_type position, quetzal::input::key::Code code);
        virtual void on_mouse_up(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::direct3d11::Window::pos_type position, quetzal::input::key::Code code);
        virtual void on_mouse_move(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::direct3d11::Window::pos_type position);

    private:

        void init_geometry();
        void init_view();
        void init_projection();

        void draw_box();

        com_ptr<ID3D11VertexShader> m_pShaderVertex;
        com_ptr<ID3D11PixelShader> m_pShaderPixel;

        com_ptr<ID3D11InputLayout> m_pInputLayout;

        com_ptr<ID3D11Buffer> m_pBufferVertex;
        com_ptr<ID3D11Buffer> m_pBufferIndex;
        com_ptr<ID3D11Buffer> m_pBufferConstant;

        DirectX::XMMATRIX m_m4World;
        DirectX::XMMATRIX m_m4View;
        DirectX::XMMATRIX m_m4Projection;

        float m_theta;
        float m_phi;
        float m_radius;

        quetzal::direct3d11::Window::pos_type m_posMousePrev;
        bool m_button_pressed;
        quetzal::input::key::Code m_button_code;
    };

} // namespace direct3d11_box

} // namespace quetzal

#endif // QUETZAL_DIRECT3D11_BOX_APPLICATION_HPP
