#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define BUFF_SIZE	1024		/* 1024B */
#define FILE_SIZE	(32*1024*1024)	/* 32M */

int main()
{
	int i;
	int fp;
	int write_num;
	char buff[BUFF_SIZE] = {'a'};
	unsigned long int count = 0;
	char *file = "/storage/emmc-test";

	while(1) {
		fp = open(file, O_RDWR | O_CREAT | O_TRUNC);
		if (fp < 0) {
			printf("open file:%s error\n", file);
			return -1;
		}

		lseek(fp, 0, SEEK_SET);	lseek(fp, 0, SEEK_SET);
		for (i=0; i< FILE_SIZE/BUFF_SIZE; i++) {
			write_num = write(fp, buff, BUFF_SIZE);
			if (write_num != BUFF_SIZE) {
				printf("write error: %d  write_num=%d\n", i, write_num);
				return -1;
			}
			usleep(1);

			if ((i%(16*1024)) == 0) {
				printf("Write %s Count=%d, size=%dKB\n", file, count, BUFF_SIZE*16);
				count++;
				fsync(fp);
				usleep(100);
			}
		}
		close(fp);
	}
	return 0;
}
