CROSS_COMPILE := arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc
AS := $(CROSS_COMPILE)as
LD := $(CROSS_COMPILE)ld

all: mmc_speed_test.c mmc_write_test.c sdcard_testing.c
	$(CC) mmc_speed_test.c -o mmc_speed_test
	$(CC) mmc_write_test.c -o mmc_write_test
	$(CC) sdcard_testing.c -o sdcard_testing
clean:
	rm -f mmc_speed_test
	rm -f mmc_write_test
	rm -f sdcard_testing
