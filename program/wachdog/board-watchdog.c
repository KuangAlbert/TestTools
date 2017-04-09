/**
 * @file board-watchdog.c
 * @brief watchdog
 * @copyright Huizhou Desay SV Automotive Co., Ltd.
 * @par Revision History:
 * @code
 * Rev1.0  20131031  uida3983  Draft version
 * Rev1.1  20140930  uida3984  Draft version
 * Rev1.2  20161203  uidp5021  Draft version
 *            Code ported to DS03H
 * @endcode
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

unsigned int oops_reboot_flag = 0;
#define DELAY_MS	500	/* ms delay */

struct sv_watchdog_data {
	struct mutex mutex_lock;
	struct delayed_work work;
	struct device_attribute acce_attr_p;
	unsigned int on_off;
	unsigned int io;
};

static int sv_watchdog_remove(struct platform_device *pdev)
{
	return 0;
}

static void sv_watchdog_work(struct work_struct *work)
{
	static int io_status;
	struct sv_watchdog_data *device = container_of(to_delayed_work(work),
			struct sv_watchdog_data, work);

	if (oops_reboot_flag == 1) {
		printk(KERN_ERR "sv_watchdog: Oops appear. system will be reboot\n");
		device->on_off = 0; /* force to stop sv_watchdog */
	}

	if (io_status == 1) {
		io_status = 0;
		gpio_direction_output(device->io, io_status);
	} else if (device->on_off == 1) {
		io_status = 1;
		gpio_direction_output(device->io, io_status);
	}

	schedule_delayed_work(&device->work,
			msecs_to_jiffies(DELAY_MS));
}

static ssize_t sv_watchdog_on_off(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned long val;
	struct sv_watchdog_data *device = container_of(attr,
			struct sv_watchdog_data, acce_attr_p);

	ret = strict_strtoul(buf, 0, &val);
	if (ret)
		printk(KERN_ERR "%s is not in hex or decimal form.\n", buf);
	else
		device->on_off = val != 0 ? 1:0;
	printk(KERN_ERR "Watch Dog is %s\n", device->on_off ? "ON":"OFF");

	return strnlen(buf, count);
}

static int sv_watchdog_probe(struct platform_device *pdev)
{
	int ret;
	struct sv_watchdog_data *device;
	enum of_gpio_flags flags;

	printk(KERN_INFO "%s\n", __func__);

	device = kzalloc(sizeof(struct sv_watchdog_data), GFP_KERNEL);
	if (!device) {
		ret = -ENOMEM;
		goto err_free_mem;
	}

	/* get gpio number from dts */
	device->io = of_get_gpio_flags(pdev->dev.of_node, 0, &flags);
	if (!gpio_is_valid(device->io)) {
		ret = device->io;
		printk(KERN_ERR "%s: Failed to get gpio from dts\n", __func__);
	}

	gpio_request(device->io, "sv_watchdog");

	device->on_off = 1;		/* default: keep sv_watchdog is on */
	INIT_DELAYED_WORK(&device->work, sv_watchdog_work);
	schedule_delayed_work(&device->work, msecs_to_jiffies(DELAY_MS));

	device->acce_attr_p.attr.name = "on_off";
	device->acce_attr_p.attr.mode = S_IWUGO;
	device->acce_attr_p.store = sv_watchdog_on_off;
	ret = sysfs_create_file(&pdev->dev.kobj, &device->acce_attr_p.attr);
	if (ret) {
		kfree(device);
		printk(KERN_ERR "%s: Failed to create attr\n", __func__);
		return ret;
	}

	return 0;
err_free_mem:
	kfree(device);
	return ret;
}

/* add DTS support -- uidp5021 */
static const struct of_device_id watchdog_of_match[] = {
	{
		.compatible = "sv_watchdog",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, watchdog_of_match);

static struct platform_driver sv_watchdog_driver = {
	.driver = {
		.name	= "sv_watchdog",
		.owner	= THIS_MODULE,
		.of_match_table = watchdog_of_match,
	},
	.probe		= sv_watchdog_probe,
	.remove		= sv_watchdog_remove,
};

static int __init sv_watchdog_init(void)
{
	return platform_driver_register(&sv_watchdog_driver);
}
module_init(sv_watchdog_init);

static void __exit sv_watchdog_exit(void)
{
	platform_driver_unregister(&sv_watchdog_driver);
}
module_exit(sv_watchdog_exit);

MODULE_DESCRIPTION("sv watch dog");
MODULE_AUTHOR("DESAY SVAUTOMOTIVE");
MODULE_LICENSE("GPL");
