#ifndef PARKING_LOT_H
#define PARKING_LOT_H

#include <stdbool.h>
#include <stddef.h>

/* Upper bound on rows * cols, to keep memory use sane on bad input. */
#define PARKING_LOT_MAX_CELLS 10000000L

/* A cell on the map. Rows and columns are 1-based; row 1 is the top. */
typedef struct {
    int row;
    int col;
} SlotPos;

typedef struct {
    SlotPos pos;
    /* Both strings belong to the lot. They stay valid until that car leaves or
     * the lot is destroyed. */
    const char *registration;
    const char *colour; /* As given when parking, so output keeps the original spelling. */
} ParkedCar;

/* A query result. Free with parked_car_list_free (this frees only the array, not
 * the strings). */
typedef struct {
    ParkedCar *items;
    size_t count;
} ParkedCarList;

void parked_car_list_free(ParkedCarList *list);

typedef enum {
    PARK_ALLOCATED,
    PARK_FULL,
    PARK_ALREADY_PARKED
} ParkStatus;

typedef enum {
    LEAVE_FREED,
    LEAVE_ALREADY_FREE,
    LEAVE_INVALID_SLOT
} LeaveStatus;

/* Opaque: the fields are private to parking_lot.c, so other code goes through
 * the functions below. This is C's version of a class with private members. */
typedef struct ParkingLot ParkingLot;

/* Builds a lot from the map lines and computes every slot's distance from the
 * entrance. map_lines is not modified. Returns NULL if rows/cols are not positive
 * or rows * cols exceeds PARKING_LOT_MAX_CELLS. Missing lines or characters count
 * as walls; extra characters are ignored. */
ParkingLot *parking_lot_create(int rows, int cols, char **map_lines, int line_count);

void parking_lot_destroy(ParkingLot *lot);

/* Number of slots reachable from the entrance. */
size_t parking_lot_usable_slots(const ParkingLot *lot);

/* Parks in the nearest free slot (smallest distance, then row, then column).
 * On PARK_ALLOCATED, *out_pos is set to the chosen slot. */
ParkStatus parking_lot_park(ParkingLot *lot, const char *registration, const char *colour,
                            SlotPos *out_pos);

LeaveStatus parking_lot_leave(ParkingLot *lot, int row, int col);

/* All parked cars, ordered by row then column. */
ParkedCarList parking_lot_parked_cars(const ParkingLot *lot);

/* Parked cars whose colour matches case-insensitively, ordered by row then column. */
ParkedCarList parking_lot_cars_with_colour(const ParkingLot *lot, const char *colour);

/* Stores the car's slot in *out_pos. Returns false if that registration isn't parked. */
bool parking_lot_slot_for_registration(const ParkingLot *lot, const char *registration,
                                       SlotPos *out_pos);

#endif /* PARKING_LOT_H */
