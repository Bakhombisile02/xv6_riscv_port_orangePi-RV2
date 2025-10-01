# Complete Hardware Specification - OrangePi RV2 for XV6 Port

## Sources Analyzed
1. ✅ Ky X1 Chip Manual.pdf (169KB extracted text)
2. ✅ OrangePi RV2 User Manual v1.1.pdf (184KB extracted text)  
3. ✅ Device Tree: x1_orangepi-rv2.dtb (decompiled)
4. ✅ Running System Analysis (dmesg, /proc, /sys)

---

## SoC: Ky® X1 (Kylin Technology)

### CPU Architecture
```
Model: Ky® X60™ RISC-V Dual-Cluster 8-Core Processor
ISA: RISC-V 64GCVB + RVA22 standard
Voltage: 0.6V to 1.05V (DVFS)
Frequency: 614.4 MHz - 1.6 GHz

Cluster 0 (Performance + AI):
  - 4 cores with 2.0 TOPS AI computing power
  - 32KB L1-Cache per core (I+D)
  - 512KB L2-Cache (shared)
  - 512KB TCM (Tight-Coupled Memory for AI)
  - 256-bit vector units

Cluster 1 (Efficiency):
  - 4 cores
  - 32KB L1-Cache per core (I+D)
  - 512KB L2-Cache (shared)
  - 256-bit vector units

Cache Protocol:
  - L1: MESI consistency protocol
  - L2: MOESI consistency protocol
```

### Interrupt Controllers (from Chip Manual)
```
CLINT: Core Local Interrupt Controller (exists!)
PLIC: Platform Level Interrupt Controller
  - Base Address: 0xe0000000 (confirmed)
  - Total interrupts: 256 (manual says 256, device tree shows 159 active)
  - Contexts: 16 (2 per hart: M-mode + S-mode)
  - Both machine mode and supervisor mode supported
```

**IMPORTANT**: Manual confirms CLINT exists, contradicting initial assumption!
- Need to find CLINT base address (not in device tree because OpenSBI handles it)
- Standard RISC-V CLINT is typically at 0x02000000
- For S-mode kernel with OpenSBI, use SBI timer calls instead

### Memory System
```
DDR Support:
  - Dual-chip selection
  - 32-bit LPDDR4/LPDDR4x @ 2666 Mbps (up to 16GB)
  - 32-bit LPDDR3 @ 1866 Mbps (up to 4GB)
  
Physical Memory Layout (from device tree):
  Region 1: 0x00000000 - 0x7FFFFFFF (2GB)
  Region 2: 0x100000000 - 0x17FFFFFFF (2GB at 4GB+)
  
  Total accessible: 4GB in this configuration
  (Board has 8GB, but organized for compatibility)
```

### UART Configuration (from Chip Manual + Device Tree)
```
Total UARTs: 10 standard + 2 R-UART (Remote UART)

Standard: Compatible with 16550A and 16750 UART standards
Baud Rate: Up to 3.6 Mbps for Fast UARTs

UART0 (Console - from device tree):
  Compatible: "ky,pxa-uart"
  Base Address: 0xd4017000
  IRQ: 42 (0x2a in device tree)
  Clock: 14.745 MHz (0xe11130 from clk-fpga)
  
  Register Layout (CRITICAL):
    reg-shift: 2 (registers at 32-bit boundaries)
    reg-io-width: 4 (32-bit access required)
    
    Offset 0 → Physical 0x00 (RBR/THR)
    Offset 1 → Physical 0x04 (IER)
    Offset 2 → Physical 0x08 (IIR/FCR)
    Offset 3 → Physical 0x0C (LCR)
    etc.
  
  Modem Control: CTSn and RTSn supported
  DMA: Channels 3 (TX) and 4 (RX)
  
  Baud Rate Calculation:
    Divisor = Clock / (16 * BaudRate)
    For 115200 baud: 14745600 / (16 * 115200) = 8
```

### Timer Configuration
```
CPU Timebase Frequency: 24 MHz (0x16e3600 Hz)
  - Read from CSR time
  - Used for timing and scheduling
  
Timer Support:
  - SSTC extension present (Supervisor Timer Compare)
  - Can use stimecmp CSR directly
  - Or use SBI timer calls (recommended with OpenSBI)
  - CLINT exists but managed by OpenSBI in M-mode
```

### Storage Interfaces
```
Boot Storage:
  - SPI Flash: 0xd420c000 (for bootloader)
    Partitions: bootinfo(64K), private(64K), fsbl(256K),
                env(64K), opensbi(192K), uboot(rest)
  
  - SD/MMC Controllers (3):
    mmc0: 0xd4280000
    mmc1: 0xd4280800
    mmc2: 0xd4281000
  
  - M.2 NVMe: PCIe 2.0 2-Lane support
```

### Network
```
Ethernet Controllers (2):
  ethernet0: 0xcac80000
  ethernet1: 0xcac81000
  Type: Gigabit Ethernet
```

---

## Boot Sequence (Confirmed from User Manual + System)

### Stage 1: SPI Flash Boot ROM
```
Location: SPI Flash @ 0xd420c000
1. FSBL (First Stage Bootloader) - 256KB
2. OpenSBI firmware - 192KB
3. U-Boot bootloader - rest of space
```

### Stage 2: OpenSBI (M-mode)
```
Version: v1.0.3
Implementation: Xuantie/T-Head (ID: 0x1)
Extensions: IPI, RFENCE, HSM, SUSP, PMU

Responsibilities:
  - Runs in M-mode
  - Handles M-mode traps
  - Provides SBI services to S-mode
  - Manages CLINT (timer, IPI)
  - Manages PMP (Physical Memory Protection)
```

### Stage 3: U-Boot (S-mode)
```
Version: v2022.10
Toolchain: riscv64-unknown-linux-gnu-gcc 13.2.1

Boot Script: /boot/boot.cmd
Config: /boot/orangepiEnv.txt

Memory Addresses (from boot.cmd):
  load_addr:       0x09000000  (temporary)
  kernel_addr_r:   (not explicitly shown, likely 0x40000000 or 0x44000000)
  ramdisk_addr_r:  (for initrd)
  fdt_addr_r:      (for device tree)

Boot Command:
  booti ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r}
  
  Loads raw kernel Image (not uImage!)
  Passes device tree blob
  Kernel entry in S-mode
```

### Stage 4: Linux Kernel (S-mode)
```
Version: 6.6.63-ky
Boot: earlycon=sbi console=ttyS0,115200

OpenSBI passes control to kernel in S-mode:
  - a0: hartid (usually 0 for boot hart)
  - a1: pointer to device tree blob (FDT)
  - Kernel entry point from Image header
```

---

## XV6 Port Strategy - REVISED

### Critical Decision Point

**Option A: Run under OpenSBI (RECOMMENDED)**
```
Pros:
  + Simpler - use existing boot infrastructure
  + SBI provides timer, IPI, console
  + Standard RISC-V approach
  + Faster to implement

Cons:
  - Depends on OpenSBI
  - Can't access M-mode directly
  - Slight overhead on SBI calls

Implementation:
  1. Use U-Boot's booti to load xv6
  2. Enter kernel in S-mode (like Linux)
  3. Use SBI for timer/IPI/console
  4. Access UART directly once initialized
```

**Option B: Replace OpenSBI (ADVANCED)**
```
Pros:
  + Full hardware control
  + Educational value
  + No dependencies
  + Direct CLINT access

Cons:
  - Must write M-mode firmware
  - More complex
  - Harder to debug
  - Longer development time

Implementation:
  1. Write M-mode startup code
  2. Configure PMP, delegation
  3. Set up CLINT for timer
  4. Delegate to S-mode xv6
```

**RECOMMENDATION**: Start with Option A, consider Option B later

---

## Memory Map for XV6

### Physical Address Space
```
0x00000000 - 0x00FFFFFF   OpenSBI (M-mode, managed by firmware)
0x01000000 - 0x3FFFFFFF   Available for kernel (up to ~1GB)
0x40000000 - 0x7FFFFFFF   Available for kernel (up to ~1GB)
                          ↑ Likely kernel load address

Region 1 Total: 2GB (0x00000000 - 0x7FFFFFFF)

0x80000000 - 0xBFFFFFFF   Not present/reserved
0xC0000000 - 0xDFFFFFFF   MMIO region
  0xd4017000              UART0
  0xd4280000              SD/MMC
  0xd420c000              SPI Flash
0xE0000000 - 0xE3FFFFFF   PLIC (64MB)
0x100000000+              Region 2 (4GB+, 2GB)
```

### Recommended XV6 Configuration
```c
// Option 1: Standard embedded load address
#define KERNBASE 0x40000000L  // 1GB offset (common)
#define PHYSTOP  0x60000000L  // Use 512MB

// Option 2: High memory (similar to QEMU)
// #define KERNBASE 0x80000000L
// #define PHYSTOP  0x88000000L  // 128MB

// Hardware addresses
#define UART0 0xd4017000L
#define UART0_IRQ 42

#define PLIC 0xe0000000L
// S-mode context = (hart * 2) + 1
#define PLIC_SENABLE(hart)    (PLIC + 0x2080 + ((hart)*2+1)*0x80)
#define PLIC_SPRIORITY(hart)  (PLIC + 0x201000 + ((hart)*2+1)*0x1000)
#define PLIC_SCLAIM(hart)     (PLIC + 0x201004 + ((hart)*2+1)*0x1000)

// No VirtIO
// CLINT exists but accessed via SBI
```

---

## UART Driver Implementation

### Register Access Macros
```c
// PXA UART uses 32-bit aligned registers (reg-shift=2)
#define UART_REG(offset) \
    ((volatile uint32_t*)(UART0 + ((offset) << 2)))

#define UART_READ(offset)  (*UART_REG(offset))
#define UART_WRITE(offset, val)  (*UART_REG(offset) = (val))

// 16550A register offsets (logical)
#define UART_RBR  0  // Receive Buffer (read)
#define UART_THR  0  // Transmit Holding (write)
#define UART_IER  1  // Interrupt Enable
#define UART_IIR  2  // Interrupt Identification (read)
#define UART_FCR  2  // FIFO Control (write)
#define UART_LCR  3  // Line Control
#define UART_MCR  4  // Modem Control
#define UART_LSR  5  // Line Status
#define UART_MSR  6  // Modem Status
#define UART_SCR  7  // Scratch

// Physical addresses:
// UART_RBR/THR → 0xd4017000 + (0 << 2) = 0xd4017000
// UART_IER     → 0xd4017000 + (1 << 2) = 0xd4017004
// UART_LCR     → 0xd4017000 + (3 << 2) = 0xd401700C
// etc.
```

### Initialization Code
```c
void uartinit(void) {
    // Disable interrupts
    UART_WRITE(UART_IER, 0x00);
    
    // Enable FIFO, clear TX/RX
    UART_WRITE(UART_FCR, 0x07);
    
    // Set baud rate (14.745 MHz clock)
    // Divisor = 14745600 / (16 * 115200) = 8
    UART_WRITE(UART_LCR, 0x80);  // DLAB = 1
    UART_WRITE(0, 8);             // DLL = 8
    UART_WRITE(1, 0);             // DLH = 0
    
    // 8N1, DLAB = 0
    UART_WRITE(UART_LCR, 0x03);
    
    // Enable receive interrupts
    UART_WRITE(UART_IER, 0x01);
}

void uartputc(int c) {
    // Wait for THR empty
    while ((UART_READ(UART_LSR) & 0x20) == 0)
        ;
    UART_WRITE(UART_THR, c);
}

int uartgetc(void) {
    // Check if data available
    if (UART_READ(UART_LSR) & 0x01) {
        return UART_READ(UART_RBR);
    }
    return -1;
}
```

---

## SBI Interface (for OpenSBI mode)

### SBI Call Implementation
```c
// kernel/sbi.h
#define SBI_EXT_BASE       0x10
#define SBI_EXT_TIME       0x54494D45  // "TIME"
#define SBI_EXT_IPI        0x735049    // "sPI"
#define SBI_EXT_RFNC       0x52464E43  // "RFNC"
#define SBI_EXT_HSM        0x48534D    // "HSM"

// Legacy extensions
#define SBI_CONSOLE_PUTCHAR  0x01
#define SBI_CONSOLE_GETCHAR  0x02
#define SBI_SHUTDOWN         0x08

struct sbiret {
    long error;
    long value;
};

// kernel/sbi.c
struct sbiret sbi_ecall(long ext, long fid,
                        long arg0, long arg1,
                        long arg2, long arg3,
                        long arg4, long arg5) {
    struct sbiret ret;
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a3 asm("a3") = arg3;
    register long a4 asm("a4") = arg4;
    register long a5 asm("a5") = arg5;
    register long a6 asm("a6") = fid;
    register long a7 asm("a7") = ext;
    
    asm volatile("ecall"
        : "+r"(a0), "+r"(a1)
        : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
        : "memory");
    
    ret.error = a0;
    ret.value = a1;
    return ret;
}

void sbi_console_putchar(int ch) {
    sbi_ecall(SBI_CONSOLE_PUTCHAR, 0, ch, 0, 0, 0, 0, 0);
}

void sbi_set_timer(uint64 stime_value) {
    sbi_ecall(SBI_EXT_TIME, 0, stime_value, 0, 0, 0, 0, 0);
}

void sbi_send_ipi(unsigned long hart_mask) {
    sbi_ecall(SBI_EXT_IPI, 0, hart_mask, 0, 0, 0, 0, 0);
}
```

---

## Build and Deploy

### 1. Cross-Compilation Setup
```bash
# Already available on system
TOOLPREFIX=riscv64-linux-gnu-
CROSS_COMPILE=${TOOLPREFIX}
```

### 2. Linker Script (kernel/kernel.ld)
```ld
OUTPUT_ARCH(riscv)
ENTRY(_entry)

SECTIONS
{
  /* Load kernel at 1GB offset */
  . = 0x40000000;
  
  .text : {
    *(.text .text.*)
    . = ALIGN(0x1000);
    _trampoline = .;
    *(trampsec)
    . = ALIGN(0x1000);
  }
  
  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*)
    . = ALIGN(16);
    *(.rodata .rodata.*)
  }
  
  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*)
    . = ALIGN(16);
    *(.data .data.*)
  }
  
  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*)
    . = ALIGN(16);
    *(.bss .bss.*)
  }
  
  PROVIDE(end = .);
}
```

### 3. Build Commands
```bash
cd /home/orangepi/xv6_riscv_port_orangePi-RV2

# Build kernel
make kernel

# Create raw binary for U-Boot
riscv64-linux-gnu-objcopy -O binary kernel/kernel kernel/Image

# Copy to boot partition
sudo cp kernel/Image /boot/xv6-kernel.bin

# Create U-Boot boot script
cat > /tmp/boot-xv6.cmd << 'BOOTSCRIPT'
setenv xv6_addr 0x40000000
load mmc 0:1 ${xv6_addr} xv6-kernel.bin
load mmc 0:1 ${fdt_addr_r} dtb/ky/x1_orangepi-rv2.dtb
booti ${xv6_addr} - ${fdt_addr_r}
BOOTSCRIPT

mkimage -C none -A riscv -T script -d /tmp/boot-xv6.cmd /boot/boot-xv6.scr
```

### 4. Boot XV6
```bash
# Interrupt U-Boot at boot, then:
=> source mmc 0:1 boot-xv6.scr

# Or edit /boot/orangepiEnv.txt to change default boot
```

---

## Testing Checklist

### Phase 1: SBI Console Boot
- [ ] Kernel loads at 0x40000000
- [ ] Entry.S receives control in S-mode
- [ ] start() function executes
- [ ] SBI console prints "xv6 kernel is booting"
- [ ] Can see output on serial console

### Phase 2: Direct UART
- [ ] UART init with reg-shift=2
- [ ] Baud rate 115200 (divisor=8)
- [ ] Can print characters
- [ ] Can receive characters
- [ ] Switch from SBI to direct UART

### Phase 3: Interrupts
- [ ] PLIC initialized with S-mode contexts
- [ ] UART interrupts enabled (IRQ 42)
- [ ] Timer interrupts via SBI
- [ ] Trap handler works
- [ ] Context switching functional

### Phase 4: Multi-core
- [ ] All 8 cores start
- [ ] Per-core stacks set up
- [ ] Spinlocks work
- [ ] IPI via SBI works

### Phase 5: Virtual Memory
- [ ] Page tables created
- [ ] satp configured
- [ ] User/kernel transitions work
- [ ] TLB shootdown via SBI RFENCE

### Phase 6: User Programs
- [ ] First user process created
- [ ] System calls work
- [ ] Shell runs!

---

## Summary of Changes from Original Plan

### New Information from PDFs:
1. **CLINT exists** - mentioned in chip manual (256 total interrupts with PLIC)
2. **Confirmed UART clock**: 14.745 MHz (from device tree clk-fpga)
3. **Confirmed timebase**: 24 MHz (from device tree)
4. **U-Boot version**: v2022.10
5. **Boot load address**: Likely 0x40000000 (standard for embedded)

### Information Unchanged (Device Tree was Complete):
- PLIC address: 0xe0000000 ✓
- UART address: 0xd4017000 ✓
- Memory regions: 2GB @ 0x0, 2GB @ 0x100000000 ✓
- OpenSBI present: v1.0.3 ✓
- reg-shift=2 for UART ✓

### Final Recommendation:
The device tree analysis was actually MORE complete than the PDF manuals!
All critical information for the port was already extracted.

**READY TO BEGIN IMPLEMENTATION** 🚀

Start with:
1. Create kernel/sbi.c and kernel/sbi.h
2. Modify kernel/entry.S for S-mode entry
3. Rewrite kernel/start.c (remove M-mode)
4. Update kernel/memlayout.h
5. Update kernel/uart.c (reg-shift=2)
6. Build and test!
