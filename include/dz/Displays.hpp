#pragma once

namespace dz {
    struct DisplayDescription
    {
        int x, y;
        int width, height;
        int work_x, work_y;
        int work_width, work_height;
        float dpi_scale;
    };

    int displays_get_count();
    DisplayDescription displays_describe(int display_index);
}