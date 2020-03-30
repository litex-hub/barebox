/*
 * drivers/misc/sram.c - generic memory mapped SRAM driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <errno.h>
#include <driver.h>
#include <malloc.h>
#include <init.h>
#include <linux/err.h>

struct sram {
	struct resource *res;
	char *name;
	struct cdev cdev;
};

static struct cdev_operations memops = {
	.read  = mem_read,
	.write = mem_write,
	.memmap = generic_memmap_rw,
};

static int sram_probe(struct device_d *dev)
{
	struct resource *iores;
	struct sram *sram;
	struct resource *res;
	void __iomem *base;
	int ret;
	const char *alias;
	char *devname;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	sram = xzalloc(sizeof(*sram));

	alias = of_alias_get(dev->device_node);
	if (alias) {
		devname = xstrdup(alias);
	} else {
		int err;

		err = cdev_find_free_index("sram");
		if (err < 0) {
			dev_err(dev, "no index found to name device\n");
			ret = err;
			goto err_device_name;
		}
		devname = xasprintf("sram%d", err);
	}
	sram->cdev.name = devname;

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	sram->cdev.size = (unsigned long)resource_size(res);
	sram->cdev.ops = &memops;
	sram->cdev.dev = dev;

	ret = devfs_create(&sram->cdev);
err_device_name:
	if (ret)
		return ret;

	return 0;
}

static __maybe_unused struct of_device_id sram_dt_ids[] = {
	{
		.compatible = "mmio-sram",
	}, {
	},
};

static struct driver_d sram_driver = {
	.name = "mmio-sram",
	.probe = sram_probe,
	.of_compatible = sram_dt_ids,
};
device_platform_driver(sram_driver);
