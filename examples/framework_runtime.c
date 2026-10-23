#include <stdint.h>

int32_t system__init_display_hardware(void) {
    // Host-side stand-in. A real kernel build must supply the target driver.
    return 0;
}

void system__log_kernel_error(void) {
}
