# Usage:
# make #does stuff

# used directories
ODIR := ./obj
LDIR := ./libs
IDIR := ./include
SDIR := ./src
OUT_DIR := ./out
INSTALL_DIR := /bin

# libraries
#  -ljansson
LIBS = -I$(LDIR) -lssl -lcrypto -lncursesw -ljansson

# compiler
FLAGS = -fdata-sections -ffunction-sections -pthread -I$(IDIR) -march=native -O2
CC := gcc
CFLAGS = $(FLAGS)
GCC_FLAGS = -Wl,--gc-sections

# linker
LL = $(CXX)

#app name
OUT := notetakingprogram
TARGET = $(shell $(CXX) -dumpmachine)
OUT_FULL = $(OUT)_$(TARGET)

# dependencides ( header files )
DEPS = $(wildcard $(IDIR)/*.h)
# source files
SRCS = $(wildcard $(SDIR)/*.c)
# object files
OBJS = $(patsubst $(SDIR)%,$(ODIR)%.o,$(SRCS))

ifeq ($(CC),gcc)
    CFLAGS += $(GCC_FLAGS)
endif

# builds main executable
#   links all object files into one executable
#   creates a softlink to OUT_FULL called OUT
# $@: $^
$(OUT_DIR)/$(OUT_FULL): $(OBJS)
	@mkdir -p $(OUT_DIR)
	@echo linking executable
	@$(LL) -o $@ $^ $(CFLAGS) $(LIBS)
	@echo creating symbolic link
	@cd out && rm -f $(OUT) && ln -s $(OUT_FULL) $(OUT)

# builds all my .o files - universal but unoptimized and slower (probably)

#C files
$(ODIR)/%.c.o: $(SDIR)/%.c $(DEPS)
	@mkdir -p $(ODIR)
	@echo 'compiling $<'
	@$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean info test help install uninstall

install: $(OUT_DIR)/$(OUT_FULL)
	cp $(OUT_DIR)/$(OUT) $(INSTALL_DIR)/
	cp $(OUT_DIR)/$(OUT_FULL) $(INSTALL_DIR)/

uninstall:
	rm $(INSTALL_DIR)/$(OUT)
	rm $(INSTALL_DIR)/$(OUT_FULL)

clean:
	@echo removing .o files from $(ODIR)
	@rm -rf $(ODIR)/*

help:
	@echo "make - compile"
	@echo "make clean - clean all object files from $(ODIR)"
	@echo "make help"
	@echo "make info - list make settings"

info:
	@echo "Compiler (CC)     $(CC)"
	@echo "Flags (CFLAGS)    $(CFLAGS)"
	@echo "Source files      $(SRCS)"
	@echo "Dependency files  $(DEPS)"
	@echo "Object files      $(OBJS)"
	@echo "Output file       ./out/$(OUT_FULL)"
	@echo "Soft link file    ./out/$(OUT)"
	@echo "Install dir       $(INSTALL_DIR)"