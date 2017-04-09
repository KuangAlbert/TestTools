CROSS_COMPILE := arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc
AS := $(CROSS_COMPILE)as
LD := $(CROSS_COMPILE)ld

all: spi_test.c
	$(CC) spi_test.c -o spi_test

clean:
	rm -f spi_test

