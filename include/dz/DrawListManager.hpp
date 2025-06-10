#pragma once
#include "DrawList.hpp"
#include <functional>
#include "Shader.hpp"
namespace dz
{
    using DrawTToVertexCountFunction = std::function<uint32_t(BufferGroup*, DrawT&)>;
    template<typename DrawT>
    struct DrawListManager
    {
    private:
        DrawTToVertexCountFunction fn_determineDrawTVertexCount;
        std::string draw_key;
    public:
        DrawListManager(const std::string& draw_key, const DrawTToVertexCountFunction& fn_determineDrawTVertexCount):
            fn_determineDrawTVertexCount(fn_determineDrawTVertexCount)
        {}
        DrawList genDrawList(BufferGroup* bg)
        {
            
        }
    };
}