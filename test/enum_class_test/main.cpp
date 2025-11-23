
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace std;

enum class X
{
    Zero,
    One,
    Two,
    Three
};

int main(int argc, char* argv[])
{
    int i = 1;
    int j = i & X::One;
    int k = X::One & X::Three;

    bool b = X::Three > X::One; 

    return 0;
}
