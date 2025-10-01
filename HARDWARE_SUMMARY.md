# OrangePi RV2 Hardware Summary

## What We Know (Confirmed from System)

### CPU: Ky(R) X1
- **Vendor**: Kylin (Chinese RISC-V vendor, mvendorid: 0x710)
- **Model**: X1 (marchid: 0x8000000058000001)
- **Cores**: 8 cores (harts 0-7)
- **Architecture**: rv64imafdcv + extensive extensions
- **MMU**: sv39 (39-bit virtual addressing)
- **Frequency**: 614.4 MHz - 1.6 GHz

### ISA Extensions (Full List)
```
rv64imafdcv_zicbom_zicboz_zicntr_zicond_zicsr_zifencei_
zihintpause_zihpm_zfh_zfhmin_zca_zcd_zba_zbb_zbc_zbs_zkt_
zve32f_zve32x_zve64d_zve64f_zve64x_zvfh_zvfhmin_zvkt_
sscofpmf_sstc_svinval_svnapot_svpbmt
```

**Key Extensions for OS**:
- `sstc`: Supervisor Timer Compare (no CLINT needed!)
- `svinval`: TLB invalidation
- `svnapot`: Naturally-aligned power-of-2
- `svpbmt`: Page-based memory types
- Vector extensions: Full vector support

### Memory
- **Total RAM**: 8 GB
- **Regions** (from device tree):
  - memory@0: Low memory region
  - memory@100000000: Main memory at 4GB physical
- **CMA Reserved**: 384 MiB at 0x58000000

### Cache
- **L1 Data**: 256 KiB total (32 KiB × 8 cores)
- **L1 Instruction**: 256 KiB total (32 KiB × 8 cores)  
- **L2**: 1 MiB (2 instances, 512 KiB each)

### PLIC (Platform-Level Interrupt Controller)
- **Base Address**: 0xe0000000
- **Interrupt Sources**: 159
- **Handlers**: 8 (one per core)
- **Contexts**: 16 (likely 2 per core: M-mode + S-mode)

### UARTs (3 available)
| Device | Base Address | IRQ | Baud Base | Type   | Usage          |
|--------|--------------|-----|-----------|--------|----------------|
| UART0  | 0xd4017000   | 74  | 921250    | UART1  | Console (ttyS0)|
| UART1  | 0xc088d000   | 42330| 1536000  | UART2  | R-UART (ttyS1) |
| UART2  | 0xd4017100   | 75  | 3600000   | UART3  | (ttyS2)        |

**Console UART** (UART0):
- Address: **0xd4017000**
- IRQ: **74**
- Type: Likely 16550a-compatible
- Current baud: 115200 (from kernel command line)

### Other Peripherals (Detected)
- **USB Controllers**: dwc3 @ 0xc0a00000
- **I2S**: Multiple @ 0xd4026000, 0xd4026800
- **Display**: DPU @ 0xc0440000, HDMI @ 0xc0400500
- **Video**: VI @ 0xc0230000, JPU @ 0xc02f8000
- **SPI**: 0xd420c000 (used for boot?)
- **GPIO**: TBD
- **SD/MMC**: TBD

### Current Boot Configuration
From kernel command line:
```
console=ttyS0,115200 console=tty1
earlycon=sbi
root=UUID=8be26b3d-4da2-4f2c-81d3-d7ab81b072bf
rootfstype=ext4
```

**Observations**:
- Uses OpenSBI for early console
- Boots from SD card (ext4 root filesystem)
- Serial console on ttyS0 @ 115200 baud
- Linux kernel built with 6.6.63-ky

---

## What We Need from PDFs

### Critical (Must Have)
1. **Kernel Load Address**: Where to place xv6 kernel in memory
2. **UART Clock**: Source frequency for baud rate calculation
3. **Memory Map**: Exact RAM layout and MMIO regions
4. **Boot Protocol**: M-mode vs S-mode entry, OpenSBI usage

### Important (Should Have)
5. **PLIC Register Layout**: Exact offsets for priority/enable/claim
6. **Timer Configuration**: How to use sstc (stimecmp)
7. **Cache Line Size**: For proper alignment
8. **Multi-core Boot**: How to bring up secondary cores

### Nice to Have
9. **SD/MMC Controller**: For storage support later
10. **Device Tree Format**: If we want to parse it
11. **GPIO Controller**: For status LEDs
12. **Ethernet Controller**: For network support

---

## Comparison: XV6 QEMU vs OrangePi RV2

| Component        | QEMU (current)  | OrangePi RV2      | Status |
|------------------|-----------------|-------------------|--------|
| **CPU Cores**    | 1-8 (configurable)| 8 (fixed)       | ✓ OK   |
| **UART Base**    | 0x10000000      | 0xd4017000        | ⚠ CHANGE|
| **UART IRQ**     | 10              | 74                | ⚠ CHANGE|
| **PLIC Base**    | 0x0c000000      | 0xe0000000        | ⚠ CHANGE|
| **CLINT**        | 0x02000000      | N/A (uses sstc)   | ⚠ CHANGE|
| **RAM Base**     | 0x80000000      | TBD (0x00000000?) | ❓ CHECK|
| **RAM Size**     | 128 MB          | 8 GB              | ✓ OK   |
| **Disk**         | VirtIO          | SD/MMC or remove  | ⚠ CHANGE|
| **Timer**        | CLINT           | SSTC (stimecmp)   | ⚠ CHANGE|
| **MMU**          | sv39            | sv39              | ✓ OK   |
| **Extensions**   | RV64GC          | RV64GCV + more    | ✓ OK   |

---

## Key Differences from QEMU

### 1. No CLINT (Core Local Interruptor)
- **QEMU**: Uses CLINT at 0x02000000 for timers
- **OrangePi**: Uses **sstc extension** (stimecmp CSR)
- **Impact**: Must rewrite timer code in `start.c`

### 2. Different UART
- **QEMU**: 16550a at 0x10000000, IRQ 10
- **OrangePi**: UART at 0xd4017000, IRQ 74
- **Impact**: Update `memlayout.h` and possibly `uart.c`

### 3. Different PLIC Address
- **QEMU**: PLIC at 0x0c000000
- **OrangePi**: PLIC at 0xe0000000  
- **Impact**: Update `memlayout.h`, verify `plic.c` offsets

### 4. More Cores
- **QEMU**: Typically tested with 2-4 cores
- **OrangePi**: 8 cores always present
- **Impact**: Test multi-core initialization thoroughly

### 5. Real Hardware Timing
- **QEMU**: Deterministic, forgiving timing
- **OrangePi**: Real hardware, strict timing requirements
- **Impact**: May need delays, proper synchronization

### 6. No VirtIO
- **QEMU**: Uses VirtIO for disk
- **OrangePi**: Need real SD/MMC driver or remove storage
- **Impact**: Major driver development or run diskless

---

## Development Strategy

### Phase 1: Minimal Boot (UART only)
Start with absolute minimum:
- Remove VirtIO disk driver
- Remove filesystem
- Just boot, initialize UART, print "Hello"
- Single core only initially

### Phase 2: Multi-Core
Once single core works:
- Enable all 8 cores
- Verify per-core stacks
- Test spinlocks and atomics

### Phase 3: Interrupts and Timers
Add interrupt support:
- Configure PLIC
- Enable UART interrupts
- Implement timer using sstc
- Test context switching

### Phase 4: Virtual Memory
Enable MMU and processes:
- Set up page tables
- Create first user process
- Test user/kernel transitions

### Phase 5: Storage (Optional)
If needed:
- Write SD/MMC driver
- Or boot rootfs over network
- Or keep as diskless system

---

## Immediate Next Steps

1. **You**: Extract info from PDFs (use PDF_EXTRACTION_GUIDE.md)
   - Focus on kernel load address
   - UART clock frequency
   - Memory map
   - Boot protocol

2. **Me**: Once you provide the info:
   - Create `kernel/orangepi_rv2.h` with all addresses
   - Update `memlayout.h` with OrangePi values
   - Modify `uart.c` if needed
   - Update `start.c` for sstc timer
   - Modify `plic.c` for correct addresses
   - Update Makefile for bare-metal build

3. **Us**: Build and test:
   - Compile first version
   - Deploy to SD card
   - Boot and debug via serial console
   - Iterate until "Hello World" works

---

## Files Created

In the repository, you now have:
- **PORTING_PLAN.md**: Detailed multi-phase porting strategy
- **PDF_EXTRACTION_GUIDE.md**: What to look for in documentation
- **HARDWARE_SUMMARY.md**: This file - what we know so far
- **README**: Original xv6 README
- **kernel/**: xv6 kernel source (unmodified)
- **user/**: xv6 user programs (unmodified)

Next step: Create modified versions based on PDF information!
