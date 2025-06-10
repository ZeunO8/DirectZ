#pragma once
#include <vector>
#include <unordered_map>
namespace dz
{
	struct DrawIndirectCommand
	{
		uint32_t vertexCount = 0;
		uint32_t instanceCount = 0;
		uint32_t firstVertex = 0;
		uint32_t firstInstance = 0;
	};
    using DrawList = std::vector<DrawIndirectCommand>;
	using ShaderDrawList = std::unordered_map<Shader*, DrawList>;
}