# Makefile for ota_packager (hardened + stripped)

CC = gcc
CFLAGS = -Wall -Wextra -O2 -s -fstack-protector-strong -D_FORTIFY_SOURCE=2
LDFLAGS = -lssl -lcrypto -Wl,-z,relro,-z,now

SRC = ota_packager.c
OUT = ota_packager

.PHONY: all clean

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)

clean:
	rm -f $(OUT)

