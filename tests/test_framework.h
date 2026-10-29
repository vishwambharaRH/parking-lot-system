#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

/* A minimal test harness. A failed CHECK prints file:line and carries on, so one
 * run reports every failure; TEST_SUMMARY() at the end of main sets the exit code. */

#include <stdio.h>
#include <string.h>

static int tests_checked = 0;
static int tests_failed = 0;

#define CHECK(condition)                                                          \
    do {                                                                          \
        tests_checked++;                                                          \
        if (!(condition)) {                                                       \
            tests_failed++;                                                       \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__,      \
                    #condition);                                                  \
        }                                                                         \
    } while (0)

/* Compares two C strings and prints both on mismatch, which makes output
 * formatting failures easy to read. */
#define CHECK_STR(actual, expected)                                               \
    do {                                                                          \
        const char *actual_ = (actual);                                           \
        const char *expected_ = (expected);                                       \
        tests_checked++;                                                          \
        if (strcmp(actual_, expected_) != 0) {                                    \
            tests_failed++;                                                       \
            fprintf(stderr, "%s:%d: CHECK_STR failed\n--- expected ---\n%s\n"     \
                            "--- actual ---\n%s\n----------------\n",             \
                    __FILE__, __LINE__, expected_, actual_);                      \
        }                                                                         \
    } while (0)

/* Runs one test function and prints its name. */
#define RUN_TEST(test)                                                            \
    do {                                                                          \
        int failed_before_ = tests_failed;                                        \
        test();                                                                   \
        printf("%s  %s\n", tests_failed == failed_before_ ? "ok  " : "FAIL", #test); \
    } while (0)

#define TEST_SUMMARY()                                                            \
    do {                                                                          \
        printf("%d checks, %d failed\n", tests_checked, tests_failed);            \
        return tests_failed == 0 ? 0 : 1;                                         \
    } while (0)

#endif /* TEST_FRAMEWORK_H */
