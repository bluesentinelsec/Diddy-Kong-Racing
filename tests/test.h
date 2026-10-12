#ifndef DKR_TEST_H
#define DKR_TEST_H

// A deliberately tiny test harness: no dependencies, one executable per suite,
// ctest runs them. CHECK* record a failure and keep going; the process exit
// status is the verdict.

#include <stdio.h>

static int sTestFailures;
static int sTestChecks;
static const char *sTestCurrent = "";

#define CHECK(cond)                                                                               \
    do {                                                                                          \
        sTestChecks++;                                                                            \
        if (!(cond)) {                                                                            \
            sTestFailures++;                                                                      \
            fprintf(stderr, "  FAIL %s:%d in %s: %s\n", __FILE__, __LINE__, sTestCurrent, #cond); \
        }                                                                                         \
    } while (0)

#define CHECK_EQ(actual, expected)                                                                               \
    do {                                                                                                         \
        long long _a = (long long) (actual), _e = (long long) (expected);                                        \
        sTestChecks++;                                                                                           \
        if (_a != _e) {                                                                                          \
            sTestFailures++;                                                                                     \
            fprintf(stderr, "  FAIL %s:%d in %s: %s == %lld, expected %lld\n", __FILE__, __LINE__, sTestCurrent, \
                    #actual, _a, _e);                                                                            \
        }                                                                                                        \
    } while (0)

#define RUN(fn)                     \
    do {                            \
        sTestCurrent = #fn;         \
        fn();                       \
        printf("  ok   %s\n", #fn); \
    } while (0)

static int test_summary(void) {
    printf("%d checks, %d failures\n", sTestChecks, sTestFailures);
    return sTestFailures == 0 ? 0 : 1;
}

#endif
