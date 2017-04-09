/*
 * Copyright (C) 2015 Huizhou Desay SV Automotive Co., Ltd
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>

#define USB_REQ_SET_FEATURE	0x03
#define USB_RT_PORT		0x23
#define USB_PORT_FEAT_TEST	21

#if 0
#define USB_TEST_J		0X01
#define USB_TEST_K		0X02
#define USB_TEST_SE0_NAK	0X03
#define USB_TEST_PACKET		0x04
#define USB_TEST_FORCE_ENABLE	0x05
#endif

void usage(void)
{
	printf("usage: host mode for hub test\n");
	printf("hub_test_mode vid pid port_num test_mode\n\n");
	printf("vid pid: VendorID ProductID: 0x0000 0x0001\n\n");
	printf("port_num:  1 - port1\n");
	printf("           2 - port2\n");
	printf("           3 - port3\n\n");
	printf("test_mode: 1 - USB_TEST_J\n");
	printf("           2 - USB_TEST_K\n");
	printf("           3 - USB_TEST_SE0_NAK\n");
	printf("           4 - USB_TEST_PACKET\n");
	printf("           5 - USB_TEST_FORCE_ENABLE\n");
}

int main(int argc, char **argv)
{
	libusb_context *ctx = NULL;	/* a libusb session */
	int ret;
	unsigned long vid, pid, portnum, test_mode;

	if (argc == 1 || strcmp(argv[1], "-h") == 0 ||
	    strcmp(argv[1], "--help") == 0 || argc != 5) {
		usage();
		return -1;
	}

	/* Parse the parameter data from string to unsigned long */
	vid = strtoul(argv[1], NULL, 0);
	pid = strtoul(argv[2], NULL, 0);
	portnum = strtoul(argv[3], NULL, 0);
	test_mode = strtoul(argv[4], NULL, 0);

	printf("vid[%04x] pid[%04x] port_num[%d] test_mode[%d]\n",
		vid, pid, portnum, test_mode);

	/* initialize a library session */
	ret = libusb_init(&ctx);
	if(ret < 0) {
		printf("libusb_init fail\n");
		return -1;
	}

	libusb_device_handle *udevh;
	udevh = libusb_open_device_with_vid_pid(ctx, vid, pid);
	if(udevh) {
		ret = libusb_control_transfer(udevh,
						USB_RT_PORT,			/* bRequesType */
						USB_REQ_SET_FEATURE,		/* bRequest */
						USB_PORT_FEAT_TEST,		/* wValue */
						(test_mode << 8) | portnum,	/* wIndex */
						NULL,				/* data */
						0,				/* wLength */
						1000);				/* timeout */
		if (ret != 0)
			printf("send set_feature message failed\n");
		else
			printf("send set_feature message success\n");

		libusb_close(udevh);
	} else {
		printf("opne hub device failed\n");
		return -1;
	}

	return 0;
}
