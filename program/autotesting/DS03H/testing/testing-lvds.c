/*
 * @file  testing-lvds.c
 * @brief MFI IC testing
 * @copyright (C) 2017 by Desay SV automotive
 * @code
 * Rev2.0  20160909  uidp3533  Porting from G5 platform
 * Rev2.1  20170213  uidp5021  Porting from DS03H platform
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
#define I2C_SLAVE_ADDR		0x2c
#define DEVICE_VERSION_REG	0x00
#define LVDS_ID0		0x35
#define LVDS_ID1		0x38
#define LVDS_ID2		0x49

char *i2c_lvds = "/dev/i2c-0";
static int fd_i2c = -1;

static int i2c_obtain(void)
{
	int ret;

	if (fd_i2c < 0)
		fd_i2c = open(i2c_lvds, O_RDWR);

	if (fd_i2c < 0) {
		printf("i2c_obtain: can not open %s\n", i2c_lvds);
		return -1;
	}

	ret = ioctl(fd_i2c, I2C_SLAVE_FORCE, I2C_SLAVE_ADDR);
	if (ret < 0)
		printf("i2c_obtain: set address error 0x%02x\nret = %d\n",
			I2C_SLAVE_ADDR, ret);

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
	char version[3];

	ret = i2c_obtain();
	if (ret < 0)
		goto end;

	/* read IC ID */
	/* SN65DSI83 ID is 0x35 0x38 0x49 0x53 0x44 0x20 0x20 0x20 0x01 */
	ret = i2c_read(DEVICE_VERSION_REG, version, 3);
	if (ret < 0) {
		printf("testing-lvds: read ID fail\n");
		printf("IC ID[0] = 0x%02x ID[1] = 0x%02x ID[2] = 0x%02x\n",
				version[0], version[1], version[2]);
		goto end;
	}

	if (version[0] == LVDS_ID0
		&& version[1] == LVDS_ID1
		&& version[2] == LVDS_ID2) {
		printf("testing-lvds:LVDS-SN65DSI83 ID match\n");
		printf("IC ID[0] = 0x%02x ID[1] = 0x%02x ID[2] = 0x%02x\n",
				version[0], version[1], version[2]);
		ret = 0;
	}

end:
	i2c_release();
	return ret;
}
