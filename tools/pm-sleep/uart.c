#include <common.h>
#include <uart.h>
#include <cpm.h>

void udelay(int d)
{
	do{
		__asm__ __volatile__ ("nop \n\t");
		d -= (1000 + 23)/ 24 * 2;
	}while(d > 0);

}

#ifdef DEBUG

#define OFF_TDR         (0x00)
#define OFF_LCR         (0x0C)
#define OFF_LSR         (0x14)

#define LSR_TDRQ        (1 << 5)
#define LSR_TEMT        (1 << 6)

static unsigned int U_IOBASE = (0x10030000 + 0xa0000000);
static unsigned int U_ADDR = 0;
void serial_setid(int uart_id)
{
	if(uart_id != 0xff)
		U_ADDR = U_IOBASE + uart_id * 0x1000;
	else
		U_ADDR = 0;
}
void serial_putc(char x)
{
	if(U_ADDR){
		REG32(U_ADDR + OFF_TDR) = x;
		while ((REG32(U_ADDR + OFF_LSR) & (LSR_TDRQ | LSR_TEMT)) != (LSR_TDRQ | LSR_TEMT));
	}
}
void serial_put_hex(unsigned int x)
{
	int i;
	unsigned int d;
	for(i = 7;i >= 0;i--) {
		d = (x  >> (i * 4)) & 0xf;
		if(d < 10) d += '0';
		else d += 'A' - 10;
		serial_putc(d);
	}
}
#else
void serial_setid(int uart_id)
{
}
void serial_putc(char x)
{
}
void serial_put_hex(unsigned int x)
{
}
#endif
