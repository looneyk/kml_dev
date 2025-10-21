//------------------------------------------------------------------------------
//
//  Copyright (c) 2010 Keith Michael Looney. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
//  qwhere
//  main.cpp
//
//------------------------------------------------------------------------------

/*
options
    display type (compact/verbose)
    date/time display (on/off)
    recurse beginning with a specified dir (only if a path is explicitly specified)

directories should not be included in output
should warn when both path and env_var are specified
*/

#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <io.h>

using namespace std;

namespace
{

    //----------------------------------------------------------------------
    long find_files(const char* szDir, const char* szFile, bool bCompact)
    {
        char szFilePattern[_MAX_PATH];
        _makepath(szFilePattern, 0, szDir, szFile, 0);

        long nFiles = 0;
        int rc = 0;

        struct _finddata_t fd;
        long hFile = _findfirst(szFilePattern, &fd);
        if (hFile != -1)
        {
            if (!bCompact)
            {
                cout << endl;
                cout << szDir << endl;
            }

            while (rc == 0)
            {
                long lSize = fd.size;
                string strTime = ctime(&(fd.time_write));
                if (bCompact)
                {
                    char szPath[_MAX_PATH];
                    _makepath(szPath, 0, szDir, fd.name, 0);
                    cout << "    " << left << setw(16) << strTime.substr(0, 24);
                    cout << "    " << right << setw(12) << lSize;
                    cout << "    " << left << szPath;
                    cout << endl;
                }
                else
                {
                    cout << "    " << left << setw(32) << fd.name;
                    cout << "    " << left << setw(16) << strTime.substr(0, 24);
                    cout << "    " << right << setw(12) << lSize;
                    cout << endl;
                }

                ++nFiles;

                rc = _findnext(hFile, &fd);
            }
        }

        _findclose(hFile);

        return nFiles;
    }

    //----------------------------------------------------------------------
    void usage()
    {
        cout << endl;
        cout << "Environment File Search" << endl;
        cout << endl;
        cout << "qwhere file_pattern [env_var]" << endl;
        cout << endl;
        cout << "If file_pattern contains a path, it will be used in place of env_var." << endl;
        cout << "Both env_var and a file_pattern containing a path should not be specified at the same time." << endl;
        cout << "env_var defaults to PATH (in this case, the current directory will also be searched)" << endl;

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    try
    {
        if (argc < 2 || argc > 3)
        {
            usage();
            return 0;
        }

        char szDrive[_MAX_DRIVE];
        char szDir[_MAX_DIR];
        char szFName[_MAX_FNAME];
        char szExt[_MAX_EXT];
        _splitpath(argv[1], szDrive, szDir, szFName, szExt);

        if (strlen(szExt) < 1)
        {
            strcpy(szExt, ".*");
        }

        string strFilePath = szDrive;
        strFilePath += szDir;

        string strFilePattern = szFName;
        strFilePattern += szExt;

        string strEnvVar = "PATH";
        string strEnv;

        if (strFilePath.empty())
        {
            if (argc > 2)
            {
                strEnvVar = argv[2];
            }

            strEnv = getenv(strEnvVar.c_str());
        }
        else
        {
            strEnv = strFilePath;
        }

        char* szEnv = new char[strEnv.length() + 1];
        if (szEnv == 0)
        {
            throw bad_alloc();
        }

        cout << endl;

        long nFiles = 0;

        if (argc < 3)
        {
            //  No environment variable specified, look in current directory first
            nFiles += find_files(".", strFilePattern.c_str(), true);
        }

        strcpy(szEnv, strEnv.c_str());

        char* pszToken = strtok(szEnv, ";");
        while (pszToken != NULL)
        {
            nFiles += find_files(pszToken, strFilePattern.c_str(), true);
            pszToken = strtok(NULL, ";");
        }

        if (nFiles > 0)
        {
            cout << endl;
        }
        cout << nFiles << " files found." << endl;

        delete szEnv;
    }
    catch (bad_alloc&)
    {
        cerr << "Error allocating memory." << endl;
        return 1;
    }
/*
    catch (...)
    {
    }
*/

    return 0;
}
