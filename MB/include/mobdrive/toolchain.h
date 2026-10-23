#ifndef MOBDRIVE_TOOLCHAIN_H
#define MOBDRIVE_TOOLCHAIN_H
#include <stddef.h>
int toolchain_build_exe(const char *cc, const char *c_source, const char *output_exe,
                        char **link_options, size_t link_option_count);
int toolchain_build_elf(const char *cc, const char *c_source, const char *output_elf,
                        char **link_options, size_t link_option_count);
#endif
