#include "utils.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void out_of_memory(void) {
    fputs("Out of memory\n", stderr);
    exit(EXIT_FAILURE);
}

void *xmalloc(size_t size) {
    /* malloc(0) may return NULL legitimately; ask for 1 byte so NULL always means failure. */
    void *ptr = malloc(size > 0 ? size : 1);
    if (ptr == NULL) {
        out_of_memory();
    }
    return ptr;
}

void *xcalloc(size_t count, size_t size) {
    void *ptr = calloc(count > 0 ? count : 1, size > 0 ? size : 1);
    if (ptr == NULL) {
        out_of_memory();
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    void *resized = realloc(ptr, size > 0 ? size : 1);
    if (resized == NULL) {
        out_of_memory();
    }
    return resized;
}

char *xstrdup(const char *s) {
    size_t length = strlen(s) + 1;
    char *copy = xmalloc(length);
    memcpy(copy, s, length);
    return copy;
}

static bool is_space(char c) {
    return isspace((unsigned char)c) != 0;
}

char *str_trim(char *s) {
    while (is_space(*s)) {
        s++;
    }
    char *end = s + strlen(s);
    while (end > s && is_space(end[-1])) {
        end--;
    }
    *end = '\0';
    return s;
}

size_t str_split(char *s, char **tokens, size_t max_tokens) {
    size_t count = 0;
    char *p = s;
    while (*p != '\0') {
        while (is_space(*p)) {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        if (count < max_tokens) {
            tokens[count] = p;
        }
        count++;
        while (*p != '\0' && !is_space(*p)) {
            p++;
        }
        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }
    return count;
}

void str_to_lower(char *s) {
    for (; *s != '\0'; s++) {
        *s = (char)tolower((unsigned char)*s);
    }
}

bool parse_int(const char *s, int *out) {
    /* strtol would quietly accept leading spaces and '+', so check the first digit ourselves. */
    const char *digits = (*s == '-') ? s + 1 : s;
    if (!isdigit((unsigned char)*digits)) {
        return false;
    }

    errno = 0;
    char *end = NULL;
    long value = strtol(s, &end, 10);
    if (errno == ERANGE || *end != '\0' || value < INT_MIN || value > INT_MAX) {
        return false;
    }
    *out = (int)value;
    return true;
}
