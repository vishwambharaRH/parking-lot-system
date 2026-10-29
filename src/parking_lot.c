#include "parking_lot.h"

#include <stdlib.h>
#include <string.h>

#include "int_vec.h"
#include "min_heap.h"
#include "str_map.h"
#include "utils.h"

#define CELL_ENTRANCE 'E'
#define CELL_ROAD '.'
#define CELL_WALL '#'
#define CELL_SLOT 'P'

typedef struct {
    SlotPos pos;
    int distance;
    int cell; /* Row-major index: (row-1) * cols + (col-1). */

    /* The fields below are only meaningful while a car is parked here. */
    char *registration; /* Owned copy; NULL when the slot is free. */
    char *colour;       /* Owned copy, original spelling. */
    int colour_id;
    size_t occupied_index; /* Where this slot's cell sits in lot->occupied. */
    size_t colour_index;   /* Where it sits in lot->cells_by_colour[colour_id]. */
} Slot;

struct ParkingLot {
    int rows;
    int cols;

    /* Usable slots sorted by (distance, row, col). A slot's index here is its
     * "rank": a lower rank is always the better slot to hand out. */
    Slot *slots;
    size_t slot_count;

    /* Cell index -> rank, or -1 if the cell is not a usable slot. */
    int *rank_of_cell;

    /* Ranks of free slots; popping gives the nearest one. */
    MinHeap free_ranks;

    /* Cells of occupied slots, in no particular order. Sorted on demand. */
    IntVec occupied;

    StrMap rank_of_registration;

    /* Lowercased colour -> colour id, and colour id -> cells holding that colour.
     * Ids are never removed; a colour with no parked cars just has an empty list. */
    StrMap colour_ids;
    IntVec *cells_by_colour;
    size_t colour_count;
    size_t colour_capacity;
};

/* ---- Building the lot ---------------------------------------------------- */

static bool is_drivable(char c) {
    return c == CELL_ROAD || c == CELL_ENTRANCE;
}

/* Copies the map into a rows * cols grid, padding short or missing lines with
 * walls. This way the BFS never has to worry about line lengths. */
static char *build_grid(int rows, int cols, char **map_lines, int line_count) {
    char *grid = xmalloc((size_t)rows * (size_t)cols);
    memset(grid, CELL_WALL, (size_t)rows * (size_t)cols);
    for (int r = 0; r < rows && r < line_count; r++) {
        size_t length = strlen(map_lines[r]);
        size_t copy = length < (size_t)cols ? length : (size_t)cols;
        memcpy(grid + (size_t)r * (size_t)cols, map_lines[r], copy);
    }
    return grid;
}

/* Breadth-first search from the entrance over drivable cells. Returns an array
 * where each drivable reachable cell holds its step count, and everything else -1. */
static int *road_distances(const char *grid, int rows, int cols) {
    size_t cell_count = (size_t)rows * (size_t)cols;
    int *distance = xmalloc(cell_count * sizeof *distance);
    for (size_t i = 0; i < cell_count; i++) {
        distance[i] = -1;
    }

    const char *entrance = memchr(grid, CELL_ENTRANCE, cell_count);
    if (entrance == NULL) {
        return distance;
    }

    /* Each cell is enqueued at most once, so a plain array works as the queue. */
    int *queue = xmalloc(cell_count * sizeof *queue);
    size_t head = 0;
    size_t tail = 0;
    int start = (int)(entrance - grid);
    distance[start] = 0;
    queue[tail++] = start;

    static const int dr[4] = {-1, 1, 0, 0};
    static const int dc[4] = {0, 0, -1, 1};
    while (head < tail) {
        int cell = queue[head++];
        int r = cell / cols;
        int c = cell % cols;
        for (int d = 0; d < 4; d++) {
            int nr = r + dr[d];
            int nc = c + dc[d];
            if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) {
                continue;
            }
            int next = nr * cols + nc;
            if (distance[next] == -1 && is_drivable(grid[next])) {
                distance[next] = distance[cell] + 1;
                queue[tail++] = next;
            }
        }
    }
    free(queue);
    return distance;
}

/* A slot's distance is 1 + the smallest distance of a reachable road/entrance
 * cell next to it, or -1 if no such neighbour exists (the slot is unusable). */
static int slot_distance(const int *road_distance, int rows, int cols, int r, int c) {
    static const int dr[4] = {-1, 1, 0, 0};
    static const int dc[4] = {0, 0, -1, 1};
    int best = -1;
    for (int d = 0; d < 4; d++) {
        int nr = r + dr[d];
        int nc = c + dc[d];
        if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) {
            continue;
        }
        int neighbour = road_distance[nr * cols + nc];
        if (neighbour >= 0 && (best == -1 || neighbour + 1 < best)) {
            best = neighbour + 1;
        }
    }
    return best;
}

static int compare_slots(const void *a, const void *b) {
    const Slot *x = a;
    const Slot *y = b;
    if (x->distance != y->distance) {
        return x->distance < y->distance ? -1 : 1;
    }
    if (x->pos.row != y->pos.row) {
        return x->pos.row < y->pos.row ? -1 : 1;
    }
    if (x->pos.col != y->pos.col) {
        return x->pos.col < y->pos.col ? -1 : 1;
    }
    return 0;
}

ParkingLot *parking_lot_create(int rows, int cols, char **map_lines, int line_count) {
    if (rows <= 0 || cols <= 0 || (long long)rows * cols > PARKING_LOT_MAX_CELLS) {
        return NULL;
    }
    size_t cell_count = (size_t)rows * (size_t)cols;

    char *grid = build_grid(rows, cols, map_lines, line_count);
    int *road_distance = road_distances(grid, rows, cols);

    ParkingLot *lot = xcalloc(1, sizeof *lot);
    lot->rows = rows;
    lot->cols = cols;

    /* Collect every reachable slot. xcalloc leaves the car fields NULL/0 (free). */
    lot->slots = xcalloc(cell_count, sizeof *lot->slots);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int cell = r * cols + c;
            if (grid[cell] != CELL_SLOT) {
                continue;
            }
            int distance = slot_distance(road_distance, rows, cols, r, c);
            if (distance < 0) {
                continue;
            }
            Slot *slot = &lot->slots[lot->slot_count++];
            slot->pos.row = r + 1;
            slot->pos.col = c + 1;
            slot->distance = distance;
            slot->cell = cell;
        }
    }
    free(grid);
    free(road_distance);

    /* Sorting once means "nearest free slot" is just "lowest free rank" from now on. */
    lot->slots = xrealloc(lot->slots, lot->slot_count * sizeof *lot->slots);
    qsort(lot->slots, lot->slot_count, sizeof *lot->slots, compare_slots);

    lot->rank_of_cell = xmalloc(cell_count * sizeof *lot->rank_of_cell);
    for (size_t i = 0; i < cell_count; i++) {
        lot->rank_of_cell[i] = -1;
    }
    min_heap_init(&lot->free_ranks, lot->slot_count);
    for (size_t rank = 0; rank < lot->slot_count; rank++) {
        lot->rank_of_cell[lot->slots[rank].cell] = (int)rank;
        min_heap_push(&lot->free_ranks, (int)rank);
    }

    str_map_init(&lot->rank_of_registration);
    str_map_init(&lot->colour_ids);
    return lot;
}

void parking_lot_destroy(ParkingLot *lot) {
    if (lot == NULL) {
        return;
    }
    for (size_t i = 0; i < lot->slot_count; i++) {
        free(lot->slots[i].registration);
        free(lot->slots[i].colour);
    }
    free(lot->slots);
    free(lot->rank_of_cell);
    min_heap_free(&lot->free_ranks);
    int_vec_free(&lot->occupied);
    str_map_free(&lot->rank_of_registration);
    str_map_free(&lot->colour_ids);
    for (size_t i = 0; i < lot->colour_count; i++) {
        int_vec_free(&lot->cells_by_colour[i]);
    }
    free(lot->cells_by_colour);
    free(lot);
}

size_t parking_lot_usable_slots(const ParkingLot *lot) {
    return lot->slot_count;
}

/* ---- Parking and leaving ------------------------------------------------- */

static Slot *slot_at_cell(const ParkingLot *lot, int cell) {
    return &lot->slots[lot->rank_of_cell[cell]];
}

/* Colours are matched case-insensitively by keying them on a lowercase copy.
 * The caller frees the returned string. */
static char *colour_key(const char *colour) {
    char *key = xstrdup(colour);
    str_to_lower(key);
    return key;
}

/* Returns the id for a colour, giving a new colour the next id and an empty list. */
static int colour_id_for_parking(ParkingLot *lot, const char *colour) {
    char *key = colour_key(colour);
    int id;
    if (!str_map_get(&lot->colour_ids, key, &id)) {
        if (lot->colour_count == lot->colour_capacity) {
            lot->colour_capacity = lot->colour_capacity > 0 ? lot->colour_capacity * 2 : 8;
            lot->cells_by_colour = xrealloc(lot->cells_by_colour,
                                            lot->colour_capacity * sizeof *lot->cells_by_colour);
        }
        id = (int)lot->colour_count;
        lot->cells_by_colour[id] = (IntVec){0};
        lot->colour_count++;
        str_map_put(&lot->colour_ids, key, id);
    }
    free(key);
    return id;
}

ParkStatus parking_lot_park(ParkingLot *lot, const char *registration, const char *colour,
                            SlotPos *out_pos) {
    int rank;
    if (str_map_get(&lot->rank_of_registration, registration, &rank)) {
        return PARK_ALREADY_PARKED;
    }
    if (!min_heap_pop(&lot->free_ranks, &rank)) {
        return PARK_FULL;
    }

    Slot *slot = &lot->slots[rank];
    slot->registration = xstrdup(registration);
    slot->colour = xstrdup(colour);
    slot->colour_id = colour_id_for_parking(lot, colour);

    slot->occupied_index = lot->occupied.size;
    int_vec_push(&lot->occupied, slot->cell);

    IntVec *same_colour = &lot->cells_by_colour[slot->colour_id];
    slot->colour_index = same_colour->size;
    int_vec_push(same_colour, slot->cell);

    str_map_put(&lot->rank_of_registration, registration, rank);
    *out_pos = slot->pos;
    return PARK_ALLOCATED;
}

/* Removing from an unordered list moves its last element into the gap, so the
 * slot owning that moved cell must learn its new index. */
static void remove_from_occupied(ParkingLot *lot, const Slot *slot) {
    size_t index = slot->occupied_index;
    int_vec_swap_remove(&lot->occupied, index);
    if (index < lot->occupied.size) {
        slot_at_cell(lot, lot->occupied.data[index])->occupied_index = index;
    }
}

static void remove_from_colour_list(ParkingLot *lot, const Slot *slot) {
    IntVec *same_colour = &lot->cells_by_colour[slot->colour_id];
    size_t index = slot->colour_index;
    int_vec_swap_remove(same_colour, index);
    if (index < same_colour->size) {
        slot_at_cell(lot, same_colour->data[index])->colour_index = index;
    }
}

LeaveStatus parking_lot_leave(ParkingLot *lot, int row, int col) {
    if (row < 1 || row > lot->rows || col < 1 || col > lot->cols) {
        return LEAVE_INVALID_SLOT;
    }
    int rank = lot->rank_of_cell[(row - 1) * lot->cols + (col - 1)];
    if (rank < 0) {
        return LEAVE_INVALID_SLOT;
    }
    Slot *slot = &lot->slots[rank];
    if (slot->registration == NULL) {
        return LEAVE_ALREADY_FREE;
    }

    remove_from_occupied(lot, slot);
    remove_from_colour_list(lot, slot);
    str_map_remove(&lot->rank_of_registration, slot->registration);

    free(slot->registration);
    free(slot->colour);
    slot->registration = NULL;
    slot->colour = NULL;

    min_heap_push(&lot->free_ranks, rank);
    return LEAVE_FREED;
}

/* ---- Queries ------------------------------------------------------------- */

void parked_car_list_free(ParkedCarList *list) {
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

/* Turns a list of occupied cells into cars ordered by row then column. Sorting
 * by row-major cell index gives exactly that order. */
static ParkedCarList cars_at_cells(const ParkingLot *lot, const IntVec *cells) {
    ParkedCarList list = {NULL, 0};
    if (cells->size == 0) {
        return list;
    }
    int *sorted = int_vec_sorted_copy(cells);
    list.items = xmalloc(cells->size * sizeof *list.items);
    for (size_t i = 0; i < cells->size; i++) {
        const Slot *slot = slot_at_cell(lot, sorted[i]);
        list.items[i].pos = slot->pos;
        list.items[i].registration = slot->registration;
        list.items[i].colour = slot->colour;
    }
    list.count = cells->size;
    free(sorted);
    return list;
}

ParkedCarList parking_lot_parked_cars(const ParkingLot *lot) {
    return cars_at_cells(lot, &lot->occupied);
}

ParkedCarList parking_lot_cars_with_colour(const ParkingLot *lot, const char *colour) {
    char *key = colour_key(colour);
    int id;
    bool known = str_map_get(&lot->colour_ids, key, &id);
    free(key);
    if (!known) {
        ParkedCarList empty = {NULL, 0};
        return empty;
    }
    return cars_at_cells(lot, &lot->cells_by_colour[id]);
}

bool parking_lot_slot_for_registration(const ParkingLot *lot, const char *registration,
                                       SlotPos *out_pos) {
    int rank;
    if (!str_map_get(&lot->rank_of_registration, registration, &rank)) {
        return false;
    }
    *out_pos = lot->slots[rank].pos;
    return true;
}
