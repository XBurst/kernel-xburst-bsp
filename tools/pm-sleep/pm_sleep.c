

#include <cache.h>

#include <common.h>
#include <cpm.h>
#include <ddr.h>
#include <smp_cp0.h>
#include <pm_sleep.h>
#include <i2c-gpio.h>
#include <gpio.h>
#include <uart.h>
#include <cpufreq.h>
#include <cpu.h>

unsigned int *extern_func;

struct sleep_param *p_slp_param;
extern void scale_cpu_freq(int status, unsigned int *cpccr);

struct save_register
{
	unsigned int ddr_autosr;
	unsigned int ddr_dlp;
	unsigned int ddr_ctrl;
	unsigned int cpm_cpccr;
	unsigned int c0_config;
	unsigned int c0_status;
	unsigned int smp_cp0_ctrl;


	unsigned char pmu_pin_func;
	unsigned char pmu_register_val;
};

static struct save_register g_save_register;
/* static void i2c_sleep_set_data(void) */
/* { */
/* 	struct i2c_gpio i2c; */
/* 	unsigned int ret; */
/* 	if(p_slp_param->pmu_addr != 0xff){ */
/* 		i2c.scl = p_slp_param->pmu_i2c_scl; */
/* 		i2c.sda = p_slp_param->pmu_i2c_sda; */
/* 		ret = i2c_read(&i2c, */
/* 			       p_slp_param->pmu_addr, p_slp_param->pmu_reg, 1, */
/* 			       &g_save_register.pmu_register_val, 1); */
/* 		if(ret != 0) */
/* 		{ */
/* 			p_slp_param->pmu_addr = -1; */
/* 		}else */
/* 			ret = i2c_write(&i2c, */
/* 				  p_slp_param->pmu_addr, p_slp_param->pmu_reg, 1, */
/* 				  &p_slp_param->pmu_register_val, 1); */
/* 		i2c_reinit(&i2c); */
/* 	} */
/* } */
/* static int i2c_restore_data(void) */
/* { */
/* 	struct i2c_gpio i2c; */
/* 	unsigned char val; */
/* 	int ret = 0; */
/* 	if(p_slp_param->pmu_addr != 0xff){ */

/* 		i2c.scl = p_slp_param->pmu_i2c_scl; */
/* 		i2c.sda = p_slp_param->pmu_i2c_sda; */
/* 		ret = i2c_write(&i2c, */
/* 			  p_slp_param->pmu_addr, p_slp_param->pmu_reg, 1, */
/* 			  &g_save_register.pmu_register_val, 1); */
/* 		if(ret != 0) */
/* 			return -1; */
/* 		ret = i2c_read(&i2c, */
/* 			       p_slp_param->pmu_addr, p_slp_param->pmu_reg, 1, */
/* 			       &val, 1); */
/* 		if(ret != 0) */
/* 			return -1; */
/* 		if(val != g_save_register.pmu_register_val) */
/* 		{ */
/* 			return -1; */
/* 		} */

/* 	} */
/* 	return 0; */
/* } */


void sleep_pm_exit();
void call_function(unsigned int pc, int arg);
int set_core_voltage(int vol_mv);
void core_sleep_enter(void)
{
	unsigned int val;
	struct save_register *p_save_register = &g_save_register;

	serial_setid(p_slp_param->uart_id);

	flush_cache_all();
	set_resume_pc((unsigned int)sleep_pm_exit);

	if(p_slp_param->prev_sleep_pc != 0xffffffff) {
		/* serial_put_hex(p_slp_param->prev_sleep_pc); */
		call_function(p_slp_param->prev_sleep_pc, 0);
	}

	p_save_register->c0_config = read_c0_config();
	p_save_register->c0_status = read_c0_status();

	ddr_writel(0x0, DDRP_DTAR);
	p_save_register->ddr_autosr = ddr_readl(DDRC_AUTOSR_EN);
	ddr_writel(0,DDRC_AUTOSR_EN); // exit auto sel-refresh
	p_save_register->ddr_dlp = ddr_readl(DDRC_DLP);
	if(!(ddr_readl(DDRP_PIR) & DDRP_PIR_DLLBYP))
	{
		ddr_writel(0xf003 , DDRC_DLP);
		/* val = ddr_readl(DDRP_DSGCR); */
		/* val |= (1 << 4); */
		/* ddr_writel(val,DDRP_DSGCR); */
	}
        /**
	 *  DDR keep selrefresh,when it exit the sleep state.
	 */
	val = ddr_readl(DDRC_CTRL);
	p_save_register->ddr_ctrl = val;
	val &= ~(0x1f << 11);  // remove power down.
	val |= (1 << 17) | (1 << 5);   // enter to hold ddr state
	ddr_writel(val,DDRC_CTRL);

	scale_cpu_freq(SCALE, &p_save_register->cpm_cpccr);

	//  DDR CLK GATE OFF
	/* cpm_set_bit(31,CPM_CLKGR); */

	p_save_register->smp_cp0_ctrl = __read_32bit_c0_register($12, 2); /* cache attr */
	__write_32bit_c0_register($12, 2, p_save_register->smp_cp0_ctrl | (1<<31));

	/* i2c_sleep_set_data(); */
	if(p_slp_param->pmu_pin_func != 0xff)
	{
		p_save_register->pmu_pin_func = get_gpio_func(p_slp_param->pmu_pin);
		set_gpio_func(p_slp_param->pmu_pin,p_slp_param->pmu_pin_func);
	}else
		p_save_register->pmu_pin_func = 0xff;

	if(p_slp_param->post_sleep_pc != 0xffffffff)
	{
		/* serial_put_hex(p_slp_param->post_sleep_pc); */
		call_function(p_slp_param->post_sleep_pc, 0);
	}
	write_c0_status(p_save_register->c0_status | 1 << 10);
	/* serial_put_hex(cpm_inl(CPM_OPCR)); */
	serial_putc('e');
	__asm__ volatile(".set push\n\t"
					 ".set mips32\n\t"
					 "sync \n\t"
					 "nop\n\t"
					 "wait\n\t"
					 "nop\n\t"
					 "nop\n\t"
					 "nop\n\t"
					 "jr %0\n\t"
					 ".set mips32 \n\t"
					 ".set pop\n\t"
					 :
					 :"r" ((unsigned int)sleep_pm_exit));
	while(1)
		serial_putc('E');

}


void core_sleep_restore(void)
{
	int val = 0;
	int bypassmode = 0;
	struct save_register *p_save_register = &g_save_register;

	serial_putc('O');

	write_c0_config(p_save_register->c0_config);
	write_c0_status(p_save_register->c0_status);

	/*
	 * For voice triger in sleep mode of pmu.
	 */
	/* if(i2c_restore_data() != 0) */
	/* { */
	/* 	serial_putc('E'); */
	/* 	serial_putc('v'); */
	/* 	__asm__ volatile(".set mips32\n\t" */
	/* 			 "wait\n\t" */
	/* 			 "nop\n\t" */
	/* 			 ".set mips32 \n\t"); */
	/* } */

	if(p_slp_param->prev_resume_pc != 0xffffffff)
	{
		__jz_cache_init();
		/* serial_put_hex(p_slp_param->prev_resume_pc); */
		call_function(p_slp_param->prev_resume_pc, p_save_register->cpm_cpccr);
	}

	if(p_slp_param->pmu_pin != 0xff)
	{
		set_gpio_func(p_slp_param->pmu_pin,p_save_register->pmu_pin_func);
	}

	scale_cpu_freq(RESTORE, &p_save_register->cpm_cpccr);

	//  DDR CLK GATE ON
	/* cpm_clear_bit(31,CPM_CLKGR); */

	bypassmode = ddr_readl(DDRP_PIR) & DDRP_PIR_DLLBYP;
	if(!bypassmode) {
		/**
		 * reset dll of ddr.
		 * WARNING: 2015-01-08
		 * 	DDR CLK GATE(CPM_DRCG 0xB00000D0), BIT6 must set to 1 (or 0x40).
		 * 	If clear BIT6, chip memory will not stable, gpu hang occur.
		 */
		/* { */
		/* 	val = ddr_readl(DDRP_DSGCR); */
		/* 	val &= ~(1 << 4); */
		/* 	ddr_writel(val,DDRP_DSGCR); */
		/* } */
#define CPM_DRCG (0xB00000D0)

		*(volatile unsigned int *)CPM_DRCG |= (1<<1);
		TCSM_DELAY(0x1ff);
		*(volatile unsigned int *)CPM_DRCG &= ~(1<<1);
		TCSM_DELAY(0x1ff);
		/*
		 * for disabled ddr enter power down.
		 */
		*(volatile unsigned int *)0xb301102c &= ~(1 << 4);
		TCSM_DELAY(0xf);

		/*
		 * reset dll of ddr too.
		 */
		*(volatile unsigned int *)CPM_DRCG |= (1<<1);
		TCSM_DELAY(0x1ff);
		*(volatile unsigned int *)CPM_DRCG &= ~(1<<1);
		TCSM_DELAY(0x1ff);

		val = DDRP_PIR_INIT | DDRP_PIR_ITMSRST  | DDRP_PIR_DLLSRST | DDRP_PIR_DLLLOCK;// | DDRP_PIR_ZCAL  ;
		ddr_writel(val, DDRP_PIR);
		val = DDRP_PGSR_IDONE | DDRP_PGSR_DLDONE | DDRP_PGSR_DIDONE;// | DDRP_PGSR_ZCDONE;
		while ((ddr_readl(DDRP_PGSR) & val) != val) {
			if(ddr_readl(DDRP_PGSR) & (DDRP_PGSR_DTERR | DDRP_PGSR_DTIERR)) {
				serial_putc('e');
				break;
			}
			serial_put_hex(ddr_readl(DDRP_ZQXCR0(0)));
			serial_put_hex(ddr_readl(DDRP_ZQXSR0(0)));
			serial_put_hex(ddr_readl(DDRP_PGSR));
			serial_putc('\r');
			serial_putc('\n');
		}
	}

	/* exit the ddr selrefresh */
	val = ddr_readl(DDRC_CTRL);
	val |= 1 << 1;
	val &= ~((1<< 17) | (1 << 5));    // exit to hold ddr state
	ddr_writel(val,DDRC_CTRL);
	/* ddr trainning. */
	if(!bypassmode){
		val = DDRP_PIR_INIT | DDRP_PIR_QSTRN;
	}else
		val = DDRP_PIR_INIT | DDRP_PIR_QSTRN | DDRP_PIR_DLLBYP;
	ddr_writel(val, DDRP_PIR);
	val = (DDRP_PGSR_IDONE | DDRP_PGSR_DLDONE | DDRP_PGSR_DIDONE | DDRP_PGSR_DTDONE);
	while ((ddr_readl(DDRP_PGSR) & val) != val) {
		if(ddr_readl(DDRP_PGSR) & (DDRP_PGSR_DTERR | DDRP_PGSR_DTIERR)) {
			serial_putc('e');
			break;
		}
		serial_put_hex(ddr_readl(DDRP_PGSR));
		serial_putc('\r');
		serial_putc('\n');
	}

	if(!bypassmode)
	{
		*(volatile unsigned int *)0xb301102c |= (1 << 4);
		TCSM_DELAY(0xf);
	}

	/* serial_putc('d'); */
	/* serial_put_hex(*(unsigned int *)p_slp_param->post_resume_pc); */

	if(!p_save_register->ddr_dlp && !bypassmode)
	{
		ddr_writel(0x0 , DDRC_DLP);
	}
	if(p_save_register->ddr_autosr) {
		ddr_writel(1,DDRC_AUTOSR_EN);   // enter auto sel-refresh
	}
	ddr_writel(p_save_register->ddr_ctrl, DDRC_CTRL);

	l2cache_enable();
//	dump_ddr_param();
	__jz_cache_init();

	/* serial_put_hex(*(unsigned int *)p_slp_param->post_resume_pc); */
	__write_32bit_c0_register($12, 2,p_save_register->smp_cp0_ctrl);
	if(p_slp_param->post_resume_pc != 0xfffffff)
	{
		call_function(p_slp_param->post_resume_pc, 0);
	}

	while(1)
		serial_putc('s');
}
