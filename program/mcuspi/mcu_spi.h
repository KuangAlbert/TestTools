/*
 *  MCU SPI device driver header file
 *
 *  Copyright (C) 2012 Huizhou Desay SV Automotive Co., Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _MCU_SPI_H_
#define _MCU_SPI_H_

#include <linux/spi/spi.h>
#include <linux/major.h>

#define SPI_NAME		"mcu_spi"

static int spi_major = 0;

#define GPIO_MCU_SPI_IRQ	(5*32+16)
#define MCU_SPI_CS		(6*32+17)

#define HQ	0x80	/* high priority frame bit7 */
#define MASTER	0x0	/* from maser frame bit6 */
#define SLAVE		0x40	/* from maser frame bit6 */
#define MAXNUMBER	0x3F	/* max running number equal to 63 */
#define STARTNUMBER	0xC1	/* start running number equal to 1 */

#define SPI_FRAME_OVERHEAD	6	/* beyond app command payload */

#define	PACKET_SIZE	6
#define	MAX_RETRY	3
#define	MAX_CHK_ERR	1
#define	MAX_FREME_ERR	3
#define	RESYNC_TIMES	10
#define	MAX_INVALID_PACKET	40

#define	SPI_BUFF_SIZE		128
#define	SPI_FRAME_SIZE		270
#define	SPI_PACKET_LIMIT	256

#define	HQ_ENABLE	1

/* timeout waiting for the radio ack packet */
#define SPI_ACK_TIMEOUT (msecs_to_jiffies(100))

enum mcu_spi_state {
	/* tx channel status */
	TX_IDLE = 0,
	ACKOK,		/* can tx new cmd now */
	RESEND,
	SENDING,
	WAITING_ACK,
	TX_TIMEOUT, 	/* tx channel timeout */
	/* rx channel status */
	RX_IDLE = 10,
	RECEIVING,
	ACK_NEEDED,	/* tx need to send  ack before further receiving */
	CS_ERROR,	/* packet to be dropped until new ack/data cmd arrives */
	RX_TIMEOUT,
};

struct mcu_spi_tx_status {
	u8 frame_id;		/* running number for each command transferred, M3 marks the id, ack command from navi uses the same id */
	enum mcu_spi_state tx_state;/* idle/ack_ok, sending, wait_ack, ack_timeout, master_timeout and other errors, etc */
	u8 total_packet;		/* total #packet for the command */
	u8 retry_counter;		/* #resend to navi, total 3 */
	u8 frame_nums;		/* nums of frame in the buffer */
	u8 packets;		/* packets in the frame */
	u8 pointing;		/* spi_tx_buf pointer of spi_write_frame */
	u8 pointed;		/* spi_tx_buf pointer of mcu_spi_write */
	u8 resync_times;		/* resynctims with radio mcu */
	u8 hq_enable;		/* hq packet enable, must wait ack packet */
} ;

struct mcu_spi_rx_status {
	u8 frame_id;		/* running number for each command received, navi side marks the id, same id for the ack command */
	enum mcu_spi_state rx_state;/* idle/cmd_complete, receiving, OVE, cs_error, cmd_timeout, resend_req, other errors, etc */
	u8 total_packet;		/* total #packet of the command */
	u8 packet_received;	/* #packet sucessfully received */
	u8 repeat_counter;		/* #resend by navi, total 3 */
	u8 frame_nums;		/* nums of frame in the buffer */
	u8 packets;		/* packets in the frame */
	u8 pointing;		/* spi_rx_buf pointer of mcu_spi_read */
	u8 pointed;		/* spi_rx_buf pointer of mcu_spi_read_work */
	u8 slave_ready;		/* if radio mcu spi ready state */
	u8 chkerr_times;		/* chsum err times */
	u8 frame_err_times;		/* frame err times */
	u8 invalid_packet_times;		/* frame err times */
} ;

/* Structure for mcu_spi_device */
struct mcu_spi_device {
	struct mutex packet_lock;	/* To lock access to spi_write and spi_read */
	spinlock_t tx_state_lock;
	spinlock_t tx_lock;
	spinlock_t rx_lock;
	wait_queue_head_t wait;	/* for app block read spi data */
	int block_flag;	/* judge app open mcu_spi device use block or not */
	struct spi_device	*spi;
	struct cdev	cdev;	/* character device */
	struct work_struct	read_work;
	struct work_struct	write_work;
	struct fasync_struct *async_queue;	/* signal for inform app */
	struct class *my_class;
	struct completion	ack_complete;	/* ack packet completion */
};

#if defined(DEBUG)
#undef SPI_ACK_TIMEOUT
#define SPI_ACK_TIMEOUT (msecs_to_jiffies(200))
#define SPI_DATA_DEBUG
#endif

#endif
