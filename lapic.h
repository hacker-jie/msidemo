#ifndef _LAPIC_H
#define _LAPIC_H

#include <linux/types.h>

void dump_esr(void);
int isr_bit_set(u32 vector);
int irr_bit_set(u32 vector);

#endif // _LAPIC_H
