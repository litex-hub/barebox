/*
 * LiteX Liteeth Ethernet
 *
 * Copyright 2017 Joel Stanley <joel@jms.id.au>
 *
 * This code was adapted for barebox by Antony Pavlov.
 */

#include <common.h>
#include <io.h>
#include <linux/iopoll.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <of_net.h>

#define DRV_NAME	"liteeth"

#define LITEETH_WRITER_SLOT		0x00
#define LITEETH_WRITER_LENGTH		0x04
#define LITEETH_WRITER_ERRORS		0x14
#define LITEETH_WRITER_EV_STATUS	0x24
#define LITEETH_WRITER_EV_PENDING	0x28
#define LITEETH_WRITER_EV_ENABLE	0x2c
#define LITEETH_READER_START		0x30
#define LITEETH_READER_READY		0x34
#define LITEETH_READER_LEVEL		0x38
#define LITEETH_READER_SLOT		0x3c
#define LITEETH_READER_LENGTH		0x40
#define LITEETH_READER_EV_STATUS	0x48
#define LITEETH_READER_EV_PENDING	0x4c
#define LITEETH_READER_EV_ENABLE	0x50
#define LITEETH_PREAMBLE_CRC		0x54
#define LITEETH_PREAMBLE_ERRORS		0x58
#define LITEETH_CRC_ERRORS		0x68

#define LITEETH_PHY_CRG_RESET		0x00
#define LITEETH_MDIO_W			0x04
#define LITEETH_MDIO_R			0x08

#define LITEETH_BUFFER_SIZE		0x800
#define MAX_PKT_SIZE			LITEETH_BUFFER_SIZE

struct liteeth {
	void __iomem *base;
	void __iomem *mdio_base;
	struct device_d *dev;
	struct mii_bus mii_bus;

	/* Link management */
	int cur_duplex;
	int cur_speed;

	/* Tx */
	int tx_slot;
	int num_tx_slots;
	void __iomem *tx_base;

	/* Rx */
	int rx_slot;
	int num_rx_slots;
	void __iomem *rx_base;
};

/* Helper routines for accessing MMIO over a wishbone bus.
 * Each 32 bit memory location contains a single byte of data, stored
 * little endian
 */
static inline void outreg8(u8 val, void __iomem *addr)
{
	iowrite32(val, addr);
}

static inline void outreg16(u16 val, void __iomem *addr)
{
	outreg8(val >> 8, addr);
	outreg8(val, addr + 4);
}

static inline u8 inreg8(void __iomem *addr)
{
	return ioread32(addr);
}

static inline u32 inreg32(void __iomem *addr)
{
	return (inreg8(addr) << 24) |
		(inreg8(addr + 0x4) << 16) |
		(inreg8(addr + 0x8) <<  8) |
		(inreg8(addr + 0xc) <<  0);
}

static int liteeth_init_dev(struct eth_device *edev)
{
	return 0;
}

static int liteeth_eth_open(struct eth_device *edev)
{
	struct liteeth *priv = edev->priv;

	/* Clear pending events? */
	outreg8(1, priv->base + LITEETH_WRITER_EV_PENDING);
	outreg8(1, priv->base + LITEETH_READER_EV_PENDING);

	return 0;
}

static int liteeth_eth_send(struct eth_device *edev, void *packet,
				int packet_length)
{
	struct liteeth *priv = edev->priv;
	void *txbuffer;
	int ret;
	u8 val;
	u8 reg;

	//pr_err("liteeth_eth_send(packet_length=%d)\n", packet_length);
	reg = inreg8(priv->base + LITEETH_READER_EV_PENDING);
	if (reg) {
		outreg8(reg, priv->base + LITEETH_READER_EV_PENDING);
	}

	/* Reject oversize packets */
	if (unlikely(packet_length > MAX_PKT_SIZE)) {
		dev_err(priv->dev, "tx packet too big\n");
		goto drop;
	}

	txbuffer = priv->tx_base + priv->tx_slot * LITEETH_BUFFER_SIZE;
	memcpy(txbuffer, packet, packet_length);
	outreg8(priv->tx_slot, priv->base + LITEETH_READER_SLOT);
	outreg16(packet_length, priv->base + LITEETH_READER_LENGTH);

	ret = readb_poll_timeout(priv->base + LITEETH_READER_READY,
			val, val, 1000);
	if (ret == -ETIMEDOUT) {
		dev_err(priv->dev, "LITEETH_READER_READY timed out\n");
		goto drop;
	}

	outreg8(1, priv->base + LITEETH_READER_START);

	priv->tx_slot = (priv->tx_slot + 1) % priv->num_tx_slots;

drop:
	return 0;
}

static int liteeth_eth_rx(struct eth_device *edev)
{
	struct liteeth *priv = edev->priv;
	u8 rx_slot;
	int len = 0;
	u8 reg;

	reg = inreg8(priv->base + LITEETH_WRITER_EV_PENDING);
	if (!reg) {
		goto done;
	}

	len = inreg32(priv->base + LITEETH_WRITER_LENGTH);
	if (len == 0 || len > 2048) {
		len = 0;
		goto done;
	}

	rx_slot = inreg8(priv->base + LITEETH_WRITER_SLOT);

	memcpy(NetRxPackets[0], priv->rx_base + rx_slot * LITEETH_BUFFER_SIZE, len);

	net_receive(edev, NetRxPackets[0], len);

	outreg8(reg, priv->base + LITEETH_WRITER_EV_PENDING);

	//pr_err("liteeth_eth_rx len=%d\n", len);
done:
	return len;
}

static void liteeth_eth_halt(struct eth_device *edev)
{
	struct liteeth *priv = edev->priv;
	pr_err("%s\n", __FUNCTION__);

	outreg8(0, priv->base + LITEETH_WRITER_EV_ENABLE);
	outreg8(0, priv->base + LITEETH_READER_EV_ENABLE);
}

static void liteeth_reset_hw(struct liteeth *priv)
{
	/* Reset, twice */
	outreg8(0, priv->base + LITEETH_PHY_CRG_RESET);
	udelay(10);
	outreg8(1, priv->base + LITEETH_PHY_CRG_RESET);
	udelay(10);
	outreg8(0, priv->base + LITEETH_PHY_CRG_RESET);
	udelay(10);
}

static int liteeth_get_ethaddr(struct eth_device *edev, unsigned char *m)
{
	return 0;
}

static int liteeth_set_ethaddr(struct eth_device *edev,
					const unsigned char *mac_addr)
{
	return 0;
}

static int liteeth_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct eth_device *edev;
	void __iomem *buf_base;
	struct liteeth *priv;
//	const char *mac_addr;
	int err;

	priv = xzalloc(sizeof(struct liteeth));
	edev = xzalloc(sizeof(struct eth_device));
	edev->priv = priv;
	priv->dev = dev;

	priv->base = dev_request_mem_region(dev, 0);
	if (IS_ERR(priv->base)) {
		err = PTR_ERR(priv->base);
		goto err;
	}

	priv->mdio_base = dev_request_mem_region(dev, 1);
	if (IS_ERR(priv->mdio_base)) {
		err = PTR_ERR(priv->mdio_base);
		goto err;
	}

	buf_base = dev_request_mem_region(dev, 2);
	if (IS_ERR(buf_base)) {
		err = PTR_ERR(buf_base);
		goto err;
	}

	err = of_property_read_u32(np, "rx-fifo-depth",
			&priv->num_rx_slots);
	if (err) {
		dev_err(dev, "unable to get rx-fifo-depth\n");
		goto err;
	}

	err = of_property_read_u32(np, "tx-fifo-depth",
			&priv->num_tx_slots);
	if (err) {
		dev_err(dev, "unable to get tx-fifo-depth\n");
		goto err;
	}

	/* Rx slots */
	priv->rx_base = buf_base;
	priv->rx_slot = 0;

	/* Tx slots come after Rx slots */
	priv->tx_base = buf_base + priv->num_rx_slots * LITEETH_BUFFER_SIZE;
	priv->tx_slot = 0;

#if 0
	mac_addr = of_get_mac_address(np);
	if (mac_addr && is_valid_ether_addr(mac_addr))
		memcpy(netdev->dev_addr, mac_addr, ETH_ALEN);
	else
		eth_hw_addr_random(netdev);
#endif

	edev->init = liteeth_init_dev;
	edev->open = liteeth_eth_open;
	edev->send = liteeth_eth_send;
	edev->recv = liteeth_eth_rx;
	edev->get_ethaddr = liteeth_get_ethaddr;
	edev->set_ethaddr = liteeth_set_ethaddr;
	edev->halt = liteeth_eth_halt;
	edev->parent = dev;

	liteeth_reset_hw(priv);

	err = eth_register(edev);
	if (err) {
		dev_err(dev, "Failed to register edev\n");
		goto err;
	}

//	err = mdiobus_register(&priv->mii_bus);
//	if (err) {
//		dev_err(dev, "Failed to register mii_bus\n");
//		goto err_mii;
//	}

	dev_info(dev, DRV_NAME " driver registered\n");

	return 0;

//err_mii:
//	eth_unregister(edev);

err:
	return err;
}

static const struct of_device_id liteeth_dt_ids[] = {
	{
		.compatible = "litex,liteeth"
	}, {
	}
};

static struct driver_d liteeth_driver = {
	.name = DRV_NAME,
	.probe = liteeth_probe,
	.of_compatible = DRV_OF_COMPAT(liteeth_dt_ids),
};
device_platform_driver(liteeth_driver);
