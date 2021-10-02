// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017 Sam Ravnborg <sam@ravnborg.org>

#include <common.h>
#include <envfs.h>
#include <init.h>
#include <gpio.h>
#include <io.h>

#include <mach/at91sam9263_matrix.h>
#include <mach/at91sam9_smc.h>
#include <mach/at91_rtt.h>
#include <mach/hardware.h>
#include <mach/iomux.h>

static int add_smc_devices(void)
{
	if (!of_machine_is_compatible("atmel,at91sam9263ek"))
		return 0;

	add_generic_device("at91sam9-smc", 0, NULL, AT91SAM9263_BASE_SMC0, 0x200,
			   IORESOURCE_MEM, NULL);
	add_generic_device("at91sam9-smc", 1, NULL, AT91SAM9263_BASE_SMC1, 0x200,
			   IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(add_smc_devices);

static struct sam9_smc_config ek_nand_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 1,
	.ncs_write_setup	= 0,
	.nwe_setup		= 1,

	.ncs_read_pulse		= 3,
	.nrd_pulse		= 3,
	.ncs_write_pulse	= 3,
	.nwe_pulse		= 3,

	.read_cycle		= 5,
	.write_cycle		= 5,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 2,
};

static int at91sam9263_smc_init(void)
{
	unsigned long csa;

	if (!of_machine_is_compatible("atmel,at91sam9263ek"))
		return 0;

	/* setup bus-width (8 or 16) */
	if (IS_ENABLED(CONFIG_MTD_NAND_ATMEL_BUSWIDTH_16))
		ek_nand_smc_config.mode |= AT91_SMC_DBW_16;
	else
		ek_nand_smc_config.mode |= AT91_SMC_DBW_8;

	csa = readl(AT91SAM9263_BASE_MATRIX + AT91SAM9263_MATRIX_EBI0CSA);
	csa |= AT91SAM9263_MATRIX_EBI0_CS3A_SMC_SMARTMEDIA;
	writel(csa, AT91SAM9263_BASE_MATRIX + AT91SAM9263_MATRIX_EBI0CSA);

	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &ek_nand_smc_config);

	return 0;
}
device_initcall(at91sam9263_smc_init);

static int at91sam9263ek_env_init(void)
{
	if (!of_machine_is_compatible("atmel,at91sam9263ek"))
		return 0;

        at91_rtt_irq_fixup(IOMEM(AT91SAM9263_BASE_RTT0));
        at91_rtt_irq_fixup(IOMEM(AT91SAM9263_BASE_RTT1));

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_at91sam9263ek);

	return 0;
}
late_initcall(at91sam9263ek_env_init);
