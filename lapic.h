#ifndef _LAPIC_H
#define _LAPIC_H

#include <linux/types.h>

struct apic_ops {
    u32 (*reg_read)(unsigned reg);
    void (*reg_write)(unsigned reg, u32 val);
    void (*icr_write)(u32 val, u32 dest);
    u32 (*id)(void);
};

void dump_esr(void);
int isr_bit_set(u32 vector);
int irr_bit_set(u32 vector);
int apic_mode(void);

#endif // _LAPIC_H
