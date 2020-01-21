#include "HPCEngine.h"
#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

INT WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
#else
int main(int argc, char** argv)
#endif
{
    // Run the engine
    return HPCEngine::run() ? 0 : 1;
}