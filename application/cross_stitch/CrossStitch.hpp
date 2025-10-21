#if !defined(QUETZAL_CROSSSTITCH_HPP)
#define QUETZAL_CROSSSTITCH_HPP
//------------------------------------------------------------------------------
// CrossStitch.hpp
//
//------------------------------------------------------------------------------

//#include "quetzal/d2d/Application.hpp"
#include "Application.hpp"

namespace quetzal
{

    //--------------------------------------------------------------------------
    class CrossStitch : public d2d::Application
    {
    public:

        CrossStitch(HINSTANCE hInstance, int nCmdShow);
        CrossStitch(const CrossStitch&) = delete;
        ~CrossStitch();

        CrossStitch& operator=(const CrossStitch&) = delete;

    protected:

        virtual LRESULT message_handler_(UINT message, WPARAM wParam, LPARAM lParam);

        virtual HRESULT create_device_resources_();
        virtual void release_device_resources_();

        virtual void render_();

    private:

        com_ptr<ID2D1SolidColorBrush> m_pLightSlateGrayBrush;
        com_ptr<ID2D1SolidColorBrush> m_pCornflowerBlueBrush;
    };

} // namespace quetzal

#endif // QUETZAL_CROSSSTITCH_HPP
