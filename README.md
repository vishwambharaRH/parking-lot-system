# Parking Lot System

Language used: **C**

Functions: 
```create_parking_lot <rows> <cols> <\newline> <maplines>```

```park <reg_no> <colour>```

```leave <row> <col>```

```status```

```registration_numbers_for_cars_with_colour <colour>``` : return registration numbers of cars with given colour

```slot_numbers_for_cars_with_colour <colour>``` : return slot numbers for cars with given color

```slot_number_for_registration_number <reg_no>```: return active slot

## Requirements
- A C11 compiler (`cc`, `gcc` or `clang`)
- `make`

## System information
Tested on macOS (Apple Clang 21, GNU Make 3.81)
Written in standard C11 plus POSIX `getline`, so it should also build on Linux with gcc or clang; on Windows, use WSL.

## Running commands
- Download/clone the repository and navigate to it.
    ```sh
   git clone https://github.com/vishwambharaRH/parking-lot-system
   cd parking-lot-system
   ```
- Build the program using make:
    ```sh
    make
    ```
    An executable is created, viz. `./parking_lot`
- Run it on an input file: 
    ```sh
    ./parking_lot input.txt
    ```
    For running and obtaining CLI output, 
    ```sh
    ./parking_lot tests/data/sample.in
    ```

    To save to an output file instead of printing in the CLI,
    ```sh
    ./parking_lot input.txt > output.txt
    ```

## Running Tests

```sh
make test
```

This builds and runs the following:
- **Unit tests** (`tests/test_*.c`) for the string helpers, the data structures, the parking lot logic, and command parsing and output formatting.
- **End-to-end tests**: runs `./parking_lot` on each `tests/data/*.in` file and checks that the output matches the matching `.out` file exactly.
- **Command-line checks**: running with no arguments or with a missing file must exit with status 1.

Each test will print `ok` or `FAIL`.

To run tests with AddressSanitizer and UndefinedBehaviorSanitizer (catching memory errors and undefined behavior):
```sh
make test SANITIZE=1
```
A list of commands for further detail:
|Command               |Purpose                             |
|----------------------|------------------------------------|
|`make`                |Build `./parking_lot`               |
|`make test`           |Build and run all tests             |
|`make test SANITIZE=1`|Run all tests sanitized             |
|`make SANITIZE=1`     |Build a sanitized version           |
|`make clean`          |Remove build folders and executables|

As the map is assumed to be valid, there is no error handling for that. For other invalid inputs, the following table shows a few situations:
| Situation | Output |
|---|---|
| A command used before `create_parking_lot` | `Parking lot not created` |
| Unknown command, or the wrong number of arguments | `Invalid command` |
| Non-numeric numbers, or non-positive `create_parking_lot` sizes | `Invalid command` |
| `leave` on a cell outside the lot, or not a usable slot | `Invalid slot` |
| `leave` on an empty slot | `Slot (r, c) is already free` |
| `park` with a registration that is already parked | `Car <reg> is already parked` |
| A second `create_parking_lot` | Replaces the current lot |

## My implementation of the solution

### Structure

```
input.txt ──lines──▶ main.c ──▶ command_processor ──▶ parking_lot
                                (parse + print)        (logic, no I/O)
                                       all built on: utils, int_vec, min_heap, str_map
```

| File | Responsibility |
|---|---|
| `src/main.c` | Opens the input file, reads it line by line with `getline`, buffers output |
| `src/command_processor.c` | Parses each line, dispatches it through a command table, formats the output |
| `src/parking_lot.c` | The grid, slot distances, parking and leaving, queries. Returns data and never prints |
| `src/min_heap.c` | Fixed-capacity min-heap of ints (free slots) |
| `src/str_map.c` | String → int hash map (registrations, colours) |
| `src/int_vec.c` | Growable int array |
| `src/utils.c` | Allocation wrappers and string helpers (trim, split, strict integer parsing) |

C has no standard containers, so the heap, hash map and dynamic array are small modules of their own. `ParkingLot` and `CommandProcessor` are opaque structs: their fields are visible only inside their own `.c` file, and other code uses their functions. This is how C keeps a module's data private.

### Algorithm

A slot's distance never changes once the lot is built, so all the path-finding happens once, in `create_parking_lot`:

1. **BFS from the entrance** over road cells (`.` and `E`). This gives each reachable road cell its step count.
2. **Slot distance** = 1 + the smallest step count among the slot's road or entrance neighbours. A slot with no reachable neighbour is not usable and is not counted.
3. **Rank the slots**: sort the usable slots once by (distance, row, column). A slot's position in that order is its *rank*, and a lower rank is always the better slot.
4. **Put every rank in a min-heap** of free slots.

After that, the nearest free slot is just the smallest free rank:

- `park` takes the top of the heap.
- `leave` puts the rank back on the heap.

The tie-break rule (smaller row, then smaller column) is part of the rank, so it never has to be checked again.

### Data structures

| Need | Structure | Cost |
|---|---|---|
| Nearest free slot | min-heap of ranks | O(log s) per `park` / `leave` |
| (row, col) → slot, and validating `leave` | array over all cells (`-1` = not a usable slot) | O(1) |
| Registration → slot, and the duplicate check | hash map | O(1) average |
| Colour → parked cars | hash map from lowercase colour to a list of slots | O(1) average to find the list |
| All parked cars | list of occupied slots | O(1) add / remove |

The occupied list and the per-colour lists are kept **unsorted** so that removal is O(1): the last element moves into the gap, and the slot that moved has its stored position updated. `status` and the colour queries sort a copy by row-major cell index, which gives row-then-column order. A query that returns k cars prints k lines anyway, so the O(k log k) sort adds little.

The hash map uses linear probing with backward-shift deletion instead of tombstones. That keeps lookups fast across many `park` / `leave` cycles.

### Complexity

With n = rows × cols, s = usable slots and k = cars in a result:

| Operation | Time |
|---|---|
| `create_parking_lot` | O(n + s log s) |
| `park`, `leave` | O(log s) |
| `slot_number_for_registration_number` | O(1) average |
| `status`, colour queries | O(k log k) |

Memory is O(n).

### Performance

On a 1000 × 1000 map with 499,500 slots and 100,000 mixed commands, the whole program runs in **0.87 s** (MacBook, `-O2`). Almost all of that time goes on printing 2.6 million output lines from the `status` commands. Building the lot itself takes about 0.02 s.

### Design decisions

- **Logic and I/O are separate.** `parking_lot` returns results (status codes and lists of cars), and only `command_processor` formats text. The logic can then be tested directly, and the output can be tested as exact strings.
- **Input is validated before it's used.** Integers are parsed strictly: `12a`, `1.5`, `+5` and values that overflow are rejected. The number of arguments is checked against the command table before any handler runs. A command used before a lot exists is rejected with a message.
- **Colours are matched case-insensitively and printed as they were entered.** Registrations are case-sensitive, because the problem only mentions case-insensitivity for colours.
- **Status columns always stay separated.** Columns are padded to the widths in the sample output, with at least one space, so long registrations or large coordinates never run into the next column.
- **If memory allocation fails, the program prints an error and exits.** This keeps NULL checks out of the rest of the code.

### Testing

`make test` runs about 5,000 checks, plus the end-to-end runs and command-line checks described above. The tests are:

- **Exact-output tests** for every command and every invalid-input case, plus the full example from the problem statement.
- **Allocation-order tests** for row and column ties, nearest-neighbour distances, unreachable slots and reuse of freed slots.
- **A randomised test** of 3,000 random lots, compared after every operation against a deliberately simple reference that shares no code with the real implementation.
- **A random-input test** that throws 2,000 sequences of random commands at the program to check it never crashes.

All tests also pass under AddressSanitizer and UndefinedBehaviorSanitizer (`make test SANITIZE=1`).

