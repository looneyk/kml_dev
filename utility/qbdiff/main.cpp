//------------------------------------------------------------------------------
// qbdiff
// main.cpp
//------------------------------------------------------------------------------

// Improve comparison speed more?

#include "quetzal/common/Arguments.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <ctime>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>

#define VERSION_STRING "1.1.0.0"

using namespace std;

namespace
{

    const string ArgDifferencesCountedMax = "DifferencesCountedMax";
    const string ArgDifferencesDisplayedMax = "DifferencesDisplayedMax";
    const string ArgRecursive = "Recursive";
    const string ArgDisplayFileTime = "DisplayFileTime";
    const string ArgDisplayMultiHeader = "DisplayMultiHeader";
    const string ArgIgnoreSizeDifference = "IgnoreSizeDifference";

    //--------------------------------------------------------------------------
    int compare_file(const string& strFile1, const string& strFile2, const quetzal::Arguments& args)
    {
        cout << "Comparing " << strFile1 << " and " << strFile2 << endl;

/*
        struct _stati64 s1;
        int fh;
        int i1;

        if( (fh = _open( "f_stat.out", _O_CREAT | _O_WRONLY | _O_TRUNC )) ==  -1 )

        // Get data associated with "fh":
        i1 = _fstati64( fh, &s1 );
        if( i1 != 0 )
        {
            printf( "Bad file handle\n" );
        }
        else
        {
            printf( "File size     : %ld\n", s1.st_size );
            printf( "Time modified : %s", ctime( &s1.st_ctime ) );
        }
        _close( fh );
*/

        struct _stati64 s1;
        int i1 = _stati64(strFile1.c_str(), &s1);
        if (i1 != 0)
        {
            cout << "Error opening file " << strFile1 << endl;
            return -1;
        }

        struct _stati64 s2;
        int i2 = _stati64(strFile2.c_str(), &s2);
        if (i2 != 0)
        {
            cout << "Error opening file " << strFile2 << endl;
            return -1;
        }

        // Display file timestamps if requested
        if (args.named_value<bool>(ArgDisplayFileTime))
        {
            string strTime = ctime(&s1.st_mtime);
            string strTime1 = strTime.substr(20, 4) + " " + strTime.substr(4, 15);
            strTime = ctime(&s2.st_mtime);
            string strTime2 = strTime.substr(20, 4) + " " + strTime.substr(4, 15);

            cout << "    ";
            cout << setw(20) << strTime1;
            cout << "    ";
            cout << setw(20) << strTime2;
            cout << endl;
        }

        // Check file size
        if (s1.st_size != s2.st_size)
        {
            cout << "    ";
            cout << setw(20) << setfill(' ') << static_cast<int>(s1.st_size);
            cout << "    ";
            cout << setw(20) << setfill(' ')  << static_cast<int>(s2.st_size);
            cout << endl;
            cout << "Files have different sizes." << endl;
            if (!args.named_value<bool>(ArgIgnoreSizeDifference))
            {
                return 1;  // Should be a separate code for this ...
            }
        }

        // Open files

        ifstream ifs1(strFile1.c_str(), ios::in | ios::binary);
        if (!ifs1)
        {
            cout << "Error opening file " << strFile1 << endl;
            return -1;
        }

        ifstream ifs2(strFile2.c_str(), ios::in | ios::binary);
        if (!ifs2)
        {
            cout << "Error opening file " << strFile2 << endl;
            return -1;
        }

        // Compare file contents

        int nAddress = 0;
        int nDifferences = 0;
        char c1;
        char c2;

        const int nDifferencesCountedMax = args.named_value<int>(ArgDifferencesCountedMax);
        const int nDifferencesDisplayedMax = args.named_value<int>(ArgDifferencesDisplayedMax);

        while (ifs1.get(c1) && ifs2.get(c2))
        {
            if (c1 != c2)
            {
                ++nDifferences;

                if (nDifferencesCountedMax >= 0 && nDifferences > nDifferencesCountedMax)
                {
                    cout << dec << "More than " << nDifferencesCountedMax << " difference";
                    cout << (nDifferencesCountedMax == 1 ? " was" : "s were");
                    cout <<" found." << endl;
                    return nDifferences;
                }

                if (nDifferencesDisplayedMax < 0 || nDifferences <= nDifferencesDisplayedMax)
                {
                    unsigned char uc1 = c1;
                    unsigned char uc2 = c2;

                    cout << "  " << hex << setfill('0');
                    cout << "  " << setw(8) << nAddress;
                    cout << "  " << setw(2) << static_cast<unsigned int>(uc1);
                    cout << "  " << setw(2) << static_cast<unsigned int>(uc2);
                    cout << endl;
                }
            }

            ++nAddress;
        }

        // ...
        // is bad necessary?
    //   if (ifs1.fail() || ifs1.bad())
        if (!ifs1.eof())
        {
            cerr << "Error reading file " << strFile1 << endl;
            return -1;
        }

        if (ifs2.fail() || ifs2.bad())
        {
            cerr << "Error reading file " << strFile2 << endl;
            return -1;
        }

        if (nDifferences == 0)
        {
//            cout << "Files are identical.  " << setw(12) << nAddress << " characters." << endl;
            cout << "Files are identical." << endl;
        }
        else if (nDifferencesCountedMax < 0 || nDifferences <= nDifferencesCountedMax)
        {
            cout << dec << nDifferences << " difference";
            cout << (nDifferences == 1 ? " was" : "s were");
            cout <<" found." << endl;
        }

        return nDifferences;
    }

    //--------------------------------------------------------------------------
    int compare_dir(const string& strDir1, const string& strDir2, const quetzal::Arguments& args)
    {
        struct _finddatai64_t fd1;
        intptr_t hFile1 = _findfirsti64(strDir1.c_str(), &fd1);
        if (hFile1 == -1L)
        {
            cout << "No files found matching " << strDir1 << endl;
            return 0;
        }

        char szDrive1[_MAX_DRIVE];
        char szDir1[_MAX_DIR];

        _splitpath(strDir1.c_str(), szDrive1, szDir1, 0, 0);

        char szPath1[_MAX_PATH];
        char szPath2[_MAX_PATH];

        int nFilesCompared = 0;

        do
        {
            _makepath(szPath1, szDrive1, szDir1, fd1.name, 0);
            _makepath(szPath2, 0, strDir2.c_str(), fd1.name, 0);

            if ((fd1.attrib & _A_SUBDIR) == 0)  // Not a directory match
            {
//                if (nFilesCompared > 0)
                {
                    cout << endl;
                }

                compare_file(szPath1, szPath2, args);

                ++nFilesCompared;
            }
            else if (args.named_value<bool>(ArgRecursive))
            {
                if (strcmp(fd1.name, ".") != 0 && strcmp(fd1.name, "..") != 0)  // Don't compare . and ..
                {
                    string strPath1(szPath1);
                    strPath1 += "\\*";
                    nFilesCompared += compare_dir(szPath1, szPath2, args);
                }
            }
        } while (_findnexti64(hFile1, &fd1) == 0);

        _findclose(hFile1);

        return nFilesCompared;
    }

    //--------------------------------------------------------------------------
    void trim_dir_separator(string& strFile)
    {
        if (strFile.length() > 0 && strFile.substr(strFile.length() - 1, 1) == "\\")
        {
            strFile = strFile.substr(0, strFile.length() - 1);
        }

        return;
    }

    //--------------------------------------------------------------------------
    void usage()
    {
        cout << endl;
        cout << "Binary File Difference" << endl;
        cout << VERSION_STRING << endl;
        cout << endl;
        cout << "qbdiff <options> file1 file2" << endl;
        cout << endl;
        cout << "<options>" << endl;
        cout << "    -c n    Count no more than n differences" << endl;
        cout << "    -d n    Display no more than n differences" << endl;
        cout << "    -s b    Recurse into subdirectories, default false"  << endl;
        cout << "    -t b    Display file timestamps, default false" << endl;
        cout << "    -h b    Display header and footer for multi-file compares, default true"  << endl;
        cout << "    -z b    Allow comparison of files with different sizes, default false"  << endl;
        cout << "b is an optional boolean parameter, true or false; without it, the default is toggled" << endl;
        cout << "file1 may specify a directory or contain wildcards in which case" << endl;
        cout << "file2 must specify a directory containing the files to be compared with those matching file1" << endl;

        return;
    }

}  // namespace

//----------------------------------------------------------------------------------
int main (int argc, char* argv[])
{
    // Parse arguments

    quetzal::Arguments args;
    args.register_argument(ArgDifferencesCountedMax, "", "-c", 1, true, "-1");
    args.register_argument(ArgDifferencesDisplayedMax, "", "-d", 1, true, "-1");
    args.register_argument(ArgRecursive, "", "-s", 1, false, "false", "true");
    args.register_argument(ArgDisplayFileTime, "", "-t", 1, false, "false", "true");
    args.register_argument(ArgDisplayMultiHeader, "", "-h", 1, false, "true", "false");
    args.register_argument(ArgIgnoreSizeDifference, "", "-z", 1, false, "false", "true");

    args.parse(argc, argv);

// cout << args << endl; // ...

    if (args.positional() != 2)
    {
        usage();
        return 1;
    }

    string strFile1 = args.positional_value(0);
    string strFile2 = args.positional_value(1);

    // Find all matching files and compare

    trim_dir_separator(strFile1);
    struct _stati64 s1;
    int i1 = _stati64(strFile1.c_str(), &s1);

    if (i1 != 0)
    {
        // Assume that file1 contains wildcards
    }
    else if ((s1.st_mode & _S_IFDIR) != 0)  // file 1 is a directory
    {
        strFile1 += "\\*";
    }
    else if ((s1.st_mode & _S_IFREG) != 0)  // file 1 is a single file
    {
    }
    else
    {
        cerr << "File type not supported for " << strFile1 << endl;
        return 1;
    }

    trim_dir_separator(strFile2);
    struct _stati64 s2;
    int i2 = _stati64(strFile2.c_str(), &s2);

    if (i2 != 0)
    {
        cerr << "Error opening " << strFile2 << endl;
        return 1;
    }
    else if ((s2.st_mode & _S_IFDIR) != 0)  // file 2 is a directory
    {
    }
    else if ((s2.st_mode & _S_IFREG) != 0)  // file 2 is a single file
    {
        if (i1 != 0 || (s1.st_mode & _S_IFDIR) != 0)  // file 1 specifies multiple files
        {
            cerr << "Cannot compare multiple files " << strFile1 << endl;
            cerr << "with a single file            " << strFile2 << endl;
            return 1;
        }
    }
    else
    {
        cerr << "File type not supported for " << strFile2 << endl;
        return 1;
    }

/*
file1 is a file
    file2 is a file
        compare file1 with file2
    file2 is a directory
        compare file1 with path2/file1
file1 contains wildcards
    file2 is a file
        error
    file2 is a directory
        compare all file1 matches with path2/file1match
file1 is a directory
    file2 is a file
        error
    file2 is a directory
        file1 += \*
        compare all file1 matches with path2/file1match
*/

    // Perform single file compare here

    if ((s1.st_mode & _S_IFREG) != 0 && (s2.st_mode & _S_IFREG) != 0)
    {
        compare_file(strFile1, strFile2, args);
        return 0;
    }

    // Perform multi file compare

    bool bDisplayMultiHeader = args.named_value<bool>(ArgDisplayMultiHeader);

    if (bDisplayMultiHeader)
    {
        cout << "Comparing " << strFile1 << " with files in " << strFile2 << endl;
    }

    int nFilesCompared = compare_dir(strFile1, strFile2, args);

    if (bDisplayMultiHeader)
    {
//        if (nFilesCompared > 0)
        {
            cout << endl;
        }

        cout << nFilesCompared << " " << (nFilesCompared == 1 ? "file" : "files") << " compared." << endl;
    }

    return 0;
}
