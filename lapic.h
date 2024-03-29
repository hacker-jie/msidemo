#ifndef _LAPIC_H
#define _LAPIC_H

#include <linux/types.h>

struct apic_ops {
    u32 (*reg_read)(unsigned reg);
    void (*reg_write)(unsigned reg, u32 val);
    void (*icr_write)(u32 val, u32 dest);
    u32 (*id)(void);
};

u32 apic_read(unsigned reg);
void apic_write(unsigned reg, u32 val);
u32 apic_id(void);
void dump_esr(void);
void dump_isr(void);
void dump_irr(void);
int isr_bit_set(u32 vector);
int irr_bit_set(u32 vector);
int apic_mode(void);
int enable_x2apic(void);

#endif // _LAPIC_H
