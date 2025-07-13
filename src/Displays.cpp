#include <dz/Displays.hpp>

#if defined(_WIN32) || defined(__linux__)

#if defined(_WIN32)
#include <Windows.h>
#include <shellscalingapi.h>
#pragma comment(lib, "Shcore.lib")

namespace {
    struct MonitorEnumData {
        int count = 0;
        int target_index = -1;
        dz::DisplayDescription result = {};
    };

    BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT lprcMonitor, LPARAM lParam)
    {
        MonitorEnumData* data = reinterpret_cast<MonitorEnumData*>(lParam);
        if (data->count == data->target_index)
        {
            MONITORINFOEX info;
            info.cbSize = sizeof(MONITORINFOEX);
            if (GetMonitorInfo(hMonitor, &info))
            {
                UINT dpi_x = 96, dpi_y = 96;
                GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);

                data->result = {
                    info.rcMonitor.left,
                    info.rcMonitor.top,
                    info.rcMonitor.right - info.rcMonitor.left,
                    info.rcMonitor.bottom - info.rcMonitor.top,
                    info.rcWork.left,
                    info.rcWork.top,
                    info.rcWork.right - info.rcWork.left,
                    info.rcWork.bottom - info.rcWork.top,
                    (float)dpi_x / 96.0f
                };
            }
            return FALSE; // stop enumeration
        }
        data->count++;
        return TRUE;
    }
}

int dz::displays_get_count()
{
    int count = 0;
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR, HDC, LPRECT, LPARAM lParam) -> BOOL {
        int* pCount = reinterpret_cast<int*>(lParam);
        (*pCount)++;
        return TRUE;
    }, reinterpret_cast<LPARAM>(&count));
    return count;
}

DisplayDescription dz::displays_describe(int display_index)
{
    MonitorEnumData data;
    data.target_index = display_index;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&data));
    return data.result;
}

#elif defined(__linux__) && !defined(ANDROID)

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

int dz::displays_get_count()
{
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return 0;
    Window root = DefaultRootWindow(dpy);
    XRRScreenResources* res = XRRGetScreenResources(dpy, root);
    int count = 0;
    if (res) {
        count = res->noutput;
        XRRFreeScreenResources(res);
    }
    XCloseDisplay(dpy);
    return count;
}

DisplayDescription dz::displays_describe(int display_index)
{
    Display* dpy = XOpenDisplay(nullptr);
    DisplayDescription desc = {};
    if (!dpy) return desc;

    Window root = DefaultRootWindow(dpy);
    XRRScreenResources* res = XRRGetScreenResources(dpy, root);
    if (!res) {
        XCloseDisplay(dpy);
        return desc;
    }

    if (display_index >= 0 && display_index < res->noutput)
    {
        RROutput output = res->outputs[display_index];
        XRROutputInfo* output_info = XRRGetOutputInfo(dpy, res, output);
        if (output_info && output_info->crtc)
        {
            XRRCrtcInfo* crtc_info = XRRGetCrtcInfo(dpy, res, output_info->crtc);
            if (crtc_info)
            {
                desc.x = crtc_info->x;
                desc.y = crtc_info->y;
                desc.width = crtc_info->width;
                desc.height = crtc_info->height;
                desc.work_x = desc.x;
                desc.work_y = desc.y;
                desc.work_width = desc.width;
                desc.work_height = desc.height;
                desc.dpi_scale = 1.0f; // DPI querying on X11 is non-standard

                XRRFreeCrtcInfo(crtc_info);
            }
        }
        if (output_info) XRRFreeOutputInfo(output_info);
    }

    XRRFreeScreenResources(res);
    XCloseDisplay(dpy);
    return desc;
}

#elif defined(ANDROID)

int dz::displays_get_count()
{
    return 1;
}

DisplayDescription dz::displays_describe(int display_index)
{
    DisplayDescription desc = {};
    desc.x = 0;
    desc.y = 0;
    desc.width = 1920;
    desc.height = 1080;
    desc.work_x = desc.x;
    desc.work_y = desc.y;
    desc.work_width = desc.width;
    desc.work_height = desc.height;
    desc.dpi_scale = 1.0f;
    return desc;
}

#endif // platform

#endif // win32/linux
