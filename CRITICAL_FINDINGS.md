# CRITICAL FINDINGS - XV6 OrangePi RV2 Port

## 🔴 MAJOR DISCOVERY: System Uses OpenSBI!

This **completely changes** our porting strategy!

### What This Means

1. **Kernel Starts in S-mode** (Supervisor), NOT M-mode (Machine)
   - OpenSBI firmware runs in M-mode
   - Our xv6 kernel runs in S-mode under OpenSBI
   - Cannot directly access M-mode CSRs!

2. **XV6's start.c Won't Work As-Is**
   - Current xv6 assumes M-mode entry
   - Tries to configure M-mode CSRs (mstatus, medeleg, etc.)
   - These will trap/fail when run in S-mode!

3. **Must Use SBI (Supervisor Binary Interface)**
   - SBI provides services to S-mode kernels
   - Console I/O, timer, IPI, etc.
   - Standard RISC-V interface

### Two Possible Approaches

#### Option A: Work WITH OpenSBI (RECOMMENDED)
**Pros**:
- Simpler initial port
- Use SBI for console, timer, IPI
- Standard interface
- Faster to get working

**Cons**:
- Depends on OpenSBI being present
- Less control over hardware
- Slightly slower (SBI call overhead)

**Changes needed**:
- Remove M-mode setup from start.c
- Add SBI console driver
- Use SBI timer calls
- Entry point stays in S-mode

#### Option B: Replace OpenSBI
**Pros**:
- Full control over hardware
- Direct access, no SBI overhead
- More educational value

**Cons**:
- Much more complex
- Need to write M-mode firmware
- Harder to debug
- Longer development time

---

## Hardware Configuration Summary

### Memory
```
Region 1: 0x00000000 - 0x7FFFFFFF (2 GB)
Region 2: 0x100000000 - 0x17FFFFFFF (2 GB, above 4GB)
```
→ Use Region 1 for simplicity

### UART (Console)
```
Type:     PXA UART (ky,pxa-uart) - NOT 16550a!
Address:  0xd4017000
IRQ:      74 (PLIC)
Baud:     115200
```
→ Need PXA UART driver OR verify 16550a compatibility

### PLIC
```
Address:  0xe0000000
Size:     64 MB
Contexts: 16 (2 per hart: M-mode + S-mode)
```
→ Standard RISC-V PLIC, but need S-mode context offsets

### Boot Sequence
```
1. U-Boot (bootloader)
2. OpenSBI (M-mode firmware)
3. Kernel (S-mode) ← XV6 starts here
```

---

## Immediate Action Items

### 1. Verify UART Type
**Question**: Is "ky,pxa-uart" 16550a compatible?

**Test**:
```c
// In early boot, try writing to standard 16550a registers
*(volatile uint8_t*)(0xd4017000 + 0) = 'A';  // THR
// If 'A' appears on console, likely compatible!
```

**If compatible**:
- Use existing xv6 uart.c with address change
- Quick win!

**If not compatible**:
- Need to write PXA UART driver
- Check PXA UART documentation
- More work, but doable

### 2. Modify start.c for S-mode Entry

**Current xv6 start.c**:
```c
void start() {
  // These WON'T WORK in S-mode!
  w_mstatus(x);       // ❌ M-mode CSR
  w_mepc((uint64)main); // ❌ M-mode CSR
  w_medeleg(0xffff);  // ❌ M-mode CSR
  // ...
  asm volatile("mret"); // ❌ M-mode instruction
}
```

**New approach for S-mode**:
```c
void start() {
  // We're already in S-mode, thanks to OpenSBI!
  // Just set up S-mode environment
  
  // Enable S-mode interrupts
  w_sstatus(r_sstatus() | SSTATUS_SIE);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);
  
  // Set up trap vector
  w_stvec((uint64)kernelvec);
  
  // Initialize timer via SBI
  sbi_set_timer(r_time() + 1000000);
  
  // Jump directly to main (already in S-mode)
  main();
}
```

### 3. Add SBI Support

Create `kernel/sbi.c` and `kernel/sbi.h`:

```c
// sbi.h
#define SBI_CONSOLE_PUTCHAR 0x01
#define SBI_CONSOLE_GETCHAR 0x02
#define SBI_SET_TIMER 0x00

struct sbiret {
  long error;
  long value;
};

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0, 
                        unsigned long arg1, unsigned long arg2,
                        unsigned long arg3, unsigned long arg4,
                        unsigned long arg5);

void sbi_console_putchar(int ch);
int sbi_console_getchar(void);
void sbi_set_timer(uint64 stime_value);
```

```c
// sbi.c
struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0, ...) {
  register unsigned long a0 asm("a0") = arg0;
  register unsigned long a1 asm("a1") = arg1;
  // ... more args ...
  register unsigned long a7 asm("a7") = fid;
  register unsigned long a6 asm("a6") = ext;
  
  asm volatile("ecall"
               : "+r"(a0), "+r"(a1)
               : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
               : "memory");
  
  struct sbiret ret;
  ret.error = a0;
  ret.value = a1;
  return ret;
}

void sbi_console_putchar(int ch) {
  sbi_ecall(0x01, 0, ch, 0, 0, 0, 0, 0);
}
```

### 4. Update Makefile

Add SBI support and adjust addresses:

```makefile
OBJS = \
  $K/entry.o \
  $K/start.o \
  $K/sbi.o \      # NEW: SBI support
  $K/console.o \
  # ... rest of files

# Add flag to indicate OpenSBI environment
CFLAGS += -DUSE_OPENSBI

# Adjust kernel load address (TBD - need to verify with U-Boot)
# Linker script may need update
```

### 5. Update Memory Layout

**kernel/memlayout.h**:
```c
// Physical memory layout

// OrangePi RV2 memory starts at 0x0
// (Linux device tree shows memory@0)
// BUT: Bootloader may load kernel at offset

// Option 1: Start at 0x40000000 (1GB) - common for embedded
#define KERNBASE 0x40000000L
#define PHYSTOP (KERNBASE + 512*1024*1024)  // Use 512MB for now

// Option 2: Start at 0x80000000 (2GB) - like QEMU
// #define KERNBASE 0x80000000L
// #define PHYSTOP (KERNBASE + 128*1024*1024)

// OrangePi RV2 hardware addresses
#define UART0 0xd4017000L
#define UART0_IRQ 74

#define PLIC 0xe0000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
// S-mode contexts (hart*2 + 1)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + ((hart)*2+1)*0x80)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + ((hart)*2+1)*0x1000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + ((hart)*2+1)*0x1000)

// No CLINT on this system - uses sstc extension
// No VIRTIO - will add SD/MMC later or run diskless
```

---

## Testing Strategy

### Phase 1: SBI Console Only (Immediate Goal)
1. Modify entry.S to accept S-mode entry
2. Create minimal start() that just jumps to main()
3. Use SBI console for output (sbi_console_putchar)
4. Get "xv6 kernel is booting" message

**Success criteria**: See text on serial console!

### Phase 2: Add UART Driver
1. Test if PXA UART is 16550a compatible
2. If yes: update address in uart.c
3. If no: write PXA UART driver
4. Switch from SBI console to direct UART

**Success criteria**: Console I/O works directly

### Phase 3: Enable Interrupts
1. Configure PLIC with S-mode contexts
2. Enable UART interrupts
3. Use SBI timer or sstc for timer interrupts
4. Test interrupt handling

**Success criteria**: Timer ticks, console interrupts work

### Phase 4: Bring Up All Cores
1. Understand how OpenSBI starts secondary cores
2. Implement SBI IPI (inter-processor interrupt)
3. Test all 8 cores reach main()

**Success criteria**: All 8 cores running

### Phase 5: Virtual Memory & Processes
1. Enable MMU (S-mode satp)
2. Create page tables
3. Test first user process
4. Full OS functionality

**Success criteria**: Shell running!

---

## Questions to Answer

1. **Where does U-Boot load the kernel?**
   - Check U-Boot config
   - Likely 0x40000000 or 0x80000000
   - Update KERNBASE accordingly

2. **What is UART clock frequency?**
   - Needed for baud rate divisor
   - Check clock tree in device tree
   - Or assume 100 MHz and test

3. **Is PXA UART 16550a compatible?**
   - Critical for UART driver choice
   - Test by writing to registers
   - Check PXA UART documentation

4. **How does OpenSBI start secondary cores?**
   - Via HSM (Hart State Management) extension
   - Need to use sbi_hart_start() call
   - Or: let OpenSBI start them automatically

---

## Files to Create/Modify

### New Files:
- [ ] `kernel/sbi.h` - SBI interface definitions
- [ ] `kernel/sbi.c` - SBI call implementation
- [ ] `kernel/orangepi_rv2.h` - Board-specific config
- [ ] `OPENSBI_NOTES.md` - OpenSBI integration notes

### Files to Modify:
- [ ] `kernel/entry.S` - Accept S-mode entry
- [ ] `kernel/start.c` - Remove M-mode code, add SBI init
- [ ] `kernel/memlayout.h` - Update addresses
- [ ] `kernel/main.c` - Add board-specific init
- [ ] `kernel/uart.c` - Update address, maybe rewrite for PXA
- [ ] `kernel/plic.c` - Update addresses, S-mode contexts
- [ ] `kernel/trap.c` - Handle S-mode traps
- [ ] `kernel/console.c` - Optional: add SBI console backend
- [ ] `Makefile` - Add sbi.o, update flags
- [ ] `kernel/kernel.ld` - Update load address

---

## Next Commands to Run

```bash
# 1. Check if we can see U-Boot environment
# (might tell us kernel load address)
sudo cat /proc/cmdline

# 2. Check if there are any U-Boot artifacts
ls /boot/uEnv.txt /boot/boot.scr 2>/dev/null

# 3. Look for OpenSBI version info
dmesg | grep -i sbi | head

# 4. Check actual memory layout
cat /proc/iomem | grep -i "kernel\|system ram" | head -20

# 5. Examine full UART device tree node
grep -A 30 "serial@d4017000" docs/orangepi-rv2.dts
```

Ready to start coding when you are!
