ARCH ?= arm
TARGET  := factory_reset_bin
SRC     := factory_reset.c
OBJDIR  := build
OBJ     := $(OBJDIR)/factory_reset.o

ifeq ($(ARCH), arm)
    CC  := /opt/vtcs_toolchain/vienna/usr/bin/arm-buildroot-linux-uclibcgnueabihf-gcc
else ifeq ($(ARCH), linux)
    CC  := gcc
endif

CFLAGS  := -O2 -Wall -Wextra -std=c99
LDFLAGS :=

all: $(TARGET)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	@echo "[LD] $(TARGET)"
	@$(CC) $(OBJ) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete -> $(TARGET)"

clean:
	@echo "Cleaning..."
	@rm -rf $(OBJDIR) $(TARGET)
	@echo "Done."

.PHONY: all clean
