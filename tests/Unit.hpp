#pragma once
#include <munit.h>
#define test_src_def(FN_NAME) MunitResult test_##FN_NAME(const MunitParameter[], void *user_data)
#define test_suite_simple_def(NAME) { (char*)#NAME, test_##NAME, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
#define test_suite_end {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}