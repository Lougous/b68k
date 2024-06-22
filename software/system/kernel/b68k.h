
#ifndef _b68k_h_
#define _b68k_h_

/* base addresses depend on slots */
#define B68K_MFP_ADDRESS  0xFFF00000
#define B68K_AV_ADDRESS   0xFFF80000  // J10
#define B68K_IO_ADDRESS   0xFFF40000  // J11
#define B68K_MMU_ADDRESS  0xFFFE0000

/* 68k CPU auto vectors */
#define B68K_IRQ_VECTOR ((volatile u32_t *)0x60) 

#define B68K_SPURIOUS_IRQ  0
#define B68K_MFP_LO_IRQ    1
#define B68K_MIO_IRQ       2
#define B68K_AV_IRQ        3
#define B68K_MFP_HI_IRQ    6  // +4 +5

/* registers structures */
#define B68K_MFP ((volatile struct b68k_mfp_regs_t *)B68K_MFP_ADDRESS)
#define B68K_IO  ((volatile struct b68k_io_regs_t *)B68K_IO_ADDRESS)
#define B68K_MMU ((volatile struct b68k_mmu_regs_t *)B68K_MMU_ADDRESS)

#define B68K_AV_AVMGR_ADDRESS   (B68K_AV_ADDRESS + 0)
#define B68K_AV_FLEX_ADDRESS    (B68K_AV_ADDRESS + 0x80)
#define B68K_AV_RAMDAC_ADDRESS  (B68K_AV_ADDRESS + 0x100)
#define B68K_AV_OPL2_ADDRESS    (B68K_AV_ADDRESS + 0x180)
#define B68K_AV_VRAM_ADDRESS    (B68K_AV_ADDRESS + 0x10000)

#define B68K_AV_AVMGR  ((volatile struct b68k_av_avmgr_regs_t *)B68K_AV_AVMGR_ADDRESS)
#define B68K_AV_FLEX   ((volatile struct b68k_av_flex_regs_t *)B68K_AV_FLEX_ADDRESS)
#define B68K_AV_RAMDAC ((volatile struct b68k_av_ramdac_regs_t *)B68K_AV_RAMDAC_ADDRESS)
#define B68K_AV_OPL2   ((volatile struct b68k_av_opl2_regs_t *)B68K_AV_OPL2_ADDRESS)

struct b68k_mfp_regs_t {
  u8_t dt;
  u8_t ad;
  u8_t stub[2];
};

#define B68K_MFP_REG_FLASH_DATA  0  // boot flash data - address auto increment

#define B68K_MFP_REG_TICKS       3  // 4-us ticks counter, 0-249

#define B68K_MFP_REG_UART_STS    4  // UART status
  //   bit 7: TX ready
  //   bit 1: RX FIFO overflow
  //   bit 0: RX FIFO underflow
#define B68K_MFP_REG_UART_DATA   5  // UART data in/out

#define B68K_MFP_REG_POST        7  // POST code

#define B68K_MFP_REG_PS2P0_STS   8  // PS/2 port 0 status
  // bit 0: data available
#define B68K_MFP_REG_PS2P0_DATA  9  // PS/2 port 0 data

#define B68K_MFP_REG_PS2P1_STS   10  // PS/2 port 1 status
  // bit 0: data available
#define B68K_MFP_REG_PS2P1_DATA  11  // PS/2 port 1 data

#define B68K_MFP_REG_SYSCFG      12  // system tick 
  // bit 0: enable timer interrupt
  // bit 5: set by bootstap loader to prevent flash to RAM boot transfer
  // bit 6: enable self immediate interrupt
  // bit 7: reset !

#define B68K_MFP_REG_SYSSTS      13  // system tick 
  // bit 0: timer interrupt pending
  // bit 6: self immediate interrupt pending

#define B68K_MFP_SYS_IRQ_TMR_MASK  0x01
#define B68K_MFP_SYS_RAM_BOOT      0x20
#define B68K_MFP_SYS_IRQ_SELF_MASK 0x40
#define B68K_MFP_SYS_RESET_MASK    0x80

#define B68K_MFP_REG_IRQCFG      14  // interrupt controller
  // bit 0: enable RTC interrupt
  // bit 1: enable PS/2 keyboard interrupt
  // bit 2: enable PS/2 mouse interrupt
  // bit 3: enable UART RX interrupt

#define B68K_MFP_REG_IRQSTS      15  // interrupt controller
  // bit 1: RTC interrupt pending
  // bit 1: PS/2 keyboard interrupt pending
  // bit 2: PS/2 mouse interrupt pending
  // bit 3: UART RX interrupt pending

#define B68K_MFP_IRQ_RTC_MASK    0x01
#define B68K_MFP_IRQ_PS2P0_MASK  0x02
#define B68K_MFP_IRQ_PS2P1_MASK  0x04
#define B68K_MFP_IRQ_SERIAL_MASK 0x08

// AV sub area
#define B68K_AV_AVMGR_STS_nSTATUS_MASK   0x01
#define B68K_AV_AVMGR_STS_CONF_DONE_MASK 0x02
#define B68K_AV_AVMGR_STS_nCONFIG_MASK   0x40
#define B68K_AV_AVMGR_STS_RSTn_MASK      0x80

#define B68K_AV_AVMGR_CFG_DAT0_MASK      0x01

#define B68K_AV_AVMGR_GPIO_B0_MASK       0x01
#define B68K_AV_AVMGR_GPIO_B1_MASK       0x02
#define B68K_AV_AVMGR_GPIO_B2_MASK       0x80

struct b68k_av_avmgr_regs_t {
  u8_t cfgnsts;
  // 0: nSTATUS
  // 1: CONF_DONE
  // 6: nCONFIG
  // 7: RSTn
  u8_t  stub0;
  u8_t dat;
  // 0: DAT0
  u8_t  stub1;
  u8_t  gpio_out;
  // 0: GPIO0
  // 1: GPIO1
  // 7: GPIO2
  u8_t  stub2;
  u8_t  gpio_in;
  // 0: GPIO0
  // 1: GPIO1
  // 7: GPIO2
  u8_t  stub3;
};

#define B68K_AV_FLEX_CFGSTS_GPU_BUSY_MASK       0x80
#define B68K_AV_FLEX_CFGSTS_APU_PCM_HFULL_MASK  0x40

struct b68k_av_flex_regs_t {
  // FLEX internal registers
  // /!\ ALL READ ACCESSES POINT TO FIRST REGISTER
  u8_t cfgsts;
  // configuration and status
  //   0: (rw) interrupt enable
  //   1: (rw) interrupt pending (write 1 to clear)
  //   6: (r)  APU PCM FIFO half full
  //   7: (r)  GPU busy
  u8_t rbcfg0_vbk;
  // read back and VRAM banking configuration
  //   0: read back address bit 16
  //   1: read back mode
  //     0: mixed low-res 128 colors / hi-res 16 colors with tile restrictions
  //     1: low-res 256 colors
  //   2: read back buffer select
  //     0: use VRAM address range 20000h-3FFFFh
  //     1: use VRAM address range 60000h-7FFFFh
  //   3: hi-res color selection
  //     0: use colors 128-191 for hi-res pixels
  //     1: use colors 192-255 for hi-res pixels
  //   6-4: bank selection (address bits 18-16) for VRAM access
  u8_t rbcfg1;
  //   7-0: read back address bits 7-0
  u8_t rbcfg2;
  //   7-0: read back address bits 15-8
  u8_t gpu0;
  // GPU register 0
  //  stop display list in progress if any and set display list address bits (8:1)
  u8_t gpu1;
  // GPU register 1
  //  set display list address bits (16:9) and start display list
  u8_t apu0;
  // APU register 0
  u8_t apu1;
  // APU register 1
};

struct b68k_av_ramdac_regs_t {
  // RAMDAC registers
  u8_t rd_ad_write;  // Address Register (RAM Write Mode)
  u8_t stub0;
  u8_t rd_color;     // Color Palette RAM
  u8_t stub1;
  u8_t rd_mask;      // Pixel read Mask Register
  u8_t stub2;
  u8_t rd_ad_read;   // Address Register (RAM Read Mode)
  u8_t stub3;
};

struct b68k_av_opl2_regs_t {
  // OPL2
  u8_t opl2_regad_sts;  // Address (write) and status (read) Register
  u8_t stub0;
  u8_t opl2_regdt;      // Data
  u8_t stub1;
};


struct b68k_io_regs_t {
  u8_t dt;
  u8_t ad;
  u8_t stub[2];
};

// register addresses

#define B68K_IO_REG_SPI_SEL   0
//   bit 5: SEL2   rw
//   bit 4: SEL1   rw
//   bit 3: SEL0   rw
#define B68K_IO_REG_SPI_CTRL  1
//   bit 7: BUSY   r
//   bit 2: CLRBF  s
//   bit 1: TXR    rw
//   bit 0: RXR    rw
#define B68K_IO_REG_SPI_DATA  2
#define B68K_IO_REG_UART_STS  4
//   bit 7: TX ready
//   bit 0: RX FIFO underflow
#define B68K_IO_REG_UART_DATA 5
#define B68K_IO_REG_I2C_CTRL  6
//   bit 7: RXIF
//   bit 5: TXBE
//   bit 2: reset
//   bit 1: start
//   bit 0: RXBF
#define B68K_IO_REG_I2C_CNT   7
#define B68K_IO_REG_I2C_DATA  8
#define B68K_IO_REG_I2C_CMD   9
//   bit 7..1: address
//   bit 0:    RWn
#define B68K_IO_REG_IRQCFG    14  // interrupt controller
//   bit 0: serial interrupt
#define B68K_IO_REG_IRQSTS    15  // interrupt controller
//   bit 0: serial interrupt pending


// B68K_IO_REG_SPI_CTRL bits
#define B68K_IO_SPI_BUSY_MASK   0x80
#define B68K_IO_SPI_CLRBF_MASK  0x04
#define B68K_IO_SPI_CLRBF_TXR   0x02
#define B68K_IO_SPI_CLRBF_RXR   0x01

// B68K_IO_REG_SPI_SEL SPI chip selects
#define B68K_IO_SPI_SEL_PSX0    (0 << 3)
#define B68K_IO_SPI_SEL_PSX1    (1 << 3)
#define B68K_IO_SPI_SEL_PSX2    (2 << 3)
#define B68K_IO_SPI_SEL_PSX3    (3 << 3)
#define B68K_IO_SPI_SEL_SDA     (4 << 3)
#define B68K_IO_SPI_SEL_SDB     (5 << 3)
#define B68K_IO_SPI_SEL_SPARE   (6 << 3)
#define B68K_IO_SPI_SEL_NONE    (7 << 3)

#define B68K_IO_IRQ_SERIAL_MASK  0x01


struct b68k_mmu_regs_t {
  u16_t base_lo;
  u16_t base_hi;
  u16_t mask_lo;
  u16_t mask_hi;
};

#endif /* _b68k_h_ */
