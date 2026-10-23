#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void mbd_io_print_i64(int64_t value) {
    printf("%" PRId64 "\n", value);
}

void mbd_io_print_f64(double value) {
    printf("%.15g\n", value);
}

void mbd_io_print_string(const char *value) {
    puts(value ? value : "(null)");
}

void mbd_io_print_bool(bool value) {
    puts(value ? "true" : "false");
}

int64_t mbd_io_string_length(const char *value) {
    return value ? (int64_t)strlen(value) : 0;
}
