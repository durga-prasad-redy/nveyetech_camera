# Makefile for cross-compiling board setup tools with OpenSSL

# Set architecture from command line (default is native)
ARCH ?= arm

# Toolchain compiler and paths
ifeq ($(ARCH),arm)
    CC := /opt/vtcs_toolchain/vienna/usr/bin/arm-buildroot-linux-uclibcgnueabihf-gcc
    OPENSSL_INC := -I$(CURDIR)/openssl/include
    OPENSSL_LIB := -L$(CURDIR)/openssl/lib/armeabi-v7a
else
    CC := gcc
    OPENSSL_INC :=
    OPENSSL_LIB :=
endif

# Compiler flags (added -std=c99 for C99 compatibility)
CFLAGS := -Wall -O2 -std=c99 $(OPENSSL_INC)
LDFLAGS_generate_keys := $(OPENSSL_LIB) -lcrypto

# Output binaries
BINARIES := device_setup

# Default target
all: $(BINARIES)

# Build set_mac_uboot
device_setup: device_setup.c
	$(CC) $(CFLAGS) -o $@ $<

# Build generate_keys with OpenSSL
generate_keys: generate_keys.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS_generate_keys)

# Clean up
clean:
	rm -f $(BINARIES)

.PHONY: all clean

