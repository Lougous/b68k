// This module manages one of the two interrupt source of the MFP, which
// collects both PS/2 ports, the debug serial interface and clock timer.
// see system.c for MFP system timer management.

#include <types.h>

#include "config.h"
#include "mem.h"
#include "proc.h"
#include "irq.h"
#include "debug.h"
#include "b68k.h"

#define _K_IRQ_COUNT   4

// IRQ services
struct irq_attr_t {
  u8_t enable;
  irq_fct_t handler;
};

struct irq_attr_t _irq_handlers[_K_IRQ_COUNT];

void __attribute__ ((interrupt)) irq_handler ()
{
  u16_t irq_sts;
  u8_t mfp_ad_save;

  {
    u16_t lbkp = k_lock();

    // save MFP address
    mfp_ad_save = B68K_MFP->ad;

    // read status
    B68K_MFP->ad = B68K_MFP_REG_IRQSTS;
    irq_sts = B68K_MFP->dt;

    // reset status
    B68K_MFP->dt = irq_sts;

    // restore MFP address
    B68K_MFP->ad = mfp_ad_save;
    
    k_unlock(lbkp);
  }

  // IRQ_TIMER
  if (irq_sts & B68K_MFP_IRQ_RTC_MASK) {
    if (_irq_handlers[IRQ_CLOCK].enable && _irq_handlers[IRQ_CLOCK].handler) {
      _irq_handlers[IRQ_CLOCK].handler();
    }
  }
  
  // IRQ_KEYBOARD
  if (irq_sts & B68K_MFP_IRQ_PS2P0_MASK) {
    if (_irq_handlers[IRQ_KEYBOARD].enable && _irq_handlers[IRQ_KEYBOARD].handler) {
      _irq_handlers[IRQ_KEYBOARD].handler();
    }
  }

  // IRQ_MOUSE
  if (irq_sts & B68K_MFP_IRQ_PS2P1_MASK) {
    if (_irq_handlers[IRQ_MOUSE].enable && _irq_handlers[IRQ_MOUSE].handler) {
      _irq_handlers[IRQ_MOUSE].handler();
    }
  }

  // IRQ_DBGSERIAL
  if (irq_sts & B68K_MFP_IRQ_SERIAL_MASK) {
    if (_irq_handlers[IRQ_DBGSERIAL].enable && _irq_handlers[IRQ_DBGSERIAL].handler) {
      _irq_handlers[IRQ_DBGSERIAL].handler();
    }
  }

 // end of interrupt
}


void irq_init (void)
{
  int i;

  for (i = 0; i < _K_IRQ_COUNT; i++) {
    _irq_handlers[i].enable  = 0;
    _irq_handlers[i].handler = 0;
  }

  // setup interrupt vector
  B68K_IRQ_VECTOR[B68K_MFP_HI_IRQ] = (u32_t)irq_handler;

  // enable MFP interrupts
  B68K_MFP->ad = B68K_MFP_REG_IRQCFG;
  B68K_MFP->dt =
    B68K_MFP_IRQ_RTC_MASK |
    B68K_MFP_IRQ_PS2P0_MASK |
    B68K_MFP_IRQ_PS2P1_MASK |
    B68K_MFP_IRQ_SERIAL_MASK;
}

void irq_register_interrupt (u32_t irqn, irq_fct_t fct)
{
  if (irqn < _K_IRQ_COUNT) _irq_handlers[irqn].handler = fct;
}

void irq_enable_interrupt (u32_t irqn)
{
  if (irqn < _K_IRQ_COUNT) {
    _irq_handlers[irqn].enable = 1;
  }
}

void irq_disable_interrupt (u32_t irqn)
{
  if (irqn < _K_IRQ_COUNT) {
    _irq_handlers[irqn].enable = 0;
  }
}

