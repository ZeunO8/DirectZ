#include <DirectZ.hpp>
int main()
{
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
