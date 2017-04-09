/*
 * @file  testing-ipod.c
 * @brief MFI IC testing
 * @copyright (C) 2016 by Desay SV automotive
 * @code
 * Rev2.0  20160909  uidp3533  Porting from G5 platform
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define I2C_MAX_RETRIES		10	/* Try 10 times */
#define I2C_SLAVE_ADDR		0x11
#define DEVICE_VERSION_REG	0x00
#define DEVICE_VERSION		0x03

char *ipod_reset = "/sys/class/gpio/ipod_reset/direction";
char *i2c_ipod = "/dev/i2c-ipod";
static int fd_reset = -1;
static int fd_i2c = -1;

static int ipod_ic_reset(char *value)
{
	int ret;
	int count;

	if (fd_reset < 0)
		fd_reset = open(ipod_reset, O_WRONLY);

	if (fd_reset < 0) {
		printf("ipod_ic_reset: can not open %s\n", ipod_reset);
		return -1;
	}

	count = strlen(value);
	lseek(fd_reset, 0, SEEK_SET);
	ret = write(fd_reset, value, count);

	if (ret == count)
		return -1;

	return 0;
}

static int i2c_obtain(void)
{
	int ret;

	if (fd_i2c < 0)
		fd_i2c = open(i2c_ipod, O_RDWR);

	if (fd_i2c < 0) {
		printf("i2c_obtain: can not open %s\n", i2c_ipod);
		return -1;
	}

	ret = ioctl(fd_i2c, I2C_SLAVE, I2C_SLAVE_ADDR);
	if (ret < 0)
		printf("i2c_obtain: set address error 0x%02x\n",
			I2C_SLAVE_ADDR);

	return ret;
}

static int i2c_read(char index, char *buf, int len)
{
	int i;
	ssize_t n;

	printf("i2c_read: reg[0x%02x]  buf[%p] len[%d]\n", index, buf, len);

	for (i = 0; i < I2C_MAX_RETRIES; i++) {
		n = write(fd_i2c, &index, 1);

		if (n == 1)
			break;
		usleep(5000);
	}

	if (i == I2C_MAX_RETRIES)
		return -1;

	for (i = 0; i < I2C_MAX_RETRIES; i++) {
		n = read(fd_i2c, buf, len);
		if (n == len)
			break;
		usleep(5000);
	}

	return i == I2C_MAX_RETRIES ? -1 : 0;
}

static int i2c_write(char index, char *data, int len)
{
	int i;
	ssize_t n;
	char *buffer;

	printf("i2c_write: reg[0x%02x]  buf[%p] len[%d]\n", index, data, len);

	buffer = (char *)malloc(len + 1);
	if (!buffer) {
		printf("malloc: no mem\n");
		return -1;
	}

	buffer[0] = index;
	memcpy(buffer + 1, data, len);

	for (i = 0; i < I2C_MAX_RETRIES; i++) {
		n = write(fd_i2c, buffer, len + 1);

		if (n == len + 1)
			break;

		if (n > 0)
			printf("i2c_write: %ld/%d written\n", n, len + 1);

		usleep(5000);
	}

	if (buffer)
		free(buffer);

	return i == I2C_MAX_RETRIES ? -1 : 0;
}

static int i2c_release(void)
{
	if (fd_i2c) {
		close(fd_i2c);
		fd_i2c = -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret;
	char version;

	/* ipod IC reset */
	ipod_ic_reset("low");
	usleep(300000);
	ipod_ic_reset("high");
	/* on MFI IC data sheet, we know that reset I/O pin
	 * must remain the same for at least 30ms.
	 * but as far as i know
	 * the first time we reset MFI IC, it need more than 100ms reset time.
	 *
	 * The first time runing testing-ipod, trace:
	 * [   36.890000] NAV2IPOD_RESET value = 0
	 * [   37.190000] NAV2IPOD_RESET value = 1
	 * i2c_read fail 0 times
	 * i2c_read fail 1 times
	 * i2c_read fail 2 times
	 * i2c_read fail 3 times
	 * i2c_read fail 4 times
	 * i2c_read fail 5 times
	 * i2c_read fail 6 times
	 * i2c_read fail 7 times
	 * i2c_read fail 8 times
	 * i2c_read fail 9 times
	 * i2c_read fail 10 times
	 * i2c_read fail 11 times
	 * i2c_read fail 12 times
	 * i2c_read fail 13 times
	 * i2c_read fail 14 times
	 * i2c_read fail 15 times
	 * i2c_read Success
	 * [   37.290000] NAV2IPOD_RESET value = 0
	 * testing-ipod: IC version match
	 * IC version 0x03
	 *
	 * After first time, runing testing-ipod, trace:
	 * [  118.230000] NAV2IPOD_RESET value = 0
	 * [  118.540000] NAV2IPOD_RESET value = 1
	 * i2c_read fail 0 times
	 * i2c_read fail 1 times
	 * i2c_read fail 2 times
	 * i2c_read fail 3 times
	 * i2c_read Success
	 * [  118.570000] NAV2IPOD_RESET value = 0
	 * testing-ipod: IC version match
	 * IC version 0x03
	 */
	usleep(100000); /* 100ms for MFI reset */

	ret = i2c_obtain();
	if (ret < 0)
		goto end;

	/* read IC version */
	ret = i2c_read(DEVICE_VERSION_REG, &version, 1);
	if (ret < 0) {
		printf("testing-ipod: read version fail\n");
		printf("IC version 0x%02x\n", version);
		goto end;
	}

	if (version == DEVICE_VERSION) {
		printf("testing-ipod: IC version match\n");
		printf("IC version 0x%02x\n", version);
		ret = 0;
	}

end:
	i2c_release();
	ipod_ic_reset("low");
	return ret;
}
