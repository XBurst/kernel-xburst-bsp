/*
 * JZSOC Clock and Power Manager
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <linux/slab.h>

#include <asm/string.h>
#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <mach/jzcpm_pwc.h>
#include <mach/platform.h>
#include <uapi/asm/setup.h>

#define DDR_SIZE_64M	64
extern int reset_keep_power(void);
extern char __initdata arcs_cmdline[COMMAND_LINE_SIZE];
static int g_is_use_rtc;
int __init check_socid(void);
int __init soc_is_x1000e(int);

int rtc_is_enabled(void)
{
	return !g_is_use_rtc;
}

void __init cpm_reset(void)
{
	unsigned int cpm_clkgr;

	cpm_clkgr = cpm_inl(CPM_CLKGR);
	cpm_clkgr |= 0x75e3ffe;
	/* default open pdma clk gate */
	cpm_clkgr &= ~(1 << 21);
	/* default open tcu clk gate */
	cpm_clkgr &= ~(1 << 18);
#ifdef CONFIG_TIMER_SYS_OST
	cpm_clkgr &= ~(1 << 20);
#endif
	cpm_outl(cpm_clkgr, CPM_CLKGR);
	/* default close otg clk gate */
	cpm_set_bit(3, CPM_CLKGR);

	cpm_outl(0,CPM_PSWC0ST);
	cpm_outl(16,CPM_PSWC1ST);
	cpm_outl(24,CPM_PSWC2ST);
	cpm_outl(8,CPM_PSWC3ST);
}
int __init setup_init(void)
{
	cpm_reset();
	g_is_use_rtc = reset_keep_power();
	return 0;
}

void __cpuinit jzcpu_timer_setup(void);
void __cpuinit jz_clocksource_init(void);
void __init init_all_clk(void);
///* used by linux-mti code */
//int coherentio;			/* init to 0, no DMA cache coherency */
//int hw_coherentio;		/* init to 0, no HW DMA cache coherency */

#define ARG_MEM_STR "mem="
#if 1
/* orig string "mem=32M" change to "mem=64M" */
static void __init ddr_param_change(char *param_addr)
{
	int num;
	char * mem_str_addr;
	char * mem_str_addr_end;

	//pr_info("%d %s, param: %s\n", __LINE__, __FILE__, param_addr);
	mem_str_addr = strstr(param_addr, ARG_MEM_STR);
	if ( !mem_str_addr ) {
		printk("ERROR!!: %s() boot args mem=32M not found!\n", __FUNCTION__);
		return;
	}
	mem_str_addr += strlen(ARG_MEM_STR);
	mem_str_addr_end = strchr(mem_str_addr, 'M');
	if ( !mem_str_addr_end ) {
		printk("ERROR!!: %s() boot args mem=32M not found!\n", __FUNCTION__);
		return;
	}

	if (mem_str_addr_end - mem_str_addr != 2) {
		printk("ERROR!!: %s() mem=32M only support 2 number!\n", __FUNCTION__);
		//printk("ERROR!!: mem=32M only support 2 number!\n");
		//BUGS();
	}

	num = sprintf(mem_str_addr, "%d", DDR_SIZE_64M);
	*(mem_str_addr+num) = 'M';

	//pr_info("%d %s, param: %s\n", __LINE__, __FILE__, param_addr);
	return ;
}
#else
//#define PARAM_BUF_SIZE (COMMAND_LINE_SIZE)
#define PARAM_BUF_SIZE (512)
static void __init ddr_param_change(char *param_addr)
{
	char param_start_buf[128] = {0};
	char *param_last_addr = NULL;
	char param_buf[PARAM_BUF_SIZE] = {0};
	unsigned int len;

	//pr_info("%d %s, param: %s\n", __LINE__, __FILE__, param_addr);
	len = strlen(param_addr);
	if(len >= PARAM_BUF_SIZE) {
		printk("ERROR!!: param_addr len %d >= PARAM_BUF_SIZE %d\n", len, PARAM_BUF_SIZE);
		//BUGS();
	}

	//param_last_addr = strchr(param_addr, 'M');
	param_last_addr = strstr(param_addr, ARG_MEM_STR);
	param_last_addr += strlen(ARG_MEM_STR);

	/* check buffer size */
	len = param_last_addr - param_addr;
	if (len > 127) {
		printk("ERROR!!: string mem= param_addr len(%d)  >= 128\n", len);
		//BUGS();
	}
	strncpy(param_start_buf, param_addr, len);
	param_last_addr = strchr(param_last_addr, 'M');

	sprintf(param_buf, "%s%d%s", param_start_buf,
		DDR_SIZE_64M, param_last_addr);

	strcpy(param_addr, param_buf);
	//pr_info("%d %s, param: %s\n", __LINE__, __FILE__, param_addr);
	return ;
}
#endif	/* if 0 */

void __init plat_mem_setup(void)
{
	/* jz mips cpu special */
	__asm__ (
		"li    $2, 0xa9000000 \n\t"
		"mtc0  $2, $5, 4      \n\t"
		"nop                  \n\t"
		::"r"(2));

	/* use IO_BASE, so that we can use phy addr on hard manual
	 * directly with in(bwlq)/out(bwlq) in io.h.
	 */
	set_io_port_base(IO_BASE);
	ioport_resource.start	= 0x00000000;
	ioport_resource.end	= 0xffffffff;
	iomem_resource.start	= 0x00000000;
	iomem_resource.end	= 0xffffffff;
	setup_init();
	init_all_clk();

	if(!soc_is_x1000e(check_socid())) {
		ddr_param_change(arcs_cmdline);
	}
	return;
}

void __init plat_time_init(void)
{
	jzcpu_timer_setup();
	jz_clocksource_init();
}
