ARCH ?= arm

SRC_VERIFY := verify_ota.c
SRC_HANDLER := ota_handler.c
SRC_SERVICE := ota_service.c
SRC_POST := post_ota_support.c

TARGET_VERIFY := verify_ota
TARGET_HANDLER := ota_handler.o
TARGET_SERVICE := ota_service
TARGET_POST := post_ota_support

HEADERS := ota_handler.h

ifeq ($(ARCH),arm)
    CC := /opt/vtcs_toolchain/vienna/usr/bin/arm-buildroot-linux-uclibcgnueabihf-gcc
    OPENSSL_INC := -Iopenssl/include
    OPENSSL_LIB := -Lopenssl/lib/armeabi-v7a
else
    CC := gcc
    OPENSSL_INC :=
    OPENSSL_LIB :=
endif

CFLAGS := -Wall -O2 -std=c99 $(OPENSSL_INC)
LDFLAGS := $(OPENSSL_LIB) -lssl -lcrypto

all: $(TARGET_VERIFY) $(TARGET_SERVICE) $(TARGET_POST)

$(TARGET_VERIFY): $(SRC_VERIFY)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(TARGET_SERVICE): $(SRC_SERVICE) $(SRC_HANDLER) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SRC_SERVICE) $(SRC_HANDLER) $(LDFLAGS)

$(TARGET_POST): $(SRC_POST)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET_VERIFY) $(TARGET_SERVICE) $(TARGET_POST) *.o
