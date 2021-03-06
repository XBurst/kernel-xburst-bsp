#ifndef __TCU_H__
#define	__TCU_H__

#define WDT_TDR			(0x00)  /* rw, 32, 0x???????? */
#define WDT_TCER		(0x04)  /* rw, 32, 0x???????? */
#define WDT_TCNT		(0x08)  /* rw, 32, 0x???????? */
#define WDT_TCSR		(0x0c)  /* rw, 32, 0x???????? */

#define TCU_TSTR		(0xF0)  /* Timer Status Register,Only Used In Tcu2 Mode */
#define TCU_TSTSR		(0xF4)  /* Timer Status Set Register */
#define TCU_TSTCR		(0xF8)  /* Timer Status Clear Register */
#define TCU_TSR			(0x1C)  /* Timer Stop Register */
#define TCU_TSSR		(0x2C)  /* Timer Stop Set Register */
#define TCU_TSCR		(0x3C)  /* Timer Stop Clear Register */
#define TCU_TER			(0x10)  /* Timer Counter Enable Register */
#define TCU_TESR		(0x14)  /* Timer Counter Enable Set Register */
#define TCU_TECR		(0x18)  /* Timer Counter Enable Clear Register */
#define TCU_TFR			(0x20)  /* Timer Flag Register */
#define TCU_TFSR		(0x24)  /* Timer Flag Set Register */
#define TCU_TFCR		(0x28)  /* Timer Flag Clear Register */
#define TCU_TMR			(0x30)  /* Timer Mask Register */
#define TCU_TMSR		(0x34)  /* Timer Mask Set Register */
#define TCU_TMCR		(0x38)  /* Timer Mask Clear Register */

#define CH_TDFR(n)		(0x40 + (n)*0x10)		/* Timer Data Full Reg */
#define CH_TDHR(n)		(0x44 + (n)*0x10)		/* Timer Data Half Reg */
#define CH_TCNT(n)		(0x48 + (n)*0x10)		/* Timer Counter Reg */
#define CH_TCSR(n)		(0x4C + (n)*0x10)		/* Timer Control Reg */

#define TCSR_PWM_BYPASS	(1 << 11)		/* clear counter to 0, only used in TCU2 mode */
#define TCSR_CNT_CLRZ	(1 << 10)		/* clear counter to 0, only used in TCU2 mode */
#define TCSR_PWM_SD		(1 << 9)		/* shut down the pwm output only used in TCU1 mode */
#define TCSR_PWM_HIGH	(1 << 8)		/* selects an initial output level for pwm output */
#define TCSR_PWM_EN		(1 << 7)		/* pwm pin output enable */
#define TCSR_PWM_IN		(1 << 6)		/* pwm pin output enable */

#endif	/* __TCU_H__ */
