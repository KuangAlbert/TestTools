/*
 *  mcu_spi.c - Linux kernel modules for communicate with radio mcu
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/signal.h>

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_device.h>

#include "mcu_spi.h"

u8 tx_packet[8], rx_packet[8], ack_packet[8];

u8 zero_packet[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
u8 sync_packet[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

struct mcu_spi_tx_status tx_status = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
struct mcu_spi_rx_status rx_status = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

u8 spi_tx_buf[SPI_BUFF_SIZE][SPI_FRAME_SIZE];	/* spi_write_data buffer */
u8 spi_rx_buf[SPI_BUFF_SIZE][SPI_FRAME_SIZE];	/* spi_read_data buffer */

static struct workqueue_struct *mcu_spi_write_wq;
static struct workqueue_struct *mcu_spi_read_wq;

static void spi_packet_analyse(struct mcu_spi_device *device);

static void mcu_spi_cs(int value)
{
	gpio_direction_output(MCU_SPI_CS, value);
	printk(KERN_INFO "\n mcu_spi_cs = %d\n",  value);
	mdelay(10);
}

static inline int spi_write_read_8bytes(struct spi_device *spi,
					const u8 *txbuf, u8 *rxbuf)
{
	struct spi_transfer	t = {
		.tx_buf		= txbuf,
		.rx_buf		= rxbuf,
		.len		= 8,
	};
	struct spi_message	m;
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spi_sync(spi, &m);
}

static void mcu_spi_printk_data_err(u8 *rx)
{
	printk(KERN_ERR "\n mcu_spi rxdata  #<%02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X>\n",
		rx[0], rx[1], rx[2], rx[3], rx[4], rx[5], rx[6], rx[7]);
}

#ifdef SPI_DATA_DEBUG
static void mcu_spi_printk_data(u8 *tx, u8 *rx)
{
	printk(KERN_INFO "\n txdata  #<%02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X>\n rxdata  #<%02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X>\n",
		tx[0], tx[1], tx[2], tx[3], tx[4], tx[5], tx[6], tx[7],
		rx[0], rx[1], rx[2], rx[3], rx[4], rx[5], rx[6], rx[7]);
}
#else
static void mcu_spi_printk_data(u8 *tx, u8 *rx)
{
	;
}
#endif

/* infrom radio to reset spi module */
static void spi_reset_radio(void)
{
	mcu_spi_cs(1);
	rx_status.chkerr_times = 0;
	rx_status.slave_ready = 0;
	rx_status.frame_err_times = 0;
	rx_status.invalid_packet_times = 0;
	mcu_spi_cs(0);
	printk(KERN_ERR "\n mcu_spi reset radio spi module......error...\n");
}

static int spi_write_data_packet(struct mcu_spi_device *device,
					u8 *txbuf, size_t len)
{
	int i, ret = 0;
	tx_status.packets = (len + 5)/PACKET_SIZE;

	for (i = 0; i < tx_status.packets; i++) {
		tx_packet[0] = tx_status.frame_id;
		tx_packet[1] = txbuf[PACKET_SIZE*i];
		tx_packet[2] = txbuf[PACKET_SIZE*i+1];
		tx_packet[3] = txbuf[PACKET_SIZE*i+2];
		tx_packet[4] = txbuf[PACKET_SIZE*i+3];
		tx_packet[5] = txbuf[PACKET_SIZE*i+4];
		tx_packet[6] = txbuf[PACKET_SIZE*i+5];

		tx_packet[7] = tx_packet[1]^tx_packet[2]^tx_packet[3]
				^tx_packet[4]^tx_packet[5]^tx_packet[6];

		pr_debug("tx frame_id = 0x%02X CS = 0x%02X  \n", tx_status.frame_id,
			tx_packet[1]^tx_packet[2]^tx_packet[3]^tx_packet[4]^tx_packet[5]^tx_packet[6]);

		if (tx_status.tx_state == ACKOK) {
			/* if received ack packet, break currently sending */
			printk(KERN_ERR "\n received mcu_spi ack packet when resending ......\n");
			break;
		} else {
			mutex_lock(&device->packet_lock);
			/* when resending data packet, if received ack packet from radio, stop resending */
			if (tx_status.tx_state != ACKOK)
				ret = spi_write_read_8bytes(device->spi, tx_packet, rx_packet);
			else
				printk(KERN_ERR "\n received mcu_spi ack packet when resending ......\n");

			if (tx_status.tx_state != ACKOK)
				mcu_spi_printk_data(tx_packet, rx_packet);

			if (tx_status.tx_state != ACKOK) {
				spi_packet_analyse(device);
			}
			mutex_unlock(&device->packet_lock);
		}
	}

	if (ret < 0) {
		printk(KERN_ERR "\n mcu spi write read ......error...\n");
		return -1;
	} else
		return len;
}

static int spi_write_frame(struct mcu_spi_device *device, size_t len)
{
	int i, ret;
	u8 check_sum;

	spin_lock(&device->tx_state_lock);
	tx_status.tx_state = SENDING;
	spin_unlock(&device->tx_state_lock);

	/* judge if running number is full */
	if (!((tx_status.frame_id & MAXNUMBER) ^ MAXNUMBER)) {
		tx_status.frame_id = 0;	/* reset running number to 0x0 */
	}

	if (tx_status.hq_enable)
		tx_status.frame_id |= HQ;
	else
		tx_status.frame_id &= ~HQ;

	spi_tx_buf[tx_status.pointing][0] = 0xFF;
	spi_tx_buf[tx_status.pointing][1] = 0xA5;
	spi_tx_buf[tx_status.pointing][2] = 0x5A;
	spi_tx_buf[tx_status.pointing][3] = ++tx_status.frame_id;
	spi_tx_buf[tx_status.pointing][4] = len;

	check_sum = spi_tx_buf[tx_status.pointing][3] ^ spi_tx_buf[tx_status.pointing][4];

	for (i = 5; i < len+5; i++) {
		check_sum = check_sum ^ spi_tx_buf[tx_status.pointing][i];
	}

	spi_tx_buf[tx_status.pointing][len+5] = check_sum;

	/* init ack completion */
	if (tx_status.hq_enable)
		init_completion(&device->ack_complete);

	ret = spi_write_data_packet(device, spi_tx_buf[tx_status.pointing], len+6);

	return ret;
}

static int spi_wait_ack_packet(struct mcu_spi_device *device)
{
	int ret;

	spin_lock(&device->tx_state_lock);
	tx_status.tx_state = WAITING_ACK;
	spin_unlock(&device->tx_state_lock);

	ret = wait_for_completion_timeout(&device->ack_complete, SPI_ACK_TIMEOUT);

	if (ret < 0)
		return ret;
	if (ret == 0) {
		printk(KERN_ERR "\n mcu_spi wait ack timed out tx_status.frame_id = 0x%02X\n", tx_status.frame_id);
		mcu_spi_printk_data_err(rx_packet);
		tx_status.tx_state = TX_TIMEOUT;
		return -ETIMEDOUT;
	}

	return 0;
}

static int spi_ack_packet_analyse(struct mcu_spi_device *device)
{
	if (rx_packet[0] == tx_status.frame_id) {
		if (rx_packet[2] == 0x10) {
			printk(KERN_ERR "\n mcu_spi received ack resend packet, but not define......error...\n\n");
		} else if (rx_packet[2] == 0x11) {
			printk(KERN_ERR "\n mcu_spi received ack slow clock packet, but not define......error...\n\n");
		} else if (rx_packet[1] == 0x0) {

			if (tx_status.tx_state != WAITING_ACK) {
				msleep(1);
				printk(KERN_DEBUG "\n mcu_spi tx_status.tx_state != WAITING_ACK 1st\n\n");
				if (tx_status.tx_state != WAITING_ACK) {
					msleep(1);
					printk(KERN_ERR "\n mcu_spi tx_status.tx_state != WAITING_ACK 2nd\n\n");
				}
				complete(&device->ack_complete);
			} else
				complete(&device->ack_complete);

			spin_lock(&device->tx_state_lock);
			tx_status.tx_state = ACKOK;
			spin_unlock(&device->tx_state_lock);

			rx_status.invalid_packet_times = 0;

			pr_debug("\n received ack ok packet frame_id = 0x%02X\n",
				tx_status.frame_id);
		} else if (rx_packet[1] == 0xFF) {
			printk(KERN_ERR "\n mcu_spi received ack error packet, but not define......error...\n\n");
		}
		return 1;
	}
	return 0;
}

static int spi_ack_checking(struct mcu_spi_device *device)
{
	int i, ret;

	ret = spi_wait_ack_packet(device);

	if (ret < 0) {
		for (i =0; i < MAX_RETRY; i++) {
			/* resending mcu_spi frame */
			printk(KERN_ERR "\n mcu_spi resending frame......\n");
			tx_status.frame_id --;

			spi_write_frame(device, spi_tx_buf[tx_status.pointing][4]);

			ret = spi_wait_ack_packet(device);
			if (ret < 0) {
				printk(KERN_ERR "\n mcu_spi wait ack timed out......error...\n");
			} else
				break;
		}
	} else
		pr_debug("mcu_spi received ack packet \n");

	return ret;
}

static int spi_write_ack_packet(struct mcu_spi_device *device,
				u8 id, u8 status)
{
	int ret;
	memset(ack_packet, 0x0, 8);
	/* set bit 6 to 0, means frame master */
	/* id &= ~0x40; */
	ack_packet[0] = id;
	ack_packet[1] = status;
	ret = spi_write_read_8bytes(device->spi, ack_packet, rx_packet);

	mcu_spi_printk_data(ack_packet, rx_packet);

	if (tx_status.hq_enable) {
		spi_ack_packet_analyse(device);
	}

	return 0;
}

static int spi_status_checking(struct mcu_spi_device *device)
{
	if (!rx_status.slave_ready) {
		if (tx_status.resync_times > RESYNC_TIMES && tx_status.retry_counter < MAX_RETRY) {
			printk(KERN_ERR "\n mcu_spi radio sync fail......error...\n\n");
			spi_reset_radio();
			tx_status.resync_times = 0;
			tx_status.retry_counter++;
		}
		tx_status.resync_times++;
		printk(KERN_INFO "mcu_spi try to resync radio mcu again...\n");
		mdelay(2);
		queue_work(mcu_spi_read_wq, &device->read_work);
		rx_status.packet_received++;
		return -1;
	} else if (rx_status.packet_received > SPI_PACKET_LIMIT) {
		if (tx_status.frame_nums == 0) {
			printk(KERN_ERR "\n mcu_spi radio CRQ pin is always low......error...\n\n");
			spi_reset_radio();
		} else {
			printk(KERN_ERR "\n mcu_spi radio CRQ pin is always low......error...\n\n");
			queue_work(mcu_spi_read_wq, &device->read_work);
			rx_status.packet_received++;
			pr_debug("received %d packet\n", rx_status.packet_received);
		}
		return -1;
	} else
		return 0;
}

static int spi_packet_checking(struct mcu_spi_device *device)
{
	u8 check_sum = rx_packet[1]^rx_packet[2]^rx_packet[3]
			^rx_packet[4]^rx_packet[5]^rx_packet[6];

	if (rx_packet[0] == 0 && rx_packet[1] == 0 &&
		rx_packet[2] == 0 && rx_packet[3] == 0 &&
		rx_packet[4] == 0 && rx_packet[5] == 0 &&
		rx_packet[6] == 0 && rx_packet[7] == 0) {
		pr_debug("mcu_spi received 0x0 packet\n");

		rx_status.invalid_packet_times ++;
		if (rx_status.invalid_packet_times > MAX_INVALID_PACKET) {
			printk(KERN_ERR "\n mcu_spi Received too invalid packet error......error......\n\n");
			spi_reset_radio();
		}

		return 1;
	} else if (rx_packet[0] == sync_packet[7] && rx_packet[1] == sync_packet[6] &&
		rx_packet[2] == sync_packet[5] && rx_packet[3] == sync_packet[4] &&
		rx_packet[4] == sync_packet[3] && rx_packet[5] == sync_packet[2] &&
		rx_packet[6] == sync_packet[1] && rx_packet[7] == sync_packet[0]) {
		printk(KERN_INFO "mcu_spi Navi & Radio SPI Sync OK\n");
		if (!rx_status.slave_ready) {
			rx_status.slave_ready = 1;
			mcu_spi_cs(1);
			queue_work(mcu_spi_write_wq, &device->write_work);
		}
		return 1;
	} else if (check_sum != rx_packet[7]) {
		if (rx_packet[0] == 0xFF && rx_packet[1] == 0xFF &&
			rx_packet[2] == 0xFF && rx_packet[3] == 0xFF &&
			rx_packet[4] == 0xFF && rx_packet[5] == 0xFF &&
			rx_packet[6] == 0xFF && rx_packet[7] == 0xFF) {
			pr_debug("mcu_spi received 0xFF packet\n");

			rx_status.invalid_packet_times ++;
			if (rx_status.invalid_packet_times > MAX_INVALID_PACKET) {
				printk(KERN_ERR "\n mcu_spi Received too invalid packet error......error......\n\n");
				spi_reset_radio();
			}
		} else {
			printk(KERN_ERR "\n mcu_spi packet check_sum......error...\n\n");
			rx_status.chkerr_times ++;
			rx_status.rx_state = CS_ERROR;

			mcu_spi_printk_data_err(rx_packet);
		}
		if (rx_status.chkerr_times > MAX_CHK_ERR) {
			printk(KERN_ERR "\n mcu_spi Received too much check_sum error......error......\n\n");
			spi_reset_radio();
		}
		return -1;
	} else
		return 0;
}

static int spi_frame_header_checking(void)
{
	u8 frame_len;
	if (rx_packet[1] == 0xFF && rx_packet[2] == 0xA5 && rx_packet[3] == 0x5A && rx_packet[4] == rx_packet[0]) {
		rx_status.frame_id = rx_packet[0];
		frame_len = rx_packet[5] + SPI_FRAME_OVERHEAD;

		/* fix rx_status.pointed inconsistent with rx_status.pointing */
		if (rx_status.total_packet != rx_status.packets) {
			if (rx_status.total_packet) {
				if (rx_status.pointed == 0)
					rx_status.pointed = SPI_BUFF_SIZE - 1;
				else
					rx_status.pointed--;

				rx_status.frame_err_times ++;

				printk(KERN_ERR "\n mcu_spi total_packet = %d, packets = %d, frame_id = 0x%02x\n\n",
					rx_status.total_packet, rx_status.packets, rx_status.frame_id);
				printk(KERN_ERR "\n mcu_spi received not integrated frame......error...\n\n");

				mcu_spi_printk_data_err(rx_packet);

				if (rx_status.frame_err_times > MAX_FREME_ERR) {
					printk(KERN_ERR "\n mcu_spi received too much not integrated frame error......error......\n\n");
					spi_reset_radio();
				}
			}
		}

		rx_status.total_packet = (frame_len % 6) ? (frame_len/6 + 1) : (frame_len/6);

		if (rx_status.pointed < (SPI_BUFF_SIZE -1))
			rx_status.pointed++;
		else
			rx_status.pointed = 0;

		rx_status.packets = 0;

		pr_debug("** rx_status.total_packet = %d, rx_status.pointed = %d\n",
			rx_status.total_packet, rx_status.pointed);

		return 0;
	} else
		return 1;
}

static void spi_copy_to_frame_buf(void)
{
	spi_rx_buf[rx_status.pointed][0 + (rx_status.packets) * 6] = rx_packet[1];
	spi_rx_buf[rx_status.pointed][1 + (rx_status.packets) * 6] = rx_packet[2];
	spi_rx_buf[rx_status.pointed][2 + (rx_status.packets) * 6] = rx_packet[3];
	spi_rx_buf[rx_status.pointed][3 + (rx_status.packets) * 6] = rx_packet[4];
	spi_rx_buf[rx_status.pointed][4 + (rx_status.packets) * 6] = rx_packet[5];
	spi_rx_buf[rx_status.pointed][5 + (rx_status.packets) * 6] = rx_packet[6];

	rx_status.packets ++;

	pr_debug("rx_status.packets = %d\n", rx_status.packets);
}

static void spi_hand_shake(struct mcu_spi_device *device)
{
	spi_write_read_8bytes(device->spi, sync_packet, rx_packet);

	mcu_spi_printk_data(sync_packet, rx_packet);

	spi_packet_checking(device);
}

static void spi_frame_analyse(struct mcu_spi_device *device)
{
	u8 i, frame_len, frame_check_sum = 0;
	frame_len = spi_rx_buf[rx_status.pointed][4] + SPI_FRAME_OVERHEAD;

	if ( frame_len > 0 && frame_len < SPI_FRAME_SIZE) {
		for ( i = 0; i< (frame_len -1); i++) {
			frame_check_sum ^= spi_rx_buf[rx_status.pointed][i];
		}
	} else {
		printk(KERN_ERR "mcu_spi frame_len too long = %d\n", frame_len);
		spi_reset_radio();
		return;
	}

	if (frame_check_sum == spi_rx_buf[rx_status.pointed][frame_len -1]
		&& (device->async_queue || device->block_flag) && frame_len != 0 ) {
		/* check Frame ID if it is HQ Frame */
		if (spi_rx_buf[rx_status.pointed][3] & 0x80 && spi_rx_buf[rx_status.pointed][3] & 0x40) {
			/* send ack packet to slave */
			spi_write_ack_packet(device, spi_rx_buf[rx_status.pointed][3], 0);
		}

		spin_lock(&device->rx_lock);
		if (rx_status.frame_nums == 0 && rx_status.pointing != rx_status.pointed) {
			printk(KERN_ERR"\n mcu_spi rx_status.pointing is not equal to rx_status.pointed ......error...\n");
			rx_status.pointing = rx_status.pointed;
		}

		rx_status.frame_nums ++;
		spin_unlock(&device->rx_lock);

		rx_status.frame_err_times = 0;
		rx_status.invalid_packet_times = 0;

		if (device->block_flag) {
			wake_up_interruptible(&device->wait);
		} else {
			/* Send signal to app program read the data */
			kill_fasync(&device->async_queue, SIGIO, POLL_IN);
			pr_debug("%s kill SIGIO\n", __func__);
		}

	} else {
		rx_status.chkerr_times ++;
		printk(KERN_ERR "\n mcu_spi frame packet check_sum error......error...\n\n");

		printk(KERN_ERR "mcu_spi frame data #[");
		for ( i = 0; i< (frame_len + 5); i++) {
			printk(KERN_ERR "%02X, ", spi_rx_buf[rx_status.pointed][i]);
		}
		printk(KERN_ERR "]\n\n");

		if (rx_status.pointed == 0)
			rx_status.pointed = SPI_BUFF_SIZE - 1;
		else
			rx_status.pointed--;

		if (rx_status.chkerr_times > MAX_CHK_ERR) {
			printk(KERN_ERR "\n mcu_spi received too much check_sum error......error......\n\n");
			spi_reset_radio();
		}
	}
}

static void spi_packet_analyse(struct mcu_spi_device *device)
{
	int ret = 0;

	if (!ret)
		ret = spi_packet_checking(device);

	if (!ret && tx_status.hq_enable) {
		ret = spi_ack_packet_analyse(device);
	}

	if (!ret) {
		ret = spi_frame_header_checking();

		if (rx_packet[0] == rx_status.frame_id) {
			spi_copy_to_frame_buf();

			if (rx_status.total_packet == rx_status.packets)
				spi_frame_analyse(device);
			ret = 0;
		}

		if (ret) {
			printk(KERN_ERR "mcu_spi received undefine packet\n");
			mcu_spi_printk_data_err(rx_packet);

			rx_status.frame_err_times ++;

			if (rx_status.frame_err_times > MAX_FREME_ERR) {
				printk(KERN_ERR "\n mcu_spi received too much undefine packet error......error......\n\n");
				spi_reset_radio();
			}
		}
	}
}

static void mcu_spi_write_work(struct work_struct *work)
{
	struct mcu_spi_device *device = container_of(work, struct mcu_spi_device, write_work);

	while (tx_status.frame_nums > 0 && rx_status.slave_ready) {
		spin_lock(&device->tx_lock);
		tx_status.frame_nums--;
		spin_unlock(&device->tx_lock);

		if (tx_status.pointing < (SPI_BUFF_SIZE - 1))
			tx_status.pointing ++;
		else
			tx_status.pointing = 0;
		pr_debug("## tx_status.frame_nums = %d, tx_status.pointing = %d\n",
			tx_status.frame_nums, tx_status.pointing);

		if (HQ_ENABLE)
			tx_status.hq_enable = 1; /* judge if it is a HQ frame, reserve interface */

		spi_write_frame(device, spi_tx_buf[tx_status.pointing][4]);

		if (tx_status.hq_enable) {
			if (spi_ack_checking(device) < 0) {
				spi_reset_radio();

				/* resending last frame after reset */
				spin_lock(&device->tx_lock);
				tx_status.frame_nums++;
				spin_unlock(&device->tx_lock);
				tx_status.frame_id --;

				if (tx_status.pointing == 0)
					tx_status.pointing = (SPI_BUFF_SIZE - 1);
				else
					tx_status.pointing --;
			}
		}
	}

	if (tx_status.frame_nums == 0 && tx_status.pointing != tx_status.pointed) {
		printk(KERN_ERR"mcu_spi tx_status.pointing is not equal to tx_status.pointed......error...\n");
		tx_status.pointing = tx_status.pointed;
	}
}

static void mcu_spi_read_work(struct work_struct *work)
{
	struct mcu_spi_device *device = container_of(work, struct mcu_spi_device, read_work);

	rx_status.rx_state = RECEIVING;

	if (!rx_status.slave_ready)
		spi_hand_shake(device);
	else {
		mutex_lock(&device->packet_lock);

		spi_write_read_8bytes(device->spi, zero_packet, rx_packet);

		mcu_spi_printk_data(zero_packet, rx_packet);

		spi_packet_analyse(device);
		mutex_unlock(&device->packet_lock);
	}

	/* judge if IRQ pin is still in low */
	if (!gpio_get_value(GPIO_MCU_SPI_IRQ) || !rx_status.slave_ready) {
		if (!spi_status_checking(device)) {
			queue_work(mcu_spi_read_wq, &device->read_work);
			rx_status.packet_received++;
			pr_debug("received %d packet\n", rx_status.packet_received);
		}
	} else {
		rx_status.packet_received = 0;
		enable_irq(device->spi->irq);
	}
}

static irqreturn_t mcu_spi_irq(int irq, void *handle)
{
	struct mcu_spi_device *device = handle;

	/* judge if IRQ pin is still in low */
	if (!gpio_get_value(GPIO_MCU_SPI_IRQ)) {
		disable_irq_nosync(device->spi->irq);
		rx_status.invalid_packet_times = 0;
		queue_work(mcu_spi_read_wq, &device->read_work);
	}

	return IRQ_HANDLED;
}

static int mcu_spi_fasync(int fd, struct file *filp, int mode)
{
	struct mcu_spi_device *dev = filp->private_data;
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

static int mcu_spi_open(struct inode *inode, struct file *filp)
{
	/* device information */
	struct mcu_spi_device *device;
	device = container_of(inode->i_cdev, struct mcu_spi_device, cdev);
	filp->private_data = device;
	dev_info(&device->spi->dev, "%s\n", __FUNCTION__);

	if ((filp->f_flags & O_NONBLOCK))
		device->block_flag = 0;
	else
		device->block_flag = 1;

	printk(KERN_ERR"mcu_spi open device use %s flag\n", (device->block_flag ? "BLOCK":"NONBLOCK"));

	mcu_spi_cs(0);
	rx_status.slave_ready = 0;

	/* detect irq state */
	if (!gpio_get_value(GPIO_MCU_SPI_IRQ) || !rx_status.slave_ready) {
		rx_status.packet_received = 0;
		queue_work(mcu_spi_read_wq, &device->read_work);
	}
	return 0;
}

static int mcu_spi_write(struct file *filp, const char *buf, size_t len, loff_t *offp)
{
	int ret = 0;
	struct mcu_spi_device *device = filp->private_data;

	if (len > SPI_FRAME_SIZE - SPI_FRAME_OVERHEAD) {
		printk(KERN_ERR "\n mcu_spi_write too long data length......error...\n");
		return -ENOMEM;
	} else if (len < 0) {
		printk(KERN_ERR "\n mcu_spi_write too short data length......error...\n");
		return -EINVAL;
	}

	if (tx_status.pointed < (SPI_BUFF_SIZE -1))
		tx_status.pointed++;
	else
		tx_status.pointed = 0;

	memset(&spi_tx_buf[tx_status.pointed][len+6], 0x0, 6);

	if (copy_from_user(&spi_tx_buf[tx_status.pointed][5], buf, len)) {
		ret = -EFAULT;
	} else {
		spi_tx_buf[tx_status.pointed][4] = len;

		spin_lock(&device->tx_lock);
		tx_status.frame_nums ++;
		spin_unlock(&device->tx_lock);

		if (tx_status.frame_nums > (SPI_BUFF_SIZE - 1)) {
			printk(KERN_ERR"\n mcu_spi spi_tx_buf  overflow......error...\n");
			msleep(100);
		}

		pr_debug("$$ tx_status.frame_nums = %d, tx_status.pointed = %d, len = %d\n",
			tx_status.frame_nums, tx_status.pointed, len);

		queue_work(mcu_spi_write_wq, &device->write_work);

		ret = len;
	}
	return ret;
}

static int mcu_spi_read(struct file *filp, char *buf, size_t len, loff_t *offp)
{
	int ret = 0;
	u8 frame_length = 0;

	struct mcu_spi_device *device = filp->private_data;

	if (rx_status.frame_nums <= 0 && device->block_flag)
		ret = wait_event_interruptible(device->wait, (rx_status.frame_nums > 0));

	/* wait_event_interruptible interrupt by other signal */
	if (ret)
		return ret;

	if (rx_status.frame_nums > 0) {
		frame_length = spi_rx_buf[rx_status.pointing][4];

		ret = copy_to_user(buf, &spi_rx_buf[rx_status.pointing][5], frame_length);

		if (ret) {
			printk(KERN_ERR"\n mcu_spi_read copy_to_user ......error...\n");
			return -EFAULT;
		}

		pr_debug("** rx_status.frame_nums = %d, rx_status.pointed = %d, rx_status.pointing = %d\n",
			rx_status.frame_nums, rx_status.pointed, rx_status.pointing);

		spin_lock(&device->rx_lock);
		rx_status.frame_nums --;

		if (rx_status.pointing < (SPI_BUFF_SIZE - 1))
			rx_status.pointing ++;
		else
			rx_status.pointing = 0;
		spin_unlock(&device->rx_lock);

		if (rx_status.frame_nums > 0 && !device->block_flag) {
			kill_fasync(&device->async_queue, SIGIO, POLL_IN);
			pr_debug("%s kill SIGIO\n", __func__);
		}
	}

	return frame_length;
}

static int mcu_spi_release(struct inode *inode, struct file *filp)
{
	struct mcu_spi_device *device = filp->private_data;
	dev_dbg(&device->spi->dev, "%s\n", __FUNCTION__);
	mcu_spi_fasync(-1, filp, 0);
	return 0;
}

static int mcu_spi_remove(struct spi_device *spi)
{
	struct mcu_spi_device *device = dev_get_drvdata(&spi->dev);

	dev_t devno = MKDEV(spi_major, 0);

	dev_dbg(&device->spi->dev, "%s\n", __FUNCTION__);

	device_destroy(device->my_class, devno);	/* delete /dev/mcu_spi */
	class_destroy(device->my_class);		/* delete /sys/class/mcu_spi_class */
	cdev_del (&device->cdev);
	unregister_chrdev_region(devno, 1);

	destroy_workqueue(mcu_spi_write_wq);
	destroy_workqueue(mcu_spi_read_wq);

	free_irq(device->spi->irq, device);

	kfree(device);

	return 0;
}

static const struct file_operations mcu_spi_fops = {
	.owner = THIS_MODULE,
	.open = mcu_spi_open,
	.write = mcu_spi_write,
	.read = mcu_spi_read,
	.fasync = mcu_spi_fasync,
	.release = mcu_spi_release,
};

/* add DTS support -- uida3984 */
static const struct of_device_id mcu_spi_of_match[] = {
	{
		.compatible = "mcu_spi",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, mcu_spi_of_match);


static int mcu_spi_probe(struct spi_device *spi)
{
	int ret;
	struct mcu_spi_device *device;
	const struct of_device_id *match;
	struct device_node	*node = spi->dev.of_node;

	dev_t devno;

	devno = MKDEV(spi_major, 0);

	printk(KERN_INFO "%s\n", __func__);

	device = kzalloc(sizeof(struct mcu_spi_device), GFP_KERNEL);
	if (device == NULL) {
		dev_err(&spi->dev, "out of memory\n");
		ret = -ENOMEM;
		goto err_free_mem;
	}

	dev_set_drvdata(&spi->dev, device);

	/* add DTS support get chip_select and max_speed_hz -- uida3984 */
	match = of_match_device(mcu_spi_of_match, &spi->dev);
	if(match) {
		of_property_read_u8(node, "chip_select", &(spi->chip_select));
		of_property_read_u32(node, "spi-max-frequency", &(spi->max_speed_hz));
	}
	else {
		spi->chip_select = 0;
		spi->max_speed_hz = 1500000;
	}

	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_3;
	ret = spi_setup(spi);
	if (ret < 0)
		return ret;

	device->spi = spi;
	rx_status.packet_received = 0;
	rx_status.pointed = SPI_BUFF_SIZE - 1;
	rx_status.pointing = 0;
	tx_status.frame_id |= MAXNUMBER;

	/* init work queue*/
	mcu_spi_write_wq = create_singlethread_workqueue("mcu_spi_write");
	if (mcu_spi_write_wq == NULL)
		return ret;
	mcu_spi_read_wq = create_singlethread_workqueue("mcu_spi_read");
	if (mcu_spi_read_wq == NULL)
		return ret;

	/* init mutex lock */
	mutex_init(&device->packet_lock);

	/* init spin lock */
	spin_lock_init(&device->tx_state_lock);
	spin_lock_init(&device->tx_lock);
	spin_lock_init(&device->rx_lock);

	init_waitqueue_head(&device->wait);

	gpio_request(MCU_SPI_CS, "gpio1_16_mux1");
	gpio_direction_output(MCU_SPI_CS, 1);
	
	mcu_spi_cs(1);

	if (spi_major)
		ret = register_chrdev_region(devno, 1, SPI_NAME); /* cat /proc/devices */
	else {
		ret = alloc_chrdev_region(&devno, 0, 1, SPI_NAME);
		spi_major = MAJOR(devno);
	}

	if (ret < 0)
		return ret;

	cdev_init(&device->cdev, &mcu_spi_fops);
	device->cdev.owner = THIS_MODULE;
	device->cdev.ops = &mcu_spi_fops;
	ret = cdev_add(&device->cdev, devno, 1);

	if (ret < 0) {
		printk(KERN_ERR "%s: cdev_add failed\n", __func__);
		kfree(&device->cdev);
	}
	device->my_class =class_create(THIS_MODULE, "mcu_spi_class");
	if (IS_ERR(device->my_class)) {
		printk(KERN_ERR "%s: failed in creating class\n", __func__);
		return -1;
	}

	/* register /dev/mcu_spi */
	device_create(device->my_class, NULL, devno, NULL, "mcu_spi");

	INIT_WORK(&device->read_work, mcu_spi_read_work);
	INIT_WORK(&device->write_work, mcu_spi_write_work);

	if (gpio_request(GPIO_MCU_SPI_IRQ, "mcu_spi_irq") < 0)
		printk(KERN_ERR "can't get mcu_spi_irq gpio\n");

	gpio_direction_input(GPIO_MCU_SPI_IRQ);

	spi->irq = gpio_to_irq(GPIO_MCU_SPI_IRQ);

	ret = request_irq(spi->irq, mcu_spi_irq, IRQF_TRIGGER_FALLING,
				spi->dev.driver->name, device);

	if (ret < 0) {
		dev_err(&spi->dev, "request irq %d err\n", spi->irq);
		goto err_free_irq;
	}

	disable_irq_nosync(device->spi->irq);

	spi_write_read_8bytes(device->spi, zero_packet, rx_packet);

	return 0;
err_free_irq:
	free_irq(spi->irq, device);
err_free_mem:
	kfree(device);
	return ret;
}

static struct spi_driver mcu_spi_driver = {
	.driver = {
		.name	= "mcu_spi",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
		.of_match_table = mcu_spi_of_match,
	},
	.probe		= mcu_spi_probe,
	.remove		= mcu_spi_remove,
};

static int __init mcu_spi_init(void)
{
	printk(KERN_INFO "%s\n", __func__);
	return spi_register_driver(&mcu_spi_driver);
}

static void __exit mcu_spi_exit(void)
{
	spi_unregister_driver(&mcu_spi_driver);
}

MODULE_AUTHOR("DESAY SV");
MODULE_DESCRIPTION("MCU COMMUNICATE driver");
MODULE_LICENSE("GPL");

module_init(mcu_spi_init);
module_exit(mcu_spi_exit);
