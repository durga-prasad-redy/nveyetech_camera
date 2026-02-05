ARCH ?= arm

ifeq ($(ARCH),arm)
    CC := /opt/vtcs_toolchain/vienna/usr/bin/arm-buildroot-linux-uclibcgnueabihf-gcc
    CFLAGS := -Wall -Werror -std=c99 -Iopenssl/include
    LDFLAGS := -Lopenssl/lib/armeabi-v7a -lssl -lcrypto
else
    CC := gcc
    CFLAGS := -Wall -Werror -std=c99
    LDFLAGS := -lssl -lcrypto
endif

TARGET := generate_device_csr
SRC := generate_device_csr.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

