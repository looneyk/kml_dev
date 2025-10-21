#if !defined(QUETZAL_D2D_APPLICATION_HPP)
#define QUETZAL_D2D_APPLICATION_HPP
//------------------------------------------------------------------------------
// Application.hpp
//
//------------------------------------------------------------------------------

#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <string>
#include "quetzal/com_ptr.hpp"
#include "quetzal/Platform.hpp"

namespace quetzal
{

namespace d2d
{

    //--------------------------------------------------------------------------
    class Application
    {
    public:

        Application(HINSTANCE hinstance, int nCmdShow, const wchar_t* name, int idMenu);
        Application(const Application&) = delete;
        virtual ~Application();

        Application& operator=(const Application&) = delete;

        void run();

        virtual LRESULT message_handler(UINT message, WPARAM wParam, LPARAM lParam);

    protected:

        virtual HRESULT create_device_resources();
        virtual void release_device_resources();

        virtual void on_render();
        virtual void on_resize(UINT w, UINT h);

        virtual LRESULT message_handler_(UINT message, WPARAM wParam, LPARAM lParam) = 0;

        virtual HRESULT create_device_resources_() = 0;
        virtual void release_device_resources_() = 0;
        virtual void render_() = 0;

        com_ptr<ID2D1Factory> m_pID2D1Factory;
        com_ptr<ID2D1HwndRenderTarget> m_pRenderTarget;

    private:

        void init_();

        Platform m_platform;
        HINSTANCE m_hinstance;
        int m_nCmdShow;
        std::wstring m_name;
        int m_idMenu;
        HWND m_hwnd;
    };

} // namespace d2d

} // namespace quetzal

#endif // QUETZAL_D2D_APPLICATION_HPP
