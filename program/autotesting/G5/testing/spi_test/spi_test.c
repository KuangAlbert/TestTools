#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>

int fd;
void input_handler(int signum)
{
	char data[16];
	int i,len;
	len = read(fd, data, 16);
	printf("App read:[");
	for (i=0; i< len; i++)
		printf("0x%02X ", data[i]);
	printf("]\n");
}

int tolower(int c)
{
	if (c >= 'A' && c <= 'Z') {
		return c + 'a' - 'A';
	} else {
		return c;
	}
}

int htoi(char s[])
{
	int i;
	int n = 0;
	if (s[0] == '0' && (s[1]=='x' || s[1]=='X')) {
		i = 2;
	} else {
		i = 0;
	}
	for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z'); ++i) {
		if (tolower(s[i]) > '9') {
			n = 16 * n + (10 + tolower(s[i]) - 'a');
		} else {
			n = 16 * n + (tolower(s[i]) - '0');
		}
	}
	return n;
}

int OpenDev(char *Dev)
{
	int fd = open(Dev,O_RDWR);
	if (-1 == fd) {
		perror("Can't Open SPI Port");
		return -1;
	} else
		return fd;
}

/* test example: ./spi_test 04 4C 03 04 00 3A 00 00 64 */
int main(int argc, char** argv)
{
	int i, tmp;
	int oflags;
	unsigned char input[32];

	fd = OpenDev("/dev/mcu_spi");

	signal(SIGIO, input_handler);
	fcntl(fd, F_SETOWN, getpid());
	oflags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, oflags | FASYNC);

	for (i=0; i< (argc-1); i++) {
		tmp = htoi(argv[i+1]);
		input[i] = (unsigned char)(tmp);
	}

	write(fd,input,(argc-1));

	sleep(1);

	close(fd);

	return 0;
}
