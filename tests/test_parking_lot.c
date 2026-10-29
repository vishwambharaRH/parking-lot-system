/* ParkingLot through its API: slot counting, allocation order, leave results and
 * queries. Maps are always valid (exactly one entrance, every line full width). */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "parking_lot.h"
#include "test_framework.h"

static char *SAMPLE_MAP[] = {
    "E.....P",
    "P#P#P.P",
    "P.....#",
    "PP#P#.P",
    "P####.P",
};

static ParkingLot *sample_lot(void) {
    return parking_lot_create(5, 7, SAMPLE_MAP, 5);
}

static bool parks_at(ParkingLot *lot, const char *registration, const char *colour, int row,
                     int col) {
    SlotPos pos;
    return parking_lot_park(lot, registration, colour, &pos) == PARK_ALLOCATED &&
           pos.row == row && pos.col == col;
}

static void test_counts_only_reachable_slots(void) {
    /* 12 P's, but (4,1) and (5,1) touch no road or entrance. */
    ParkingLot *lot = sample_lot();
    CHECK(parking_lot_usable_slots(lot) == 10);
    parking_lot_destroy(lot);

    /* A slot next to only other slots and walls is unreachable. */
    char *map[] = {"E.P", "##P"};
    lot = parking_lot_create(2, 3, map, 2);
    CHECK(parking_lot_usable_slots(lot) == 1);
    parking_lot_destroy(lot);
}

static void test_sample_allocation_order(void) {
    ParkingLot *lot = sample_lot();
    CHECK(parks_at(lot, "KA-01-HH-1234", "White", 2, 1));
    CHECK(parks_at(lot, "KA-01-HH-9999", "White", 2, 3));
    CHECK(parks_at(lot, "KA-01-BB-0001", "Black", 2, 5));
    CHECK(parks_at(lot, "KA-01-HH-7777", "Red", 1, 7));
    CHECK(parks_at(lot, "KA-01-HH-2701", "Blue", 2, 7));
    CHECK(parks_at(lot, "KA-01-HH-3141", "Black", 4, 7));
    CHECK(parks_at(lot, "KA-01-HH-5555", "Grey", 4, 4));
    CHECK(parks_at(lot, "KA-01-HH-6666", "White", 5, 7));
    CHECK(parks_at(lot, "KA-01-HH-8888", "Red", 3, 1));
    CHECK(parks_at(lot, "KA-01-HH-1111", "Blue", 4, 2));
    SlotPos pos;
    CHECK(parking_lot_park(lot, "KA-01-HH-0000", "Red", &pos) == PARK_FULL);
    parking_lot_destroy(lot);
}

static void test_ties_break_on_row_then_column(void) {
    /* (1,3) and (2,2) are both distance 2: row 1 wins. */
    char *rows_map[] = {"E.P", "#P#"};
    ParkingLot *lot = parking_lot_create(2, 3, rows_map, 2);
    CHECK(parks_at(lot, "A", "Red", 1, 3));
    CHECK(parks_at(lot, "B", "Red", 2, 2));
    parking_lot_destroy(lot);

    /* (1,1) and (1,3) are both distance 1 from the entrance: column 1 wins. */
    char *cols_map[] = {"PEP"};
    lot = parking_lot_create(1, 3, cols_map, 1);
    CHECK(parks_at(lot, "A", "Red", 1, 1));
    CHECK(parks_at(lot, "B", "Red", 1, 3));
    parking_lot_destroy(lot);
}

static void test_distance_uses_nearest_neighbour(void) {
    /* Slot (2,2) has road above at (1,2), 7 steps away the long way round, and
     * road below at (3,2), 1 step away. It must use the nearer one (distance 2),
     * so it beats (4,3) (distance 3). Taking the first neighbour found (above)
     * would give 8 and the wrong order. */
    char *map[] = {
        "....",
        "#P#.",
        "E...",
        "##P#",
    };
    ParkingLot *lot = parking_lot_create(4, 4, map, 4);
    CHECK(parking_lot_usable_slots(lot) == 2);
    CHECK(parks_at(lot, "A", "Red", 2, 2));
    CHECK(parks_at(lot, "B", "Red", 4, 3));
    parking_lot_destroy(lot);
}

static void test_freed_slot_is_reused_first(void) {
    ParkingLot *lot = sample_lot();
    CHECK(parks_at(lot, "A", "Red", 2, 1));
    CHECK(parks_at(lot, "B", "Red", 2, 3));
    CHECK(parking_lot_leave(lot, 2, 1) == LEAVE_FREED);
    CHECK(parks_at(lot, "C", "Red", 2, 1));
    CHECK(parks_at(lot, "D", "Red", 2, 5));
    parking_lot_destroy(lot);
}

static void test_leave_results(void) {
    ParkingLot *lot = sample_lot();
    CHECK(parks_at(lot, "A", "Red", 2, 1));
    CHECK(parking_lot_leave(lot, 0, 1) == LEAVE_INVALID_SLOT);
    CHECK(parking_lot_leave(lot, 1, 0) == LEAVE_INVALID_SLOT);
    CHECK(parking_lot_leave(lot, 6, 1) == LEAVE_INVALID_SLOT);
    CHECK(parking_lot_leave(lot, 1, 8) == LEAVE_INVALID_SLOT);
    CHECK(parking_lot_leave(lot, -1, -1) == LEAVE_INVALID_SLOT);
    CHECK(parking_lot_leave(lot, 1, 1) == LEAVE_INVALID_SLOT); /* Entrance. */
    CHECK(parking_lot_leave(lot, 1, 2) == LEAVE_INVALID_SLOT); /* Road. */
    CHECK(parking_lot_leave(lot, 2, 2) == LEAVE_INVALID_SLOT); /* Wall. */
    CHECK(parking_lot_leave(lot, 4, 1) == LEAVE_INVALID_SLOT); /* Unreachable slot. */
    CHECK(parking_lot_leave(lot, 2, 3) == LEAVE_ALREADY_FREE);
    CHECK(parking_lot_leave(lot, 2, 1) == LEAVE_FREED);
    CHECK(parking_lot_leave(lot, 2, 1) == LEAVE_ALREADY_FREE);
    parking_lot_destroy(lot);
}

static void test_duplicate_registration(void) {
    ParkingLot *lot = sample_lot();
    SlotPos pos;
    CHECK(parks_at(lot, "A", "Red", 2, 1));
    CHECK(parking_lot_park(lot, "A", "Blue", &pos) == PARK_ALREADY_PARKED);
    /* Registrations are case-sensitive, so this is a different car. */
    CHECK(parks_at(lot, "a", "Red", 2, 3));
    /* Once A leaves, it can park again. */
    CHECK(parking_lot_leave(lot, 2, 1) == LEAVE_FREED);
    CHECK(parks_at(lot, "A", "Blue", 2, 1));
    parking_lot_destroy(lot);
}

static void test_queries_in_row_column_order(void) {
    ParkingLot *lot = sample_lot();
    /* Allocation order is (2,1) (2,3) (2,5) (1,7) — not row order. */
    CHECK(parks_at(lot, "A", "White", 2, 1));
    CHECK(parks_at(lot, "B", "Black", 2, 3));
    CHECK(parks_at(lot, "C", "WHITE", 2, 5));
    CHECK(parks_at(lot, "D", "white", 1, 7));

    ParkedCarList all = parking_lot_parked_cars(lot);
    CHECK(all.count == 4);
    if (all.count == 4) {
        CHECK_STR(all.items[0].registration, "D");
        CHECK_STR(all.items[1].registration, "A");
        CHECK_STR(all.items[2].registration, "B");
        CHECK_STR(all.items[3].registration, "C");
        CHECK(all.items[0].pos.row == 1 && all.items[0].pos.col == 7);
    }
    parked_car_list_free(&all);

    ParkedCarList white = parking_lot_cars_with_colour(lot, "wHiTe");
    CHECK(white.count == 3);
    if (white.count == 3) {
        CHECK_STR(white.items[0].registration, "D");
        CHECK_STR(white.items[1].registration, "A");
        CHECK_STR(white.items[2].registration, "C");
        /* Each car keeps the spelling it was parked with. */
        CHECK_STR(white.items[0].colour, "white");
        CHECK_STR(white.items[2].colour, "WHITE");
    }
    parked_car_list_free(&white);

    ParkedCarList none = parking_lot_cars_with_colour(lot, "Purple");
    CHECK(none.count == 0 && none.items == NULL);
    parking_lot_destroy(lot);
}

static void test_queries_after_leave(void) {
    ParkingLot *lot = sample_lot();
    CHECK(parks_at(lot, "A", "Red", 2, 1));
    CHECK(parks_at(lot, "B", "Red", 2, 3));
    CHECK(parks_at(lot, "C", "Red", 2, 5));
    CHECK(parking_lot_leave(lot, 2, 1) == LEAVE_FREED); /* Forces a swap-remove. */

    ParkedCarList red = parking_lot_cars_with_colour(lot, "red");
    CHECK(red.count == 2);
    if (red.count == 2) {
        CHECK_STR(red.items[0].registration, "B");
        CHECK_STR(red.items[1].registration, "C");
    }
    parked_car_list_free(&red);

    SlotPos pos;
    CHECK(!parking_lot_slot_for_registration(lot, "A", &pos));
    CHECK(parking_lot_slot_for_registration(lot, "C", &pos) && pos.row == 2 && pos.col == 5);
    CHECK(parking_lot_leave(lot, 2, 5) == LEAVE_FREED);
    CHECK(parking_lot_leave(lot, 2, 3) == LEAVE_FREED);

    ParkedCarList empty = parking_lot_parked_cars(lot);
    CHECK(empty.count == 0);
    red = parking_lot_cars_with_colour(lot, "Red"); /* Known colour, no cars left. */
    CHECK(red.count == 0);
    parking_lot_destroy(lot);
}

static void test_rejects_bad_dimensions(void) {
    CHECK(parking_lot_create(0, 7, SAMPLE_MAP, 5) == NULL);
    CHECK(parking_lot_create(5, 0, SAMPLE_MAP, 5) == NULL);
    CHECK(parking_lot_create(-5, 7, SAMPLE_MAP, 5) == NULL);
    CHECK(parking_lot_create(100000, 100000, SAMPLE_MAP, 5) == NULL);
    parking_lot_destroy(NULL); /* Must be a no-op. */
}

/* ---- Random lots checked against a naive model --------------------------- */
/* The model shares no code with the lot: distances come from repeated relaxation
 * instead of BFS, and the nearest free slot from a full scan instead of a heap. */

#define MODEL_MAX 10

typedef struct {
    int rows;
    int cols;
    char grid[MODEL_MAX][MODEL_MAX + 1];
    int road[MODEL_MAX][MODEL_MAX];
    char registration[MODEL_MAX][MODEL_MAX][8]; /* "" = free */
    char colour[MODEL_MAX][MODEL_MAX][8];
} Model;

static const int DR[4] = {-1, 1, 0, 0};
static const int DC[4] = {0, 0, -1, 1};

static bool model_drivable(const Model *m, int r, int c) {
    return m->grid[r][c] == '.' || m->grid[r][c] == 'E';
}

static bool model_inside(const Model *m, int r, int c) {
    return r >= 0 && r < m->rows && c >= 0 && c < m->cols;
}

static void model_compute_roads(Model *m) {
    for (int r = 0; r < m->rows; r++) {
        for (int c = 0; c < m->cols; c++) {
            m->road[r][c] = m->grid[r][c] == 'E' ? 0 : -1;
        }
    }
    for (bool changed = true; changed;) {
        changed = false;
        for (int r = 0; r < m->rows; r++) {
            for (int c = 0; c < m->cols; c++) {
                if (!model_drivable(m, r, c)) {
                    continue;
                }
                for (int d = 0; d < 4; d++) {
                    int nr = r + DR[d];
                    int nc = c + DC[d];
                    if (!model_inside(m, nr, nc) || m->road[nr][nc] < 0) {
                        continue;
                    }
                    if (m->road[r][c] < 0 || m->road[nr][nc] + 1 < m->road[r][c]) {
                        m->road[r][c] = m->road[nr][nc] + 1;
                        changed = true;
                    }
                }
            }
        }
    }
}

static int model_slot_distance(const Model *m, int r, int c) {
    if (m->grid[r][c] != 'P') {
        return -1;
    }
    int best = -1;
    for (int d = 0; d < 4; d++) {
        int nr = r + DR[d];
        int nc = c + DC[d];
        if (model_inside(m, nr, nc) && model_drivable(m, nr, nc) && m->road[nr][nc] >= 0 &&
            (best < 0 || m->road[nr][nc] + 1 < best)) {
            best = m->road[nr][nc] + 1;
        }
    }
    return best;
}

static bool same_colour(const char *a, const char *b) {
    for (; *a != '\0' && *b != '\0'; a++, b++) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
    }
    return *a == *b;
}

/* Checks a query result against the model's cars, in row-major order. */
static bool list_matches_model(const Model *m, ParkedCarList list, const char *colour) {
    size_t i = 0;
    for (int r = 0; r < m->rows; r++) {
        for (int c = 0; c < m->cols; c++) {
            if (m->registration[r][c][0] == '\0' ||
                (colour != NULL && !same_colour(m->colour[r][c], colour))) {
                continue;
            }
            if (i >= list.count || list.items[i].pos.row != r + 1 ||
                list.items[i].pos.col != c + 1 ||
                strcmp(list.items[i].registration, m->registration[r][c]) != 0 ||
                strcmp(list.items[i].colour, m->colour[r][c]) != 0) {
                return false;
            }
            i++;
        }
    }
    return i == list.count;
}

static bool random_lot_matches_model(unsigned seed) {
    srand(seed);
    Model m;
    memset(&m, 0, sizeof m);
    m.rows = 1 + rand() % MODEL_MAX;
    m.cols = 1 + rand() % MODEL_MAX;
    const char palette[] = "..PP#";
    char *lines[MODEL_MAX];
    for (int r = 0; r < m.rows; r++) {
        for (int c = 0; c < m.cols; c++) {
            m.grid[r][c] = palette[rand() % 5];
        }
        lines[r] = m.grid[r];
    }
    m.grid[rand() % m.rows][rand() % m.cols] = 'E'; /* Valid map: exactly one entrance. */
    model_compute_roads(&m);

    ParkingLot *lot = parking_lot_create(m.rows, m.cols, lines, m.rows);
    bool ok = true;
    size_t usable = 0;
    for (int r = 0; r < m.rows; r++) {
        for (int c = 0; c < m.cols; c++) {
            usable += model_slot_distance(&m, r, c) >= 0 ? 1 : 0;
        }
    }
    ok = ok && parking_lot_usable_slots(lot) == usable;

    const char *colours[] = {"Red", "RED", "red", "Blue", "bLuE", "Green"};
    for (int step = 0; step < 300 && ok; step++) {
        char registration[8];
        snprintf(registration, sizeof registration, "R%d", rand() % 15);
        int op = rand() % 6;

        if (op <= 2) {
            const char *colour = colours[rand() % 6];
            bool duplicate = false;
            int best_r = -1;
            int best_c = -1;
            int best_d = 0;
            /* Row-major scan with a strict '<' gives the row/column tie-break. */
            for (int r = 0; r < m.rows; r++) {
                for (int c = 0; c < m.cols; c++) {
                    duplicate = duplicate || strcmp(m.registration[r][c], registration) == 0;
                    int d = model_slot_distance(&m, r, c);
                    if (d >= 0 && m.registration[r][c][0] == '\0' && (best_r < 0 || d < best_d)) {
                        best_r = r;
                        best_c = c;
                        best_d = d;
                    }
                }
            }
            SlotPos pos;
            ParkStatus status = parking_lot_park(lot, registration, colour, &pos);
            if (duplicate) {
                ok = status == PARK_ALREADY_PARKED;
            } else if (best_r < 0) {
                ok = status == PARK_FULL;
            } else {
                ok = status == PARK_ALLOCATED && pos.row == best_r + 1 && pos.col == best_c + 1;
                strcpy(m.registration[best_r][best_c], registration);
                strcpy(m.colour[best_r][best_c], colour);
            }
        } else if (op == 3) {
            int r = rand() % (m.rows + 2) - 1; /* Includes out-of-range rows/cols. */
            int c = rand() % (m.cols + 2) - 1;
            LeaveStatus status = parking_lot_leave(lot, r + 1, c + 1);
            if (!model_inside(&m, r, c) || model_slot_distance(&m, r, c) < 0) {
                ok = status == LEAVE_INVALID_SLOT;
            } else if (m.registration[r][c][0] == '\0') {
                ok = status == LEAVE_ALREADY_FREE;
            } else {
                ok = status == LEAVE_FREED;
                m.registration[r][c][0] = '\0';
            }
        } else if (op == 4) {
            const char *colour = colours[rand() % 6];
            ParkedCarList by_colour = parking_lot_cars_with_colour(lot, colour);
            ParkedCarList all = parking_lot_parked_cars(lot);
            ok = list_matches_model(&m, by_colour, colour) && list_matches_model(&m, all, NULL);
            parked_car_list_free(&by_colour);
            parked_car_list_free(&all);
        } else {
            SlotPos pos;
            bool found = parking_lot_slot_for_registration(lot, registration, &pos);
            int found_r = -1;
            int found_c = -1;
            for (int r = 0; r < m.rows; r++) {
                for (int c = 0; c < m.cols; c++) {
                    if (strcmp(m.registration[r][c], registration) == 0) {
                        found_r = r;
                        found_c = c;
                    }
                }
            }
            ok = found == (found_r >= 0) &&
                 (!found || (pos.row == found_r + 1 && pos.col == found_c + 1));
        }
    }
    parking_lot_destroy(lot);
    if (!ok) {
        fprintf(stderr, "random lot with seed %u disagrees with the model\n", seed);
    }
    return ok;
}

static void test_random_lots_match_model(void) {
    for (unsigned seed = 1; seed <= 3000; seed++) {
        CHECK(random_lot_matches_model(seed));
    }
}

int main(void) {
    RUN_TEST(test_counts_only_reachable_slots);
    RUN_TEST(test_sample_allocation_order);
    RUN_TEST(test_ties_break_on_row_then_column);
    RUN_TEST(test_distance_uses_nearest_neighbour);
    RUN_TEST(test_freed_slot_is_reused_first);
    RUN_TEST(test_leave_results);
    RUN_TEST(test_duplicate_registration);
    RUN_TEST(test_queries_in_row_column_order);
    RUN_TEST(test_queries_after_leave);
    RUN_TEST(test_rejects_bad_dimensions);
    RUN_TEST(test_random_lots_match_model);
    TEST_SUMMARY();
}
