/* getline() is POSIX, not ISO C, so ask for it explicitly. */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>

#include "command_processor.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *input = fopen(argv[1], "r");
    if (input == NULL) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }

    /* A large output buffer turns many small writes into few big ones, which
     * matters with 100,000 commands. */
    static char output_buffer[1 << 16];
    setvbuf(stdout, output_buffer, _IOFBF, sizeof output_buffer);

    CommandProcessor *processor = command_processor_create(stdout);

    /* getline grows the buffer as needed, so lines of any length are read whole. */
    char *line = NULL;
    size_t capacity = 0;
    while (getline(&line, &capacity, input) != -1) {
        command_processor_process_line(processor, line);
    }
    command_processor_finish(processor);

    free(line);
    command_processor_destroy(processor);
    fclose(input);
    return EXIT_SUCCESS;
}
