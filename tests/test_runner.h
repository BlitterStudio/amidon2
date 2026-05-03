#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <cstdio>
#include <cstring>
#include <cstdlib>

extern int g_tests_run;
extern int g_tests_passed;
extern int g_tests_failed;

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        fprintf(stderr, "  FAIL: %s:%d: %s != %s\n", __FILE__, __LINE__, #a, #b); \
        g_tests_failed++; return; \
    } } while(0)

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        fprintf(stderr, "  FAIL: %s:%d: %s is false\n", __FILE__, __LINE__, #x); \
        g_tests_failed++; return; \
    } } while(0)

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))

#define ASSERT_STREQ(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        fprintf(stderr, "  FAIL: %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); \
        g_tests_failed++; return; \
    } } while(0)

#define RUN_TEST(func) do { \
    g_tests_run++; \
    printf("  %-50s", #func); \
    int _before = g_tests_failed; \
    func(); \
    if (g_tests_failed == _before) { g_tests_passed++; printf("OK\n"); } \
    else printf("FAILED\n"); \
    } while(0)

#define TEST_SUMMARY() do { \
    printf("\n%d tests, %d passed, %d failed\n", g_tests_run, g_tests_passed, g_tests_failed); \
    return g_tests_failed > 0 ? 1 : 0; \
    } while(0)

#endif
