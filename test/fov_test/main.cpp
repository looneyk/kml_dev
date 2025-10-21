
#include <cmath>
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{ 
    float fov_nominal = 1.047198f;

    for (float f = 0.f; f < 5.f; f += .1f)
    {
        float fov = 2 * atan(1.f / (f * tan(fov_nominal)));
        cout << f << "\t" << fov << endl;
    }

    for (float fov = 3.14f; fov > 0.f; fov -= .1f)
    {
        float f = 1.f / (tan(fov / 2.f) * tan(fov_nominal));
        cout << fov << "\t" << f << endl;
    }

    return 0;
}
