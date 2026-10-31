#if !defined(QUETZAL_TEXTURE_TEST_APPLICATION_HPP)
#define QUETZAL_TEXTURE_TEST_APPLICATION_HPP
//------------------------------------------------------------------------------
// texture_test
// Application.hpp
//------------------------------------------------------------------------------

#include "quetzal/direct3d11/Application.hpp"
#include "quetzal/direct3d11/Camera.hpp"
#include "quetzal/direct3d11/Texture.hpp"
#include "quetzal/direct3d11/Statistics.hpp"
#include <DirectXMath.h>

namespace quetzal
{

namespace texture_test
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

        virtual void on_size(quetzal::framework::Window::size_type size);
        virtual void on_key_down(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::input::key::Code code);
        virtual void on_mouse_move(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::framework::Window::position_type position);

    private:

        void init_geometry();
        void init_projection();

        void check_input(float dt);

        void draw_cube(float x, float y, float z);

        com_ptr<ID3D11VertexShader> m_pVertexShader;
        com_ptr<ID3D11PixelShader> m_pPixelShader;

        com_ptr<ID3D11InputLayout> m_pInputLayout;

        com_ptr<ID3D11Buffer> m_pBufferVertex;
        com_ptr<ID3D11Buffer> m_pBufferIndex;
        com_ptr<ID3D11Buffer> m_pBufferConstant;

//        direct3d11::Texture m_texture;
        com_ptr<ID3D11ShaderResourceView> m_pShaderResourceView;

        size_t m_nIndices;

        DirectX::XMMATRIX m_m4World;

        DirectX::XMVECTOR m_up; // World up vector
        direct3d11::Camera m_camera;
        float m_speed;

        Timestamp m_tFrame;
        Timestamp m_t;

        quetzal::direct3d11::Statistics m_statistics;
    };

} // namespace texture_test

} // namespace quetzal

#endif // QUETZAL_TEXTURE_TEST_APPLICATION_HPP
