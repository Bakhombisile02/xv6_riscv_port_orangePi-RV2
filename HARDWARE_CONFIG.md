# OrangePi RV2 Hardware Configuration (from Device Tree)

## Memory Layout

### RAM Configuration
```
Memory Region 1 (Low 2GB):
  Base: 0x0000000000000000
  Size: 0x80000000 (2 GB)
  
Memory Region 2 (High 2GB):  
  Base: 0x0000000100000000 (4 GB physical)
  Size: 0x80000000 (2 GB)

Total: 4 GB accessible (though system reports 8GB total)
```

**Key findings**:
- Memory is split across two regions (likely for 32-bit compatibility)
- Region 1: 0x00000000 - 0x7FFFFFFF (2GB)
- Region 2: 0x100000000 - 0x17FFFFFFF (2GB at 4GB+ physical)
- For xv6, we'll use **Region 1** starting at **0x00000000**

### Boot Configuration (from chosen node)
```
bootargs: earlycon=sbi console=ttyS0,115200n8 loglevel=8 swiotlb=65536 rdinit=/init
stdout-path: serial0:115200n8
```

**Key findings**:
- Uses **OpenSBI** for early console (earlycon=sbi)
- Primary console: **ttyS0** (serial@d4017000)
- Baud rate: **115200**
- This means kernel starts in **S-mode** with OpenSBI in M-mode!

## UART Configuration

### Primary Console UART (serial0 / ttyS0)
```
Compatible: "ky,pxa-uart"
Base Address: 0xd4017000
Register Size: 0x100
IRQ: 0x2a (42 decimal) → actual IRQ 74 in PLIC (0x2a + base offset)
Clocks: Referenced (need to determine frequency)
```

### Available UARTs
| Serial | Alias  | Base Address | IRQ (hex) | IRQ (dec) | Usage        |
|--------|--------|--------------|-----------|-----------|--------------|
| serial0| ttyS0  | 0xd4017000   | 0x2a      | 42/74     | Console      |
| serial1| ttyS1  | 0xc088d000   | ?         | ?         | R-UART       |
| serial2| ttyS2  | 0xd4017100   | 0x2c      | 44        | Available    |
| serial3| ttyS3  | 0xd4017200   | 0x2d      | 45        | Available    |
| serial4| ttyS4  | 0xd4017300   | 0x2e      | 46        | Available    |
| serial5| ttyS5  | 0xd4017400   | 0x2f      | 47        | Available    |
| ...    | ...    | ...          | ...       | ...       | More UARTs   |

**UART Type**: "ky,pxa-uart" - PXA-style UART (similar to 16550 but not identical!)

## PLIC (Interrupt Controller)

```
Compatible: "riscv,plic0"
Base Address: 0xe0000000
Size: 0x4000000 (64 MB)
#interrupt-cells: 1

Interrupts-extended (context routing):
  Hart 0: M-mode (0x0b), S-mode (0x09)
  Hart 1: M-mode (0x0b), S-mode (0x09)
  Hart 2: M-mode (0x0b), S-mode (0x09)
  Hart 3: M-mode (0x0b), S-mode (0x09)
  Hart 4: M-mode (0x0b), S-mode (0x09)
  Hart 5: M-mode (0x0b), S-mode (0x09)
  Hart 6: M-mode (0x0b), S-mode (0x09)
  Hart 7: M-mode (0x0b), S-mode (0x09)
```

**Key findings**:
- PLIC at 0xe0000000 (confirmed)
- Each hart has both M-mode and S-mode contexts
- Standard RISC-V PLIC layout
- IRQ numbers in device tree are relative (need base offset)

## Clocks

Multiple clock references found - need to extract clock tree to determine UART clock frequency.
Preliminary: clocks referenced as `<0x03 0x3a 0x03 0xb4>` format (phandle + clock ID)

## Storage

### SD/MMC Controllers
```
mmc0: sdh@d4280000
mmc1: sdh@d4280800  
mmc2: sdh@d4281000
```

### Ethernet
```
ethernet0: ethernet@cac80000
ethernet1: ethernet@cac81000
```

## CPU Configuration (from device tree - implied)

8 cores with RISC-V PLIC routing suggests:
- 8 harts (0-7)
- Each with M-mode and S-mode interrupt contexts
- Likely boots all cores (need to verify secondary core startup)

---

## Critical Information for XV6 Port

### 1. Memory Layout for XV6

**Recommended configuration**:
```c
#define KERNBASE 0x00000000L    // Start of RAM
#define PHYSTOP  0x80000000L    // End of 2GB region (or use less)
```

**Why start at 0x00000000?**
- Device tree shows RAM at 0x0
- Simplifies address translation
- Avoids high memory complications

**Alternative** (if bootloader loads at specific address):
```c
#define KERNBASE 0x40000000L    // 1GB offset (common for embedded)
#define PHYSTOP  0x80000000L    // Still use up to 2GB
```

### 2. UART Configuration

**Address and IRQ**:
```c
#define UART0 0xd4017000L
#define UART0_IRQ 74    // Need to verify actual PLIC IRQ number
```

**Type**: PXA UART, not 16550a!
- Need to implement PXA UART driver or verify compatibility
- Register layout may differ from standard 16550a

### 3. PLIC Configuration

```c
#define PLIC 0xe0000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING  (PLIC + 0x1000)
// Context: hart * 2 (M-mode=0, S-mode=1)
#define PLIC_SENABLE(hart)    (PLIC + 0x2080 + ((hart)*2+1)*0x80)
#define PLIC_SPRIORITY(hart)  (PLIC + 0x201000 + ((hart)*2+1)*0x1000)
#define PLIC_SCLAIM(hart)     (PLIC + 0x201004 + ((hart)*2+1)*0x1000)
```

### 4. Boot Protocol

**Critical finding**: System uses **OpenSBI**!

**Implications**:
- Kernel starts in **S-mode**, not M-mode
- M-mode is handled by OpenSBI firmware
- Can use SBI calls for early console, timer, etc.
- start.c needs major changes (no M-mode setup!)

**Boot sequence**:
```
1. OpenSBI (M-mode) - loaded by bootloader
2. XV6 kernel (S-mode) - loaded by OpenSBI
3. XV6 assumes S-mode entry, OpenSBI handles M-mode
```

**Changes needed**:
- Remove M-mode CSR accesses in start.c
- Use SBI calls for:
  - Early console output
  - Timer programming
  - IPI (inter-processor interrupts)
- Modify entry point assumptions

### 5. Timer

With OpenSBI and sstc extension:
- Use SBI timer calls OR
- Use sstc extension directly (stimecmp CSR)
- No CLINT needed!

### 6. Clock Frequency

**Need to determine**:
- UART clock frequency (for baud rate divisor)
- CPU frequency (for timing calculations)

**How to find**:
- Check clock tree in device tree
- Or: measure by timing known events
- Or: assume standard frequency (100 MHz APB clock common)

---

## Device Tree Analysis Summary

### Strengths
- Complete hardware description available
- Standard RISC-V PLIC
- OpenSBI support built-in
- Multiple UARTs available

### Challenges
1. **PXA UART**: Not standard 16550a, need driver adaptation
2. **OpenSBI dependency**: Must work with SBI or replace it
3. **Clock frequencies**: Not explicit in device tree dump
4. **Memory split**: Two regions, need to handle properly
5. **IRQ mapping**: Need to verify PLIC IRQ number mapping

### Recommended Approach

**Phase 1**: Work WITH OpenSBI
- Start kernel in S-mode
- Use SBI for console and timer
- Simplifies initial bring-up
- Can replace SBI later if needed

**Phase 2**: Replace OpenSBI (optional)
- Rewrite start.c for M-mode entry
- Direct hardware access
- More control, but more complexity

---

## Next Steps

1. **Verify UART compatibility**:
   - Check if PXA UART is 16550a compatible
   - May need custom driver

2. **Determine clock frequency**:
   - Extract from full device tree
   - Or measure empirically
   - Critical for UART baud rate

3. **Understand bootloader**:
   - How does U-Boot load kernel?
   - Where in memory?
   - Does it load OpenSBI first?

4. **Test SBI interface**:
   - Use SBI console for early debugging
   - Verify SBI calls work

5. **Update xv6 code**:
   - Modify for S-mode entry
   - Update memory map
   - Adapt UART driver
   - Update PLIC addresses
