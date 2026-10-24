CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
BUILD_DIR = build
SOURCE_DIR = MB
EXAMPLE_DIR = examples
INCLUDE_DIR = $(SOURCE_DIR)/include

ifeq ($(OS),Windows_NT)
APP_EXT = .exe
UI_LINK_OPTIONS = --link-options -luser32 --link-options -lgdi32
MAKE_BUILD_DIR = if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
else
APP_EXT =
UI_LINK_OPTIONS = --link-options -lX11
MAKE_BUILD_DIR = mkdir -p "$(BUILD_DIR)"
endif

COMPILER = $(BUILD_DIR)/mobdrive$(APP_EXT)
ELF_COMPILER = $(BUILD_DIR)/mobdrive-elf$(APP_EXT)
ELF_CC ?= $(if $(filter Windows_NT,$(OS)),x86_64-elf-gcc,$(CC))

SOURCES = $(SOURCE_DIR)/src/cli/main.c $(SOURCE_DIR)/src/frontend/lexer.c $(SOURCE_DIR)/src/frontend/parser.c \
          $(SOURCE_DIR)/src/project/project.c $(SOURCE_DIR)/src/sema/semantic.c $(SOURCE_DIR)/src/sema/typecheck.c \
          $(SOURCE_DIR)/src/codegen/codegen.c $(SOURCE_DIR)/src/driver/toolchain.c $(SOURCE_DIR)/src/compiler/compiler.c
HEADERS = $(wildcard $(INCLUDE_DIR)/mobdrive/*.h)
DEMOS = $(BUILD_DIR)/hello$(APP_EXT) $(BUILD_DIR)/imported$(APP_EXT) $(BUILD_DIR)/ffi$(APP_EXT) \
        $(BUILD_DIR)/stdlib_demo$(APP_EXT) $(BUILD_DIR)/concept_demo$(APP_EXT) $(BUILD_DIR)/gc_demo$(APP_EXT) \
        $(BUILD_DIR)/framework_demo$(APP_EXT) $(BUILD_DIR)/time_demo$(APP_EXT) $(BUILD_DIR)/ui_demo$(APP_EXT) \
        $(BUILD_DIR)/language_showcase$(APP_EXT)

all: $(COMPILER) $(ELF_COMPILER) $(DEMOS)

$(BUILD_DIR):
	$(MAKE_BUILD_DIR)

$(COMPILER): $(SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $(SOURCES) -o $@

$(ELF_COMPILER): $(SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $(SOURCES) -o $@

$(BUILD_DIR)/hello$(APP_EXT): $(COMPILER) $(EXAMPLE_DIR)/hello.mbd | $(BUILD_DIR)
	./$(COMPILER) build $(EXAMPLE_DIR)/hello.mbd -o $@

$(BUILD_DIR)/imported$(APP_EXT): $(COMPILER) $(EXAMPLE_DIR)/with_import.mbd $(EXAMPLE_DIR)/mode.mbd | $(BUILD_DIR)
	./$(COMPILER) build $(EXAMPLE_DIR)/with_import.mbd -o $@

$(BUILD_DIR)/ffi$(APP_EXT): $(COMPILER) $(EXAMPLE_DIR)/ffi.mbd $(EXAMPLE_DIR)/ffi.c | $(BUILD_DIR)
	./$(COMPILER) build $(EXAMPLE_DIR)/ffi.mbd -o $@ --link-options $(EXAMPLE_DIR)/ffi.c

$(BUILD_DIR)/stdlib_demo$(APP_EXT): $(COMPILER) $(EXAMPLE_DIR)/stdlib_demo.mbd $(SOURCE_DIR)/stdlib/io.mbd $(SOURCE_DIR)/stdlib/math.mbd $(SOURCE_DIR)/stdlib/runtime.c | $(BUILD_DIR)
	./$(COMPILER) build $(EXAMPLE_DIR)/stdlib_demo.mbd -o $@ --link-options $(SOURCE_DIR)/stdlib/runtime.c

$(BUILD_DIR)/concept_demo$(APP_EXT): $(COMPILER) $(EXAMPLE_DIR)/concept_demo.mbd $(EXAMPLE_DIR)/mode.mbd $(EXAMPLE_DIR)/concept_runtime.c | $(BUILD_DIR)
	./$(COMPILER) build $(EXAMPLE_DIR)/concept_demo.mbd -o $@ --link-options $(EXAMPLE_DIR)/concept_runtime.c

$(BUILD_DIR)/gc_demo$(APP_EXT): $(COMPILER) $(EXAMPLE_DIR)/gc_demo.mbd $(SOURCE_DIR)/stdlib/gc.mbd $(SOURCE_DIR)/stdlib/gc_runtime.c | $(BUILD_DIR)
	./$(COMPILER) build $(EXAMPLE_DIR)/gc_demo.mbd -o $@ --link-options $(SOURCE_DIR)/stdlib/gc_runtime.c

$(BUILD_DIR)/framework_demo$(APP_EXT): $(COMPILER) $(EXAMPLE_DIR)/framework_demo.mbd $(SOURCE_DIR)/stdlib/gc.mbd $(SOURCE_DIR)/stdlib/gc_runtime.c $(SOURCE_DIR)/stdlib/ui.mbd $(SOURCE_DIR)/stdlib/ui_runtime.c $(EXAMPLE_DIR)/framework_runtime.c | $(BUILD_DIR)
	./$(COMPILER) build $(EXAMPLE_DIR)/framework_demo.mbd -o $@ --link-options $(SOURCE_DIR)/stdlib/gc_runtime.c --link-options $(SOURCE_DIR)/stdlib/ui_runtime.c --link-options $(EXAMPLE_DIR)/framework_runtime.c $(UI_LINK_OPTIONS)

$(BUILD_DIR)/time_demo$(APP_EXT): $(COMPILER) $(EXAMPLE_DIR)/time_demo.mbd $(SOURCE_DIR)/stdlib/time.mbd $(SOURCE_DIR)/stdlib/time_runtime.c | $(BUILD_DIR)
	./$(COMPILER) build $(EXAMPLE_DIR)/time_demo.mbd -o $@ --link-options $(SOURCE_DIR)/stdlib/time_runtime.c

$(BUILD_DIR)/ui_demo$(APP_EXT): $(COMPILER) $(EXAMPLE_DIR)/ui_demo.mbd $(SOURCE_DIR)/stdlib/ui.mbd $(SOURCE_DIR)/stdlib/ui_runtime.c | $(BUILD_DIR)
	./$(COMPILER) build $(EXAMPLE_DIR)/ui_demo.mbd -o $@ --link-options $(SOURCE_DIR)/stdlib/ui_runtime.c $(UI_LINK_OPTIONS)

$(BUILD_DIR)/language_showcase$(APP_EXT): $(COMPILER) $(EXAMPLE_DIR)/language_showcase.mbd $(SOURCE_DIR)/stdlib/io.mbd $(SOURCE_DIR)/stdlib/runtime.c $(SOURCE_DIR)/stdlib/math.mbd $(SOURCE_DIR)/stdlib/gc.mbd $(SOURCE_DIR)/stdlib/gc_runtime.c $(SOURCE_DIR)/stdlib/time.mbd $(SOURCE_DIR)/stdlib/time_runtime.c $(SOURCE_DIR)/stdlib/ui.mbd $(SOURCE_DIR)/stdlib/ui_runtime.c | $(BUILD_DIR)
	./$(COMPILER) build $(EXAMPLE_DIR)/language_showcase.mbd -o $@ --link-options $(SOURCE_DIR)/stdlib/runtime.c --link-options $(SOURCE_DIR)/stdlib/gc_runtime.c --link-options $(SOURCE_DIR)/stdlib/time_runtime.c --link-options $(SOURCE_DIR)/stdlib/ui_runtime.c $(UI_LINK_OPTIONS)

kernel-elf: $(ELF_COMPILER) $(EXAMPLE_DIR)/kernel_minimal.mbd
	./$(ELF_COMPILER) build $(EXAMPLE_DIR)/kernel_minimal.mbd -o $(BUILD_DIR)/kernel_minimal.elf --cc $(ELF_CC)

clean:
	$(RM) $(BUILD_DIR)/*

.PHONY: all clean kernel-elf
