#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stddef.h>

/* Allocation wrappers: on failure they print "Out of memory" to stderr and exit,
 * so callers never need to check for NULL. */
void *xmalloc(size_t size);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);

/* Removes leading and trailing whitespace (including '\r' from Windows line
 * endings) in place. Returns a pointer to the first non-space character of s. */
char *str_trim(char *s);

/* Splits s in place on runs of spaces/tabs, writing '\0' after each token.
 * Stores at most max_tokens pointers in tokens, but returns the total number of
 * tokens found, so the caller can tell when there were too many. */
size_t str_split(char *s, char **tokens, size_t max_tokens);

/* ASCII-lowercases s in place, used for case-insensitive colour matching. */
void str_to_lower(char *s);

/* Parses the whole string as a base-10 int into *out. Returns false (and leaves
 * *out alone) for empty input, trailing characters ("12a"), or overflow. */
bool parse_int(const char *s, int *out);

#endif /* UTILS_H */
