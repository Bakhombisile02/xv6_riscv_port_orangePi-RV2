# XV6-RISCV Port to OrangePi RV2 (Bare Metal)

## Hardware Overview - OrangePi RV2

### CPU Information
- **SoC**: Ky(R) X1 (Custom Chinese RISC-V chip)
- **Architecture**: rv64imafdcv with extensive extensions
- **Cores**: 8-core (harts 0-7)
- **ISA Extensions**: 
  - Base: rv64imafdcv (Integer, Multiply, Atomic, Float, Double, Compressed, Vector)
  - Zicbom, Zicboz (Cache management)
  - Zba, Zbb, Zbc, Zbs (Bit manipulation)
  - Vector extensions: zve32f, zve32x, zve64d, zve64f, zve64x
  - Crypto: zkt, zvkt
  - Supervisor: sstc (Stimecmp), svinval, svnapot, svpbmt
- **MMU**: sv39 (39-bit virtual addressing)
- **Cache**: 
  - L1d: 256 KiB (8 instances, 32KB per core)
  - L1i: 256 KiB (8 instances, 32KB per core)
  - L2: 1 MiB (2 instances)
- **Frequency**: 614.4 MHz - 1.6 GHz
- **Memory**: 8GB RAM

### Current Detected Hardware (from dmesg/iomem)
- **PLIC**: 0xe0000000 (159 interrupts, 8 handlers, 16 contexts)
- **UART0 (Console)**: 0xd4017000 (ttyS0, IRQ 74, base_baud 921250)
- **UART1 (R-UART)**: 0xc088d000 (ttyS1, IRQ 42330, base_baud 1536000) 
- **UART2**: 0xd4017100 (ttyS2, IRQ 75, base_baud 3600000)

### Memory Layout (Current System)
- Memory regions detected:
  - memory@0 (likely low memory region)
  - memory@100000000 (likely main memory starting at 4GB)
- CMA pool: 0x58000000 (384 MiB)
- Kernel likely loads at 0x40000000 or similar (need to verify from PDFs)

---

## Current XV6-RISCV (QEMU) Configuration

### Memory Map (QEMU virt machine)
```
0x00001000  - Boot ROM (QEMU provided)
0x02000000  - CLINT (Core Local Interruptor)
0x0C000000  - PLIC (Platform Level Interrupt Controller)
0x10000000  - UART0 (16550a)
0x10001000  - VirtIO disk
0x80000000  - RAM start / Kernel load address
0x80000000+ - Kernel text, data, heap
PHYSTOP     - End of usable RAM (0x80000000 + 128MB = 0x88000000)
```

### Key Components
1. **entry.S**: Initial boot code, sets up stack, jumps to start()
2. **start.c**: Machine mode setup, delegates to supervisor mode, jumps to main()
3. **main.c**: Kernel initialization in supervisor mode
4. **memlayout.h**: Memory map definitions
5. **plic.c**: PLIC interrupt controller driver (QEMU specific addresses)
6. **uart.c**: 16550a UART driver
7. **virtio_disk.c**: VirtIO block device driver (QEMU specific)

---

## Porting Strategy

### Phase 1: Hardware Discovery & Documentation
**Goal**: Extract exact hardware specifications from PDFs

#### From PDFs, we need to identify:
1. **Memory Map**:
   - Where does the bootloader load the kernel? (vs QEMU's 0x80000000)
   - RAM start address and size
   - MMIO regions for peripherals
   - Reserved memory regions

2. **UART Configuration**:
   - UART base address (we see 0xd4017000 for console)
   - UART type (16550a compatible? or different?)
   - Clock source and baud rate calculation
   - IRQ number for UART (74 detected)

3. **PLIC Details**:
   - Base address: 0xe0000000 (confirmed from dmesg)
   - Number of interrupt sources: 159 (confirmed)
   - Context configuration for 8 cores
   - Priority and enable register offsets

4. **CLINT or Timer**:
   - Does the Ky X1 have a CLINT at standard address?
   - Timer interrupt mechanism (seems to support sstc extension)
   - MTIME and MTIMECMP addresses if applicable

5. **Boot Process**:
   - What bootloader is used? (U-Boot? OpenSBI?)
   - What state is the CPU in when kernel starts? (M-mode? S-mode?)
   - Device tree location if passed
   - Initial stack setup

### Phase 2: Minimal Boot
**Goal**: Get to a point where we can print "Hello" via UART

#### Files to modify:
1. **kernel/memlayout.h**:
   ```c
   // Change from QEMU addresses to OrangePi RV2 addresses
   #define UART0 0xd4017000L      // Console UART (was 0x10000000)
   #define UART0_IRQ 74           // UART interrupt (was 10)
   
   #define PLIC 0xe0000000L       // PLIC base (was 0x0c000000)
   // Update PLIC register offsets if different
   
   #define KERNBASE 0x40000000L   // TBD: verify from boot process
   #define PHYSTOP (KERNBASE + 8*1024*1024*1024) // 8GB RAM
   
   // Remove VIRTIO and CLINT if not present
   ```

2. **kernel/uart.c**:
   - Verify if 16550a compatible or needs custom driver
   - May need to adjust register offsets
   - Update baud rate calculation based on clock frequency

3. **kernel/entry.S**:
   - Update load address comment
   - May need to handle different boot protocol
   - Verify mhartid handling for 8 cores

4. **kernel/start.c**:
   - Verify PMP (Physical Memory Protection) configuration
   - May need to adjust timer initialization for sstc
   - Ensure delegation works for all 8 cores

5. **kernel/plic.c**:
   - Update base address to 0xe0000000
   - Verify register layout matches
   - Update IRQ numbers and enable bits
   - Test with 8 cores (was typically 2-4 in QEMU)

6. **kernel/kernel.ld** (linker script):
   - Update kernel load address to match bootloader expectation
   - Verify memory layout

7. **Makefile**:
   - Remove QEMU targets
   - Add bare-metal build targets
   - Consider adding U-Boot image generation (mkimage)

#### Testing approach:
- Build kernel binary
- Use existing bootloader (U-Boot on SD card) to load kernel
- Initially remove all device drivers except UART
- Get to first printf() in main()

### Phase 3: Core Functionality
**Goal**: Multi-core boot, interrupts, and timers

#### Tasks:
1. **Multi-core Support**:
   - Verify all 8 cores can boot and reach main()
   - Test per-core stack allocation (needs 8 x 4KB)
   - Verify cpuid() returns correct hart ID

2. **Timer Interrupts**:
   - Implement timer using sstc extension (stimecmp)
   - Or implement CLINT if available on Ky X1
   - Verify timer interrupts arrive correctly

3. **PLIC Integration**:
   - Enable UART interrupts via PLIC
   - Test interrupt claim and completion
   - Verify interrupt routing to all cores

4. **Console I/O**:
   - Get console input working
   - Test console output under interrupts
   - Verify no data loss

### Phase 4: Memory Management
**Goal**: Virtual memory, page tables, and process support

#### Tasks:
1. **Physical Memory Allocator**:
   - Update kalloc.c to use correct PHYSTOP
   - Verify 8GB memory can be managed
   - Consider NUMA if L2 cache is split

2. **Virtual Memory**:
   - Verify sv39 page table format works
   - Test kernel page table setup
   - Test user page table creation

3. **Process Support**:
   - Enable process creation
   - Test context switching
   - Verify trampoline page works

### Phase 5: Storage
**Goal**: Replace VirtIO with real storage

#### Options:
1. **SD/MMC Controller**:
   - Write driver for onboard SD/eMMC
   - Implement block device interface
   - Port filesystem code

2. **USB Storage**:
   - Use USB host controller
   - Simpler but requires USB stack

3. **Network Boot**:
   - Use network device
   - Boot from NFS or similar

### Phase 6: Additional Peripherals
**Goal**: Expand hardware support

#### Potential drivers:
1. **Network**: Ethernet controller
2. **GPIO**: For LED/button control
3. **I2C**: For sensor communication
4. **SPI**: For additional peripherals
5. **Display**: HDMI/LCD if desired

---

## Critical Information Needed from PDFs

### Must extract from documentation:
1. **Ky X1 Chip Manual**:
   - Complete memory map
   - PLIC register layout and interrupt routing
   - Timer/CLINT implementation
   - Cache coherency model
   - Boot ROM behavior

2. **OrangePi RV2 Schematic**:
   - UART connections and routing
   - Crystal oscillator frequency (for UART baud)
   - Reset circuit
   - Power sequencing

3. **OrangePi RV2 User Manual**:
   - Boot process (SD card, SPI flash, etc.)
   - Default bootloader configuration
   - Memory configuration
   - How to load custom kernels

---

## Build Process Changes

### Current (QEMU):
```bash
make qemu          # Builds and runs in QEMU
```

### Proposed (Bare Metal):
```bash
make kernel        # Build kernel binary
make kernel.img    # Wrap for U-Boot (if needed)
make install       # Copy to SD card or flash
make load-uart     # Load via UART bootloader (for development)
```

### Boot Options:
1. **SD Card Boot** (recommended for development):
   - Replace kernel on boot partition
   - U-Boot loads from FAT partition
   
2. **SPI Flash Boot** (for production):
   - Flash kernel to SPI
   - Faster boot, but harder to update

3. **UART/USB Boot** (for recovery):
   - Use bootloader's recovery mode
   - Load kernel over UART or USB

---

## Development Workflow

### Recommended approach:
1. Keep QEMU build working as reference
2. Add `#ifdef ORANGEPI_RV2` for platform-specific code
3. Create separate `kernel/orangepi_rv2.c` for board-specific init
4. Use conditional compilation in Makefile

### Testing strategy:
1. **Unit test on QEMU**: Verify logic changes don't break QEMU
2. **Build for OrangePi**: Cross-compile with correct addresses
3. **Deploy**: Copy to SD card
4. **Boot**: Connect serial console, power on
5. **Debug**: Use UART printf() debugging initially
6. **Iterate**: Update, rebuild, reflash

---

## Risk Mitigation

### Potential issues:
1. **Bootloader compatibility**: May need to adjust kernel format
2. **Cache coherency**: 8-core system may have coherency issues
3. **Undocumented hardware**: Ky X1 may have quirks
4. **Timer differences**: May not have standard CLINT
5. **Memory layout**: Bootloader may use unexpected addresses

### Mitigation strategies:
1. Start minimal (just UART, single core)
2. Incremental bring-up (add one feature at a time)
3. Keep detailed notes of working configurations
4. Maintain fallback to known-good kernel
5. Use existing Linux device tree as reference

---

## Success Criteria

### Phase 1 (Minimal Boot):
- [ ] Kernel loads and executes
- [ ] Can print to console via UART
- [ ] Single core reaches main()

### Phase 2 (Core Functions):
- [ ] All 8 cores boot successfully
- [ ] Timer interrupts working
- [ ] UART interrupts via PLIC working
- [ ] Console I/O fully functional

### Phase 3 (OS Functions):
- [ ] Virtual memory working
- [ ] Process creation working
- [ ] Context switching working
- [ ] User mode programs can run

### Phase 4 (Complete):
- [ ] Filesystem support
- [ ] Multiple processes
- [ ] Shell working
- [ ] All xv6 utilities functional

---

## Next Steps

1. **Extract hardware specifications from PDFs**:
   - Read Ky X1 Chip Manual for memory map
   - Read schematic for UART clock source
   - Read user manual for boot process

2. **Create hardware configuration header**:
   - `kernel/orangepi_rv2.h` with all addresses
   - Document differences from QEMU

3. **Modify kernel for minimal boot**:
   - Update memlayout.h
   - Update entry.S if needed
   - Update start.c for correct memory
   - Verify UART driver

4. **Build and test**:
   - Compile kernel
   - Deploy to SD card
   - Boot and verify console output

5. **Iterate based on results**:
   - Debug boot failures
   - Add missing initialization
   - Progressively enable features
