/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2013 Juergen Beisert <kernel@pengutronix.de>, Pengutronix
 */

#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <types.h>
#include <driver.h>

struct firmware_handler {
	char *id; /* unique identifier for this firmware device */
	char *model; /* description for this device */
	struct device_d *dev;
	void *priv;
	struct device_node *device_node;
	/* called once to prepare the firmware's programming cycle */
	int (*open)(struct firmware_handler*);
	/* called multiple times to program the firmware with the given data */
	int (*write)(struct firmware_handler*, const void*, size_t);
	/* called once to finish programming cycle */
	int (*close)(struct firmware_handler*);
};

struct firmware_mgr;

int firmwaremgr_register(struct firmware_handler *);

struct firmware_mgr *firmwaremgr_find(const char *);
#ifdef CONFIG_FIRMWARE
struct firmware_mgr *firmwaremgr_find_by_node(struct device_node *np);
int firmwaremgr_load_file(struct firmware_mgr *, const char *path);
char *firmware_get_searchpath(void);
void firmware_set_searchpath(const char *path);
#else
static inline struct firmware_mgr *firmwaremgr_find_by_node(struct device_node *np)
{
	return NULL;
}

static inline int firmwaremgr_load_file(struct firmware_mgr *mgr, const char *path)
{
	return -ENOSYS;
}

static inline char *firmware_get_searchpath(void)
{
	return NULL;
}

static inline void firmware_set_searchpath(const char *path)
{
}

#endif

void firmwaremgr_list_handlers(void);

#define get_builtin_firmware(name, start, size) \
	{							\
		extern char _fw_##name##_start[];		\
		extern char _fw_##name##_end[];			\
		*start = (typeof(*start)) _fw_##name##_start;	\
		*size = _fw_##name##_end - _fw_##name##_start;	\
	}

#endif /* FIRMWARE_H */
