#pragma once
#include "Window.hpp"
struct DirectRegistry;
namespace dz {
    /**
    * @brief returns the beginning of DirectRegistry window_ptrs
    */
    std::vector<WINDOW*>::iterator dr_get_windows_begin();
    /**
    * @brief returns the end of DirectRegistry window_ptrs
    */
    std::vector<WINDOW*>::iterator dr_get_windows_end();
}