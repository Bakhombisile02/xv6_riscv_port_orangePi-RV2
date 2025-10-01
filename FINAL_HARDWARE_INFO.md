# Final Hardware Configuration - READY TO CODE!

## OpenSBI Confirmed!
```
SBI specification: v1.0
SBI implementation ID: 0x1 (Xuantie/T-Head)
SBI Version: 0x10003 (v1.0.3)

Extensions detected:
- IPI (Inter-Processor Interrupt)
- RFENCE (Remote Fence)
- HSM (Hart State Management)
- SUSP (Suspend)
- PMU (Performance Monitoring)
```

## U-Boot Memory Addresses (from /boot/boot.cmd)
```bash
load_addr:       0x09000000  # Temporary load address
kernel_addr_r:   (check env) # Where kernel Image loaded
ramdisk_addr_r:  (check env) # Where initrd loaded
fdt_addr_r:      (check env) # Where device tree loaded
```

**Boot command**: `booti ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r}`
- Uses ARM64/RISC-V booti format
- Loads Image (raw kernel binary, not uImage!)
- Device tree passed as third argument

## UART Details (from Device Tree)
```
Compatible: "ky,pxa-uart"
Address:    0xd4017000
Size:       0x100 (256 bytes)
IRQ:        0x2a (42 decimal)
Clocks:     func=0x3a, gate=0xb4 (referenced)
Clock:      0xe11130 (14.745 MHz from clk-fpga!)
Resets:     <0x1d 0x01>
DMA:        RX channel 4, TX channel 3
Register:
  reg-shift:    0x02 (registers are 32-bit aligned!)
  reg-io-width: 0x04 (32-bit access required!)
Status:     okay
```

**CRITICAL**: `reg-shift = 0x02` means:
- Registers at offsets 0, 4, 8, 12, ... (not 0, 1, 2, 3 like 16550a!)
- Access as 32-bit words (reg-io-width = 4)
- This IS compatible with 16550a register layout, just wider spacing!

**UART Clock**: 14.745 MHz (0xe11130 hex)
- For 115200 baud: divisor = 14745600 / (16 * 115200) ≈ 8

## CPU Timebase
```
timebase-frequency: 0x16e3600 (24 MHz = 24,000,000 Hz)
```
This is the frequency of the `time` CSR - used for timing!

## Memory Map for XV6

Based on U-Boot boot.cmd, kernel likely loaded around:
- **0x40000000** - Common kernel load address for embedded RISC-V
- **0x09000000** - Temporary load location (too low for kernel)

### Recommended XV6 Configuration:
```c
// Assume kernel loaded at 0x40000000 (1GB)
#define KERNBASE 0x40000000L
#define PHYSTOP  0xC0000000L  // Use up to 3GB (2GB for kernel)

// Or try 0x80000000 (2GB) like QEMU
// #define KERNBASE 0x80000000L
// #define PHYSTOP  0x88000000L  // 128MB like QEMU
```

---

## Complete Hardware Map for XV6

### kernel/memlayout.h
```c
// Physical memory layout for OrangePi RV2

// qemu -machine virt is set up differently than OrangePi RV2.
// OrangePi RV2 with OpenSBI:
//
// 00000000 -- start of RAM (2GB region)
// ???????? -- OpenSBI firmware (M-mode)
// 40000000 -- kernel loads here (assumed)
// 40000000+ -- kernel text and data
// end      -- start of kernel page allocation area
// PHYSTOP  -- end RAM used by the kernel

// OpenSBI is in M-mode, we run in S-mode!
// No direct access to M-mode CSRs.

// UART0 (console) - PXA-style UART
#define UART0 0xd4017000L
#define UART0_IRQ 42  // Device tree value (verify with PLIC)

// OrangePi has no VirtIO
// #define VIRTIO0 removed

// PLIC (Platform Level Interrupt Controller)
#define PLIC 0xe0000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)

// S-mode context offsets (hart * 2 + 1 for S-mode)
// Each hart has M-mode context (n*2) and S-mode context (n*2+1)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + ((hart)*2+1)*0x80)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + ((hart)*2+1)*0x1000)  
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + ((hart)*2+1)*0x1000)

// Kernel memory layout
#define KERNBASE 0x40000000L
#define PHYSTOP (KERNBASE + 512*1024*1024)  // 512MB for now

// Trampoline and stacks stay the same
#define TRAMPOLINE (MAXVA - PGSIZE)
#define KSTACK(p) (TRAMPOLINE - ((p)+1)* 2*PGSIZE)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)
```

### kernel/uart.c Changes
```c
// PXA UART has registers at 32-bit offsets (reg-shift=2)
#define Reg(reg) ((volatile unsigned int *)(UART0 + ((reg)<<2)))

// Example: RHR is at offset 0, but physically at UART0+0
// THR is at offset 0, but physically at UART0+0
// IER is at offset 1, but physically at UART0+4
// etc.

#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

// UART clock is 14.745 MHz
// For 115200 baud: divisor = 14745600 / (16 * 115200) ≈ 8
void uartinit(void) {
  // ... existing init code ...
  
  // Set baud rate divisor (for 14.745 MHz clock)
  WriteReg(LCR, LCR_BAUD_LATCH);
  WriteReg(0, 8);  // LSB of divisor
  WriteReg(1, 0);  // MSB of divisor  
  WriteReg(LCR, LCR_EIGHT_BITS);
  
  // ... rest of init
}
```

### kernel/start.c Changes  
```c
// OpenSBI starts us in S-mode, NOT M-mode!
// Remove all M-mode CSR accesses

void start() {
  // We're already in S-mode thanks to OpenSBI
  // No M-mode setup needed!
  
  // Get hart ID (OpenSBI preserves this in a1 register)
  // OR use S-mode accessible tp register
  int id = r_tp();  // OpenSBI sets this up
  
  // If tp not set, we may need to read from a1
  // (passed by OpenSBI at entry)
  
  // Enable S-mode interrupts
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);
  
  // Set trap vector
  w_stvec((uint64)kernelvec);
  
  // Request timer interrupt via SBI
  // (or use sstc extension directly if available)
  
  // Jump to main - we're already in S-mode!
  main();
}
```

### kernel/entry.S Changes
```assembly
# OpenSBI loads kernel and jumps here in S-mode
# Hart ID may be in a1 register (OpenSBI convention)
.section .text
.global _entry
_entry:
    # Save hart ID from a1 (passed by OpenSBI)
    mv t0, a1
    
    # Set up stack
    la sp, stack0
    li a0, 1024*4
    mv a1, t0       # Use saved hart ID
    addi a1, a1, 1
    mul a0, a0, a1
    add sp, sp, a0
    
    # Store hart ID in tp for later use
    mv tp, t0
    
    # Jump to start() in start.c
    call start
spin:
    j spin
```

### kernel/sbi.h (NEW FILE)
```c
#ifndef SBI_H
#define SBI_H

struct sbiret {
  long error;
  long value;
};

// SBI Extension IDs  
#define SBI_EXT_BASE 0x10
#define SBI_EXT_TIME 0x54494D45
#define SBI_EXT_IPI  0x735049
#define SBI_EXT_RFNC 0x52464E43
#define SBI_EXT_HSM  0x48534D

// Legacy extensions (still widely used)
#define SBI_CONSOLE_PUTCHAR 0x01
#define SBI_CONSOLE_GETCHAR 0x02

// Function declarations
void sbi_console_putchar(int ch);
int sbi_console_getchar(void);
void sbi_set_timer(uint64 stime_value);
void sbi_send_ipi(unsigned long hart_mask);

#endif
```

### kernel/sbi.c (NEW FILE)
```c
#include "types.h"
#include "riscv.h"
#include "sbi.h"

static struct sbiret sbi_ecall(unsigned long ext, unsigned long fid,
                                unsigned long arg0, unsigned long arg1,
                                unsigned long arg2, unsigned long arg3,
                                unsigned long arg4, unsigned long arg5) {
  struct sbiret ret;
  register unsigned long a0 asm("a0") = arg0;
  register unsigned long a1 asm("a1") = arg1;
  register unsigned long a2 asm("a2") = arg2;
  register unsigned long a3 asm("a3") = arg3;
  register unsigned long a4 asm("a4") = arg4;
  register unsigned long a5 asm("a5") = arg5;
  register unsigned long a6 asm("a6") = fid;
  register unsigned long a7 asm("a7") = ext;
  
  asm volatile("ecall"
               : "+r"(a0), "+r"(a1)
               : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
               : "memory");
  
  ret.error = a0;
  ret.value = a1;
  return ret;
}

void sbi_console_putchar(int ch) {
  sbi_ecall(0x01, 0, ch, 0, 0, 0, 0, 0);
}

int sbi_console_getchar(void) {
  struct sbiret ret = sbi_ecall(0x02, 0, 0, 0, 0, 0, 0, 0);
  return ret.error;
}

void sbi_set_timer(uint64 stime_value) {
  sbi_ecall(SBI_EXT_TIME, 0, stime_value, 0, 0, 0, 0, 0);
}

void sbi_send_ipi(unsigned long hart_mask) {
  sbi_ecall(SBI_EXT_IPI, 0, hart_mask, 0, 0, 0, 0, 0);
}
```

---

## Build Instructions

### 1. Update Makefile
```makefile
# Add SBI support
OBJS = \
  $K/entry.o \
  $K/start.o \
  $K/sbi.o \      # NEW
  $K/console.o \
  # ... rest

# Update toolchain (already have riscv64-linux-gnu-)
TOOLPREFIX = riscv64-linux-gnu-

# Add compilation flag
CFLAGS += -DORANGEPI_RV2

# Kernel linker script - update load address
# Edit kernel/kernel.ld to start at 0x40000000
```

### 2. Build
```bash
cd /home/orangepi/xv6_riscv_port_orangePi-RV2

# Clean build
make clean

# Build kernel
make $K/kernel

# Create raw binary for U-Boot
riscv64-linux-gnu-objcopy -O binary $K/kernel $K/kernel.bin

# Copy to /boot (requires sudo)
sudo cp $K/kernel.bin /boot/xv6-kernel.bin
```

### 3. Boot
Edit /boot/orangepiEnv.txt to boot xv6:
```
# ... existing config ...
# Add custom boot command
# Or manually interrupt U-Boot and run:
# load mmc 0:1 ${kernel_addr_r} xv6-kernel.bin
# booti ${kernel_addr_r} - ${fdt_addr_r}
```

---

## Next Steps - Implementation Order

1. ✅ Hardware analysis DONE
2. ⏳ Create SBI files (sbi.h, sbi.c)
3. ⏳ Modify entry.S for S-mode entry
4. ⏳ Rewrite start.c for S-mode
5. ⏳ Update memlayout.h with addresses
6. ⏳ Update uart.c for PXA UART (reg-shift=2)
7. ⏳ Update plic.c for S-mode contexts
8. ⏳ Update kernel.ld for 0x40000000 load address
9. ⏳ Update Makefile
10. ⏳ Build and test!

**Ready to start coding!** 🚀
