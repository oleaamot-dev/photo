#include <stdio.h>
#include <stdarg.h>

void
print_warning(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "WARNING: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}


void
print_error(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

