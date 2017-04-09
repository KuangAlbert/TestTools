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
	for(i=0; i< len; i++)
	printf("0x%02X ", data[i]);
	printf("]\n");
}

int getnum(char *s1[], char *s2, int argc)
{
	int i;

	for (i = 0; i < argc; i++) {
//		printf("s1:%s s2:%s\n",s1[i],s2);
		if(strcmp(s1[i],s2) == 0)
			return i;
	}
	return -1;
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
	int fd = open(Dev,O_RDWR);
	if(-1 == fd)
	{
		perror("Can't Open SPI Port");
		return -1;
	}
	else
		return fd;
}

int write_test(int fd,char *input, int len)
{
	int i;
	printf("fd= %d\n",fd);
	printf("input= \n");
	for (i = 0; i < len; i++)
		printf("%d ",input[i]);
	printf("\nlen= %d\n",len);
}

/* test example:
*  ./spi_test -l 10 -d FF 5A A5 03 04 05 06 07 08 09  -t 1
*/
int main(int argc, char *argv[])
{
	int i, t,tmp,len;
	int oflags;
	int ms=0;
	int num=0;
	unsigned char input[256];

/* while(1); */
{
	fd = OpenDev("/dev/mcu_spi");

	signal(SIGIO, input_handler);
	fcntl(fd, F_SETOWN, getpid());
	oflags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, oflags | FASYNC);

	if ((num = getnum(argv,"-l",argc)) != -1) {
		len = atoi(argv[num+1]);
//		printf("len = %d\n",len);
	}

	if((num = getnum(argv,"-s",argc)) != -1)
		ms = atoi(argv[num+1]);
	else
		ms = 0;
/* 04 54 01 02 00, */
	input[0] = 0x04;			/* cmd type */
	input[1] = 0x54;			/* cmd id hi */
	input[2] = 0x01;			/* cmd id low */
	input[3] = (unsigned char)(len) + 1;	/* cmd data size */
	input[4] = 0;				/* cmd flag */

	if ((num = getnum(argv,"-f",argc)) != -1) {
		input[3] = 1;
		tmp = atoi(argv[num+1]);
//		printf("time= %d\n",tmp);
		for(i=0; i < tmp; i++) {
			input[5] = (unsigned char)(i);
			write(fd,input,6);
			usleep(ms*1000);
		}
		close(fd);
		return 1;
	}

	if((num = getnum(argv,"-d",argc)) != -1)
		if(strcmp(argv[num+1],"-0") != 0) {
			for(i=0; i < len; i++) {
				tmp = htoi(argv[i+4]);
				input[5+i] = (unsigned char)(tmp);
			}
		}
		else {
			if ((num = getnum(argv,"-r",argc)) != -1) {
				for(i=0; i < len; i++) {
					input[5+i] = (unsigned char)(255-i);
				}
			}
			else {
				for(i=0; i < len; i++) {
					input[5+i] = (unsigned char)(i);
				}
			}
		}

	if((num = getnum(argv,"-t",argc)) != -1)
		t = atoi(argv[num+1]);

//	printf("need to send %d times each %d ms\n",t,ms);
	for(i = 0; i < t; i++) {
		write(fd,input,(5+len));
		usleep(ms*1000);
	}
//	while(1);
	close(fd);
}//while(1)
	return 0;
}
