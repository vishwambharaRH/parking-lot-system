/* Input-parsing helpers: every command line goes through these. */

#include <stdlib.h>

#include "test_framework.h"
#include "utils.h"

static void test_trim_removes_spaces_tabs_and_crlf(void) {
    char a[] = "  park X\r\n";
    CHECK_STR(str_trim(a), "park X");
    char b[] = "\t status \t";
    CHECK_STR(str_trim(b), "status");
    char c[] = "no-spaces";
    CHECK_STR(str_trim(c), "no-spaces");
}

static void test_trim_blank_and_empty(void) {
    char a[] = "   \r\n";
    CHECK_STR(str_trim(a), "");
    char b[] = "";
    CHECK_STR(str_trim(b), "");
}

static void test_split_collapses_whitespace(void) {
    char line[] = "  park \tKA-01-HH-1234   White ";
    char *tokens[4];
    CHECK(str_split(line, tokens, 4) == 3);
    CHECK_STR(tokens[0], "park");
    CHECK_STR(tokens[1], "KA-01-HH-1234");
    CHECK_STR(tokens[2], "White");
}

static void test_split_counts_tokens_beyond_max(void) {
    char line[] = "park A B C D";
    char *tokens[2];
    CHECK(str_split(line, tokens, 2) == 5);
    CHECK_STR(tokens[0], "park");
    CHECK_STR(tokens[1], "A");
}

static void test_split_empty(void) {
    char empty[] = "";
    char spaces[] = "    ";
    char *tokens[2];
    CHECK(str_split(empty, tokens, 2) == 0);
    CHECK(str_split(spaces, tokens, 2) == 0);
}

static void test_to_lower(void) {
    char a[] = "WhItE";
    str_to_lower(a);
    CHECK_STR(a, "white");
    char b[] = "KA-01-HH-1234";
    str_to_lower(b);
    CHECK_STR(b, "ka-01-hh-1234");
}

static void test_parse_int_accepts_integers(void) {
    int value = 0;
    CHECK(parse_int("42", &value) && value == 42);
    CHECK(parse_int("0", &value) && value == 0);
    CHECK(parse_int("-3", &value) && value == -3);
    CHECK(parse_int("007", &value) && value == 7);
    CHECK(parse_int("2147483647", &value) && value == 2147483647);
}

static void test_parse_int_rejects_non_integers(void) {
    int value = 99;
    CHECK(!parse_int("", &value));
    CHECK(!parse_int("abc", &value));
    CHECK(!parse_int("12a", &value));
    CHECK(!parse_int("1.5", &value));
    CHECK(!parse_int(" 5", &value));
    CHECK(!parse_int("+5", &value));
    CHECK(!parse_int("-", &value));
    CHECK(!parse_int("2147483648", &value));
    CHECK(!parse_int("99999999999999999999", &value));
    CHECK(value == 99); /* Left untouched on failure. */
}

static void test_xstrdup_copies(void) {
    const char *original = "KA-01";
    char *copy = xstrdup(original);
    CHECK_STR(copy, "KA-01");
    CHECK(copy != original);
    free(copy);
}

int main(void) {
    RUN_TEST(test_trim_removes_spaces_tabs_and_crlf);
    RUN_TEST(test_trim_blank_and_empty);
    RUN_TEST(test_split_collapses_whitespace);
    RUN_TEST(test_split_counts_tokens_beyond_max);
    RUN_TEST(test_split_empty);
    RUN_TEST(test_to_lower);
    RUN_TEST(test_parse_int_accepts_integers);
    RUN_TEST(test_parse_int_rejects_non_integers);
    RUN_TEST(test_xstrdup_copies);
    TEST_SUMMARY();
}
