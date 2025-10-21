//------------------------------------------------------------------------------
//
// Copyright (c) 2013 Keith Michael Looney. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
// Cubes
// application.hpp
//
//------------------------------------------------------------------------------

#if !defined(QUETZAL_CUBES_APPLICATION_HPP)
#define QUETZAL_CUBES_APPLICATION_HPP

#include <D3DX9Math.h>
#include "quetzal/dx/Application.hpp"
#include "quetzal/dx/Statistics.hpp"
//#include "Display.hpp"
#include "quetzal/com_ptr.hpp"

namespace quetzal
{

namespace cubes
{

class Cube; // ...

    //----------------------------------------------------------------------
    class application : public quetzal::dx::Application
    {
    public:

        application(HINSTANCE hInstance, std::wstring strCaption, D3DDEVTYPE devType, DWORD requestedVP);
        ~application();

        virtual bool checkDeviceCaps();
        virtual void onLostDevice();
        virtual void onResetDevice();
        virtual void update(Timer::Timestamp dt);
        virtual void draw();

        virtual bool input_key_down(Timer::Timestamp ts, quetzal::device::KeyModifier::Modifiers modifiers, int keycode);
        virtual bool input_key_up(Timer::Timestamp ts, quetzal::device::KeyModifier::Modifiers modifiers, int keycode);
        virtual bool input_pointer_move(Timer::Timestamp ts, quetzal::device::KeyModifier::Modifiers modifiers, quetzal::dx::vector_type position);

    private:

        application(const application&); // Not copyable
        application& operator=(const application&); // Not assignable

        void init(); // init_geometry, initProjectionMatrix, ...
        void final();

        void init_geometry();
        void init_effects();
        void initProjectionMatrix();
        void initViewMatrix();

        void check_input(Timer::Timestamp dt);

        void init_display();
        void init_display(int nColor0, float fraction1);

        void draw_pyramid();
        void draw_cube(/*const Cube& cube, */float x, float y, float z);

        quetzal::dx::Statistics m_statistics;
        unsigned int m_nVertices;
        unsigned int m_nIndices;

//        Display m_display;
        bool m_bCycle;
        int m_nColors;
        int m_nColor;
        D3DXCOLOR m_color;

        bool m_smooth;
        float m_tFrameDuration;

        quetzal::com_ptr<IDirect3DVertexBuffer9> m_pVertexBuffer;
        quetzal::com_ptr<IDirect3DIndexBuffer9> m_pIndexBuffer;

        D3DXMATRIX m_matrixProjection;
        D3DXMATRIX m_matrixView;

        D3DXVECTOR3 m_camera;
        D3DXVECTOR3 m_direction;
        float m_speed;
        float m_scale;

        Timer::Timestamp m_tFrame;
        Timer::Timestamp m_t;

        quetzal::com_ptr<ID3DXEffect> m_pEffect;
        D3DXHANDLE m_hTechnique;
        D3DXHANDLE m_hWVP;
    };

} // namespace cubes

} // namespace quetzal

#endif // QUETZAL_CUBES_APPLICATION_HPP
