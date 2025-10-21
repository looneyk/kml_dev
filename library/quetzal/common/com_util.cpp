//------------------------------------------------------------------------------
//
// Copyright (c) 2010 Keith Michael Looney. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
// com_util.cpp
//
//------------------------------------------------------------------------------

#include "com_util.hpp"
#include "Registry.hpp"
#include <strmif.h>

using namespace std;

//------------------------------------------------------------------------------
HRESULT quetzal::common::register_server(CLSID clsid, const wstring& strFile, const wstring& strDescription, const wstring& strProgID, const wstring& strProgID1, const wstring& strType, const wstring& strThreadingModel)
{
    wchar_t szCLSID[CHARS_IN_GUID];
    StringFromGUID2(clsid, szCLSID, CHARS_IN_GUID);

    try
    {
        Registry::Key rk(HKEY_CLASSES_ROOT, wstring(L"CLSID\\") + szCLSID);
        rk.set_default(strDescription);

        Registry::Key rkSub(rk, strType);
        rkSub.set_default(strFile);
        rkSub.set(wstring(L"ThreadingModel"), strThreadingModel);

        rkSub.create(rk, L"VersionIndependentProgID");
        rkSub.set_default(strProgID);

        rkSub.create(rk, L"ProgID");
        rkSub.set_default(strProgID1);

        rk.create(HKEY_CLASSES_ROOT, strProgID);
        rk.set_default(strDescription);

        rkSub.create(rk, L"CLSID");
        rkSub.set_default(szCLSID);

        rkSub.create(rk, L"CurVer");
        rkSub.set_default(strProgID1);

        rk.create(HKEY_CLASSES_ROOT, strProgID1);
        rk.set_default(strDescription);

        rkSub.create(rk, L"CLSID");
        rkSub.set_default(szCLSID);
    }
    catch (quetzal::XRegistry& e)
    {
        return HRESULT_FROM_WIN32(e.error());
    }
    catch (exception&)
    {
        return E_FAIL;
    }

    return ERROR_SUCCESS;
}

//---------------------------------------------------------------------------
HRESULT quetzal::common::unregister_server(CLSID clsid)
{
    wchar_t szCLSID[CHARS_IN_GUID];
    StringFromGUID2(clsid, szCLSID, CHARS_IN_GUID);

    try
    {
        Registry::Key rkHKCR(HKEY_CLASSES_ROOT);

        wstring strKey = wstring(L"CLSID\\") + szCLSID + L"\\ProgID";
        Registry::Key rk;
        rk.open(HKEY_CLASSES_ROOT, strKey);
        if (rk.opened())
        {
            wstring str;
            bool bOK = rk.query_default(str);
            if (bOK)
            {
                rkHKCR.delete_key(str, true);
            }
        }

        strKey = wstring(L"CLSID\\") + szCLSID + L"\\VersionIndependentProgID";
        rk.open(HKEY_CLASSES_ROOT, strKey);
        if (rk.opened())
        {
            wstring str;
            bool bOK = rk.query_default(str);
            if (bOK)
            {
                rkHKCR.delete_key(str, true);
            }
        }

        rkHKCR.delete_key(wstring(L"CLSID\\") + szCLSID, true);
    }
    catch (quetzal::XRegistry& e)
    {
        return HRESULT_FROM_WIN32(e.error());
    }
    catch (exception&)
    {
        return E_FAIL;
    }

    return ERROR_SUCCESS;
}
