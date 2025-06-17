#include <DirectZ.hpp>
#include <windows.h>
#include <dxgi.h>
#include <dxgi1_2.h> // for IDXGIFactory1 or newer
#include <comdef.h>  // for _com_ptr_t if you want smart COM ptrs
#include <initguid.h> // sometimes needed for __uuidof
int main()
{
    // ::LoadLibraryA("d3d12.dll");
    // IDXGIFactory* dummy = nullptr;
    // CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&dummy);
    // if (dummy)
    //     dummy->Release();
    int ret = 0;

    // call app-lib implementation of init
    if ((ret = init({
        .title = "Example Window",
        .x = 128,
        .y = 128,
        .width = 640,
        .height = 480
    })))
        return ret;

    //
    while (poll_events())
    {
        update();
        render();
    }
    return ret;
}