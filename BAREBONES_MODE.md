# xv6 Barebones Mode - No Persistent Storage

## Overview

This version of xv6 has been stripped down to a **barebones configuration** that boots on the OrangePi RV2 without requiring any persistent storage (SD card).

## Changes Made

### 1. Storage System Removed

**Files Modified:**
- `Makefile` - Removed SDHCI driver, replaced with disk_stub
- `kernel/disk_stub.c` - Updated to provide no-op disk operations instead of panicking
- `kernel/main.c` - Commented out buffer cache, inode table, file table, and disk initialization
- `kernel/proc.c` - Disabled file system initialization (fsinit) and user program loading (kexec)

**Files No Longer Used:**
- `kernel/sdhci.c` - SD card driver (not compiled)
- `kernel/sdhci.h` - SD card definitions (not needed)
- `kernel/ramdisk.c` - RAM-based storage (not compiled)
- `kernel/virtio_disk.c` - QEMU disk driver (not compiled)

### 2. File System Disabled

The file system layers have been disabled in the boot sequence:
- ✗ Buffer cache (`binit()`) - skipped
- ✗ Inode table (`iinit()`) - skipped  
- ✗ File table (`fileinit()`) - skipped
- ✗ Disk driver (`virtio_disk_init()`) - skipped
- ✗ File system mount (`fsinit()`) - skipped

### 3. User Programs Removed

The system no longer builds or loads user programs:
- Makefile sections for user programs are commented out
- No `/init` program is loaded
- No shell or user utilities
- `fs.img` is not created
- `mkfs` utility is not built

### 4. Boot Sequence

**New Boot Flow:**
```
1. Hardware Reset
   └─> Boot ROM

2. U-Boot Bootloader
   └─> Loads kernel to 0x40000000

3. OpenSBI (M-mode supervisor)
   └─> Provides SBI interface

4. xv6 Kernel (S-mode)
   ├─> Console init
   ├─> Memory allocator
   ├─> Virtual memory
   ├─> Process table
   ├─> Trap handling
   ├─> Interrupt controller (PLIC)
   ├─> First process (initproc)
   └─> Scheduler

5. Scheduler Running
   └─> System is alive but has no user processes
```

## What Works

✅ **Kernel boots** - Full kernel initialization  
✅ **Console output** - UART console at 0xd4017000  
✅ **Memory management** - Physical and virtual memory  
✅ **Process scheduler** - Can create and schedule processes  
✅ **Interrupts** - PLIC and trap handling  
✅ **Timer** - Via SBI calls  

## What Doesn't Work

❌ **File system** - No disk, no files  
❌ **User programs** - No /init, no shell  
❌ **Persistent storage** - No SD card access  
❌ **exec()** - Cannot load programs  
❌ **System calls** - File-related syscalls unavailable  

## Use Cases

This barebones configuration is useful for:

1. **Kernel Development** - Test kernel features without storage complexity
2. **Hardware Bring-up** - Verify basic hardware functionality
3. **Educational Purposes** - Study kernel internals without filesystem
4. **Embedded Systems** - Minimal footprint for simple applications
5. **Debugging** - Isolate kernel issues from storage/filesystem bugs

## Building

To build the barebones kernel:

```bash
# Requires RISC-V cross-compiler
make clean
make kernel/kernel

# The resulting kernel/kernel can be loaded via U-Boot
```

## Booting on OrangePi RV2

### Via U-Boot

1. Copy `kernel/kernel` to SD card (FAT32 partition)
2. Boot OrangePi RV2, interrupt U-Boot
3. Load kernel:
   ```
   fatload mmc 0:1 0x40000000 kernel
   go 0x40000000
   ```

### Via TFTP (for development)

1. Setup TFTP server with `kernel/kernel`
2. Configure U-Boot networking
3. Load and boot:
   ```
   setenv serverip 192.168.1.100
   setenv ipaddr 192.168.1.50
   tftpboot 0x40000000 kernel
   go 0x40000000
   ```

## Expected Output

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

The system will boot and run the scheduler, but since there are no user processes, it will idle.

## Memory Footprint

**Reduced footprint compared to full xv6:**

| Component | Full xv6 | Barebones |
|-----------|----------|-----------|
| Kernel code | ~280 KB | ~200 KB |
| BSS (ramdisk) | 8 MB | 0 KB |
| Total | ~8.3 MB | ~200 KB |

**Savings: ~97% reduction in memory usage!**

## Restoring Persistent Storage

To restore the SDHCI driver and persistent storage:

1. Edit `Makefile`:
   - Change `$K/disk_stub.o` back to `$K/sdhci.o`
   
2. Edit `kernel/main.c`:
   - Uncomment `binit()`, `iinit()`, `fileinit()`, `virtio_disk_init()`
   
3. Edit `kernel/proc.c`:
   - Uncomment `fsinit(ROOTDEV)` in `forkret()`
   - Uncomment `kexec("/init", ...)` in `forkret()`
   - Change `p->cwd = 0` back to `p->cwd = namei("/")`

4. Edit `Makefile`:
   - Uncomment user programs section
   - Uncomment `mkfs` build
   - Uncomment `fs.img` creation
   - Add back disk to QEMU options

5. Rebuild:
   ```bash
   make clean
   make
   ```

## Technical Details

### Process Structure

Even in barebones mode, the process structure is intact:
- `initproc` is created by `userinit()`
- It has no working directory (`cwd = 0`)
- It has no code to execute (no kexec)
- It's marked as RUNNABLE but does nothing
- Scheduler continues to run

### Interrupt Handling

Interrupts still work:
- Timer interrupts via SBI
- UART interrupts for console
- PLIC routes device interrupts
- Context switching between kernel and (empty) user space

### Memory Layout

```
Physical Memory:
  0x00000000 - 0x80000000: I/O devices
  0x80000000 - 0x88000000: RAM (128 MB in QEMU)
                            (8 GB on real hardware)

Virtual Memory (kernel):
  0x00000000 - 0x80000000: Direct-mapped devices
  0x80000000 - ...:        Direct-mapped RAM
  TRAMPOLINE:              Trampoline page
  TRAPFRAME:               Trap frame pages
```

## Troubleshooting

### Kernel doesn't boot
- Check UART is configured correctly (0xd4017000, reg-shift=2)
- Verify OpenSBI is loaded and working
- Check kernel load address (should be 0x40000000)

### No console output
- UART may need initialization
- Check baud rate (115200)
- Verify console device permissions

### System hangs
- Expected behavior - no user processes to run
- Scheduler is running but all processes are blocked/empty
- Use debugger to verify kernel is alive

## Future Enhancements

Possible additions while maintaining barebones nature:

1. **Built-in test program** - Embed a simple test in kernel
2. **Serial console shell** - Minimal kernel-mode command interface
3. **Memory diagnostics** - Commands to inspect system state
4. **Performance counters** - Track kernel operations
5. **Hardware tests** - Verify peripherals without filesystem

## References

- Original xv6: https://github.com/mit-pdos/xv6-riscv
- OrangePi RV2 Manual: `docs/OrangePi_RV2_X1_User Manual_v1.1.pdf`
- Ky X1 Chip Manual: `docs/Ky X1 Chip Manual.pdf`
- RISC-V Spec: https://riscv.org/specifications/

## License

Same as xv6: MIT License

---

**Last Updated**: October 2025  
**Status**: Barebones mode active  
**Target**: OrangePi RV2 (Ky X1 SoC)
