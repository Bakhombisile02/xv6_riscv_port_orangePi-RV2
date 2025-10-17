# Barebones xv6 Conversion - Test Plan

## Overview
This document describes how to test the barebones xv6 configuration after removing persistent storage.

## Prerequisites
- RISC-V 64-bit cross-compiler toolchain
- OrangePi RV2 hardware (for hardware testing)
- QEMU RISC-V emulator (for emulation testing)
- U-Boot installed on SD card (for hardware boot)

## Compilation Test

### Build the Kernel
```bash
cd /home/runner/work/xv6_riscv_port_orangePi-RV2/xv6_riscv_port_orangePi-RV2
make clean
make kernel/kernel
```

### Expected Output
```
riscv64-unknown-elf-gcc ... -c kernel/sbi.c
riscv64-unknown-elf-gcc ... -c kernel/entry.S
...
riscv64-unknown-elf-ld ... -o kernel/kernel
```

### Success Criteria
- ✅ No compilation errors
- ✅ No warnings (except allowed ones)
- ✅ `kernel/kernel` binary created
- ✅ File size approximately 200-250 KB

### Common Issues
- **Toolchain not found**: Install RISC-V toolchain or set TOOLPREFIX
- **Missing dependencies**: Ensure all kernel source files are present
- **Linker errors**: Check kernel.ld and object file list

## Kernel Inspection Test

### Check Kernel Binary
```bash
riscv64-unknown-elf-objdump -h kernel/kernel | grep -E "text|data|bss"
riscv64-unknown-elf-nm kernel/kernel | grep -E "main|consoleinit|userinit"
riscv64-unknown-elf-size kernel/kernel
```

### Expected Output
```
.text segment: ~150-200 KB (code)
.data segment: ~1-5 KB (initialized data)
.bss segment: ~50-100 KB (uninitialized data - NO large ramdisk!)
```

### Success Criteria
- ✅ .text section exists and contains kernel code
- ✅ .bss section is small (<1 MB, not 8+ MB like with ramdisk)
- ✅ main, consoleinit, userinit symbols present
- ✅ Total size < 500 KB

## Emulation Test (QEMU)

### Run in QEMU
```bash
make qemu
```

### Expected Console Output
```
xv6 kernel is booting (barebones mode - no persistent storage)

hart 1 starting
hart 2 starting
```

### Success Criteria
- ✅ Kernel boots without errors
- ✅ "barebones mode" message appears
- ✅ No file system initialization messages
- ✅ No "disk" messages
- ✅ System doesn't panic
- ✅ Multiple harts start

### What NOT to Expect
- ❌ "init: starting sh" (no user programs)
- ❌ Shell prompt
- ❌ File system messages
- ❌ Disk I/O activity

### Test Console Output
- Press Ctrl-A X to exit QEMU
- Serial console should work (type and see characters)

## Hardware Boot Test (OrangePi RV2)

### Preparation
1. Copy `kernel/kernel` to SD card FAT32 partition
2. Insert SD card into OrangePi RV2
3. Connect serial console (115200 baud, 8N1)
4. Power on the board

### U-Boot Commands
```
# At U-Boot prompt:
fatload mmc 0:1 0x40000000 kernel
go 0x40000000
```

### Expected Serial Output
```
xv6 kernel is booting (barebones mode - no persistent storage)

hart 1 starting
hart 2 starting
hart 3 starting
hart 4 starting
hart 5 starting
hart 6 starting
hart 7 starting
```

### Success Criteria
- ✅ U-Boot loads kernel successfully
- ✅ Kernel boots on hardware
- ✅ Console output works via UART
- ✅ All 8 harts start (OrangePi RV2 specific)
- ✅ No crashes or panics
- ✅ System remains stable

## Memory Test

### Check Memory Layout
In GDB or via kernel output:
```
info mem
x/10x 0x80000000   # Check RAM
x/10x 0xd4017000   # Check UART
```

### Success Criteria
- ✅ Kernel loaded at correct address
- ✅ Physical memory accessible
- ✅ Virtual memory working
- ✅ UART registers accessible
- ✅ No SDHCI access (0xd4280000 not mapped)

## Functional Tests

### Test 1: Console Output
```c
// Kernel should print boot messages
printf("Test message\n");
```
- ✅ Console output works

### Test 2: Timer
```c
// Check that timer interrupts work
// Watch for timer ticks in serial output
```
- ✅ Timer interrupts occur

### Test 3: Memory Allocation
```c
// kalloc should work
void *p = kalloc();
```
- ✅ Memory allocation succeeds
- ✅ No out-of-memory errors

### Test 4: Process Scheduling
```c
// initproc created and scheduled
// Scheduler runs without hanging
```
- ✅ Scheduler starts
- ✅ System doesn't hang

## Negative Tests (What Should NOT Work)

### Test 1: File Operations
```c
// These should fail or be no-ops:
open("file", O_RDWR);
read(fd, buf, size);
write(fd, buf, size);
```
- ✅ No panic (graceful failure)

### Test 2: Disk I/O
```c
// These should be no-ops:
virtio_disk_rw(buf, 0);
virtio_disk_rw(buf, 1);
```
- ✅ Returns immediately (no panic)

### Test 3: User Programs
```c
// This should fail/be skipped:
exec("/init", argv);
```
- ✅ No user programs loaded
- ✅ No shell started

## Comparison Test

### Before (With Storage)
```
Kernel size: ~280 KB
BSS size: 8 MB (ramdisk)
Boot messages: "init: starting sh"
User processes: Yes
File system: Yes
```

### After (Barebones)
```
Kernel size: ~200 KB
BSS size: <100 KB (no ramdisk)
Boot messages: "barebones mode"
User processes: No
File system: No
```

### Success Criteria
- ✅ Kernel size reduced
- ✅ Memory footprint reduced
- ✅ Boot messages changed
- ✅ No storage dependencies

## Regression Tests

### Verify Core Functionality Still Works
1. ✅ Console I/O
2. ✅ Memory management
3. ✅ Interrupt handling
4. ✅ Process creation
5. ✅ Context switching
6. ✅ Timer
7. ✅ PLIC

### Verify Storage Removed
1. ✅ No SDHCI driver compiled
2. ✅ No ramdisk allocated
3. ✅ No virtio_disk compiled
4. ✅ No file system initialization
5. ✅ No user programs built
6. ✅ No fs.img created
7. ✅ No mkfs built

## Performance Test

### Measure Boot Time
```bash
# Record time from power-on to "hart 7 starting"
```
- Should be faster than full xv6 (no FS init)

### Measure Memory Usage
```bash
# Check available memory after boot
```
- Should have more free memory (no ramdisk)

## Documentation Test

### Verify Documentation
1. ✅ BAREBONES_MODE.md exists and is complete
2. ✅ BAREBONES_QUICK_REF.md exists
3. ✅ README.md updated with barebones notice
4. ✅ All restoration instructions are clear
5. ✅ Examples and code snippets are correct

## Restoration Test

### Restore Full Functionality
Follow instructions in BAREBONES_MODE.md to restore:
1. Edit Makefile
2. Edit main.c
3. Edit proc.c
4. Edit vm.c
5. Rebuild

### Success Criteria
- ✅ SDHCI driver compiles
- ✅ User programs build
- ✅ fs.img created
- ✅ System boots with full functionality
- ✅ Shell starts
- ✅ File system works

## Automated Test Script

```bash
#!/bin/bash
# test_barebones.sh

echo "Testing barebones xv6..."

# Test 1: Clean build
echo "Test 1: Clean build"
make clean >/dev/null 2>&1
if make kernel/kernel >/dev/null 2>&1; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

# Test 2: Check binary size
echo "Test 2: Binary size check"
size=$(stat -f%z kernel/kernel 2>/dev/null || stat -c%s kernel/kernel)
if [ $size -lt 500000 ]; then
    echo "✅ Binary size OK ($size bytes)"
else
    echo "❌ Binary too large ($size bytes)"
    exit 1
fi

# Test 3: Check for ramdisk in BSS
echo "Test 3: Ramdisk removal check"
bss_size=$(riscv64-unknown-elf-size kernel/kernel | awk 'NR==2 {print $3}')
if [ $bss_size -lt 1000000 ]; then
    echo "✅ No large ramdisk in BSS"
else
    echo "❌ BSS too large (ramdisk present?)"
    exit 1
fi

# Test 4: Check symbols
echo "Test 4: Symbol check"
if riscv64-unknown-elf-nm kernel/kernel | grep -q "disk_stub"; then
    echo "✅ disk_stub present"
else
    echo "❌ disk_stub not found"
    exit 1
fi

if ! riscv64-unknown-elf-nm kernel/kernel | grep -q "sdhci_init"; then
    echo "✅ sdhci driver not compiled"
else
    echo "❌ sdhci driver still present"
    exit 1
fi

echo ""
echo "All tests passed! ✅"
```

## Test Results Checklist

- [ ] Compilation succeeds without errors
- [ ] Binary size < 500 KB
- [ ] BSS size < 1 MB (no ramdisk)
- [ ] QEMU boot successful
- [ ] Hardware boot successful (if available)
- [ ] Console output works
- [ ] No file system initialization
- [ ] No user programs loaded
- [ ] Memory usage reduced
- [ ] Boot time acceptable
- [ ] Documentation complete
- [ ] Restoration procedure works

## Troubleshooting

### Issue: Kernel doesn't compile
- Check RISC-V toolchain installation
- Verify all source files present
- Check Makefile syntax

### Issue: Kernel panics on boot
- Check commented-out sections in main.c
- Verify disk_stub.c doesn't panic
- Check memory addresses

### Issue: No console output
- Verify UART address (0xd4017000)
- Check baud rate (115200)
- Verify serial cable connection

### Issue: Binary too large
- Check if ramdisk is still compiled
- Verify SDHCI driver not included
- Check object file list in Makefile

---

**Test Plan Version**: 1.0  
**Date**: October 2025  
**Status**: Ready for testing
