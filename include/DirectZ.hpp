#pragma once
#include <dz/Shader.hpp>
#include <dz/Window.hpp>
struct DirectRegistry;
DirectRegistry* dz_create_registry();
void dz_free_registry(DirectRegistry* directreg);
#include <dz/math.hpp>