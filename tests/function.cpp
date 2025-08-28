#include <dz/function.hpp>
#include <functional>
#include <iostream>
#include "munit.h"

#define test_fn_signature bool(int, double, size_t, float, long, std::tuple<float, int>)

static MunitResult test_dz_function_lambda(const MunitParameter params[], void* user_data)
{
    using FN_Def = dz::function<test_fn_signature>;
    FN_Def my_int_only_triple([=](auto x, auto y, auto z, auto w, auto t, auto l) -> bool {
        y *= 3;
        // std::cout << "i: " << i << ", o: " << o << std::endl;
        return true;
    });
    for (size_t c = 1; c <= 10000; c++)
        munit_assert(my_int_only_triple(1, 2.4, 100, 1.0f, 10000, {2.f, -1}));
    return MUNIT_OK;
}

static MunitResult test_std_function_lambda(const MunitParameter params[], void* user_data)
{
    using FN_Def = std::function<test_fn_signature>;
    FN_Def my_int_only_triple([=](auto x, auto y, auto z, auto w, auto t, auto l) -> bool {
        y *= 3;
        // std::cout << "i: " << i << ", o: " << o << std::endl;
        return true;
    });
    for (size_t c = 1; c <= 10000; c++)
        munit_assert(my_int_only_triple(1, 2.4, 100, 1.0f, 10000, {2.f, -1}));
    return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
    {
        (char*)"/dz::function/lambda",
        test_dz_function_lambda,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL
    },
    {
        (char*)"/std::function/lambda",
        test_std_function_lambda,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL
    }
};

static const MunitSuite test_suite = {
    (char*)"/function.cpp",
    test_suite_tests,
    NULL,
    1,
    MUNIT_SUITE_OPTION_NONE
};

#include <stdlib.h>

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
    return munit_suite_main(&test_suite, (void*) "dz", argc, argv);
}

// struct _12Byte {
//     int x;
//     double y;
//     bool set(int _x, double _y) {
//         x = _x;
//         y = _y;
//         return true;
//     }
// };

// int main()
//     {
//     }
//     {
//         _12Byte _12byte{1, 1.3};
//         FN_Def my_set(&_12byte, &_12Byte::set);
//         std::cout << (my_set(1, 2.4) ? "true" : "false") << std::endl;

//         auto _12byte2 = _12byte;
//         FN_Def my_13_set(&_12byte2, &_12Byte::set);
//         std::cout << (my_13_set(131, 42.) ? "true" : "false") << std::endl;

//         {
//             FN_Def my_int_second_triple([_12byte, _12byte2](int i, const double& o) -> bool {
//                 ((double&)o) *= 3;
//                 std::cout << "i: " << i << ", o: " << o << std::endl;
//                 return true;
//             });
//             std::cout << (my_int_second_triple(3, 84.) ? "true" : "false") << std::endl;
//         }
//     }
//     return 0;
// }