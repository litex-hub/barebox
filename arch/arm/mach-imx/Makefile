# SPDX-License-Identifier: GPL-2.0-only

obj-$(CONFIG_ARCH_IMX1)  += imx1.o
obj-$(CONFIG_ARCH_IMX25) += imx25.o
obj-$(CONFIG_ARCH_IMX21) += imx21.o
obj-$(CONFIG_ARCH_IMX27) += imx27.o
obj-$(CONFIG_ARCH_IMX31) += imx31.o
obj-$(CONFIG_ARCH_IMX35) += imx35.o
obj-$(CONFIG_ARCH_IMX50) += imx50.o imx5.o
pbl-$(CONFIG_ARCH_IMX50) += imx50.o imx5.o
obj-$(CONFIG_ARCH_IMX51) += imx51.o imx5.o
pbl-$(CONFIG_ARCH_IMX51) += imx51.o imx5.o
obj-$(CONFIG_ARCH_IMX53) += imx53.o imx5.o esdctl-v4.o
pbl-$(CONFIG_ARCH_IMX53) += imx53.o imx5.o esdctl-v4.o
obj-$(CONFIG_ARCH_IMX6) += imx6.o
CFLAGS_imx6.o := -march=armv7-a
lwl-$(CONFIG_ARCH_IMX6) += imx6-mmdc.o
obj-$(CONFIG_ARCH_IMX7) += imx7.o
obj-$(CONFIG_ARCH_VF610) += vf610.o
obj-pbl-$(CONFIG_ARCH_IMX8M) += imx8m.o
lwl-$(CONFIG_ARCH_IMX8M) += atf.o
obj-$(CONFIG_IMX_IIM)	+= iim.o
obj-$(CONFIG_NAND_IMX) += nand.o
lwl-$(CONFIG_ARCH_IMX_EXTERNAL_BOOT_NAND) += external-nand-boot.o
obj-y += devices.o imx.o
obj-pbl-y += esdctl.o boot.o
obj-$(CONFIG_BAREBOX_UPDATE) += imx-bbu-internal.o
obj-$(CONFIG_BAREBOX_UPDATE_IMX_EXTERNAL_NAND) += imx-bbu-external-nand.o
obj-$(CONFIG_RESET_IMX_SRC) += src.o
lwl-y += cpu_init.o
pbl-y += xload-spi.o xload-common.o xload-imx-nand.o xload-gpmi-nand.o
