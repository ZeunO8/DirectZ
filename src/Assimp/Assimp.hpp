#pragma once
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <dz/math.hpp>
template <typename T, typename R>
R AssimpConvert(T val)
{
	if constexpr (std::is_same_v<T, aiMatrix4x4>) {
		if constexpr (std::is_same_v<R, mat<float, 4, 4>>)
        {
            mat<float, 4, 4> m(0.0f);
            m[0][0] = val.a1;
            m[1][0] = val.a2;
            m[2][0] = val.a3;
            m[3][0] = val.a4;
            m[0][1] = val.b1;
            m[1][1] = val.b2;
            m[2][1] = val.b3;
            m[3][1] = val.b4;
            m[0][2] = val.c1;
            m[1][2] = val.c2;
            m[2][2] = val.c3;
            m[3][2] = val.c4;
            m[0][3] = val.d1;
            m[1][3] = val.d2;
            m[2][3] = val.d3;
            m[3][3] = val.d4;
			return m;
        }
    }
	else if constexpr (std::is_same_v<T, mat<float, 4, 4>>) {
		if constexpr (std::is_same_v<R, aiMatrix4x4>)
        {
            return aiMatrix4x4(val[0][0], val[1][0], val[2][0], val[3][0],
                val[0][1], val[1][1], val[2][1], val[3][1],
                val[0][2], val[1][2], val[2][2], val[3][2],
                val[0][3], val[1][3], val[2][3], val[3][3]);
        }
    }
    else if constexpr (std::is_same_v<T, aiVector2D>) {
        if constexpr (std::is_same_v<R, vec<float, 2>>) {
            return vec<float, 2>(val.x, val.y);
        }
    }
    else if constexpr (std::is_same_v<T, aiVector3D>) {
        if constexpr (std::is_same_v<R, vec<float, 2>>) {
            return vec<float, 2>(val.x, val.y);
        }
        else if constexpr (std::is_same_v<R, vec<float, 3>>) {
            return vec<float, 3>(val.x, val.y, val.z);
        }
        else if constexpr (std::is_same_v<R, vec<float, 4>>) {
            return vec<float, 4>(val.x, val.y, val.z, 1.0f);
        }
    }
    else if constexpr (std::is_same_v<T, aiQuaternion>) {
        if constexpr (std::is_same_v<R, quat<float>>) {
            return quat<float>(val.w, val.x, val.y, val.z);
        }
    }
    else if constexpr (std::is_same_v<T, aiColor4D>) {
        if constexpr (std::is_same_v<R, vec<float, 3>>) {
            return vec<float, 3>(val.r, val.g, val.b);
        }
        else if constexpr (std::is_same_v<R, vec<float, 4>>) {
            return vec<float, 4>(val.r, val.g, val.b, val.a);
        }
    }
    R r;
    return r;
}
