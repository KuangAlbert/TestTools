/*
 * Copyright (C) 2015 Huizhou Desay SV Automotive Co., Ltd
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>

void usage(void)
{
	printf("usage: full speed device mode for hub test\n");
	printf("hub_full_test_mode\n\n");
}

int main(int argc, char **argv)
{
	libusb_context *ctx = NULL;
	int ret;
	unsigned long vid, pid, portnum, test_mode;
	char buf[128];
	libusb_device_handle *udevh;

	if (argc > 1 && (strcmp(argv[1], "-h") == 0 ||
	    strcmp(argv[1], "--help") == 0)) {
		usage();
		return -1;
	}

	/* 1) J6 role switch to host mode */
	sprintf(buf, "echo %s > /sys/kernel/debug/48890000.usb/mode", "host");
	system(buf);
	sleep(1);

	/* 2) otg_hub role switch to device mode */
	ret = libusb_init(&ctx);
	if(ret < 0) {
		printf("libusb_init fail\n");
		return -1;
	}
	udevh = libusb_open_device_with_vid_pid(ctx, 0x0424, 0x2530);
	if(udevh) {
		/*
		 * wValue: defined by datasheet as below
		 * 0xc17c: 10 millisecond timout
		 * 0xc47c: 1  second timeout
		 * 0xc77c: 20 second timeout
		 * 0xc67c: 10 second timout
		 * 0xc57c: 5  second timout
		 */
		ret = libusb_control_transfer(udevh,
						0x41,		/* bRequesType */
						0x08,		/* bRequest */
						0xc77c,		/* wValue */
						0x0,		/* wIndex */
						NULL,		/* data */
						0,		/* wLength */
						1000);		/* timeout */
		if (ret != 0) {
			printf("send otg message failed\n\n");
			libusb_close(udevh);
			return -1;
		} else {
			printf("send otg message success\n\n");
		}

		libusb_close(udevh);
	} else {
		printf("opne hub device failed\n");
		return -1;
	}

	return 0;
}
