# 🎯 START HERE - XV6 OrangePi RV2 Port

## Project Status: ✅ ANALYSIS COMPLETE - READY TO CODE

All hardware analysis is finished! We have everything needed to begin implementation.

---

## 📚 Documentation Summary

| File | Purpose | Status |
|------|---------|--------|
| **COMPLETE_HARDWARE_SPEC.md** | ⭐ **READ THIS FIRST** - Complete hardware spec | ✅ Final |
| CRITICAL_FINDINGS.md | OpenSBI discovery & implications | ✅ Complete |
| FINAL_HARDWARE_INFO.md | Hardware config with code examples | ✅ Complete |
| HARDWARE_CONFIG.md | Device tree analysis | ✅ Complete |
| HARDWARE_SUMMARY.md | Initial hardware detection | ✅ Complete |
| PORTING_PLAN.md | Original 6-phase strategy | ✅ Complete |
| PDF_EXTRACTION_GUIDE.md | What to look for in manuals | ✅ Complete |
| README_PORT.md | Project overview | ✅ Complete |

### 📁 Source Documents
- `docs/Ky X1 Chip Manual.pdf` (4.9MB) - Extracted ✅
- `docs/OrangePi_RV2_X1_User Manual_v1.1.pdf` (11MB) - Extracted ✅
- `docs/OPI RV2 V1_1_SCH_20250508(1).pdf` (2.3MB) - Schematic
- `docs/orangepi-rv2.dts` - Device tree source ✅
- `docs/chip_manual.txt` - Extracted text (169KB)
- `docs/user_manual.txt` - Extracted text (184KB)

---

## 🔑 Critical Hardware Information

### CPU
```
Ky X1: 8-core RISC-V (rv64imafdcv)
2 Clusters × 4 cores each
Frequency: 614 MHz - 1.6 GHz
Extensions: Vector, AI, sstc (timer)
```

### Memory
```
8GB LPDDR4X total
Layout: 2GB @ 0x0, 2GB @ 0x100000000
Kernel loads at: 0x40000000 (recommended)
```

### UART (Console)
```
Address: 0xd4017000
IRQ: 42
Clock: 14.745 MHz
Baud: 115200 (divisor = 8)
Type: PXA UART (16550A-compatible)
**CRITICAL**: reg-shift=2 (32-bit aligned!)
```

### Interrupts
```
PLIC: 0xe0000000
CLINT: Exists (managed by OpenSBI)
Use SBI calls for timer/IPI
```

### Boot Sequence
```
SPI Flash → OpenSBI (M-mode) → U-Boot (S-mode) → Kernel (S-mode)
           v1.0.3                 v2022.10
```

---

## 🎯 Implementation Strategy

### **Option A: Run under OpenSBI (RECOMMENDED)**

This is the path of least resistance and standard RISC-V approach.

#### Pros:
- ✅ Simple - use existing boot infrastructure
- ✅ Standard RISC-V SBI interface
- ✅ Faster to implement
- ✅ Can switch to direct hardware later

#### What you need to do:
1. **Add SBI support** (kernel/sbi.c, kernel/sbi.h)
2. **Modify entry** (kernel/entry.S - accept S-mode, preserve hart ID)
3. **Rewrite start.c** (remove M-mode CSR access)
4. **Update memory map** (kernel/memlayout.h)
5. **Fix UART driver** (kernel/uart.c - add reg-shift=2)
6. **Update PLIC** (kernel/plic.c - S-mode contexts)
7. **Update linker** (kernel/kernel.ld - load at 0x40000000)
8. **Update Makefile** (add sbi.o, flags)

#### First boot goal:
```
See "xv6 kernel is booting" via SBI console!
```

---

## 📝 Step-by-Step Implementation

### Phase 1: Minimal Boot (SBI Console)
**Goal**: Print "Hello World" via SBI

**Files to create**:
```bash
kernel/sbi.h      # SBI interface definitions
kernel/sbi.c      # SBI ecall wrapper
```

**Files to modify**:
```bash
kernel/entry.S    # Accept S-mode entry
kernel/start.c    # Remove M-mode, add SBI init
kernel/main.c     # Use SBI console initially
Makefile          # Add sbi.o
```

**Test**: Serial console shows boot messages

---

### Phase 2: Direct UART
**Goal**: Switch from SBI to direct UART

**Files to modify**:
```bash
kernel/memlayout.h  # UART0 address
kernel/uart.c       # Add reg-shift=2 support
kernel/plic.c       # Update addresses
```

**Test**: Console I/O works directly

---

### Phase 3: Interrupts & Timer
**Goal**: PLIC and timer working

**Files to modify**:
```bash
kernel/plic.c   # S-mode contexts
kernel/trap.c   # S-mode trap handling  
kernel/start.c  # SBI timer setup
```

**Test**: Timer ticks, keyboard interrupts

---

### Phase 4: Multi-Core
**Goal**: All 8 cores running

**Files to modify**:
```bash
kernel/start.c   # Multi-core init
kernel/main.c    # Per-core startup
```

**Test**: All harts reach main()

---

### Phase 5: Virtual Memory
**Goal**: MMU and processes

**Files to modify**:
```bash
kernel/vm.c      # Page tables
kernel/proc.c    # Process creation
```

**Test**: First user process runs

---

### Phase 6: Complete System
**Goal**: Shell and utilities

**Test**: Can run all xv6 programs!

---

## 🚀 Quick Start Commands

### 1. Verify Toolchain
```bash
riscv64-linux-gnu-gcc --version  # Should be >= 13.2.1
```

### 2. Create SBI Files (first task)
```bash
cd /home/orangepi/xv6_riscv_port_orangePi-RV2/kernel

# Copy code from COMPLETE_HARDWARE_SPEC.md
# Create sbi.h and sbi.c
```

### 3. Build (once files are ready)
```bash
make clean
make kernel

# Create boot image
riscv64-linux-gnu-objcopy -O binary kernel/kernel kernel/Image

# Deploy to boot partition
sudo cp kernel/Image /boot/xv6-kernel.bin
```

### 4. Boot
```bash
# Interrupt U-Boot and run:
=> load mmc 0:1 0x40000000 xv6-kernel.bin
=> booti 0x40000000 - ${fdt_addr_r}
```

---

## 📋 Checklist

### Pre-Implementation
- [x] Hardware analysis complete
- [x] PDFs analyzed
- [x] Device tree extracted
- [x] Boot sequence understood
- [x] Memory map determined

### Phase 1 Implementation
- [ ] Create kernel/sbi.h
- [ ] Create kernel/sbi.c
- [ ] Modify kernel/entry.S
- [ ] Modify kernel/start.c
- [ ] Update Makefile
- [ ] Test build
- [ ] Deploy and boot
- [ ] See "xv6 kernel is booting"

### Future Phases
- [ ] Phase 2: Direct UART
- [ ] Phase 3: Interrupts
- [ ] Phase 4: Multi-core
- [ ] Phase 5: Virtual memory
- [ ] Phase 6: Complete system

---

## 🎓 Key Learnings

1. **OpenSBI is present** - Must work with S-mode entry
2. **PXA UART is 16550A-compatible** - Just needs reg-shift=2
3. **CLINT exists** - But managed by OpenSBI
4. **Device tree** - More complete than PDF manuals!
5. **U-Boot** - Uses standard booti command

---

## 💡 Pro Tips

1. **Start simple**: Get SBI console working first
2. **Test incrementally**: Each phase should boot and print
3. **Keep QEMU working**: Use `#ifdef` for platform differences
4. **Use SBI early**: Don't fight it - it simplifies things
5. **Serial console**: Your best debugging friend

---

## 🆘 Need Help?

### If kernel doesn't boot:
1. Check U-Boot load address (0x40000000)
2. Verify entry.S saves hart ID
3. Test SBI with simple ecall
4. Check serial console connection

### If UART doesn't work:
1. Verify reg-shift=2 in code
2. Check baud rate divisor (should be 8)
3. Test with SBI console first
4. Verify UART0 address (0xd4017000)

### Common Issues:
- **Hang at boot**: Entry point wrong or M-mode CSR access
- **No output**: UART configuration or baud rate
- **Trap**: M-mode CSR in S-mode code
- **Multi-core fails**: Secondary cores not started via SBI

---

## 🎉 Ready to Code!

You have everything you need:
- ✅ Complete hardware specification
- ✅ Working boot infrastructure  
- ✅ Example code in docs
- ✅ Clear implementation path
- ✅ Testing checklist

**Next step**: Read `COMPLETE_HARDWARE_SPEC.md` and start coding!

Good luck! 🚀

---

**Last Updated**: October 1, 2025
**Status**: Analysis Complete - Ready for Implementation
**Location**: `/home/orangepi/xv6_riscv_port_orangePi-RV2/`
