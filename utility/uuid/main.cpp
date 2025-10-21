
#include <windows.h>

int main(int argc, char* argv[])
{
    argc;
    argv;

    UUID uuid0;
    UUID uuid1;
    UUID uuid2;
    UUID uuid3;
    RPC_STATUS x = UuidCreateSequential(&uuid0);
    x = UuidCreateSequential(&uuid1);
    x = UuidCreateSequential(&uuid2);
    x = UuidCreateSequential(&uuid3);

    return 0;
}