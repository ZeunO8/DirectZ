#pragma once
#include <cassert>
#include <cmath>
#include <random>
#include <chrono>
#include <stdexcept>
#include <string.h>
namespace dz
{
    template <typename T, size_t N>
    struct vec
    {
    private:
        void load_args(size_t& i)
        { }
        template <typename... Args>
        void load_args(size_t& i, T arg, Args... args)
        {
            data[i++] = arg;
            load_args(i, args...);
        }
        void load_all(T val)
        {
            for (size_t i = 0; i < N; ++i)
                data[i] = val;
        }

    public:
        T data[N];
        template <typename... Args>
        vec(Args... args)
        {
            if constexpr (sizeof...(args) == 0)
                load_all(T());
            else if constexpr (sizeof...(args) == 1)
                load_all(args...);
            else
            {
                size_t i = 0;
                load_args(i, args...);
            }
        }

        template <typename OT, size_t ON>
        vec(const vec<OT, ON>& other)
        {
            size_t i = 0;
            for (; i < N && i < ON; ++i)
                data[i] = other.data[i];
            if (i < N)
                for (; i < N; ++i)
                    data[i] = T();
        }

        vec& operator=(const vec& other)
        {
            memcpy(data, other.data, sizeof(T) * N);
            return *this;
        }
        inline T& operator[](size_t i)
        {
            return data[i];
        }
        inline const T& operator[](size_t i) const
        {
            return data[i];
        }
        template <typename O>
        vec& operator*=(const O& other)
        {
            for (size_t i = 0; i < N; ++i)
            {
                if constexpr (std::is_same_v<O, T>)
                    data[i] *= other;
                else if constexpr (std::is_same_v<O, vec<T, N>>)
                    data[i] *= other.data[i];
                else
                    throw std::runtime_error("Unsupported O type");
            }
            return *this;
        }
        template <typename O>
        vec& operator/=(const O& other)
        {
            for (size_t i = 0; i < N; ++i)
            {
                if constexpr (std::is_same_v<O, T>)
                    data[i] /= other;
                else if constexpr (std::is_same_v<O, vec<T, N>>)
                    data[i] /= other.data[i];
                else
                    throw std::runtime_error("Unsupported O type");
            }
            return *this;
        }
        template <typename O>
        vec& operator+=(const O& other)
        {
            for (size_t i = 0; i < N; ++i)
            {
                if constexpr (std::is_same_v<O, T>)
                    data[i] += other;
                else if constexpr (std::is_same_v<O, vec<T, N>>)
                    data[i] += other.data[i];
                else
                    throw std::runtime_error("Unsupported O type");
            }
            return *this;
        }
        template <typename O>
        vec& operator-=(const O& other)
        {
            for (size_t i = 0; i < N; ++i)
            {
                if constexpr (std::is_same_v<O, T>)
                    data[i] -= other;
                else if constexpr (std::is_same_v<O, vec<T, N>>)
                    data[i] -= other.data[i];
                else
                    throw std::runtime_error("Unsupported O type");
            }
            return *this;
        }
        template <typename O>
        vec operator*(const O& other) const
        {
            auto new_vec = *this;
            return (new_vec *= other);
        }
        template <typename O>
        vec operator/(const O& other) const
        {
            auto new_vec = *this;
            return (new_vec /= other);
        }
        template <typename O>
        vec operator+(const O& other) const
        {
            auto new_vec = *this;
            return (new_vec += other);
        }
        template <typename O>
        vec operator-(const O& other) const
        {
            auto new_vec = *this;
            return (new_vec -= other);
        }
        vec operator-() const
        {
            auto new_vec = *this;
            for (size_t i = 0; i < N; ++i)
                new_vec.data[i] *= T(-1);
            return new_vec;
        }
        T length() const
        {
            T sum = T(0);
            for (size_t i = 0; i < N; ++i)
            {
                sum += data[i] * data[i];
            }
            return std::sqrt(sum);
        }
        vec normalize() const
        {
            T invLen = T(1) / length();
            vec result;
            for (size_t i = 0; i < N; ++i)
            {
                result.data[i] = data[i] * invLen;
            }
            return result;
        }
    };

    template <typename T, size_t C, size_t R>
    struct mat
    {
        T matrix[C][R];
        mat(T initial = T())
        {
            auto sizebytes = sizeof(T) * C * R;
            memset(matrix, 0, sizebytes);
            for (size_t c = 0; c < C && c < R; c++)
                matrix[c][c] = initial;
        }
        mat& operator=(const mat& other)
        {
            memcpy(matrix, other.matrix, sizeof(T) * C * R);
            return *this;
        }
        inline vec<T, R>& operator[](size_t i)
        {
            return *(vec<T, R>*)matrix[i];
        }
        inline const vec<T, R>& operator[](size_t i)const
        {
            return *(vec<T, R>*)matrix[i];
        }
        template <size_t N>
        mat& translate(const vec<T, N>& v)
        {
            static_assert(C == 4 && R == 4 && N == 3);
            auto& pos = (vec<T, N>&)(*this)[3];
            pos += v;
            return *this;
        }
        template <size_t N>
        mat& scale(const vec<T, N>& v)
        {
            for (size_t l = 0; l < N; ++l)
            {
                auto& y = (vec<T, N>&)(*this)[l];
                y *= v[l];
            }
            return *this;
        }
        template <size_t N>
        mat& rotate(T angle, const vec<T, N>& axis)
        {
            static_assert(N == 2 || N == 3, "Rotation axis must be 2D or 3D");
            constexpr size_t D = N;

            T c = std::cos(angle);
            T s = std::sin(angle);
            T ic = T(1) - c;

            vec<T, 3> a{};
            if constexpr (N == 2)
            {
                a[0] = axis[0];
                a[1] = axis[1];
                a[2] = T(0); // implicit Z = 0 for 2D rotation
            }
            else
            {
                a = axis.normalize();
            }

            T x = a[0];
            T y = a[1];
            T z = a[2];

            // Precompute components of rotation (Rodrigues for N==3, planar rot for N==2)
            T r00, r01, r02;
            T r10, r11, r12;
            T r20, r21, r22;

            if constexpr (N == 2)
            {
                r00 =  c; r01 = -s; r02 = 0;
                r10 =  s; r11 =  c; r12 = 0;
                r20 =  0; r21 =  0; r22 = 1; // identity for unused Z
            }
            else
            {
                r00 = x * x * ic + c;
                r01 = x * y * ic - z * s;
                r02 = x * z * ic + y * s;

                r10 = y * x * ic + z * s;
                r11 = y * y * ic + c;
                r12 = y * z * ic - x * s;

                r20 = z * x * ic - y * s;
                r21 = z * y * ic + x * s;
                r22 = z * z * ic + c;
            }

            // In-place rotation: apply to each column in the matrix
            for (size_t cidx = 0; cidx < C; ++cidx)
            {
                vec<T, R>& col = (vec<T, R>&)(*this)[cidx];

                T vx = (D > 0) ? col[0] : T(0);
                T vy = (D > 1) ? col[1] : T(0);
                T vz = (D > 2) ? col[2] : T(0);

                if constexpr (N == 2)
                {
                    col[0] = vx * r00 + vy * r01;
                    col[1] = vx * r10 + vy * r11;
                }
                else
                {
                    col[0] = vx * r00 + vy * r01 + vz * r02;
                    col[1] = vx * r10 + vy * r11 + vz * r12;
                    col[2] = vx * r20 + vy * r21 + vz * r22;
                }
                // Leave remaining rows [3..R) untouched
            }

            return *this;
        }
        template <size_t N>
        mat& rotateAround(T angle, const vec<T, N>& axis, const vec<T, N>& point)
        {
            static_assert(N == 2 || N == 3, "Rotation axis must be 2D or 3D");

            // Step 1: Translate to origin (move pivot to origin)
            translate<N>(-point);

            // Step 2: Rotate around origin
            rotate<N>(angle, axis);

            // Step 3: Translate back to original pivot position
            translate(point);

            return *this;
        }
        mat<T, C, R> inverse() const
        {
            static_assert(C == R, "Inverse only defined for square matrices");
            constexpr size_t N = C;
            mat<T, N, N> result;
            mat<T, N, N> copy = *this;
            result = mat<T, N, N>((T)1);

            for (size_t i = 0; i < N; ++i)
            {
                T pivot = copy.matrix[i][i];
                if (std::abs(pivot) < std::numeric_limits<T>::epsilon())
                {
                    size_t swap_row = i + 1;
                    while (swap_row < N && std::abs(copy.matrix[swap_row][i]) < std::numeric_limits<T>::epsilon())
                        ++swap_row;
                    if (swap_row == N)
                        assert(false && "Matrix is singular and cannot be inverted");
                    for (size_t k = 0; k < N; ++k)
                    {
                        std::swap(copy.matrix[i][k], copy.matrix[swap_row][k]);
                        std::swap(result.matrix[i][k], result.matrix[swap_row][k]);
                    }
                    pivot = copy.matrix[i][i];
                }

                T inv_pivot = (T)1 / pivot;
                for (size_t j = 0; j < N; ++j)
                {
                    copy.matrix[i][j] *= inv_pivot;
                    result.matrix[i][j] *= inv_pivot;
                }

                for (size_t row = 0; row < N; ++row)
                {
                    if (row == i)
                        continue;
                    T factor = copy.matrix[row][i];
                    for (size_t col = 0; col < N; ++col)
                    {
                        copy.matrix[row][col] -= factor * copy.matrix[i][col];
                        result.matrix[row][col] -= factor * result.matrix[i][col];
                    }
                }
            }

            return result;
        }
        template <size_t C2, size_t R2>
        mat<T, C, R>& operator*=(const mat<T, C2, R2>& rhs)
        {
            static_assert(C == R2, "Matrix multiplication dimension mismatch");

            // Allocate fixed-size temporary buffer on stack
            // result size is C2 columns, R rows
            T temp[C2][R];

            for (size_t c = 0; c < C2; c++)
            {
                for (size_t r = 0; r < R; r++)
                {
                    T sum = T(0);

                    for (size_t k = 0; k < C; k++)
                    {
                        sum += matrix[k][r] * rhs.matrix[c][k];
                    }

                    temp[c][r] = sum;
                }
            }

            // Now copy the temp buffer back into matrix, assuming dimensions match
            static_assert(C2 == C && R == R2, "operator*= requires output size equals current matrix size");

            memcpy(matrix, temp, sizeof(temp));

            return *this;
        }
        template <size_t C2, size_t R2>
        mat<T, C2, R> operator*(const mat<T, C2, R2>& rhs) const
        {
            mat<T, C, R> temp(*this);
            return temp *= rhs;
        }
    };

    template<typename T>
    mat<T, 4, 4> lookAt(const vec<T, 3>& eye, const vec<T, 3>& center, const vec<T, 3>& up)
    {
        vec<T, 3> f = (center - eye);
        T f_len = std::sqrt(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
        f /= f_len;

        vec<T, 3> up_n = up;
        T up_len = std::sqrt(up[0]*up[0] + up[1]*up[1] + up[2]*up[2]);
        up_n /= up_len;

        vec<T, 3> s = {
            f[1] * up_n[2] - f[2] * up_n[1],
            f[2] * up_n[0] - f[0] * up_n[2],
            f[0] * up_n[1] - f[1] * up_n[0]
        };
        T s_len = std::sqrt(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);
        s /= s_len;

        vec<T, 3> u = {
            s[1] * f[2] - s[2] * f[1],
            s[2] * f[0] - s[0] * f[2],
            s[0] * f[1] - s[1] * f[0]
        };

        mat<T, 4, 4> result((T)1);
        result[0][0] = s[0]; result[1][0] = s[1]; result[2][0] = s[2];
        result[0][1] = u[0]; result[1][1] = u[1]; result[2][1] = u[2];
        result[0][2] = -f[0]; result[1][2] = -f[1]; result[2][2] = -f[2];
        result[3][0] = - (s[0]*eye[0] + s[1]*eye[1] + s[2]*eye[2]);
        result[3][1] = - (u[0]*eye[0] + u[1]*eye[1] + u[2]*eye[2]);
        result[3][2] = + (f[0]*eye[0] + f[1]*eye[1] + f[2]*eye[2]);
        return result;
    }

	template<typename T>
	mat<T, 4, 4> perspectiveRH_ZO(T fovy, T aspect, T zNear, T zFar)
	{
		assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

		T const tanHalfFovy = tan(fovy / static_cast<T>(2));

		mat<T, 4, 4> result(static_cast<T>(0));
		result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
		result[1][1] = static_cast<T>(1) / (tanHalfFovy);
		result[2][2] = zFar / (zNear - zFar);
		result[2][3] = - static_cast<T>(1);
		result[3][2] = -(zFar * zNear) / (zFar - zNear);
		return result;
	}

	template<typename T>
	mat<T, 4, 4> perspectiveRH_NO(T fovy, T aspect, T zNear, T zFar)
	{
		assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

		T const tanHalfFovy = tan(fovy / static_cast<T>(2));

		mat<T, 4, 4> result(static_cast<T>(0));
		result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
		result[1][1] = static_cast<T>(1) / (tanHalfFovy);
		result[2][2] = - (zFar + zNear) / (zFar - zNear);
		result[2][3] = - static_cast<T>(1);
		result[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
		return result;
	}

	template<typename T>
	mat<T, 4, 4> perspectiveLH_ZO(T fovy, T aspect, T zNear, T zFar)
	{
		assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

		T const tanHalfFovy = tan(fovy / static_cast<T>(2));

		mat<T, 4, 4> result(static_cast<T>(0));
		result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
		result[1][1] = static_cast<T>(1) / (tanHalfFovy);
		result[2][2] = zFar / (zFar - zNear);
		result[2][3] = static_cast<T>(1);
		result[3][2] = -(zFar * zNear) / (zFar - zNear);
		return result;
	}

	template<typename T>
	mat<T, 4, 4> perspectiveLH_NO(T fovy, T aspect, T zNear, T zFar)
	{
		assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

		T const tanHalfFovy = tan(fovy / static_cast<T>(2));

		mat<T, 4, 4> result(static_cast<T>(0));
		result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
		result[1][1] = static_cast<T>(1) / (tanHalfFovy);
		result[2][2] = (zFar + zNear) / (zFar - zNear);
		result[2][3] = static_cast<T>(1);
		result[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
		return result;
	}

	template<typename T>
	mat<T, 4, 4> perspective(T fovy, T aspect, T zNear, T zFar)
	{
// #if GLM_CONFIG_CLIP_CONTROL == GLM_CLIP_CONTROL_LH_ZO
// 			return perspectiveLH_ZO(fovy, aspect, zNear, zFar);
// #elif GLM_CONFIG_CLIP_CONTROL == GLM_CLIP_CONTROL_LH_NO
// 			return perspectiveLH_NO(fovy, aspect, zNear, zFar);
#if defined(RENDERER_VULKAN)
        return perspectiveRH_ZO(fovy, aspect, zNear, zFar);
#elif defined(RENDERER_GL)
        return perspectiveRH_NO(fovy, aspect, zNear, zFar);
#endif
	}

	template<typename T>
	mat<T, 4, 4> infinitePerspectiveRH_NO(T fovy, T aspect, T zNear)
	{
		T const range = tan(fovy / static_cast<T>(2)) * zNear;
		T const left = -range * aspect;
		T const right = range * aspect;
		T const bottom = -range;
		T const top = range;

		mat<T, 4, 4> result(static_cast<T>(0));
		result[0][0] = (static_cast<T>(2) * zNear) / (right - left);
		result[1][1] = (static_cast<T>(2) * zNear) / (top - bottom);
		result[2][2] = - static_cast<T>(1);
		result[2][3] = - static_cast<T>(1);
		result[3][2] = - static_cast<T>(2) * zNear;
		return result;
	}
	
	template<typename T>
	mat<T, 4, 4> infinitePerspectiveRH_ZO(T fovy, T aspect, T zNear)
	{
		T const range = tan(fovy / static_cast<T>(2)) * zNear;
		T const left = -range * aspect;
		T const right = range * aspect;
		T const bottom = -range;
		T const top = range;

		mat<T, 4, 4> result(static_cast<T>(0));
		result[0][0] = (static_cast<T>(2) * zNear) / (right - left);
		result[1][1] = (static_cast<T>(2) * zNear) / (top - bottom);
		result[2][2] = - static_cast<T>(1);
		result[2][3] = - static_cast<T>(1);
		result[3][2] = - zNear;
		return result;
	}

	template<typename T>
	mat<T, 4, 4> infinitePerspectiveLH_NO(T fovy, T aspect, T zNear)
	{
		T const range = tan(fovy / static_cast<T>(2)) * zNear;
		T const left = -range * aspect;
		T const right = range * aspect;
		T const bottom = -range;
		T const top = range;

		mat<T, 4, 4> result(T(0));
		result[0][0] = (static_cast<T>(2) * zNear) / (right - left);
		result[1][1] = (static_cast<T>(2) * zNear) / (top - bottom);
		result[2][2] = static_cast<T>(1);
		result[2][3] = static_cast<T>(1);
		result[3][2] = - static_cast<T>(2) * zNear;
		return result;
	}

	template<typename T>
	mat<T, 4, 4> infinitePerspectiveLH_ZO(T fovy, T aspect, T zNear)
	{
		T const range = tan(fovy / static_cast<T>(2)) * zNear;
		T const left = -range * aspect;
		T const right = range * aspect;
		T const bottom = -range;
		T const top = range;

		mat<T, 4, 4> result(T(0));
		result[0][0] = (static_cast<T>(2) * zNear) / (right - left);
		result[1][1] = (static_cast<T>(2) * zNear) / (top - bottom);
		result[2][2] = static_cast<T>(1);
		result[2][3] = static_cast<T>(1);
		result[3][2] = - zNear;
		return result;
	}
    
	template<typename T>
	mat<T, 4, 4> infinitePerspective(T fovy, T aspect, T zNear)
	{
// #if GLM_CONFIG_CLIP_CONTROL == GLM_CLIP_CONTROL_LH_ZO
// 			return infinitePerspectiveLH_ZO(fovy, aspect, zNear);
// #elif GLM_CONFIG_CLIP_CONTROL == GLM_CLIP_CONTROL_LH_NO
// 			return infinitePerspectiveLH_NO(fovy, aspect, zNear);
#if defined(RENDERER_VULKAN)
			return infinitePerspectiveRH_ZO(fovy, aspect, zNear);
#elif defined(RENDERER_GL)
			return infinitePerspectiveRH_NO(fovy, aspect, zNear);
#endif
	}

    template<typename T>
    mat<T, 4, 4> orthographic(T left, T right, T bottom, T top, T znear, T zfar)
    {
        mat<T, 4, 4> result((T)1);
        result[0][0] = (T)2 / (right - left);
        result[1][1] = (T)2 / (top - bottom);
        result[2][2] = -(T)2 / (zfar - znear);
        result[3][0] = -(right + left) / (right - left);
        result[3][1] = -(top + bottom) / (top - bottom);
        result[3][2] = -(zfar + znear) / (zfar - znear);
        return result;
    }

	template<typename T>
	T radians(T degrees)
	{
		return degrees * T(0.01745329251994329576923690768489);
	}

	template<typename T>
	T degrees(T radians)
	{
		return radians * T(57.295779513082320876798154814105);
	}

	struct Random
	{
	private:
		inline static std::random_device _randomDevice = {};
		inline static std::mt19937 _mt19937 = std::mt19937(_randomDevice());

        inline static void reseed_mt19937_with_current_time()
        {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = now.time_since_epoch();
            auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
            std::uint32_t new_seed = static_cast<std::uint32_t>(nanos & 0xFFFFFFFF);
            _mt19937.seed(new_seed);
        }

	public:
		template <typename T>
		static const T value(const T min, const T max, const size_t seed = (std::numeric_limits<size_t>::max)())
		{
			std::mt19937* mt19937Pointer = 0;
			if (seed != (std::numeric_limits<size_t>::max)())
			{
				mt19937Pointer = new std::mt19937(seed);
			}
			else
			{
                reseed_mt19937_with_current_time();
				mt19937Pointer = &Random::_mt19937;
			}
			if constexpr (std::is_floating_point<T>::value)
			{
				std::uniform_real_distribution<T> distrib(min, max);
				auto value = distrib(*mt19937Pointer);
				if (seed != (std::numeric_limits<size_t>::max)())
				{
					delete mt19937Pointer;
					mt19937Pointer = 0;
				}
				return value;
			}
			else if constexpr (std::is_integral<T>::value)
			{
				std::uniform_int_distribution<T> distrib(min, max);
				auto value = distrib(*mt19937Pointer);
				if (seed != (std::numeric_limits<size_t>::max)()) // ∞
				{
					delete mt19937Pointer;
					mt19937Pointer = 0;
				}
				return value;
			}
			throw std::runtime_error("Type is not supported by Random::value");
		};
		template <typename T>
		static const T value(const T min, const T max, std::mt19937& mt19937)
		{
			if constexpr (std::is_floating_point<T>::value)
			{
				std::uniform_real_distribution<T> distrib(min, max);
				auto value = distrib(mt19937);
				return value;
			}
			else if constexpr (std::is_integral<T>::value)
			{
				std::uniform_int_distribution<T> distrib(min, max);
				auto value = distrib(mt19937);
				return value;
			}
			throw std::runtime_error("Type is not supported by Random::value");
		};
		template <typename T>
		static const T valueFromRandomRange(const std::vector<std::pair<T, T>>& ranges,
																				const size_t seed = (std::numeric_limits<size_t>::max)())
		{
			auto rangesSize = ranges.size();
			auto rangesData = ranges.data();
			size_t rangeIndex = Random::value<size_t>(0, rangesSize - 1, seed);
			auto& range = rangesData[rangeIndex];
			return Random::value(range.first, range.second, seed);
		};
		template <typename T>
		static const T valueFromRandomRange(const std::vector<std::pair<T, T>>& ranges, std::mt19937& mt19937)
		{
			auto rangesSize = ranges.size();
			auto rangesData = ranges.data();
			size_t rangeIndex = Random::value<size_t>(0, rangesSize - 1, mt19937);
			auto& range = rangesData[rangeIndex];
			return Random::value(range.first, range.second, mt19937);
		};
	};

    template <typename T, size_t N>
    struct AABB
    {
        vec<T, N> min;
        vec<T, N> max;
        AABB() = default;
        AABB(vec<T, N> min, vec<T, N> max):
            min(min),
            max(max)
        {};
    };
}