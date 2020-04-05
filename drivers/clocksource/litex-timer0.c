// SPDX-License-Identifier: GPL-2.
/*
 * Copyright (C) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <clock.h>
#include <linux/err.h>
#include <linux/clk.h>

#define TIMER0_LOAD 0x00
#define TIMER0_RELOAD 0x10
#define TIMER0_EN 0x20
#define TIMER0_UPDATE_VALUE 0x24
#define TIMER0_VALUE 0x28

static void __iomem *timer_base;

/* Helper routines for accessing MMIO over a wishbone bus.
 * Each 32 bit memory location contains a single byte of data, stored
 * little endian
 */
static inline void outreg8(u8 val, int reg)
{
	iowrite32(val, timer_base + reg);
}

static inline void outreg32(u32 val, int reg)
{
	outreg8(val >> 24, reg);
	outreg8(val >> 16, reg + 0x4);
	outreg8(val >> 8, reg + 0x8);
	outreg8(val, reg + 0xc);
}

static inline u8 inreg8(int reg)
{
	return ioread32(timer_base + reg);
}

static inline u32 inreg32(int reg)
{
	return (inreg8(reg) << 24) |
		(inreg8(reg + 0x4) << 16) |
		(inreg8(reg + 0x8) <<  8) |
		(inreg8(reg + 0xc) <<  0);
}

static uint64_t litex_cs_read(void)
{
	outreg8(1, TIMER0_UPDATE_VALUE);
	return (uint64_t)(0xffffffff - inreg32(TIMER0_VALUE));
}

static struct clocksource litex_cs = {
	.read	= litex_cs_read,
	.mask   = CLOCKSOURCE_MASK(32),
};

static int litex_timer_probe(struct device_d *dev)
{
	struct resource *iores;
	struct clk *clk;

	/* use only one timer */
	if (timer_base)
		return -EBUSY;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	timer_base = IOMEM(iores->start);

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		int ret;
		ret = PTR_ERR(clk);
		dev_err(dev, "clock not found: %d\n", ret);
		return ret;
	}

	clocks_calc_mult_shift(&litex_cs.mult, &litex_cs.shift,
		clk_get_rate(clk), NSEC_PER_SEC, 1);

	/* disable timer */
	outreg8(0, TIMER0_EN);

	/* use timer in periodic mode */
	outreg32(0, TIMER0_LOAD);

	/* max counter value */
	outreg32(0xffffffff, TIMER0_RELOAD);

	init_clock(&litex_cs);

	/* enable timer */
	outreg8(1, TIMER0_EN);

	return 0;
}

static __maybe_unused struct of_device_id litex_timer_dt_ids[] = {
	{
		.compatible = "litex,timer0",
	}, {
		/* sentinel */
	}
};

static struct driver_d litex_timer_driver = {
	.probe	= litex_timer_probe,
	.name	= "litex-timer0",
	.of_compatible = DRV_OF_COMPAT(litex_timer_dt_ids),
};

static int litex_timer_init(void)
{
	return platform_driver_register(&litex_timer_driver);
}
coredevice_initcall(litex_timer_init);
