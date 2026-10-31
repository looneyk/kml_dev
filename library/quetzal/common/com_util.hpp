#if !defined(QUETZAL_COM_UTIL_HPP)
#define QUETZAL_COM_UTIL_HPP
//------------------------------------------------------------------------------
// quetzal common
// com_util.hpp
//
// add com exception
//------------------------------------------------------------------------------

#include <objbase.h>
#include <string>
#include <cassert>

namespace quetzal
{

    //----------------------------------------------------------------------
    template<typename T>
    void com_release(T*& p)
    {
        if (p != nullptr)
        {
            p->Release();
            p = nullptr;
        }

        return;
    }

    //----------------------------------------------------------------------
    template<typename T, typename U>
    HRESULT STDMETHODCALLTYPE com_interface(U* pUnknown, void** ppv)
    {
        assert(pUnknown);
        assert(ppv);

        IUnknown* pUnk = static_cast<T*>(pUnknown);
        pUnk->AddRef();
        *ppv = pUnk;

        return S_OK;
    }

    HRESULT register_server(CLSID clsid, const std::wstring& strFile, const std::wstring& strDescription, const std::wstring& strProgID, const std::wstring& strProgID1, const std::wstring& strType = L"InprocServer32", const std::wstring& strThreadingModel = L"Both");
    HRESULT unregister_server(CLSID clsid);

} // namespace quetzal

#endif // QUETZAL_COM_UTIL_HPP
