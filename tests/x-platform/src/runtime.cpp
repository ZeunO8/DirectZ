#include <DirectZ.hpp>
int main()
{
    EventInterface* ei = 0;

    // call app-lib implementation of init
    if (!(ei = init({
        .title = "Example Window",
        .x = 128,
        .y = 128,
        .width = 640,
        .height = 480
    })))
        return 1;

    //
    while (poll_events())
    {
        update();
        render();
    }
    return 0;
}
