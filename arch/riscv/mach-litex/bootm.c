#include <bootm.h>
#include <boot.h>
#include <common.h>
#include <command.h>
#include <driver.h>
#include <environment.h>
#include <image.h>
#include <init.h>
#include <fs.h>
#include <libfile.h>
#include <linux/list.h>
#include <xfuncs.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/sizes.h>
#include <libbb.h>
#include <magicvar.h>
#include <binfmt.h>
#include <restart.h>
#include <globalvar.h>

#include <asm/byteorder.h>

//.section    .text, "ax", @progbits
//.global     boot_helper
//boot_helper:
//	jr x13

//extern void boot_helper(unsigned long r1, unsigned long r2, unsigned long r3, unsigned long addr);

#define CSR_MSTATUS_MIE 0x8

// (__vexriscv__)
#define CSR_IRQ_MASK 0xBC0
#define CSR_IRQ_PENDING 0xFC0

#define CSR_DCACHE_INFO 0xCC0

static inline void irq_setmask(unsigned int mask)
{
//(__vexriscv__)
	asm volatile ("csrw %0, %1" :: "i"(CSR_IRQ_MASK), "r"(mask));
}

#define csrs(reg, bit) ({ \
  if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32) \
	asm volatile ("csrrs x0, " #reg ", %0" :: "i"(bit)); \
  else \
	asm volatile ("csrrs x0, " #reg ", %0" :: "r"(bit)); })

#define csrc(reg, bit) ({ \
  if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32) \
	asm volatile ("csrrc x0, " #reg ", %0" :: "i"(bit)); \
  else \
	asm volatile ("csrrc x0, " #reg ", %0" :: "r"(bit)); })

static inline void irq_setie(unsigned int ie)
{
//(__vexriscv__)
	if (ie) {
		csrs(mstatus, CSR_MSTATUS_MIE);
	} else {
		csrc(mstatus, CSR_MSTATUS_MIE);
	}
}

static void flush_cpu_dcache(void)
{
// (__vexriscv__)
	unsigned long cache_info;
	asm volatile ("csrr %0, %1" : "=r"(cache_info) : "i"(CSR_DCACHE_INFO));
	unsigned long cache_way_size = cache_info & 0xFFFFF;
	unsigned long cache_line_size = (cache_info >> 20) & 0xFFF;
	for (register unsigned long idx = 0; idx < cache_way_size; idx += cache_line_size) {
		asm volatile("mv x10, %0 \n .word(0b01110000000001010101000000001111)"::"r"(idx));
	}
}

#define MAIN_RAM_BASE 0xc0000000L
#define EMULATOR_RAM_BASE 0x20000000L

#define L2_SIZE 8192
static void flush_l2_cache(void)
{
#ifdef L2_SIZE
	unsigned int i;
	for (i = 0; i < 2 * L2_SIZE / 4; i++) {
		((volatile unsigned int *) MAIN_RAM_BASE)[i];
	}
#endif
}

/* litex/soc/software/bios/boot.c : boot() */
void __attribute__((noreturn)) lboot(unsigned long r1, unsigned long r2, unsigned long r3, unsigned long addr)
{
	void (*kernel)(int r1, int r2, int r3, int r4) = addr;

	irq_setmask(0);
	irq_setie(0);

/* FIXME: understand why flushing icache on Vexriscv make boot fail  */
//#ifndef __vexriscv__
//	flush_cpu_icache();
//#endif

	flush_cpu_dcache();

	flush_l2_cache();

	//boot_helper(r1, r2, r3, addr);
	kernel(0, 0, 0, addr);

	while(1)
		;
}


static int do_bootm_linux(struct image_data *data)
{
	printf("do_bootm_linux\n");
	if (!data->os_file) {
		return -EINVAL;
	}

	file_to_sdram(data->os_file, MAIN_RAM_BASE);
	lboot(0, 0, 0, MAIN_RAM_BASE);
//	file_to_sdram(data->initrd_file, EMULATOR_RAM_BASE);
	//lboot(0, 0, 0, EMULATOR_RAM_BASE);

	return 0;
}

static struct image_handler image_handler = {
	.name = "RISCV Linux Image",
	.bootm = do_bootm_linux,
	.filetype = filetype_riscv_linux_image,
	.ih_os = IH_OS_LINUX,
};

static int linux_register_image_handler(void)
{
	register_image_handler(&image_handler);

	return 0;
}
late_initcall(linux_register_image_handler);
