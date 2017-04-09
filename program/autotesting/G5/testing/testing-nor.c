#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define BLOCK_SIZE	(64*1024)
#define SIZE		(64*1024)
#define MTD_FILE_FLAG	"/dev/mtd2"

int tim_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
	int nsec;
	if ( x->tv_sec > y->tv_sec )
		return   -1;
	if ((x->tv_sec==y->tv_sec) && (x->tv_usec>y->tv_usec))
		return   -1;
	result->tv_sec = ( y->tv_sec-x->tv_sec );
	result->tv_usec = ( y->tv_usec-x->tv_usec );
	if (result->tv_usec<0)
	{
		result->tv_sec--;
		result->tv_usec+=1000000;
	}
	return	0;
}

int main(int argc, char** argv)
{
	int i;
	int fd;
	int ret;
	int count = (SIZE)/(BLOCK_SIZE);
	char *buff;
	struct timeval start_read,stop_read,diff_read;

	if ((fd = open(MTD_FILE_FLAG, O_RDWR)) < 0) {
		printf("    FILE: %s open error\n", MTD_FILE_FLAG);
		return -1;
	}
	buff = (char *)malloc(BLOCK_SIZE);
	gettimeofday(&start_read,0);
	for (i=0; i<count; i++) {
		lseek(fd, 0, SEEK_SET);
		ret = read(fd, buff, BLOCK_SIZE);
		if (ret != BLOCK_SIZE)
			return -1;
	}
	gettimeofday(&stop_read,0);
	tim_subtract(&diff_read,&start_read,&stop_read);

	free(buff);
	close(fd);
	printf("    NOR read %dKB in %d.%d Second\n", SIZE/1024, diff_read.tv_sec, diff_read.tv_usec/1000);
	printf("    NOR read speed:%d.%dMB/s\n", (SIZE/1024)/(diff_read.tv_sec*1000 + diff_read.tv_usec/1000),(SIZE/1024)%(diff_read.tv_sec*1000 + diff_read.tv_usec/1000));

	return 0;
}
