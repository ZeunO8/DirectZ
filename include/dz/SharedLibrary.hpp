#pragma once

#include <memory>

namespace dz {
    struct SharedLibrary;

    SharedLibrary* sl_create(const char* sl_path);

    void sl_free(SharedLibrary*);

    void* sl_get_proc(const SharedLibrary* sl, const char* proc_name);

    template <typename F>
    F sl_get_proc_tmpl(const SharedLibrary* sl, const char* proc_name) {
        return (F)sl_get_proc(sl, proc_name);
    }
}