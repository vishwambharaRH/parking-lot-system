#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <stdio.h>

/* Reads input one line at a time, calls the parking lot and writes each command's
 * output to a FILE*. It keeps state between lines because create_parking_lot
 * takes the next <rows> lines as the map. */
typedef struct CommandProcessor CommandProcessor;

/* out is where results are written: stdout in main, a temporary file in tests. */
CommandProcessor *command_processor_create(FILE *out);

void command_processor_destroy(CommandProcessor *processor);

/* Handles one input line, without its trailing newline. The line may be modified
 * (it is split in place). */
void command_processor_process_line(CommandProcessor *processor, char *line);

/* Call once after the last line. If the input ended partway through a map, the
 * lot is built from the lines received so far (missing rows become walls). */
void command_processor_finish(CommandProcessor *processor);

#endif /* COMMAND_PROCESSOR_H */
