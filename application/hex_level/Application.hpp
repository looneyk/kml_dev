#if !defined(QUETZAL_HEX_LEVEL_APPLICATION_HPP)
#define QUETZAL_HEX_LEVEL_APPLICATION_HPP
//------------------------------------------------------------------------------
// hex_level
// Application.hpp
//------------------------------------------------------------------------------

#include "quetzal/direct3d11/Application.hpp"
#include "quetzal/direct3d11/Camera.hpp"
#include "quetzal/direct3d11/Statistics.hpp"
#include "Level.hpp"
#include <DirectXMath.h>

namespace quetzal
{

namespace hex_level
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

        void init_geometry();
        void init_projection();

        void check_input(float dt);

        void load_level(const std::string& filename);

        void draw_level();
        void draw_cell(const Cell& cell);
        void draw_wall(const Wall& wall);

        com_ptr<ID3D11VertexShader> m_pVertexShader;
        com_ptr<ID3D11PixelShader> m_pPixelShader;

        com_ptr<ID3D11InputLayout> m_pInputLayout;

        com_ptr<ID3D11Buffer> m_pBufferVertex;
        com_ptr<ID3D11Buffer> m_pBufferIndex;
        com_ptr<ID3D11Buffer> m_pBufferConstant;

        com_ptr<ID3D11ShaderResourceView> m_pShaderResourceView;
        com_ptr<ID3D11SamplerState> m_pSamplerState;

        DirectX::XMMATRIX m_m4World;

        DirectX::XMVECTOR m_up; // World up vector
        direct3d11::Camera m_camera;
        float m_speed;
        direct3d11::Window::pos_type m_posMouse;

        Timestamp m_tFrame;
        Timestamp m_t;

        Level m_level;

        quetzal::direct3d11::Statistics m_statistics;
    };

} // namespace hex_level

} // namespace quetzal

#endif // QUETZAL_HEX_LEVEL_APPLICATION_HPP
