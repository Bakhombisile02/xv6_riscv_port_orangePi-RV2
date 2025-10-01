# What to Extract from PDFs

## From: Ky X1 Chip Manual.pdf

### Priority 1: Memory Map
Look for sections titled "Memory Map", "Address Space", or "System Memory":
- [ ] DRAM base address (likely 0x00000000 or 0x40000000 or 0x80000000)
- [ ] DRAM size and layout
- [ ] MMIO region start/end
- [ ] Boot ROM address
- [ ] On-chip SRAM address (if any)

### Priority 2: UART Configuration
Search for "UART", "Serial", "Console":
- [ ] UART0 register base (confirm 0xd4017000)
- [ ] UART register layout (is it 16550a compatible?)
- [ ] UART clock source frequency
- [ ] Baud rate divisor calculation
- [ ] FIFO configuration
- [ ] DMA support (if any)

### Priority 3: PLIC (Interrupt Controller)
Search for "PLIC", "Interrupt Controller":
- [ ] PLIC base address (confirm 0xe0000000)
- [ ] Number of interrupt sources (confirm 159)
- [ ] Priority register offset and layout
- [ ] Pending register offset
- [ ] Enable register offset per context
- [ ] Claim/complete register offset
- [ ] Context mapping (which context for each hart?)
- [ ] IRQ assignments for all peripherals

### Priority 4: Timer/CLINT
Search for "Timer", "CLINT", "MTIME", "Machine Timer":
- [ ] CLINT base address (if exists)
- [ ] MTIME register address
- [ ] MTIMECMP register addresses (per hart)
- [ ] Timer clock frequency
- [ ] SSTC support (Supervisor Timer Compare) - seems supported
- [ ] STIMECMP configuration if using SSTC

### Priority 5: Boot Process
Search for "Boot", "Reset", "Initialization":
- [ ] Reset vector address
- [ ] Boot ROM behavior
- [ ] SPI flash memory map (if kernel boots from flash)
- [ ] SD/MMC controller address (if booting from SD)
- [ ] CPU initial state (M-mode? which registers set?)
- [ ] Multi-core bring-up (how are secondary cores started?)

### Priority 6: Cache and Coherency
Search for "Cache", "L1", "L2", "Coherency":
- [ ] Cache sizes (confirmed: L1=32KB per core, L2=1MB shared)
- [ ] Cache line size
- [ ] Cache coherency protocol
- [ ] Cache management instructions (fence, flush, etc.)
- [ ] Non-cacheable memory regions

### Priority 7: Other Peripherals
If planning to use them:
- [ ] SD/MMC controller base address
- [ ] Ethernet controller base address
- [ ] USB controller base address
- [ ] GPIO controller base address
- [ ] I2C/SPI controller addresses

---

## From: OPI RV2 V1_1_SCH_20250508(1).pdf (Schematic)

### Priority 1: UART Hardware
Look at UART section:
- [ ] Which UART is connected to debug header/USB-serial?
- [ ] Crystal oscillator frequency for UART clock
- [ ] UART pin assignments (TX, RX, RTS, CTS)
- [ ] Any level shifters or buffers?

### Priority 2: Memory
Look at DRAM section:
- [ ] DDR4 chip model and size
- [ ] Memory address strapping
- [ ] ECC support?

### Priority 3: Boot Configuration
Look at boot selection:
- [ ] Boot mode selection pins/switches
- [ ] SPI flash chip (model, size)
- [ ] SD card slot connection
- [ ] Boot priority order

### Priority 4: Power
Look at power section:
- [ ] Power-on sequence
- [ ] Reset circuit
- [ ] Power domains

---

## From: OrangePi_RV2_X1_User Manual_v1.1.pdf

### Priority 1: Boot Process
Look for "Boot", "Getting Started":
- [ ] Default boot device (SD card? SPI flash?)
- [ ] Bootloader type (U-Boot version?)
- [ ] How to access bootloader console
- [ ] U-Boot memory addresses for loading kernel
- [ ] U-Boot commands to load custom kernel

### Priority 2: Serial Console
Look for "Serial Console", "Debug":
- [ ] Which UART is the console? (likely UART0 at 0xd4017000)
- [ ] Default baud rate (115200?)
- [ ] Pin header location
- [ ] USB-to-serial chip (if any)

### Priority 3: Building Custom Images
Look for "Development", "Building":
- [ ] Recommended toolchain
- [ ] Kernel load address
- [ ] Device tree usage
- [ ] Image format (uImage? Image? raw binary?)
- [ ] How to create bootable SD card

### Priority 4: Memory Map
Look for "Memory", "Address Space":
- [ ] Where bootloader loads kernel
- [ ] Reserved memory regions
- [ ] Device tree location in memory

---

## Quick Start: Minimum Info Needed

To get started with minimal boot, we absolutely need:

1. **Kernel Load Address**: Where does U-Boot load the kernel?
   - Check: User manual, U-Boot default settings
   - Typical values: 0x40000000, 0x80000000, 0x82000000

2. **UART Clock Frequency**: What clock drives the UART?
   - Check: Chip manual or schematic (crystal frequency)
   - Needed to calculate correct baud rate divisor

3. **Boot Protocol**: What state is CPU in when kernel starts?
   - Check: User manual, chip manual boot section
   - M-mode or S-mode? Are traps delegated?

4. **Memory Size and Layout**: How much RAM and where?
   - Check: All three documents
   - We know it's 8GB, but what's the base address?

---

## How to Search PDFs

### If you can't send me the PDFs:

Please search for these keywords and send me relevant sections:

**Critical searches**:
1. Search "0xd4017000" or "d4017000" → UART base address documentation
2. Search "0xe0000000" or "e0000000" → PLIC documentation  
3. Search "memory map" → System address space
4. Search "boot" → Boot process
5. Search "U-Boot" → Bootloader configuration

**For each search**, please copy:
- The section title
- Any memory addresses or register offsets
- Any diagrams or tables
- Related configuration values

### If you can send PDFs:

I cannot access files on your local machine from this SSH session. You would need to:
1. Copy PDFs to the OrangePi RV2 (via scp or USB)
2. Then I can read them directly

Or:
1. Extract the text yourself
2. Paste relevant sections in our conversation

---

## Example: What Good Documentation Looks Like

When you find the UART section, it should look something like:

```
UART0 Configuration
Base Address: 0xd4017000
IRQ: 74

Registers:
  Offset 0x00: RBR/THR (Receive/Transmit Buffer)
  Offset 0x04: IER (Interrupt Enable)
  Offset 0x08: IIR/FCR (Interrupt ID/FIFO Control)
  ...

Clock Source: APB_CLK (100 MHz)
Baud Rate = Clock / (16 * divisor)
```

This tells us:
- ✓ Base address (confirmed)
- ✓ IRQ number (confirmed)
- ✓ Register layout (16550a compatible?)
- ✓ Clock frequency (needed!)
- ✓ Baud calculation (needed!)

---

## What to Do Next

1. **Locate PDFs on your local machine** (already done):
   - /Users/dlaba556/Downloads/Ky X1 Chip Manual.pdf
   - /Users/dlaba556/Downloads/OPI RV2 V1_1_SCH_20250508(1).pdf
   - /Users/dlaba556/Downloads/OrangePi_RV2_X1_User Manual_v1.1.pdf

2. **Search for critical information**:
   - Start with boot process and memory map
   - Then UART clock and configuration
   - Then PLIC details

3. **Share findings**:
   - Paste relevant sections in our chat
   - Include page numbers for reference
   - Include any diagrams or tables

4. **I will update the porting plan**:
   - Fill in TBD values
   - Create hardware configuration header
   - Begin code modifications
