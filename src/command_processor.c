#include "command_processor.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "parking_lot.h"
#include "utils.h"

/* Enough for the longest command (name + 2 args) plus one extra, so str_split can
 * tell us when a line has too many arguments. */
#define MAX_TOKENS 4

/* Column widths of the status table, taken from the sample output. */
#define STATUS_SLOT_WIDTH 10
#define STATUS_REGISTRATION_WIDTH 18

struct CommandProcessor {
    FILE *out;
    ParkingLot *lot; /* NULL until the first create_parking_lot. */

    /* Set while collecting map lines for a create_parking_lot command. */
    bool reading_map;
    int pending_rows;
    int pending_cols;
    char **pending_map;
    int pending_count;
    int pending_capacity;
};

typedef void (*CommandHandler)(CommandProcessor *processor, char **args);

typedef struct {
    const char *name;
    size_t arg_count;
    bool needs_lot;
    CommandHandler handler;
} Command;

/* ---- Output helpers ------------------------------------------------------ */

static void print_slot(FILE *out, SlotPos pos) {
    fprintf(out, "(%d, %d)", pos.row, pos.col);
}

/* Prints text left-aligned in a column of the given width, always followed by at
 * least one space so long values like "(1000, 1000)" don't run into the next column. */
static void print_column(FILE *out, const char *text, int width) {
    int padding = width - (int)strlen(text);
    if (padding < 1) {
        padding = 1;
    }
    fputs(text, out);
    for (int i = 0; i < padding; i++) {
        fputc(' ', out);
    }
}

/* ---- create_parking_lot -------------------------------------------------- */

static void free_pending_map(CommandProcessor *processor) {
    for (int i = 0; i < processor->pending_count; i++) {
        free(processor->pending_map[i]);
    }
    free(processor->pending_map);
    processor->pending_map = NULL;
    processor->pending_count = 0;
    processor->pending_capacity = 0;
    processor->reading_map = false;
}

static void complete_create(CommandProcessor *processor) {
    parking_lot_destroy(processor->lot);
    /* The dimensions were checked in handle_create, so this cannot return NULL. */
    processor->lot = parking_lot_create(processor->pending_rows, processor->pending_cols,
                                        processor->pending_map, processor->pending_count);
    free_pending_map(processor);
    fprintf(processor->out, "Created a parking lot with %zu usable slots\n",
            parking_lot_usable_slots(processor->lot));
}

static void add_map_line(CommandProcessor *processor, const char *line) {
    /* Grown as lines arrive instead of sized from <rows> up front, so a bogus huge
     * row count doesn't allocate memory for lines that never come. */
    if (processor->pending_count == processor->pending_capacity) {
        processor->pending_capacity =
            processor->pending_capacity > 0 ? processor->pending_capacity * 2 : 16;
        processor->pending_map =
            xrealloc(processor->pending_map,
                     (size_t)processor->pending_capacity * sizeof *processor->pending_map);
    }
    processor->pending_map[processor->pending_count++] = xstrdup(line);
    if (processor->pending_count == processor->pending_rows) {
        complete_create(processor);
    }
}

static void handle_create(CommandProcessor *processor, char **args) {
    int rows;
    int cols;
    if (!parse_int(args[0], &rows) || !parse_int(args[1], &cols) || rows <= 0 || cols <= 0 ||
        (long long)rows * cols > PARKING_LOT_MAX_CELLS) {
        /* Without valid dimensions we can't know how many map lines follow, so they
         * will each be reported as invalid commands. */
        fputs("Invalid command\n", processor->out);
        return;
    }
    processor->reading_map = true;
    processor->pending_rows = rows;
    processor->pending_cols = cols;
}

/* ---- Lot commands -------------------------------------------------------- */

static void handle_park(CommandProcessor *processor, char **args) {
    SlotPos pos;
    switch (parking_lot_park(processor->lot, args[0], args[1], &pos)) {
    case PARK_ALLOCATED:
        fputs("Allocated slot: ", processor->out);
        print_slot(processor->out, pos);
        fputc('\n', processor->out);
        break;
    case PARK_FULL:
        fputs("Sorry, parking lot is full\n", processor->out);
        break;
    case PARK_ALREADY_PARKED:
        fprintf(processor->out, "Car %s is already parked\n", args[0]);
        break;
    }
}

static void handle_leave(CommandProcessor *processor, char **args) {
    SlotPos pos;
    if (!parse_int(args[0], &pos.row) || !parse_int(args[1], &pos.col)) {
        fputs("Invalid command\n", processor->out);
        return;
    }
    switch (parking_lot_leave(processor->lot, pos.row, pos.col)) {
    case LEAVE_FREED:
        fputs("Slot ", processor->out);
        print_slot(processor->out, pos);
        fputs(" is free\n", processor->out);
        break;
    case LEAVE_ALREADY_FREE:
        fputs("Slot ", processor->out);
        print_slot(processor->out, pos);
        fputs(" is already free\n", processor->out);
        break;
    case LEAVE_INVALID_SLOT:
        fputs("Invalid slot\n", processor->out);
        break;
    }
}

static void handle_status(CommandProcessor *processor, char **args) {
    (void)args;
    ParkedCarList cars = parking_lot_parked_cars(processor->lot);
    if (cars.count == 0) {
        fputs("Parking lot is empty\n", processor->out);
        return;
    }
    print_column(processor->out, "Slot", STATUS_SLOT_WIDTH);
    print_column(processor->out, "Registration No", STATUS_REGISTRATION_WIDTH);
    fputs("Colour\n", processor->out);

    for (size_t i = 0; i < cars.count; i++) {
        char slot[32];
        snprintf(slot, sizeof slot, "(%d, %d)", cars.items[i].pos.row, cars.items[i].pos.col);
        print_column(processor->out, slot, STATUS_SLOT_WIDTH);
        print_column(processor->out, cars.items[i].registration, STATUS_REGISTRATION_WIDTH);
        fprintf(processor->out, "%s\n", cars.items[i].colour);
    }
    parked_car_list_free(&cars);
}

static void handle_registrations_for_colour(CommandProcessor *processor, char **args) {
    ParkedCarList cars = parking_lot_cars_with_colour(processor->lot, args[0]);
    if (cars.count == 0) {
        fputs("Not found\n", processor->out);
        return;
    }
    for (size_t i = 0; i < cars.count; i++) {
        fprintf(processor->out, "%s%s", i > 0 ? ", " : "", cars.items[i].registration);
    }
    fputc('\n', processor->out);
    parked_car_list_free(&cars);
}

static void handle_slots_for_colour(CommandProcessor *processor, char **args) {
    ParkedCarList cars = parking_lot_cars_with_colour(processor->lot, args[0]);
    if (cars.count == 0) {
        fputs("Not found\n", processor->out);
        return;
    }
    for (size_t i = 0; i < cars.count; i++) {
        if (i > 0) {
            fputs(", ", processor->out);
        }
        print_slot(processor->out, cars.items[i].pos);
    }
    fputc('\n', processor->out);
    parked_car_list_free(&cars);
}

static void handle_slot_for_registration(CommandProcessor *processor, char **args) {
    SlotPos pos;
    if (!parking_lot_slot_for_registration(processor->lot, args[0], &pos)) {
        fputs("Not found\n", processor->out);
        return;
    }
    print_slot(processor->out, pos);
    fputc('\n', processor->out);
}

static const Command COMMANDS[] = {
    {"create_parking_lot", 2, false, handle_create},
    {"park", 2, true, handle_park},
    {"leave", 2, true, handle_leave},
    {"status", 0, true, handle_status},
    {"registration_numbers_for_cars_with_colour", 1, true, handle_registrations_for_colour},
    {"slot_numbers_for_cars_with_colour", 1, true, handle_slots_for_colour},
    {"slot_number_for_registration_number", 1, true, handle_slot_for_registration},
};

/* ---- Public API ---------------------------------------------------------- */

CommandProcessor *command_processor_create(FILE *out) {
    CommandProcessor *processor = xcalloc(1, sizeof *processor);
    processor->out = out;
    return processor;
}

void command_processor_destroy(CommandProcessor *processor) {
    if (processor == NULL) {
        return;
    }
    free_pending_map(processor);
    parking_lot_destroy(processor->lot);
    free(processor);
}

void command_processor_process_line(CommandProcessor *processor, char *line) {
    char *text = str_trim(line);
    if (processor->reading_map) {
        add_map_line(processor, text);
        return;
    }
    if (*text == '\0') {
        return; /* Blank lines between commands are ignored. */
    }

    char *tokens[MAX_TOKENS];
    size_t token_count = str_split(text, tokens, MAX_TOKENS);
    size_t arg_count = token_count - 1;

    for (size_t i = 0; i < sizeof COMMANDS / sizeof COMMANDS[0]; i++) {
        const Command *command = &COMMANDS[i];
        if (strcmp(tokens[0], command->name) != 0) {
            continue;
        }
        if (arg_count != command->arg_count) {
            break;
        }
        if (command->needs_lot && processor->lot == NULL) {
            fputs("Parking lot not created\n", processor->out);
            return;
        }
        command->handler(processor, tokens + 1);
        return;
    }
    fputs("Invalid command\n", processor->out);
}

void command_processor_finish(CommandProcessor *processor) {
    if (processor->reading_map) {
        complete_create(processor);
    }
}
