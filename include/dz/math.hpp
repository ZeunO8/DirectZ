#pragma once
#include <cassert>
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
                load_all(T(0));
            else if constexpr (sizeof...(args) == 1)
                load_all(args...);
            else
            {
                size_t i = 0;
                load_args(i, args...);
            }
        }
        vec& operator=(const vec& other)
        {
            memcpy(data, other.data, sizeof(T) * N);
            return *this;
        }
        T& operator[](size_t i)
        {
            return data[i];
        }
        const T& operator[](size_t i) const
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
            }
            return *this;
        }
        template <typename O>
        vec operator*(const O& other)
        {
            auto new_vec = *this;
            return (new_vec *= other);
        }
        template <typename O>
        vec operator/(const O& other)
        {
            auto new_vec = *this;
            return (new_vec /= other);
        }
        template <typename O>
        vec operator+(const O& other)
        {
            auto new_vec = *this;
            return (new_vec += other);
        }
        template <typename O>
        vec operator-(const O& other)
        {
            auto new_vec = *this;
            return (new_vec -= other);
        }
        vec operator-()
        {
            auto new_vec = *this;
            for (size_t i = 0; i < N; ++i)
                new_vec.data[i] *= T(-1);
            return new_vec;
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
        vec<T, R>& operator[](size_t i)
        {
            return *(vec<T, R>*)matrix[i];
        }
        const vec<T, R>& operator[](size_t i)const
        {
            return *(vec<T, R>*)matrix[i];
        }
        template <size_t N>
        mat& translate(const vec<T, N>& v)
        {
            static_assert(C == 4 && R == 4 && N == 3); // Todo remove this resriction and come up with an algorithm for all matrix sizes
            matrix[0][3] += v.data[0];
            matrix[1][3] += v.data[1];
            matrix[2][3] += v.data[2];
            return *this;
        }
        template <size_t N>
        mat& scale(const vec<T, N>& v)
        {
            static_assert(C == 4 && R == 4 && N == 3); // Todo remove this resriction and come up with an algorithm for all matrix sizes
            // x
            matrix[0][0] *= v.data[0];
            matrix[1][0] *= v.data[0];
            matrix[2][0] *= v.data[0];
            // y
            matrix[0][1] *= v.data[1];
            matrix[1][1] *= v.data[1];
            matrix[2][1] *= v.data[1];
            // z
            matrix[0][2] *= v.data[2];
            matrix[1][2] *= v.data[2];
            matrix[2][2] *= v.data[2];
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
    };
}