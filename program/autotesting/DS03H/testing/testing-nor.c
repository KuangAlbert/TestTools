#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <string.h>

#define BLOCK_SIZE	(64*1024)
#define SIZE		(64*1024)
#define MTD_FILE_FLAG	"/dev/mtd2"

int tim_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
	int nsec;
	if (x->tv_sec > y->tv_sec)
		return   -1;
	if ((x->tv_sec == y->tv_sec) && (x->tv_usec > y->tv_usec))
		return   -1;
	result->tv_sec = (y->tv_sec-x->tv_sec);
	result->tv_usec = (y->tv_usec-x->tv_usec);
	if (result->tv_usec < 0) {
		result->tv_sec--;
		result->tv_usec += 1000000;
	}
	return	0;
}

int main(int argc, char **argv)
{
	int i;
	int fd;
	int ret = -1;
	int count = (SIZE)/(BLOCK_SIZE);
	char *buff;
	struct timeval start_read, stop_read, diff_read;
	struct mtd_info_user mtdInfo;
	struct erase_info_user erase;
	char mtd_test_string[] = "test norflash read and write";
	char test_result[50] = {0};

	fd = open(MTD_FILE_FLAG, O_RDWR);

	if (fd < 0) {
		printf("    FILE: %s open error\n", MTD_FILE_FLAG);
		return -1;
	}
	buff = (char *)malloc(BLOCK_SIZE);
	gettimeofday(&start_read, 0);

	/* read 64k */
	for (i = 0; i < count; i++) {
		lseek(fd, 0, SEEK_SET);
		ret = read(fd, buff, BLOCK_SIZE);
		if (ret != BLOCK_SIZE)
			return -1;
	}
	gettimeofday(&stop_read, 0);
	tim_subtract(&diff_read, &start_read, &stop_read);

	/* read mtd info */
	memset(&mtdInfo, 0, sizeof(struct mtd_info_user));

	ret = ioctl(fd, MEMGETINFO, &mtdInfo);
	if (ret < 0) {
		printf("    read norflash info fail\n");
		return -1;
	}

	/* erase 4k */
	erase.start = 0;
	erase.length = mtdInfo.erasesize;

	if (ioctl(fd, MEMERASE, &erase)) {
		printf("ioctl erase nor error");
		return -1;
	}

	printf("    mtdInfo: type=%d, flags=%d, size=%d, erasesize=%d\n",
		mtdInfo.type, mtdInfo.flags, mtdInfo.size, mtdInfo.erasesize);

	/* write string */
	lseek(fd, 0, SEEK_SET);
	ret = write(fd, mtd_test_string, sizeof(mtd_test_string));
	if (ret != sizeof(mtd_test_string)) {
		printf("    write norflash fail\n");
		return -1;
	}

	/* read string */
	lseek(fd, 0, SEEK_SET);
	ret = read(fd, test_result, sizeof(mtd_test_string));
	if (ret != sizeof(mtd_test_string)) {
		printf("    read norflash fail\n");
		return -1;
	}

	printf("    norflash read and write test\n");
	printf("    test result is [%s]\n    mtd_test_string is [%s]\n",
			test_result, mtd_test_string);

	/* compare string */
	if (0 == strcmp(mtd_test_string, test_result)) {
		printf("    norflash read and write sucess\n");
		ret = 0;
	}


	free(buff);
	close(fd);
	printf("    NOR read %dKB in %d.%d Second\n", SIZE/1024,
			diff_read.tv_sec, diff_read.tv_usec/1000);

	printf("    NOR read speed:%d.%dMB/s\n",
		(SIZE/1024)/(diff_read.tv_sec*1000 + diff_read.tv_usec/1000),
		(SIZE/1024)%(diff_read.tv_sec*1000 + diff_read.tv_usec/1000));

	return ret;
}
