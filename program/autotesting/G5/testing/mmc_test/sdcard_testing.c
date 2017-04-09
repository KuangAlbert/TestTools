#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

static inline void __list_add(struct list_head *new,
				struct list_head *prev,
				struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static LIST_HEAD(time_list);

struct run_time {
	struct list_head list;
	struct timeval time;
};

int set_timeval_to_list(struct timeval *t)
{
	struct run_time *p;
	p = (struct run_time *)malloc(sizeof(struct run_time));

	p->time.tv_sec = t->tv_sec;
	p->time.tv_usec = t->tv_usec;
	list_add(&p->list, &time_list);
	return 0;
}

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

int get_timeval_result(struct timeval *max, struct timeval *min, struct timeval *avg)
{
	int count = 0;
	struct run_time *p;

	if (max==NULL || min==NULL || avg==NULL)
		return -1;

	avg->tv_sec = 0;
	avg->tv_usec = 0;

	list_for_each_entry(p, &time_list, list) {
		/* printf("  Time: %ld.%03ld\n", p->time.tv_sec, (p->time.tv_usec%(1000*1000))/1000); */

		if (count == 0) {
			max->tv_sec = min->tv_sec = p->time.tv_sec;
			max->tv_usec = min->tv_usec = p->time.tv_usec;
		}

		avg->tv_sec += p->time.tv_sec;
		avg->tv_usec += p->time.tv_usec;

		if (max->tv_sec <= p->time.tv_sec ) {
			max->tv_sec = p->time.tv_sec;
			if (max->tv_usec < p->time.tv_usec)
				max->tv_usec = p->time.tv_usec;
		}

		if (min->tv_sec >= p->time.tv_sec ) {
			min->tv_sec = p->time.tv_sec;
			if (min->tv_usec > p->time.tv_usec)
				min->tv_usec = p->time.tv_usec;
		}
		count++;
	}
	avg->tv_sec = avg->tv_sec/count;
	avg->tv_usec = avg->tv_usec/count;

	return 0;
}

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

int clear_cache(int delay)
{
	sync();
	system("echo 3 > /proc/sys/vm/drop_caches");
	usleep(100000);
	system("echo 3 > /proc/sys/vm/drop_caches");
	usleep(delay);
	return 0;
}

int byte_verify(unsigned char *src, unsigned char *dst, int size)
{
	int i;
	int ret = 0;

	for (i=0; i<size; i++) {
		if (src[i] != dst[i]) {
			ret = -1;
			printf("0x%08X: 0x%02X 0x%02X\n", i, src[i], dst[i]);
		}
	}

	return ret;
}

int read_file(int fd, int count, int bs)
{
	int i;
	int ret;
	char *buff;

	buff = (char *)malloc(bs);
	if (buff == NULL) {
		printf("no memory\n");
		return -1;
	}

	for (i=0; i<count; i++) {
		ret = read(fd, buff, bs);
		if (ret != bs) {
			printf("read fail\n");
				perror("");
				return -1;
		}
	}

	free(buff);
	return 0;
}

int write_file(int fd, int count, int bs)
{
	int i;
	int ret;
	char *buff;

	buff = (char *)malloc(bs);
	if (buff == NULL) {
		printf("no memory\n");
		return -1;
	}

	for (i=0; i< count; i++) {
		ret = write(fd, buff, bs);
		if (ret != bs) {
			printf("write fail\n");
			perror("");
			return -1;
		}
	}
	fsync(fd);

	free(buff);
	return 0;
}

int file_verify(int fd, int count, int bs, char magic)
{
	int i;
	int ret;
	char *buff_src;
	char *buff_dst;
	off_t cur_offset;

	buff_src = (char *)malloc(bs);
	buff_dst = (char *)malloc(bs);
	if (buff_src == NULL || buff_dst == NULL) {
		printf("no memory\n");
		return -1;
	}
	memset(buff_src, magic, bs);
	memset(buff_dst, 0, bs);

	cur_offset = lseek(fd, 0, SEEK_CUR);
	for (i=0; i< count; i++) {
		ret = write(fd, buff_src, bs);
		if (ret != bs) {
			printf("Write fail\n");
			perror("");
			return -1;
		}
	}
	fsync(fd);
	clear_cache(300000); /* 300ms */

	/* memset(buff_src, magic+1, bs); */
	lseek(fd, cur_offset, SEEK_SET);
	for (i=0; i<count; i++) {
		ret = read(fd, buff_dst, bs);
		if (ret != bs) {
			printf("Read fail\n");
			perror("");
			return -1;
		}
		ret = byte_verify(buff_src, buff_dst, bs);
		if (ret == -1) {
			printf("Verify fail\n");
			return -1;
		}
	}

	free(buff_src);
	free(buff_dst);
	return 0;
}

static void usage()
{
	printf("Usage:\n");
	printf("-m setup read write or verify mode\n");
	printf("-f filename\n");
	printf("-s size\n");
	printf("-b block size\n");
	printf("-t how many times check\n\n");
	printf("ex: 50MB 4KB block size\n");
	printf("# ./sdcard_testing -m read -f testfile -s 52428800 -b 4096 -t 5\n");
	printf("# ./sdcard_testing -m write -f testfile -s 52428800 -b 4096 -t 5\n");
	printf("# ./sdcard_testing -m verify -f testfile -s 52428800 -b 4096 -t 5\n\n");
	printf("ex: 50MB 1MB block size\n");
	printf("# ./sdcard_testing -m read -f testfile -s 52428800 -b 1048576 -t 5\n");
	printf("# ./sdcard_testing -m write -f testfile -s 52428800 -b 1048576 -t 5\n");
	printf("# ./sdcard_testing -m verify -f testfile -s 52428800 -b 1048576 -t 5\n\n");
	exit(0);
}

static char optstr[] = "m:f:s:b:t:";

int main(int argc, char *argv[])
{
	unsigned int c;
	int size;	/* file size */
	int bs;		/* block size */
	int times;
	char mode[64];
	char filename[128];
	int count;

	int i;
	int ret;
	int file_fd;
	int file_size;
	char magic;
	struct timeval t_start;
	struct timeval t_stop;
	struct timeval t_diff;
	long int high;
	long int left;
	long int low;
	struct timeval max;
	struct timeval min;
	struct timeval avg;

	while ((c = getopt(argc, argv, optstr)) != -1) {

		switch (c) {
		case 'm':
			sprintf(mode, "%s", optarg);
			break;
		case 'f':
			sprintf(filename, "%s", optarg);
			break;
		case 's':
			size = strtoul(optarg, 0, 0);
			break;
		case 'b':
			bs = strtoul(optarg, 0, 0);
			break;
		case 't':
			times = strtoul(optarg, 0, 0);
			break;
		default:
			usage();
			break;
		}
	}

	if (argc < 7)
		usage();

	printf("Test Mode:\t%s\n", mode);
	printf("File Name:\t%s\n", filename);
	printf("File Size:\t%d MBytes\n", size/1024/1024);
	printf("Block Size:\t%d Bytes\n", bs);
	printf("Times:\t\t%d times\n\n", times);

	count = size/bs;
	if (strcmp(mode, "read") == 0) {
		file_fd = open(filename, O_RDONLY);
		if (file_fd < 0) {
			/* try to create File */
			if (errno == ENOENT) { /* No such file or directory */
				file_fd = open(filename, O_CREAT | O_RDWR, 0644);
				if (file_fd > 0) {
					printf("  Create %s size %dMB\n", filename, size/1024/1024);
					lseek(file_fd, 0, SEEK_SET);
					write_file(file_fd, size/1024/1024, 1024*1024);
					clear_cache(2000000); /* 2s */
				}
			}
		}

		if (file_fd < 0) {
			printf("open %s fail\n", filename);
			perror("");
			return -1;
		}

		file_size = lseek(file_fd, 0, SEEK_END);
		if (file_size < size) {
			printf("File size is less than %dMB\n", size/1024/1024);
			return -1;
		}

		for (i=0; i<times; i++) {
			clear_cache(300000);  /* 300ms */
			lseek(file_fd, 0, SEEK_SET);
			gettimeofday(&t_start, 0);
			ret = read_file(file_fd, count, bs);
			if (ret == -1) {
				printf("Read file Fail\n");
				return -1;
			}
			gettimeofday(&t_stop, 0);
			tim_subtract(&t_diff, &t_start, &t_stop);
			set_timeval_to_list(&t_diff);

			high = size/(t_diff.tv_sec*1000*1000 + t_diff.tv_usec);
			left = size%(t_diff.tv_sec*1000*1000 + t_diff.tv_usec);
			low = left*10/(t_diff.tv_sec*1000*1000 + t_diff.tv_usec);

			printf("  Read Speed: %ld.%01ldMB/s  (Read %dMByte in %ld.%03ld Seconds  bs=%dByte try:%02d)\n", \
				high, low, size/1024/1024, t_diff.tv_sec, (t_diff.tv_usec%(1000*1000))/1000, bs, i+1);
		}

		get_timeval_result(&max, &min, &avg);

		high = size/(min.tv_sec*1000*1000 + min.tv_usec);
		left = size%(min.tv_sec*1000*1000 + min.tv_usec);
		low = left*10/(min.tv_sec*1000*1000 + min.tv_usec);
		printf("  Max Read Speed: %ld.%01ldMB/s\n", high, low);

		high = size/(max.tv_sec*1000*1000 + max.tv_usec);
		left = size%(max.tv_sec*1000*1000 + max.tv_usec);
		low = left*10/(max.tv_sec*1000*1000 + max.tv_usec);
		printf("  Min Read Speed: %ld.%01ldMB/s\n", high, low);

		high = size/(avg.tv_sec*1000*1000 + avg.tv_usec);
		left = size%(avg.tv_sec*1000*1000 + avg.tv_usec);
		low = left*10/(avg.tv_sec*1000*1000 + avg.tv_usec);
		printf("  AVG Read Speed: %ld.%01ldMB/s\n", high, low);

	} else if (strcmp(mode, "write") == 0) {
		file_fd = open(filename, O_CREAT | O_WRONLY, 0644);
		if (file_fd < 0) {
			printf("open %s fail\n", filename);
			perror("");
			return -1;
		}

		for (i=0; i<times; i++) {
			clear_cache(300000);  /* 300ms */
			lseek(file_fd, 0, SEEK_SET);
			gettimeofday(&t_start, 0);
			ret = write_file(file_fd, count, bs);
			gettimeofday(&t_stop, 0);
			tim_subtract(&t_diff, &t_start, &t_stop);

			set_timeval_to_list(&t_diff);

			high = size/(t_diff.tv_sec*1000*1000 + t_diff.tv_usec);
			left = size%(t_diff.tv_sec*1000*1000 + t_diff.tv_usec);
			low = left*10/(t_diff.tv_sec*1000*1000 + t_diff.tv_usec);

			printf("  Write Speed: %ld.%01ldMB/s  (Write %dMByte in %ld.%03ld Seconds  bs=%dByte try:%02d)\n", \
				high, low, size/1024/1024, t_diff.tv_sec, (t_diff.tv_usec%(1000*1000))/1000, bs, i+1);
		}

		get_timeval_result(&max, &min, &avg);

		high = size/(min.tv_sec*1000*1000 + min.tv_usec);
		left = size%(min.tv_sec*1000*1000 + min.tv_usec);
		low = left*10/(min.tv_sec*1000*1000 + min.tv_usec);
		printf("  Max Write Speed: %ld.%01ldMB/s\n", high, low);

		high = size/(max.tv_sec*1000*1000 + max.tv_usec);
		left = size%(max.tv_sec*1000*1000 + max.tv_usec);
		low = left*10/(max.tv_sec*1000*1000 + max.tv_usec);
		printf("  Min Write Speed: %ld.%01ldMB/s\n", high, low);

		high = size/(avg.tv_sec*1000*1000 + avg.tv_usec);
		left = size%(avg.tv_sec*1000*1000 + avg.tv_usec);
		low = left*10/(avg.tv_sec*1000*1000 + avg.tv_usec);
		printf("  AVG Write Speed: %ld.%01ldMB/s\n", high, low);

	} else if (strcmp(mode, "verify") == 0) {
		file_fd = open(filename, O_CREAT | O_RDWR, 0644);
		if (file_fd < 0) {
			printf("open %s fail\n", filename);
			perror("");
			return -1;
		}

		for (i=0; i<times; i++) {
			magic = 0xA5;
			lseek(file_fd, 0, SEEK_SET);
			ret = file_verify(file_fd, count, bs, magic);
			if (ret == 0) {
				printf("%dMByte Write then Read, Verify OK  try:%02d\n", size/1024/1024, i+1);
			} else {
				printf("%dMByte Write then Read, Verify fail  try:%02d\n", size/1024/1024, i+1);
				return -1;
			}
		}
	} else {
		printf("Test Mode ERROR !\n");
	}

	return 0;
}
