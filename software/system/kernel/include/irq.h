
#ifndef _irq_h_
#define _irq_h_

#define IRQ_NONE      -1
#define IRQ_CLOCK     0
#define IRQ_KEYBOARD  1
#define IRQ_MOUSE     2
#define IRQ_DBGSERIAL 3

typedef void (* irq_fct_t)(void);

extern void irq_init (void);
extern void irq_register_interrupt(u32_t irqn, irq_fct_t fct);
extern void irq_enable_interrupt(u32_t irqn);
extern void irq_disable_interrupt(u32_t irqn);
//extern void irq_mask(void);
//extern void irq_unmask(void);

extern void __start_irq(void);
extern void __exit_irq();

#endif /* _irq_h_ */
