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
#define I2C_SLAVE_ADDR		0x21

char *i2c_decoder = "/dev/i2c-0";
static int fd_i2c = -1;
static unsigned char test_num[1] = {0x68};

static int i2c_obtain(void)
{
	int ret;

	if (fd_i2c < 0)
		fd_i2c = open(i2c_decoder, O_RDWR);

	if (fd_i2c < 0) {
		printf("i2c_obtain: can not open %s\n", i2c_decoder);
		return -1;
	}

	ret = ioctl(fd_i2c, I2C_SLAVE_FORCE, I2C_SLAVE_ADDR);
	if (ret < 0)
		printf("i2c_obtain: set address error 0x%02x\nret = %d\n",
			I2C_SLAVE_ADDR, ret);

	return ret;
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
	int i;
	ssize_t n;
	unsigned char test[5] = {0};
	struct i2c_rdwr_ioctl_data e2prom_data;
	char reg_addr = 0x05;
	char buf[2] = {0x05, 0x68};

	ret = i2c_obtain();
	if (ret < 0)
		goto end;

	e2prom_data.nmsgs = 2;
	e2prom_data .msgs = (struct i2c_msg *)malloc
		(e2prom_data.nmsgs * sizeof(struct i2c_msg));

	if (!e2prom_data.msgs) {
		ret = -1;
		printf("testing-decoder:Memory alloc error\n");
		goto end;
	}

	for (i = 0; i < I2C_MAX_RETRIES; i++) {
		n = write(fd_i2c, &index, 1);

		if (n == 1)
			break;
		usleep(5000);
	}

	if (i == I2C_MAX_RETRIES)
		return -1;


	/* write 0x68 to 0x05 */
	e2prom_data.nmsgs = 1;
	e2prom_data.msgs[0].len = 2;
	e2prom_data.msgs[0].addr = I2C_SLAVE_ADDR;
	e2prom_data.msgs[0].flags = 0;
	e2prom_data.msgs[0].buf = buf;

	for (i = 0; i < I2C_MAX_RETRIES; i++) {
		n = ioctl(fd_i2c, I2C_RDWR, (unsigned long)&e2prom_data);
		if (n >= 0)
			break;
		usleep(5000);
	}

	if (i == I2C_MAX_RETRIES) {
		ret = -1;
		printf("testing-decoder:ioctl write error\n");
		goto end;
	}

	e2prom_data.nmsgs = 2;
	e2prom_data.msgs[0].len = 1;
	e2prom_data.msgs[0].addr = I2C_SLAVE_ADDR;
	e2prom_data.msgs[0].flags = 0;
	e2prom_data.msgs[0].buf = &reg_addr;

	e2prom_data.msgs[1].len = 1;
	e2prom_data.msgs[1].addr = I2C_SLAVE_ADDR;
	e2prom_data.msgs[1].flags = I2C_M_RD;
	e2prom_data.msgs[1].buf = test;

	for (i = 0; i < I2C_MAX_RETRIES; i++) {
		n  = ioctl(fd_i2c, I2C_RDWR, (unsigned long)&e2prom_data);
		if (n >= 0)
			break;
		usleep(5000);
	}

	if (i == I2C_MAX_RETRIES) {
		ret = -1;
		printf("testing-decoder:ioctl read error\n");
		goto end;
	}

	if (test[0] == test_num[0]) {
		printf("testing-decoder:Decoder-ADV7180 match\n");
		printf(" test number is :  0x%02x\n", test[0]);
		ret = 0;
	}

end:
	i2c_release();
	return ret;
}
