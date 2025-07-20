/**
 * @file math.hpp
 * @brief Defines various useful N dimensional structures such as matrices and vectors as well as helper functions
 */
#pragma once
#include <cassert>
#include <cmath>
#include <random>
#include <chrono>
#include <stdexcept>
#include <string.h>
#include "Renderer.hpp"
namespace dz
{
    /**
    * @brief Template struct representing an N-dimensional vector of type T.
    * 
    * This class supports construction from multiple arguments or a single scalar to fill all components,
    * copy construction from vectors of different types and sizes, 
    * element access, and arithmetic operations (component-wise or scalar).
    * It also supports vector length computation and normalization.
    * 
    * @tparam T The scalar type of the vector components.
    * @tparam N The dimensionality of the vector.
    */
    template <typename T, size_t N>
    struct vec
    {
    private:

        /**
        * @brief Base case for recursively loading constructor arguments into data array.
        * 
        * This function ends the recursion when no more arguments are left to load.
        * 
        * @param i Reference to the current index in the data array.
        */
        void load_args(size_t& i)
        { }

        /**
        * @brief Recursive variadic template helper to load constructor arguments into data array.
        * 
        * Loads the first argument into data[i] and recursively calls itself for the remaining arguments.
        * 
        * @tparam Args Variadic template parameter pack representing argument types.
        * @param i Reference to the current index in the data array.
        * @param arg The current argument to load into the vector.
        * @param args Remaining arguments.
        */
        template <typename... Args>
        void load_args(size_t& i, T arg, Args... args)
        {
            data[i++] = arg;
            load_args(i, args...);
        }

        /**
        * @brief Helper to fill the entire vector with the given scalar value.
        * 
        * Used to initialize all components to the same value.
        * 
        * @param val The scalar value to assign to each component.
        */
        void load_all(T val)
        {
            for (size_t i = 0; i < N; ++i)
                data[i] = val;
        }

    public:

        /**
        * @brief Underlying array storing vector components.
        */
        T data[N];

        /**
        * @brief Variadic constructor to initialize vector components.
        * 
        * Behaves differently depending on the number of arguments passed:
        * - If no arguments are provided, initializes all components to T() (default constructed).
        * - If one argument is provided, initializes all components to that value.
        * - If multiple arguments are provided, initializes each component to the corresponding argument.
        * 
        * @tparam Args Variadic template arguments representing component values.
        * @param args Values used to initialize vector components.
        */
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

        /**
        * @brief Copy constructor to convert from vectors of different types and/or sizes.
        * 
        * Copies component-wise up to the minimum of source and destination size,
        * zero-initializes remaining components if destination vector is larger.
        * 
        * @tparam OT The source vector component type.
        * @tparam ON The source vector dimension.
        * @param other The vector to copy from.
        */
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

        /**
        * @brief Assignment operator for component-wise copy.
        * 
        * Copies all components from the other vector using memcpy for efficiency.
        * 
        * @param other The vector to copy from.
        * @return Reference to this vector after assignment.
        */
        vec& operator=(const vec& other)
        {
            memcpy(data, other.data, sizeof(T) * N);
            return *this;
        }

        /**
        * @brief Index operator for non-const access to vector components.
        * 
        * No bounds checking is performed.
        * 
        * @param i The component index to access.
        * @return Reference to the component at index i.
        */
        inline T& operator[](size_t i)
        {
            return data[i];
        }

        /**
        * @brief Index operator for const access to vector components.
        * 
        * No bounds checking is performed.
        * 
        * @param i The component index to access.
        * @return Const reference to the component at index i.
        */
        inline const T& operator[](size_t i) const
        {
            return data[i];
        }

        /**
        * @brief Compound multiplication-assignment operator.
        * 
        * Supports multiplication by a scalar or component-wise multiplication by another vector.
        * Throws std::runtime_error if the operand type is unsupported.
        * 
        * @tparam O Operand type (scalar T or vec<T, N>).
        * @param other The scalar or vector to multiply by.
        * @return Reference to this vector after multiplication.
        * @throws std::runtime_error if unsupported operand type is used.
        */
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

        /**
        * @brief Compound division-assignment operator.
        * 
        * Supports division by a scalar or component-wise division by another vector.
        * Throws std::runtime_error if the operand type is unsupported.
        * 
        * @tparam O Operand type (scalar T or vec<T, N>).
        * @param other The scalar or vector to divide by.
        * @return Reference to this vector after division.
        * @throws std::runtime_error if unsupported operand type is used.
        */
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

        /**
        * @brief Compound addition-assignment operator.
        * 
        * Supports addition by a scalar or component-wise addition by another vector.
        * Throws std::runtime_error if the operand type is unsupported.
        * 
        * @tparam O Operand type (scalar T or vec<T, N>).
        * @param other The scalar or vector to add.
        * @return Reference to this vector after addition.
        * @throws std::runtime_error if unsupported operand type is used.
        */
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

        /**
        * @brief Compound subtraction-assignment operator.
        * 
        * Supports subtraction by a scalar or component-wise subtraction by another vector.
        * Throws std::runtime_error if the operand type is unsupported.
        * 
        * @tparam O Operand type (scalar T or vec<T, N>).
        * @param other The scalar or vector to subtract.
        * @return Reference to this vector after subtraction.
        * @throws std::runtime_error if unsupported operand type is used.
        */
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

        /**
        * @brief Multiplication operator returning a new vector.
        * 
        * Supports multiplication by a scalar or component-wise multiplication by another vector.
        * 
        * @tparam O Operand type (scalar T or vec<T, N>).
        * @param other The scalar or vector to multiply by.
        * @return A new vector resulting from the multiplication.
        */
        template <typename O>
        vec operator*(const O& other) const
        {
            auto new_vec = *this;
            return (new_vec *= other);
        }

        /**
        * @brief Division operator returning a new vector.
        * 
        * Supports division by a scalar or component-wise division by another vector.
        * 
        * @tparam O Operand type (scalar T or vec<T, N>).
        * @param other The scalar or vector to divide by.
        * @return A new vector resulting from the division.
        */
        template <typename O>
        vec operator/(const O& other) const
        {
            auto new_vec = *this;
            return (new_vec /= other);
        }

        /**
        * @brief Addition operator returning a new vector.
        * 
        * Supports addition by a scalar or component-wise addition by another vector.
        * 
        * @tparam O Operand type (scalar T or vec<T, N>).
        * @param other The scalar or vector to add.
        * @return A new vector resulting from the addition.
        */
        template <typename O>
        vec operator+(const O& other) const
        {
            auto new_vec = *this;
            return (new_vec += other);
        }

        /**
        * @brief Subtraction operator returning a new vector.
        * 
        * Supports subtraction by a scalar or component-wise subtraction by another vector.
        * 
        * @tparam O Operand type (scalar T or vec<T, N>).
        * @param other The scalar or vector to subtract.
        * @return A new vector resulting from the subtraction.
        */
        template <typename O>
        vec operator-(const O& other) const
        {
            auto new_vec = *this;
            return (new_vec -= other);
        }

        /**
        * @brief Unary negation operator.
        * 
        * Returns a new vector with all components negated.
        * 
        * @return A new vector with negated components.
        */
        vec operator-() const
        {
            auto new_vec = *this;
            for (size_t i = 0; i < N; ++i)
                new_vec.data[i] *= T(-1);
            return new_vec;
        }

        /**
        * @brief Computes the Euclidean length (magnitude) of the vector.
        * 
        * Calculates the square root of the sum of squares of all components.
        * 
        * @return The length of the vector as type T.
        */
        T length() const
        {
            T sum = T(0);
            for (size_t i = 0; i < N; ++i)
            {
                sum += data[i] * data[i];
            }
            return std::sqrt(sum);
        }

        /**
        * @brief Returns a normalized (unit length) copy of the vector.
        * 
        * Divides all components by the vector length.
        * 
        * @return A new vector normalized to length 1.
        */
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

    template <typename T, size_t N>
    struct color_vec : public vec<T, N>
    {
        using Base = vec<T, N>;

        using Base::data;
        using Base::operator=;
        using Base::operator+=;
        using Base::operator-=;
        using Base::operator*=;
        using Base::operator/=;
        using Base::operator+;
        using Base::operator-;
        using Base::operator*;
        using Base::operator/;
        using Base::operator[];
        using Base::length;
        using Base::normalize;

        template <typename... Args>
        color_vec(Args&&... args) : Base(std::forward<Args>(args)...) { }

        template <typename OT, size_t ON>
        color_vec(const vec<OT, ON>& other) : Base(other) { }

        color_vec() = default;
        color_vec(const color_vec&) = default;
        color_vec& operator=(const color_vec&) = default;
        color_vec(color_vec&&) noexcept = default;
        color_vec& operator=(color_vec&&) noexcept = default;
    };

    /**
    * @brief A generic fixed-size matrix template supporting common matrix operations.
    * 
    * This template represents a matrix with dimensions C columns by R rows, where
    * each element is of type T. It supports initialization, assignment, indexing,
    * transformation (translation, scaling, rotation), inversion, and multiplication.
    * 
    * The internal storage is column-major, meaning matrix[c][r] accesses the element
    * in the c-th column and r-th row.
    * 
    * @tparam T The scalar numeric type used in the matrix (e.g., float, double).
    * @tparam C The number of columns in the matrix.
    * @tparam R The number of rows in the matrix.
    */
    template <typename T, size_t C, size_t R>
    struct mat
    {
        /**
        * @brief Raw matrix storage, column-major order.
        * 
        * Access elements as matrix[column][row].
        */
        T matrix[C][R];

        /**
        * @brief Constructs a matrix initialized to zero with optional diagonal initialization.
        * 
        * The matrix elements are zeroed. If an initial value is provided, it
        * is assigned to the diagonal elements (for square or rectangular matrices
        * where diagonal exists).
        * 
        * @param initial The value to initialize the diagonal elements with. Default is zero.
        */
        mat(T initial = T())
        {
            auto sizebytes = sizeof(T) * C * R;
            memset(matrix, 0, sizebytes);

            // Initialize diagonal elements up to min(C, R) with initial value
            for (size_t c = 0; c < C && c < R; c++)
                matrix[c][c] = initial;
        }

        /**
        * @brief Copy assignment operator.
        * 
        * Copies all elements from another matrix of the same type and dimensions.
        * 
        * @param other The source matrix to copy from.
        * @return Reference to this matrix after assignment.
        */
        mat& operator=(const mat& other)
        {
            memcpy(matrix, other.matrix, sizeof(T) * C * R);
            return *this;
        }

        /**
        * @brief Provides access to the column vector at index i (non-const).
        * 
        * This returns a reference to the i-th column as a vector of length R.
        * 
        * @param i The column index to access (0-based).
        * @return Reference to the column vector.
        */
        inline vec<T, R>& operator[](size_t i)
        {
            return *(vec<T, R>*)matrix[i];
        }

        /**
        * @brief Provides access to the column vector at index i (const).
        * 
        * @param i The column index to access (0-based).
        * @return Const reference to the column vector.
        */
        inline const vec<T, R>& operator[](size_t i) const
        {
            return *(const vec<T, R>*)matrix[i];
        }

        /**
        * @brief Applies a translation to a 4x4 matrix by adding a vector to its position component.
        * 
        * This function assumes a 4x4 matrix representing an affine transform and
        * translates it by the vector `v` applied to the fourth column (position).
        * 
        * @tparam N The dimension of the translation vector (must be 3).
        * @param v The translation vector to add.
        * @return Reference to the translated matrix (this).
        */
        template <size_t N>
        mat& translate(const vec<T, N>& v)
        {
            static_assert(C == 4 && R == 4 && N == 3, "translate requires a 4x4 matrix and 3D vector");
            auto& pos = (vec<T, N>&)(*this)[3];
            pos += v;
            return *this;
        }

        template <size_t N>
        inline static mat translate_static(const vec<T, N>& v)
        {
            mat m(1.0f);
            static_assert(C == 4 && R == 4 && N == 3, "translate requires a 4x4 matrix and 3D vector");
            auto& pos = (vec<T, N>&)(m)[3];
            pos += v;
            return m;
        }

        /**
        * @brief Applies a component-wise scale to the matrix columns.
        * 
        * Multiplies each of the first N columns of the matrix by the corresponding
        * scalar value from the vector `v`.
        * 
        * @tparam N The number of scale components (usually matching matrix dimension).
        * @param v Vector containing scaling factors.
        * @return Reference to the scaled matrix (this).
        */
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
        inline static mat scale_static(const vec<T, N>& v)
        {
            mat m(1.0f);
            for (size_t l = 0; l < N; ++l)
            {
                auto& y = (vec<T, N>&)(m)[l];
                y *= v[l];
            }
            return m;
        }

        /**
        * @brief Rotates the matrix in-place around the specified axis by the given angle.
        * 
        * Supports 2D rotation around a 2D axis (angle in radians) or 3D rotation
        * around an arbitrary 3D axis using Rodrigues' rotation formula.
        * 
        * The rotation is applied directly to each column of the matrix.
        * 
        * @tparam N Dimension of the rotation axis (2 or 3).
        * @param angle The angle to rotate by, in radians.
        * @param axis The axis of rotation (2D or 3D vector).
        * @return Reference to the rotated matrix (this).
        * 
        * @note For 2D rotations, the third component is assumed zero.
        */
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

            // Precompute rotation matrix components
            T r00, r01, r02;
            T r10, r11, r12;
            T r20, r21, r22;

            if constexpr (N == 2)
            {
                r00 =  c; r01 = -s; r02 = 0;
                r10 =  s; r11 =  c; r12 = 0;
                r20 =  0; r21 =  0; r22 = 1;
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

            // Apply rotation matrix to each column vector
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
                // Remaining rows (3 and above) remain unchanged
            }

            return *this;
        }

        template <size_t N>
        inline static mat rotate_static(T angle, const vec<T, N>& axis)
        {
            static_assert(N == 2 || N == 3, "Rotation axis must be 2D or 3D");
            constexpr size_t D = N;

            mat m(1.0f);

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

            // Precompute rotation matrix components
            T r00, r01, r02;
            T r10, r11, r12;
            T r20, r21, r22;

            if constexpr (N == 2)
            {
                r00 =  c; r01 = -s; r02 = 0;
                r10 =  s; r11 =  c; r12 = 0;
                r20 =  0; r21 =  0; r22 = 1;
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

            // Apply rotation matrix to each column vector
            for (size_t cidx = 0; cidx < C; ++cidx)
            {
                vec<T, R>& col = (vec<T, R>&)(m)[cidx];

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
                // Remaining rows (3 and above) remain unchanged
            }

            return m;
        }

        /**
        * @brief Rotates the matrix around a specified pivot point.
        * 
        * Performs an in-place rotation around the `point` by:
        * - Translating the matrix so that the pivot moves to the origin,
        * - Rotating by the given angle around the axis,
        * - Translating back to the original pivot location.
        * 
        * @tparam N Dimension of the rotation axis (2 or 3).
        * @param angle The angle of rotation in radians.
        * @param axis The rotation axis vector.
        * @param point The pivot point around which to rotate.
        * @return Reference to the rotated matrix (this).
        */
        template <size_t N>
        mat& rotateAround(T angle, const vec<T, N>& axis, const vec<T, N>& point)
        {
            static_assert(N == 2 || N == 3, "Rotation axis must be 2D or 3D");

            // Translate pivot to origin
            translate<N>(-point);

            // Rotate around origin
            rotate<N>(angle, axis);

            // Translate pivot back to original location
            translate(point);

            return *this;
        }

        /**
        * @brief Computes the inverse of a square matrix.
        * 
        * Uses Gaussian elimination with partial pivoting to invert the matrix.
        * 
        * @return The inverse matrix.
        * 
        * @throws Assertion failure if the matrix is singular (non-invertible).
        */
        mat<T, C, R> inverse() const
        {
            static_assert(C == R, "Inverse only defined for square matrices");
            constexpr size_t N = C;
            mat<T, N, N> result;
            mat<T, N, N> copy = *this;
            result = mat<T, N, N>((T)1);  // Initialize result to identity

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

        /**
        * @brief Performs in-place multiplication by another matrix.
        * 
        * The matrix is multiplied by rhs, and the result replaces the current matrix.
        * 
        * @tparam C2 Number of columns in the right-hand matrix.
        * @tparam R2 Number of rows in the right-hand matrix.
        * @param rhs The matrix to multiply by.
        * @return Reference to the resulting matrix (this).
        * 
        * @note Requires the dimensions to satisfy C == R2, and output size matches current matrix.
        */
        template <size_t C2, size_t R2>
        mat<T, C, R>& operator*=(const mat<T, C2, R2>& rhs)
        {
            static_assert(C == R2, "Matrix multiplication dimension mismatch");

            // Temporary storage for results
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

            // Enforce that output size matches current matrix dimensions
            static_assert(C2 == C && R == R2, "operator*= requires output size equals current matrix size");

            memcpy(matrix, temp, sizeof(temp));

            return *this;
        }

        /**
        * @brief Returns a new matrix which is the product of this matrix and rhs.
        * 
        * Does not modify the current matrix.
        * 
        * @tparam C2 Number of columns in the right-hand matrix.
        * @tparam R2 Number of rows in the right-hand matrix.
        * @param rhs The matrix to multiply by.
        * @return The resulting matrix product.
        */
        template <size_t C2, size_t R2>
        mat<T, C2, R> operator*(const mat<T, C2, R2>& rhs) const
        {
            mat<T, C, R> temp(*this);
            return temp *= rhs;
        }

        constexpr mat<T, R, C> transpose()
        {
            mat<T, R, C> result {};
            for (size_t i = 0; i < C; ++i)
            {
                for (size_t j = 0; j < R; ++j)
                {
                    result[j][i] = (*this)[i][j];
                }
            }
            return result;
        }
    };

    /**
    * @brief Constructs a right-handed view matrix simulating a camera "look at" transform.
    *
    * This function creates a 4x4 view matrix that positions and orients a virtual camera
    * in 3D space. The camera is placed at the position `eye`, looking towards the point
    * `center`, with the specified `up` vector defining the camera's vertical orientation.
    * 
    * The resulting matrix transforms world coordinates into view (camera) space.
    * 
    * The algorithm follows the standard "look at" computation:
    * - `f` is the forward vector pointing from eye to center (normalized).
    * - `up_n` is the normalized up vector.
    * - `s` (side) is the normalized cross product of `f` and `up_n`, representing the camera's right direction.
    * - `u` is the cross product of `s` and `f`, representing the true up vector perpendicular to both.
    * 
    * The matrix columns correspond to these orthonormal basis vectors, and the translation
    * part moves the world such that the camera is effectively at the origin looking down the negative Z-axis.
    *
    * @tparam T Numeric type (float, double).
    * @param eye Position of the camera in world space.
    * @param center Target point the camera is looking at.
    * @param up Up direction vector used to orient the camera. Should not be parallel to the line from eye to center.
    * @return mat<T, 4, 4> View matrix that transforms world space to camera (view) space.
    *
    * @note It is important that `up` and the vector from `eye` to `center` are not collinear,
    *       otherwise the cross products will fail to produce valid orthonormal vectors.
    * @note This implementation assumes a right-handed coordinate system where the camera
    *       looks towards the negative Z-axis in view space.
    */
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

    /**
    * @brief Creates a perspective projection matrix suitable for rendering 3D scenes.
    *
    * This function generates a 4x4 perspective projection matrix based on the given
    * vertical field of view, aspect ratio, and near/far clipping planes.
    * 
    * The matrix maps 3D points to normalized device coordinates (NDC) for rendering.
    * 
    * The specific projection variant depends on the renderer target:
    * - For Vulkan (`RENDERER_VULKAN`), a right-handed coordinate system with a zero to one (ZO) depth range is used.
    * - For OpenGL (`RENDERER_GL`), a right-handed coordinate system with a negative one to one (NO) depth range is used.
    *
    * @tparam T Numeric type (e.g. float, double).
    * @param fovy Field of view angle in radians (vertical).
    * @param aspect Aspect ratio (width / height).
    * @param zNear Distance to near clipping plane (must be > 0).
    * @param zFar Distance to far clipping plane (must be > zNear).
    * @return mat<T, 4, 4> The resulting perspective projection matrix.
    *
    * @note This function switches behavior depending on the rendering backend macros.
    *       The code assumes right-handed coordinate system for both Vulkan and OpenGL.
    *       The commented-out lines hint at alternative left-handed implementations.
    */
	template<typename T>
	mat<T, 4, 4> perspective(T fovy, T aspect, T zNear, T zFar)
	{
// #if GLM_CONFIG_CLIP_CONTROL == GLM_CLIP_CONTROL_LH_ZO
// 			return perspectiveLH_ZO(fovy, aspect, zNear, zFar);
// #elif GLM_CONFIG_CLIP_CONTROL == GLM_CLIP_CONTROL_LH_NO
// 			return perspectiveLH_NO(fovy, aspect, zNear, zFar);
#if defined(RENDERER_VULKAN)
        auto m = perspectiveRH_ZO(fovy, aspect, zNear, zFar);
#elif defined(RENDERER_GL)
        auto m = perspectiveRH_NO(fovy, aspect, zNear, zFar);
#endif
        // m[1][1] *= -1.0f;
        return m;
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
    
    /**
    * @brief Creates an infinite perspective projection matrix.
    *
    * Generates a projection matrix that has a far plane at infinity, which can be
    * useful for scenes where the far plane should not clip distant objects.
    * 
    * The infinite far plane reduces depth buffer precision issues for very large
    * view distances.
    * 
    * The choice of projection variant depends on renderer backend macros:
    * - Vulkan uses right-handed coordinate system with zero to one depth range.
    * - OpenGL uses right-handed coordinate system with negative one to one depth range.
    *
    * @tparam T Numeric type (e.g. float, double).
    * @param fovy Field of view angle in radians (vertical).
    * @param aspect Aspect ratio (width / height).
    * @param zNear Distance to near clipping plane (must be > 0).
    * @return mat<T, 4, 4> The resulting infinite perspective projection matrix.
    *
    * @note Like `perspective()`, this function conditionally compiles for different rendering APIs.
    */
	template<typename T>
	mat<T, 4, 4> infinitePerspective(T fovy, T aspect, T zNear)
	{
#if defined(RENDERER_VULKAN)
        auto m = infinitePerspectiveRH_ZO(fovy, aspect, zNear);
#elif defined(RENDERER_GL)
        auto m = infinitePerspectiveRH_NO(fovy, aspect, zNear);
#endif
        // m[1][1] *= -1.0f;
        return m;
	}

    /**
    * @brief Creates an orthographic projection matrix.
    *
    * Generates a 4x4 orthographic projection matrix that maps a defined box volume
    * to normalized device coordinates.
    * 
    * Orthographic projection preserves parallel lines and does not apply perspective distortion.
    *
    * @tparam T Numeric type (e.g. float, double).
    * @param left Left boundary of the view volume.
    * @param right Right boundary of the view volume.
    * @param bottom Bottom boundary of the view volume.
    * @param top Top boundary of the view volume.
    * @param znear Near clipping plane distance.
    * @param zfar Far clipping plane distance.
    * @return mat<T, 4, 4> The resulting orthographic projection matrix.
    *
    * @note This matrix assumes a right-handed coordinate system with the negative
    *       z-axis pointing into the screen.
    */
    template<typename T>
    mat<T, 4, 4> orthographic(T left, T right, T bottom, T top, T znear, T zfar)
    {
        mat<T, 4, 4> result((T)1);
        result[0][0] = (T)2 / (right - left);
        result[1][1] = (T)2 / (top - bottom);
        result[2][2] = -(T)1 / (zfar - znear);
        result[3][0] = -(right + left) / (right - left);
        result[3][1] = -(top + bottom) / (top - bottom);
        result[3][2] = -(znear) / (zfar - znear);
        // result[1][1] *= -1.0f;
        return result;
    }

    /**
    * @brief Converts an angle from degrees to radians.
    *
    * Performs the conversion using the exact constant π / 180.
    *
    * @tparam T Numeric type (float, double, etc.).
    * @param degrees Angle in degrees.
    * @return Angle in radians.
    */
	template<typename T>
	T radians(T degrees)
	{
		return degrees * T(0.01745329251994329576923690768489);
	}

    /**
    * @brief Converts an angle from radians to degrees.
    *
    * Performs the conversion using the exact constant 180 / π.
    *
    * @tparam T Numeric type (float, double, etc.).
    * @param radians Angle in radians.
    * @return Angle in degrees.
    */
	template<typename T>
	T degrees(T radians)
	{
		return radians * T(57.295779513082320876798154814105);
	}

    /**
    * @brief Utility class for generating random values with optional seeding and support for multiple numeric types.
    *
    * Provides static methods to generate uniformly distributed random numbers for integral and floating-point types.
    * Supports generating random values within a specified range, optionally with a user-provided seed or external random engine.
    * Also supports choosing random values from multiple ranges.
    */
    struct Random
    {
    private:

        /**
        * @brief Random device used to seed the default Mersenne Twister engine.
        */
        inline static std::random_device _randomDevice = {};

        /**
        * @brief Default Mersenne Twister random number generator initialized with _randomDevice.
        */
        inline static std::mt19937 _mt19937 = std::mt19937(_randomDevice());

        /**
        * @brief Helper function to reseed the default mt19937 engine using current high-resolution time.
        *
        * This improves randomness by using a time-based seed instead of fixed or device-based seed.
        */
        inline static void reseed_mt19937_with_current_time()
        {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = now.time_since_epoch();
            auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
            std::uint32_t new_seed = static_cast<std::uint32_t>(nanos & 0xFFFFFFFF);
            _mt19937.seed(new_seed);
        }

    public:

        /**
        * @brief Generate a uniformly distributed random value in [min, max].
        *
        * If a seed is provided (not the default max size_t), a new temporary mt19937 engine is created and seeded with the given seed.
        * Otherwise, the internal static mt19937 engine is reseeded with the current time and used.
        *
        * Supports floating point and integral types. Throws std::runtime_error if called with unsupported type.
        *
        * @tparam T Numeric type for the random value (must be integral or floating point).
        * @param min Minimum value of the random range (inclusive).
        * @param max Maximum value of the random range (inclusive for integral, [min,max) for floating point).
        * @param seed Optional seed to create a temporary mt19937 engine. Default is max size_t (no seed).
        * @return Randomly generated value of type T.
        * @throws std::runtime_error if T is not supported.
        */
        template <typename T>
        static const T value(const T min, const T max, const size_t seed = (std::numeric_limits<size_t>::max)())
        {
            std::mt19937* mt19937Pointer = nullptr;

            if (seed != (std::numeric_limits<size_t>::max)())
            {
                // Create a temporary engine with the provided seed
                mt19937Pointer = new std::mt19937(seed);
            }
            else
            {
                // Reseed static engine with current time and use it
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
                    mt19937Pointer = nullptr;
                }

                return value;
            }
            else if constexpr (std::is_integral<T>::value)
            {
                std::uniform_int_distribution<T> distrib(min, max);
                auto value = distrib(*mt19937Pointer);

                if (seed != (std::numeric_limits<size_t>::max)())
                {
                    delete mt19937Pointer;
                    mt19937Pointer = nullptr;
                }

                return value;
            }

            throw std::runtime_error("Type is not supported by Random::value");
        };

        /**
        * @brief Generate a uniformly distributed random value in [min, max] using a provided mt19937 engine.
        *
        * Supports floating point and integral types. Throws std::runtime_error if called with unsupported type.
        *
        * @tparam T Numeric type for the random value.
        * @param min Minimum value of the random range (inclusive).
        * @param max Maximum value of the random range (inclusive for integral, [min,max) for floating point).
        * @param mt19937 Reference to an external mt19937 random engine to use.
        * @return Randomly generated value of type T.
        * @throws std::runtime_error if T is not supported.
        */
        template <typename T>
        static const T value(const T min, const T max, std::mt19937& mt19937)
        {
            if constexpr (std::is_floating_point<T>::value)
            {
                std::uniform_real_distribution<T> distrib(min, max);
                return distrib(mt19937);
            }
            else if constexpr (std::is_integral<T>::value)
            {
                std::uniform_int_distribution<T> distrib(min, max);
                return distrib(mt19937);
            }
            throw std::runtime_error("Type is not supported by Random::value");
        };

        /**
        * @brief Generate a uniformly distributed random value from multiple specified ranges.
        *
        * Selects one range at random, then generates a random value within that range.
        * Optionally accepts a seed to create a temporary mt19937 engine, otherwise uses internal engine reseeded with current time.
        *
        * @tparam T Numeric type for the random value.
        * @param ranges Vector of pairs representing inclusive ranges [first, second].
        * @param seed Optional seed to create a temporary mt19937 engine. Default is max size_t (no seed).
        * @return Randomly generated value of type T from one of the specified ranges.
        */
        template <typename T>
        static const T valueFromRandomRange(const std::vector<std::pair<T, T>>& ranges,
                                        const size_t seed = (std::numeric_limits<size_t>::max)())
        {
            auto rangesSize = ranges.size();
            auto rangesData = ranges.data();

            // Select a random index to pick a range
            size_t rangeIndex = Random::value<size_t>(0, rangesSize - 1, seed);

            // Generate random value from the selected range
            auto& range = rangesData[rangeIndex];
            return Random::value(range.first, range.second, seed);
        };

        /**
        * @brief Generate a uniformly distributed random value from multiple specified ranges using external mt19937 engine.
        *
        * Selects one range at random, then generates a random value within that range.
        *
        * @tparam T Numeric type for the random value.
        * @param ranges Vector of pairs representing inclusive ranges [first, second].
        * @param mt19937 Reference to an external mt19937 random engine to use.
        * @return Randomly generated value of type T from one of the specified ranges.
        */
        template <typename T>
        static const T valueFromRandomRange(const std::vector<std::pair<T, T>>& ranges, std::mt19937& mt19937)
        {
            auto rangesSize = ranges.size();
            auto rangesData = ranges.data();

            // Select a random index to pick a range
            size_t rangeIndex = Random::value<size_t>(0, rangesSize - 1, mt19937);

            // Generate random value from the selected range
            auto& range = rangesData[rangeIndex];
            return Random::value(range.first, range.second, mt19937);
        };
    };

    /**
    * @brief Axis-Aligned Bounding Box (AABB) template structure.
    *
    * Represents an N-dimensional bounding box defined by minimum and maximum corner points.
    *
    * @tparam T The numeric type of the bounding box coordinates (e.g., float, double).
    * @tparam N The dimension of the bounding box (e.g., 2 for 2D, 3 for 3D).
    */
    template <typename T, size_t N>
    struct AABB
    {
        vec<T, N> min;  /**< Minimum corner point of the bounding box. */
        vec<T, N> max;  /**< Maximum corner point of the bounding box. */

        /**
        * @brief Default constructor.
        *
        * Leaves min and max uninitialized.
        */
        AABB() = default;

        /**
        * @brief Construct an AABB with specified minimum and maximum corners.
        *
        * @param min The minimum corner vector of the bounding box.
        * @param max The maximum corner vector of the bounding box.
        */
        AABB(vec<T, N> min, vec<T, N> max)
            : min(min)
            , max(max)
        {}
    };
}