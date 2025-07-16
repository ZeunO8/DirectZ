#pragma once
#include "Provider.hpp"

namespace dz::ecs {
    struct Light : Provider<Light> {
        int type = 0;
    };
}