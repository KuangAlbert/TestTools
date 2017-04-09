/*
 * Copyright (C) 2012 Huizhou Desay SV Automotive Co., Ltd
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <memory.h>

#define BUFSIZE (1024*1024)
int main(int argc, char **argv)
{
	int i = 0;
	int times = 0;
	int checksum_src = 0;
	int checksum_des = 0;
	char ch = 0x5A;
	char *bufsrc;
	char *bufdes;

	printf("    DDR copy test start\n");
	for (times = 0; times < 20; times++) {
		bufsrc = (char *)malloc(BUFSIZE);
		bufdes = (char *)malloc(BUFSIZE);

		checksum_src = 0;
		checksum_des = 0;

		for (i = 0; i < BUFSIZE-1; i++) {
			bufsrc[i] = ch;
			ch++;
			checksum_src = bufsrc[i] ^ checksum_src;
		}

		bufsrc[BUFSIZE-1] = checksum_src;

		memset(bufdes, BUFSIZE-1, 0xA5);
		memcpy(bufdes, bufsrc, BUFSIZE-1);

		for (i = 0; i < BUFSIZE-1; i++)
			checksum_des = bufdes[i] ^ checksum_des;

		if (checksum_des != checksum_src) {
			printf("    DDR CheckSum ERROR ... %d\n", checksum_des);
			return -1;
		}

		free(bufsrc);
		free(bufdes);
	}
	printf("    DDR test end\n");

	return 0;
}

