#include <DirectZ.hpp>
#include "Directz.cpp.hpp"
SharedMemoryPtr<DirectRegistry> dr_shm;
bool dr_shm_initialized = init_direct_registry(dr_shm);
DirectRegistry* dr_ptr = dr_shm.ptr;