// SPDX-License-Identifier: GPL-2.0-only
/*
 * fixed regulator support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <malloc.h>
#include <init.h>
#include <regulator.h>
#include <of.h>
#include <of_gpio.h>
#include <gpio.h>
#include <gpiod.h>

struct regulator_fixed {
	int gpio;
	int always_on;
	struct regulator_dev rdev;
	struct regulator_desc rdesc;
};

static int regulator_fixed_enable(struct regulator_dev *rdev)
{
	struct regulator_fixed *fix = container_of(rdev, struct regulator_fixed, rdev);

	if (!gpio_is_valid(fix->gpio))
		return 0;

	return gpio_direction_active(fix->gpio, true);
}

static int regulator_fixed_disable(struct regulator_dev *rdev)
{
	struct regulator_fixed *fix = container_of(rdev, struct regulator_fixed, rdev);

	if (fix->always_on)
		return 0;

	if (!gpio_is_valid(fix->gpio))
		return 0;

	return gpio_direction_active(fix->gpio, false);
}

const static struct regulator_ops fixed_ops = {
	.enable = regulator_fixed_enable,
	.disable = regulator_fixed_disable,
};

static int regulator_fixed_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct regulator_fixed *fix;
	u32 delay;
	int ret;

	if (!dev->device_node)
		return -EINVAL;

	fix = xzalloc(sizeof(*fix));
	fix->gpio = -EINVAL;

	if (of_get_property(np, "gpio", NULL)) {
		fix->gpio = gpiod_get(dev, NULL, GPIOD_ASIS);
		if (fix->gpio < 0) {
			ret = fix->gpio;
			goto err;
		}
	}

	fix->rdesc.ops = &fixed_ops;
	fix->rdev.desc = &fix->rdesc;
	fix->rdev.dev = dev;

	if (!of_property_read_u32(np, "off-on-delay-us", &delay))
		fix->rdesc.off_on_delay = delay;

	if (of_find_property(np, "regulator-always-on", NULL) ||
	    of_find_property(np, "regulator-boot-on", NULL)) {
		fix->always_on = 1;
		regulator_fixed_enable(&fix->rdev);
	}

	ret = of_regulator_register(&fix->rdev, np);
	if (ret)
		return ret;

	return 0;
err:
	free(fix);

	return ret;
}

static struct of_device_id regulator_fixed_of_ids[] = {
	{ .compatible = "regulator-fixed", },
	{ }
};

static struct driver_d regulator_fixed_driver = {
	.name  = "regulator-fixed",
	.probe = regulator_fixed_probe,
	.of_compatible = DRV_OF_COMPAT(regulator_fixed_of_ids),
};
coredevice_platform_driver(regulator_fixed_driver);
