#include <DirectZ.hpp>

namespace dz {
    struct SharedLibrary {
#if defined(_WIN32)
        HMODULE ptr = nullptr;
#elif defined(__linux__) || defined(MACOS) || defined(ANDROID) || defined(IOS)
        void* ptr = nullptr;
#endif
    };
}

dz::SharedLibrary* dz::sl_create(const char* sl_path) {
    return new SharedLibrary{
#if defined(_WIN32)
        .ptr = LoadLibraryA(sl_path)
#elif defined(__linux__) || defined(MACOS) || defined(ANDROID) || defined(IOS)
        .ptr = dlopen(_strn__.c_str(), RTLD_NOW | RTLD_GLOBAL)
#endif
    };
}
    
void dz::sl_free(SharedLibrary* sl_ptr) {
    auto& sl = *sl_ptr;
    if (sl.ptr) {
#if defined(_WIN32)
        FreeLibrary(sl.ptr);
#elif defined(__linux__) || defined(MACOS) || defined(ANDROID) || defined(IOS)
        dlclose(sl.ptr);
#endif
        sl.ptr = nullptr;
    }
    delete sl_ptr;
    return;
}

void* dz::sl_get_proc(const SharedLibrary* sl_ptr, const char* proc_name) {
    auto& sl = *sl_ptr;
#ifdef _WIN32
    void* proc = (void*)GetProcAddress(sl.ptr, proc_name);
#elif defined(__linux__) || defined(MACOS) || defined(ANDROID) || defined(IOS)
    void* proc = dlsym(sl.ptr, proc_name);
#endif
    return proc;
}