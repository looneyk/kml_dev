
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace std;

map<unsigned int, int> state_priority
{
    {0x00008000, 20},
    {0x00006000, 19},
    {0x00002000, 18},
    {0x00005400, 17},
    {0x00005000, 16},
    {0x00004c00, 15},
    {0x00004800, 14},
    {0x00004400, 13},
    {0x00004000, 12},
    {0x00001400, 11},
    {0x00001000, 10},
    {0x00000a00, 9},
    {0x00000900, 8},
    {0x00000800, 7},
    {0x00000600, 6},
    {0x00000500, 5},
    {0x00000400, 4},
    {0x00000200, 3},
    {0x00000100, 2},
    {0x00000000, 1}
};

map<int, unsigned int> state_priority_order
{
    {20, 0x00008000},
    {19, 0x00006000},
    {18, 0x00002000},
    {17, 0x00005400},
    {16, 0x00005000},
    {15, 0x00004c00},
    {14, 0x00004800},
    {13, 0x00004400},
    {12, 0x00004000},
    {11, 0x00001400},
    {10, 0x00001000},
    {9, 0x00000a00},
    {8, 0x00000900},
    {7, 0x00000800},
    {6, 0x00000600},
    {5, 0x00000500},
    {4, 0x00000400},
    {3, 0x00000200},
    {2, 0x00000100},
    {1, 0x00000000}
};

vector<unsigned int> states
{
    0x00008000,
    0x00006000,
    0x00002000,
    0x00005400,
    0x00005000,
    0x00004c00,
    0x00004800,
    0x00004400,
    0x00004000,
    0x00001400,
    0x00001000,
    0x00000a00,
    0x00000900,
    0x00000800,
    0x00000600,
    0x00000500,
    0x00000400,
    0x00000200,
    0x00000100,
    0x00000000
};

vector<unsigned int> available_states
{
    0x00008000,
    0x00006000,
    0x00002000,
    0x00005400,
    0x00005000,
    0x00004c00,
    0x00004800,
    0x00004400,
    0x00004000,
    0x00001400,
    0x00001000,
    0x00000a00,
    0x00000900,
    0x00000800,
    0x00000600,
    0x00000500,
    0x00000400,
    0x00000200,
    0x00000100,
    0x00000000
};

unsigned int match(unsigned int state)
{
//    int priorityMax = -1;
//    unsigned int stateMax = 0;
//
//    for (auto wt : state_priority)
//    {
//        if ((state & wt.first) == state) // Check if this is a possibility
//        {
//            if (state_priority[state] > priorityMax) // Check against previous priority
//            {
//                priorityMax = state_priority[state];
//                stateMax = wt.first;
//            }
//        }
//    }
//
//    return stateMax;

    for (auto state_available : available_states)
    {
        if ((state & state_available) == state_available)
        {
            return state_available;
        }
    }

    return 0;
}

unsigned int priority(unsigned int state, unsigned int state_available)
{
    if ((state & state_available) == state_available) // Check if this is a possibility
    {
        return state_priority[state_available];
    }

    return 0;
}

unsigned int f(unsigned state, unsigned int state_available)
{
    return (state_available & state) == state;
}

int main(int argc, char* argv[]
{
    cout << setw(6) << "" << "state_available" << endl;

    cout << "state ";

    for (auto state_available : available_states)
    {
        cout << setw(6) << hex << state_available;
    }

    cout << endl;

    for (auto state : states)
    {
        cout << setw(6) << hex << state;

        for (auto state_available : available_states)
        {
//            cout << setw(6) << hex << f(state_available, state);
//            cout << setw(6) << dec << priority(state, state_available);
            cout << setw(6) << hex << match(state);
        }

        cout << endl;
    }


    return 0;
}
