#ifndef MOBDRIVE_COMPILER_H
#define MOBDRIVE_COMPILER_H
#include <stddef.h>
int compiler_check(const char *source);
int compiler_build(const char *source, const char *output_exe, const char *cc,
                   char **link_options, size_t link_count);
int compiler_build_elf(const char *source, const char *output_elf, const char *cc,
                       char **link_options, size_t link_count);
#endif
