//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <cassert>

using namespace std;

namespace
{

    const int s_nCycles = 4096;
    const int s_nBits = 9216; // 256 * 3 * 12

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    int nBits = 0;
/*
    int nBitPrev = 0;

    for (int nCycle = 0; nCycle < s_nCycles; ++nCycle)
    {
        int nBit = (s_nBits * (nCycle + 1) + s_nCycles / 2) / s_nCycles;
        int nBitsCycle = nBit - nBitPrev;

        nBitPrev = nBit;

if (nCycle < 16 || nCycle > 4080)
        cout << nCycle;

        for (int i = 0; i < nBitsCycle; ++i)
        {
if (nCycle < 16 || nCycle > 4080)
            cout << "\t" << nBits;
            ++nBits;
        }

if (nCycle < 16 || nCycle > 4080)
        cout << endl;
    }
*/
    for (int nCycle = 0; nCycle < s_nCycles; ++nCycle)
    {
if (nCycle < 16 || nCycle > 4080)
        cout << nCycle;

        int nBit = (s_nBits * (nCycle + 1) + s_nCycles / 2) / s_nCycles;
        while (nBits < nBit)
        {
if (nCycle < 16 || nCycle > 4080)
            cout << "\t" << nBits;
            ++nBits;
        }

if (nCycle < 16 || nCycle > 4080)
        cout << endl;
    }

    cout << nBits << " total" << endl;
//    assert(nBits == s_nBits);

    string s;
    getline(cin, s);

    return 0;
}
