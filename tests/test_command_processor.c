/* Input handling and output formatting: text in, exact text out. */

#include <stdlib.h>

#include "command_processor.h"
#include "test_framework.h"
#include "utils.h"

/* Feeds the input to a fresh CommandProcessor line by line, the way main does,
 * and returns everything it wrote (caller frees). */
static char *run(const char *input) {
    FILE *out = tmpfile();
    CommandProcessor *processor = command_processor_create(out);

    char *text = xstrdup(input);
    char *line = text;
    while (*line != '\0') {
        char *newline = strchr(line, '\n');
        if (newline != NULL) {
            *newline = '\0';
        }
        command_processor_process_line(processor, line);
        if (newline == NULL) {
            break;
        }
        line = newline + 1;
    }
    command_processor_finish(processor);
    command_processor_destroy(processor);
    free(text);

    long size = ftell(out);
    rewind(out);
    char *output = xcalloc((size_t)size + 1, 1);
    size_t read = fread(output, 1, (size_t)size, out);
    output[read] = '\0';
    fclose(out);
    return output;
}

#define CHECK_OUTPUT(input, expected)                                             \
    do {                                                                          \
        char *output_ = run(input);                                               \
        CHECK_STR(output_, expected);                                             \
        free(output_);                                                            \
    } while (0)

/* The sample lot from the problem statement. */
#define SAMPLE_LOT                                                                \
    "create_parking_lot 5 7\n"                                                    \
    "E.....P\n"                                                                   \
    "P#P#P.P\n"                                                                   \
    "P.....#\n"                                                                   \
    "PP#P#.P\n"                                                                   \
    "P####.P\n"
#define SAMPLE_CREATED "Created a parking lot with 10 usable slots\n"

#define STATUS_HEADER "Slot      Registration No   Colour\n"

/* ---- The full example from the problem statement ------------------------- */

static void test_problem_statement_example(void) {
    CHECK_OUTPUT(SAMPLE_LOT
                 "park KA-01-HH-1234 White\n"
                 "park KA-01-HH-9999 White\n"
                 "park KA-01-BB-0001 Black\n"
                 "park KA-01-HH-7777 Red\n"
                 "park KA-01-HH-2701 Blue\n"
                 "park KA-01-HH-3141 Black\n"
                 "park KA-01-HH-5555 Grey\n"
                 "park KA-01-HH-6666 White\n"
                 "park KA-01-HH-8888 Red\n"
                 "park KA-01-HH-1111 Blue\n"
                 "leave 1 7\n"
                 "leave 2 1\n"
                 "status\n"
                 "park KA-01-P-333 White\n"
                 "park DL-12-AA-9999 White\n"
                 "park MH-04-AY-1111 Black\n"
                 "registration_numbers_for_cars_with_colour White\n"
                 "slot_numbers_for_cars_with_colour White\n"
                 "slot_number_for_registration_number KA-01-HH-3141\n"
                 "slot_number_for_registration_number MH-04-AY-1111\n"
                 "slot_number_for_registration_number TN-07-ZZ-0000\n",
                 SAMPLE_CREATED
                 "Allocated slot: (2, 1)\n"
                 "Allocated slot: (2, 3)\n"
                 "Allocated slot: (2, 5)\n"
                 "Allocated slot: (1, 7)\n"
                 "Allocated slot: (2, 7)\n"
                 "Allocated slot: (4, 7)\n"
                 "Allocated slot: (4, 4)\n"
                 "Allocated slot: (5, 7)\n"
                 "Allocated slot: (3, 1)\n"
                 "Allocated slot: (4, 2)\n"
                 "Slot (1, 7) is free\n"
                 "Slot (2, 1) is free\n"
                 STATUS_HEADER
                 "(2, 3)    KA-01-HH-9999     White\n"
                 "(2, 5)    KA-01-BB-0001     Black\n"
                 "(2, 7)    KA-01-HH-2701     Blue\n"
                 "(3, 1)    KA-01-HH-8888     Red\n"
                 "(4, 2)    KA-01-HH-1111     Blue\n"
                 "(4, 4)    KA-01-HH-5555     Grey\n"
                 "(4, 7)    KA-01-HH-3141     Black\n"
                 "(5, 7)    KA-01-HH-6666     White\n"
                 "Allocated slot: (2, 1)\n"
                 "Allocated slot: (1, 7)\n"
                 "Sorry, parking lot is full\n"
                 "DL-12-AA-9999, KA-01-P-333, KA-01-HH-9999, KA-01-HH-6666\n"
                 "(1, 7), (2, 1), (2, 3), (5, 7)\n"
                 "(4, 7)\n"
                 "Not found\n"
                 "Not found\n");
}

/* ---- Output formatting, one command at a time ---------------------------- */

static void test_create_output(void) {
    CHECK_OUTPUT(SAMPLE_LOT, SAMPLE_CREATED);
    CHECK_OUTPUT("create_parking_lot 1 3\nE.P\n", "Created a parking lot with 1 usable slots\n");
    CHECK_OUTPUT("create_parking_lot 1 3\nE..\n", "Created a parking lot with 0 usable slots\n");
}

static void test_park_output(void) {
    CHECK_OUTPUT("create_parking_lot 1 2\nEP\npark A Red\npark B Red\n",
                 "Created a parking lot with 1 usable slots\n"
                 "Allocated slot: (1, 2)\n"
                 "Sorry, parking lot is full\n");
    CHECK_OUTPUT("create_parking_lot 1 2\nE.\npark A Red\n",
                 "Created a parking lot with 0 usable slots\n"
                 "Sorry, parking lot is full\n");
}

static void test_leave_output(void) {
    CHECK_OUTPUT(SAMPLE_LOT "park A Red\nleave 2 1\nleave 2 1\nleave 2 3\n",
                 SAMPLE_CREATED
                 "Allocated slot: (2, 1)\n"
                 "Slot (2, 1) is free\n"
                 "Slot (2, 1) is already free\n"
                 "Slot (2, 3) is already free\n");
}

static void test_leave_invalid_slot_output(void) {
    CHECK_OUTPUT(SAMPLE_LOT
                 "leave 0 0\n"
                 "leave 6 1\n"
                 "leave 1 8\n"
                 "leave -1 3\n"
                 "leave 1 1\n"  /* Entrance. */
                 "leave 1 2\n"  /* Road. */
                 "leave 2 2\n"  /* Wall. */
                 "leave 4 1\n", /* Unreachable slot. */
                 SAMPLE_CREATED
                 "Invalid slot\nInvalid slot\nInvalid slot\nInvalid slot\n"
                 "Invalid slot\nInvalid slot\nInvalid slot\nInvalid slot\n");
}

static void test_status_output(void) {
    CHECK_OUTPUT(SAMPLE_LOT "status\n", SAMPLE_CREATED "Parking lot is empty\n");

    /* Rows come out in row, then column order, not in parking order. */
    CHECK_OUTPUT(SAMPLE_LOT "park A White\npark B Black\npark C Red\npark D Blue\nstatus\n",
                 SAMPLE_CREATED
                 "Allocated slot: (2, 1)\n"
                 "Allocated slot: (2, 3)\n"
                 "Allocated slot: (2, 5)\n"
                 "Allocated slot: (1, 7)\n"
                 STATUS_HEADER
                 "(1, 7)    D                 Blue\n"
                 "(2, 1)    A                 White\n"
                 "(2, 3)    B                 Black\n"
                 "(2, 5)    C                 Red\n");

    /* Emptying the lot again brings back the empty message (no header). */
    CHECK_OUTPUT(SAMPLE_LOT "park A Red\nleave 2 1\nstatus\n",
                 SAMPLE_CREATED
                 "Allocated slot: (2, 1)\n"
                 "Slot (2, 1) is free\n"
                 "Parking lot is empty\n");
}

static void test_status_columns_stay_separated(void) {
    /* A 7-character slot and a 22-character registration overflow their columns;
     * there must still be at least one space before the next column. */
    CHECK_OUTPUT("create_parking_lot 1 12\nE..........P\n"
                 "park ABCDEFGHIJKLMNOPQRSTUV Red\nstatus\n",
                 "Created a parking lot with 1 usable slots\n"
                 "Allocated slot: (1, 12)\n"
                 STATUS_HEADER
                 "(1, 12)   ABCDEFGHIJKLMNOPQRSTUV Red\n");

    /* An 18-character registration exactly fills its column and still gets one space. */
    CHECK_OUTPUT("create_parking_lot 1 2\nEP\npark ABCDEFGHIJKLMNOPQR Red\nstatus\n",
                 "Created a parking lot with 1 usable slots\n"
                 "Allocated slot: (1, 2)\n"
                 STATUS_HEADER
                 "(1, 2)    ABCDEFGHIJKLMNOPQR Red\n");
}

static void test_colour_query_output(void) {
    CHECK_OUTPUT(SAMPLE_LOT
                 "park A White\npark B Black\npark C White\n"
                 "registration_numbers_for_cars_with_colour White\n"
                 "slot_numbers_for_cars_with_colour White\n"
                 "registration_numbers_for_cars_with_colour Black\n"
                 "slot_numbers_for_cars_with_colour Black\n"
                 "registration_numbers_for_cars_with_colour Green\n"
                 "slot_numbers_for_cars_with_colour Green\n",
                 SAMPLE_CREATED
                 "Allocated slot: (2, 1)\n"
                 "Allocated slot: (2, 3)\n"
                 "Allocated slot: (2, 5)\n"
                 "A, C\n"
                 "(2, 1), (2, 5)\n"
                 "B\n"
                 "(2, 3)\n"
                 "Not found\n"
                 "Not found\n");
}

static void test_colour_query_after_all_leave(void) {
    CHECK_OUTPUT(SAMPLE_LOT
                 "park A White\nleave 2 1\n"
                 "registration_numbers_for_cars_with_colour White\n"
                 "slot_numbers_for_cars_with_colour White\n",
                 SAMPLE_CREATED
                 "Allocated slot: (2, 1)\n"
                 "Slot (2, 1) is free\n"
                 "Not found\n"
                 "Not found\n");
}

static void test_colour_matching_is_case_insensitive(void) {
    CHECK_OUTPUT(SAMPLE_LOT
                 "park A WHITE\npark B white\npark C White\n"
                 "registration_numbers_for_cars_with_colour wHiTe\n"
                 "slot_numbers_for_cars_with_colour WHITE\n"
                 "status\n",
                 SAMPLE_CREATED
                 "Allocated slot: (2, 1)\n"
                 "Allocated slot: (2, 3)\n"
                 "Allocated slot: (2, 5)\n"
                 "A, B, C\n"
                 "(2, 1), (2, 3), (2, 5)\n"
                 STATUS_HEADER
                 "(2, 1)    A                 WHITE\n"
                 "(2, 3)    B                 white\n"
                 "(2, 5)    C                 White\n");
}

static void test_registration_lookup_output(void) {
    CHECK_OUTPUT(SAMPLE_LOT
                 "park KA-01 Red\n"
                 "slot_number_for_registration_number KA-01\n"
                 "slot_number_for_registration_number ka-01\n" /* Case-sensitive. */
                 "slot_number_for_registration_number KA-02\n"
                 "leave 2 1\n"
                 "slot_number_for_registration_number KA-01\n",
                 SAMPLE_CREATED
                 "Allocated slot: (2, 1)\n"
                 "(2, 1)\n"
                 "Not found\n"
                 "Not found\n"
                 "Slot (2, 1) is free\n"
                 "Not found\n");
}

static void test_duplicate_registration_output(void) {
    CHECK_OUTPUT(SAMPLE_LOT "park A Red\npark A Blue\nleave 2 1\npark A Blue\n",
                 SAMPLE_CREATED
                 "Allocated slot: (2, 1)\n"
                 "Car A is already parked\n"
                 "Slot (2, 1) is free\n"
                 "Allocated slot: (2, 1)\n");
}

/* ---- Input handling ------------------------------------------------------ */

static void test_windows_line_endings(void) {
    CHECK_OUTPUT("create_parking_lot 1 3\r\nE.P\r\npark A Red\r\nstatus\r\n"
                 "registration_numbers_for_cars_with_colour red\r\n",
                 "Created a parking lot with 1 usable slots\n"
                 "Allocated slot: (1, 3)\n"
                 STATUS_HEADER
                 "(1, 3)    A                 Red\n"
                 "A\n");
}

static void test_extra_whitespace_and_blank_lines(void) {
    CHECK_OUTPUT("\n   \n"
                 "  create_parking_lot   1\t3  \n"
                 "E.P   \n"
                 "\n"
                 "\t park \t A    Red  \n"
                 "   \n"
                 "leave  1   3\n",
                 "Created a parking lot with 1 usable slots\n"
                 "Allocated slot: (1, 3)\n"
                 "Slot (1, 3) is free\n");
}

static void test_last_line_without_newline(void) {
    CHECK_OUTPUT("create_parking_lot 1 2\nEP\npark A Red",
                 "Created a parking lot with 1 usable slots\n"
                 "Allocated slot: (1, 2)\n");
}

static void test_commands_before_create(void) {
    CHECK_OUTPUT("park A Red\n"
                 "leave 1 1\n"
                 "status\n"
                 "registration_numbers_for_cars_with_colour Red\n"
                 "slot_numbers_for_cars_with_colour Red\n"
                 "slot_number_for_registration_number A\n",
                 "Parking lot not created\n"
                 "Parking lot not created\n"
                 "Parking lot not created\n"
                 "Parking lot not created\n"
                 "Parking lot not created\n"
                 "Parking lot not created\n");
}

static void test_unknown_commands(void) {
    CHECK_OUTPUT(SAMPLE_LOT
                 "fly away\n"
                 "PARK A Red\n"
                 "Status\n"
                 "parkA Red\n"
                 "slot_numbers_for_cars_with_color Red\n" /* US spelling isn't a command. */
                 "regstration_numbers_for_cars_with_colour Red\n",
                 SAMPLE_CREATED
                 "Invalid command\nInvalid command\nInvalid command\n"
                 "Invalid command\nInvalid command\nInvalid command\n");
}

static void test_wrong_argument_counts(void) {
    CHECK_OUTPUT(SAMPLE_LOT
                 "park\n"
                 "park A\n"
                 "park A Red Extra\n"
                 "leave\n"
                 "leave 1\n"
                 "leave 1 2 3\n"
                 "status now\n"
                 "registration_numbers_for_cars_with_colour\n"
                 "registration_numbers_for_cars_with_colour Red Blue\n"
                 "slot_numbers_for_cars_with_colour\n"
                 "slot_numbers_for_cars_with_colour Red Blue\n"
                 "slot_number_for_registration_number\n"
                 "slot_number_for_registration_number A B\n"
                 "status\n", /* The lot is untouched. */
                 SAMPLE_CREATED
                 "Invalid command\nInvalid command\nInvalid command\n"
                 "Invalid command\nInvalid command\nInvalid command\n"
                 "Invalid command\n"
                 "Invalid command\nInvalid command\n"
                 "Invalid command\nInvalid command\n"
                 "Invalid command\nInvalid command\n"
                 "Parking lot is empty\n");
}

static void test_wrong_argument_count_before_create(void) {
    /* The argument count is checked before whether a lot exists. */
    CHECK_OUTPUT("park A\nstatus now\n", "Invalid command\nInvalid command\n");
}

static void test_non_numeric_leave(void) {
    CHECK_OUTPUT(SAMPLE_LOT
                 "leave a 1\n"
                 "leave 1 b\n"
                 "leave 1.5 2\n"
                 "leave 2x 1\n"
                 "leave 99999999999 1\n",
                 SAMPLE_CREATED
                 "Invalid command\nInvalid command\nInvalid command\n"
                 "Invalid command\nInvalid command\n");
}

static void test_bad_create_arguments(void) {
    CHECK_OUTPUT("create_parking_lot\n"
                 "create_parking_lot 5\n"
                 "create_parking_lot 5 7 9\n"
                 "create_parking_lot x 7\n"
                 "create_parking_lot 5 y\n"
                 "create_parking_lot 0 7\n"
                 "create_parking_lot 5 -7\n"
                 "status\n", /* None of these created a lot. */
                 "Invalid command\nInvalid command\nInvalid command\n"
                 "Invalid command\nInvalid command\nInvalid command\n"
                 "Invalid command\n"
                 "Parking lot not created\n");
}

static void test_create_again_replaces_lot(void) {
    CHECK_OUTPUT("create_parking_lot 1 2\nEP\n"
                 "park A Red\n"
                 "create_parking_lot 1 3\nPEP\n"
                 "status\n"
                 "slot_number_for_registration_number A\n"
                 "park A Red\n",
                 "Created a parking lot with 1 usable slots\n"
                 "Allocated slot: (1, 2)\n"
                 "Created a parking lot with 2 usable slots\n"
                 "Parking lot is empty\n"
                 "Not found\n"
                 "Allocated slot: (1, 1)\n");
}

static void test_map_lines_are_not_commands(void) {
    /* While reading the map, lines are never treated as commands. */
    CHECK_OUTPUT("create_parking_lot 2 4\nEP..\n....\nstatus\n",
                 "Created a parking lot with 1 usable slots\n"
                 "Parking lot is empty\n");
}

static void test_random_commands_never_crash(void) {
    /* Random command lines on a valid lot. Run with SANITIZE=1, this also catches
     * memory errors; the check here is that every line produced output. */
    const char *words[] = {
        "park", "leave", "status", "registration_numbers_for_cars_with_colour",
        "slot_numbers_for_cars_with_colour", "slot_number_for_registration_number",
        "create_parking_lot", "1", "2", "7", "-1", "0", "x", "Red", "rED", "A", "B",
        "99999999999", "\t", "\r", "",
    };
    size_t word_count = sizeof words / sizeof words[0];
    srand(7);
    for (int round = 0; round < 2000; round++) {
        /* 30 lines of at most 4 words of at most 41 characters fit easily. */
        char input[8192] = SAMPLE_LOT;
        int command_lines = 0;
        for (int l = 0; l < 30; l++) {
            int n = 1 + rand() % 4;
            const char *first = words[rand() % 6]; /* Never create, so line counts stay fixed. */
            strcat(input, first);
            for (int w = 1; w < n; w++) {
                strcat(input, " ");
                const char *word = words[rand() % word_count];
                strcat(input, strcmp(word, "create_parking_lot") == 0 ? "Red" : word);
            }
            strcat(input, "\n");
            command_lines++;
        }
        char *output = run(input);
        /* Every command prints at least one line; status may print more. */
        int output_lines = 0;
        for (const char *p = output; *p != '\0'; p++) {
            output_lines += *p == '\n';
        }
        CHECK(output_lines >= 1 + command_lines);
        free(output);
    }
}

int main(void) {
    RUN_TEST(test_problem_statement_example);
    RUN_TEST(test_create_output);
    RUN_TEST(test_park_output);
    RUN_TEST(test_leave_output);
    RUN_TEST(test_leave_invalid_slot_output);
    RUN_TEST(test_status_output);
    RUN_TEST(test_status_columns_stay_separated);
    RUN_TEST(test_colour_query_output);
    RUN_TEST(test_colour_query_after_all_leave);
    RUN_TEST(test_colour_matching_is_case_insensitive);
    RUN_TEST(test_registration_lookup_output);
    RUN_TEST(test_duplicate_registration_output);
    RUN_TEST(test_windows_line_endings);
    RUN_TEST(test_extra_whitespace_and_blank_lines);
    RUN_TEST(test_last_line_without_newline);
    RUN_TEST(test_commands_before_create);
    RUN_TEST(test_unknown_commands);
    RUN_TEST(test_wrong_argument_counts);
    RUN_TEST(test_wrong_argument_count_before_create);
    RUN_TEST(test_non_numeric_leave);
    RUN_TEST(test_bad_create_arguments);
    RUN_TEST(test_create_again_replaces_lot);
    RUN_TEST(test_map_lines_are_not_commands);
    RUN_TEST(test_random_commands_never_crash);
    TEST_SUMMARY();
}
