#include <DirectZ.hpp>
#include "Directz.cpp.hpp"
DirectRegistry* dr_ptr = dz::make_direct_registry();

void set_direct_registry(DirectRegistry* new_dr_ptr) {
    if (new_dr_ptr == dr_ptr)
        return;
    free_direct_registry(dr_ptr);
    dr_ptr = new_dr_ptr;
}