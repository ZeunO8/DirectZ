#pragma once

#include "math.hpp"
#include <tuple>
#include <string>
#include <cstdint>
#include <chrono>
#include <array>
#include <optional>
#include <mutex>

namespace dz {
    enum class D7Type : std::uint8_t
    {
        None = 0,
        X = 1 << 0,
        Y = 1 << 1,
        Z = 1 << 2,
        T = 1 << 3,
        U = 1 << 4,
        u = 1 << 5,
        a = 1 << 6,
        Count = 7,
        All = 0x7F
    };

    // Enable bitwise ops
    inline constexpr D7Type operator|(D7Type lhs, D7Type rhs)
    {
        return static_cast<D7Type>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    }
    inline constexpr bool operator&(D7Type lhs, D7Type rhs)
    {
        return static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs);
    }

    using StreamScalar      = float;
    using StreamInteger     = std::int64_t;
    using StreamString      = std::string;
    using StreamTimestamp   = std::chrono::time_point<std::chrono::steady_clock>;
    using StreamIdentifier  = std::uint64_t;

    using StreamPoint = std::tuple<StreamScalar, StreamScalar, StreamScalar, StreamTimestamp, StreamIdentifier, StreamInteger, StreamString>;

    // Trait to determine expected type per axis
    template<D7Type V> struct ExpectedType;

    template<> struct ExpectedType<D7Type::X> { using type = StreamScalar; };
    template<> struct ExpectedType<D7Type::Y> { using type = StreamScalar; };
    template<> struct ExpectedType<D7Type::Z> { using type = StreamScalar; };
    template<> struct ExpectedType<D7Type::T> { using type = StreamTimestamp; };
    template<> struct ExpectedType<D7Type::U> { using type = StreamIdentifier; };
    template<> struct ExpectedType<D7Type::u> { using type = StreamInteger; };
    template<> struct ExpectedType<D7Type::a> { using type = StreamString; };

    // Converts enum to index
    inline constexpr std::size_t D7TypeToIndex(D7Type v)
    {
        return static_cast<std::size_t>(v);
    }

    template<D7Type Axis>
    inline typename ExpectedType<Axis>::type const& GetStreamPointValue(const StreamPoint& tup)
    {
        constexpr auto I = (size_t)Axis;
        return std::get<I>(tup);
    }

    template<D7Type Axis>
    inline typename ExpectedType<Axis>::type& GetStreamPointValue(StreamPoint& tup)
    {
        constexpr auto I = (size_t)Axis;
        return std::get<I>(tup);
    }

    struct WINDOW;
    struct UPeaceAI;

    template<D7Type type>
    constexpr size_t D7TypeToIndex();

    template<> constexpr size_t D7TypeToIndex<D7Type::X>() { return 0; }
    template<> constexpr size_t D7TypeToIndex<D7Type::Y>() { return 1; }
    template<> constexpr size_t D7TypeToIndex<D7Type::Z>() { return 2; }
    template<> constexpr size_t D7TypeToIndex<D7Type::T>() { return 3; }
    template<> constexpr size_t D7TypeToIndex<D7Type::U>() { return 4; }
    template<> constexpr size_t D7TypeToIndex<D7Type::u>() { return 5; }
    template<> constexpr size_t D7TypeToIndex<D7Type::a>() { return 6; }

    struct D7Stream {

        std::vector<size_t*> stream_indexes;
        std::vector<StreamPoint> stream_points;

        /**
        * @brief adds a stream point to the History
        */
        size_t* addStreamPoint(const StreamPoint& point);

        /**
        * @brief removes a streampoint from the history at a specified index pointer
        * Has a O(N-I) time complexity where N = stream_points.size & I = index
        * ( this is due to updating the stream pointers )
        */
        bool removeStreamPoint(size_t* index_ptr);

        /**
        * @brief calling this function with just the first argument defined results in rewinding history in the stream at a global level
        * You can filter by D7Type, a_buff, uid & Uid
        */
        bool rewindNPoints(
            size_t N = 1,
            D7Type filter = D7Type::Count,
            const StreamString& a_buff = "",
            StreamInteger uid = 0,
            StreamIdentifier Uid = 0
        );

        /**
        * @brief calling this function with just the first argument defined results in fastforwarding history in the stream at a global level
        * You can filter by D7Type, a_buff, uid & Uid
        */
        bool forwardNPoints(
            size_t N = 1,
            D7Type filter = D7Type::Count,
            const StreamString& a_buff = "",
            StreamInteger uid = 0,
            StreamIdentifier Uid = 0
        );

        void printStreamPoints(D7Type filter = D7Type::None, const StreamString& a_buff = "", StreamInteger uid = 0, StreamIdentifier Uid = 0);

        D7Stream& operator << (const StreamPoint& point);

    private:
        std::mutex stream_points_mutex;
        size_t* current_point_index = 0;
    };
}