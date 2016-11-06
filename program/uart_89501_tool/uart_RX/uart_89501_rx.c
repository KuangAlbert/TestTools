#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/termios.h>
#include <string.h>
#include "serial.h"

#define DEV_NAME  "/dev/ttyS7"

/* uart8 */
int main(int argc, char *argv[])
{
	int fd;
	int len, i, ret;
	char buf[50];
	char temp_char[200] = {0};

	fd = open(DEV_NAME, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror(DEV_NAME);
		return -1;
	}

	ret = set_port_attr(
				fd,
				B9600,/* B1200 B2400 B4800 B9600 .. B115200 */
				8,	/* 5, 6, 7, 8 */
				"1",	/* "1", "1.5", "2" */
				'N',    /* N(o), O(dd), E(ven) */
				150,	/* VTIME */
				255);	/* VTIME */
	if (ret < 0) {
		printf("set uart arrt faile\n");
		exit(-1);
	}

	while (1) {
		len = read(fd, buf, 50);
		if (len < 0) {
			printf("read error\n");
			return -1;
		}
		if (len != 1) {
			for (i = 0; i < len ; i++)
				printf("%c", buf[i]);
		}
	}
	return 0;
}

