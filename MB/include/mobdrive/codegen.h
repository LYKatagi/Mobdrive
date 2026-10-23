#ifndef MOBDRIVE_CODEGEN_H
#define MOBDRIVE_CODEGEN_H
#include "project.h"
int codegen_emit_c(const Project *project, const char *output_path);
int codegen_emit_elf_c(const Project *project, const char *output_path);
#endif
