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
    char sendbuf[16]={0};

	len = read(fd, data, 16);
	printf("App read:[");
	for(i=0; i< len; i++)
	printf("0x%02X ", data[i]);
	printf("]\n");
#if 0
    if(data[1] == 0x03 && data[2] == 0x01)
    {
        sendbuf[0] = 0x02;
        sendbuf[1] = 0x02;
        sendbuf[2] = 0x01;
        //sendbuff[3] = data[3];
        sendbuf[3] = 0x0A;
        sendbuf[4] = 0x00;
        for(i=5;i< 15;i++);
            sendbuf[i] = data[i];
        write(fd,sendbuf,15);
        printf("write sucessfull !\n");
    }
    else
    {
        printf("MCU Cmd ID is error ！n");

    }
#endif

}

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + 'a' - 'A';
    }
    else
    {
        return c;
    }
}

int htoi(char s[])
{
    int i;
    int n = 0;
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))
    {
        i = 2;
    }
    else
    {
        i = 0;
    }
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)
    {
        if (tolower(s[i]) > '9')
        {
            n = 16 * n + (10 + tolower(s[i]) - 'a');
        }
        else
        {
            n = 16 * n + (tolower(s[i]) - '0');
        }
    }
    return n;
}

int OpenDev(char *Dev)
{
	int fd = open(Dev,O_RDWR|O_NONBLOCK);
	if(-1 == fd)
	{
		perror("Can't Open SPI Port");
		return -1;
	}
	else
		return fd;
}

/* test example: ./spi_test 04 4C 03 04 00 3A 00 00 64 */
/* test example: ./spi_test 02 02 01 0A 00 00 01 02 03 04 05 06 07 08 09 */
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


   /*  for(i=0; i< (argc-1); i++) { */
		/* tmp = htoi(argv[i+1]); */
		/* input[i] = (unsigned char)(tmp); */
	/* } */
    /* for(i=0;i<100;i++) */
    /* { */
		/* write(fd,input,(argc-1)); */
		/* sleep(1); */
    /* } */
    input[0] = 0x02;
    input[1] = 0x02;
    input[2] = 0x01;
    input[3] = 0x04;
    input[4] = 0x00;
    input[5] = 0x01;
    input[6] = 0x01;
    input[7] = 0x01;
    input[8] = 0x00;
    while(1)
    {
        for(i=0;i < 10; i++)
        {
            input[8] = i;
            write(fd,input,9);
            sleep(1);
        }
    }

	close(fd);

	return 0;
}
