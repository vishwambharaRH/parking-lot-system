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
- macOS or Linux. On Windows, use WSL. The program uses POSIX `getline`.


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
|`make`                |Build `./parking-lot`               |
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

## System information
Tested on macOS (Apple Clang 21, GNU Make 3.81)
Written in standard C11 plus POSIX `getline`, so it should also build on Linux with gcc or clang; on Windows, use WSL.

## My implementation


