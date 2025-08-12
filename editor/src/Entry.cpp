#include <DirectZ.hpp>
using namespace dz;
int main() {
    auto window = window_create({
        .title = "DirectZ Editor",
        .width = 1280,
        .height = 768,
        .borderless = true,
        .vsync = true
    });
    while(windows_poll_events()) {
        windows_render();
    }
}