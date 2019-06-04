#include <linux/kernel.h>
#include "apic-defs.h"
#include "lapic.h"

static u32 x2apic_read(unsigned reg)
{
	unsigned a, d;

	asm volatile ("rdmsr" : "=a"(a), "=d"(d) : "c"(APIC_BASE_MSR + reg/16));
	return a | (u64)d << 32;
}

static void x2apic_write(unsigned reg, u32 val)
{
	asm volatile ("wrmsr" : : "a"(val), "d"(0), "c"(APIC_BASE_MSR + reg/16));
}

void dump_esr(void)
{
	u32 esr;
	
	x2apic_write(APIC_ESR, 0);
	esr = x2apic_read(APIC_ESR);

	printk("[ESR] send ill = %x, received ill = %x, ill reg access = %x, redir ipi = %x\n",
		(esr & APIC_ESR_SENDILL) ? 1 : 0,
		(esr & APIC_ESR_RECVILL) ? 1 : 0,
		(esr & APIC_ESR_ILLREGA) ? 1 : 0,
		(esr & 0x10) ? 1 : 0);
}

int isr_bit_set(u32 vector)
{
	u32 isr, offset, bit;

	offset = vector / 32;
	bit = vector % 32;

	isr = x2apic_read(APIC_ISR + offset * 16);
	return (isr & (1 << bit)) ?  1 : 0;  
}

int irr_bit_set(u32 vector)
{
	u32 irr, offset, bit;

	offset = vector / 32;
	bit = vector % 32;

	irr = x2apic_read(APIC_IRR + offset * 16);
	return (irr & (1 << bit)) ?  1 : 0;  
}
