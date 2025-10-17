# Quick Reference - Barebones xv6

## Status
**Barebones mode active** - No persistent storage, no file system, no user programs.

## What Changed

### Modified Files (7)
1. **Makefile** - Removed SDHCI driver, disabled user programs and fs.img
2. **kernel/main.c** - Commented out binit, iinit, fileinit, virtio_disk_init
3. **kernel/proc.c** - Disabled fsinit and kexec in forkret
4. **kernel/disk_stub.c** - Changed from panic to no-op for disk operations
5. **kernel/vm.c** - Commented out SDHCI memory mapping
6. **README.md** - Added barebones mode notice
7. **BAREBONES_MODE.md** - Full documentation (new file)

### Files Not Compiled
- kernel/sdhci.c (SD card driver)
- kernel/sdhci.h (SD card definitions)
- mkfs/mkfs (file system creation tool)
- user programs (all _* binaries)

### Build Products Not Created
- fs.img (file system image)
- User programs (_init, _sh, _cat, etc.)
- mkfs utility

## Boot Flow

```
Hardware → U-Boot → OpenSBI → xv6 Kernel → Scheduler (idle)
                                   ↓
                           No user processes
                           No file system
                           No storage access
```

## Memory Footprint
- **Before**: ~8.3 MB (with ramdisk)
- **After**: ~200 KB (kernel only)
- **Reduction**: 97%

## Key Points

✅ Kernel boots successfully  
✅ Console output works  
✅ Memory management active  
✅ Scheduler running  
✅ No storage dependencies  

❌ No file system  
❌ No user programs  
❌ No persistent storage  

## Building

```bash
# Just build the kernel
make clean
make kernel/kernel
```

## Restoration

To restore full functionality, see detailed instructions in `BAREBONES_MODE.md`.

Quick steps:
1. Edit Makefile: Change disk_stub.o → sdhci.o
2. Edit main.c: Uncomment binit, iinit, fileinit, virtio_disk_init
3. Edit proc.c: Uncomment fsinit and kexec, fix cwd initialization
4. Edit vm.c: Uncomment SDHCI memory mapping
5. Edit Makefile: Uncomment user programs, mkfs, fs.img sections
6. Rebuild: `make clean && make`

## Use Cases

- **Hardware bring-up**: Test basic kernel functionality
- **Kernel development**: Isolate kernel from storage complexity
- **Educational**: Study minimal OS without filesystem overhead
- **Embedded**: Minimal footprint for simple applications
- **Debugging**: Reduce moving parts when troubleshooting

## Documentation

- `BAREBONES_MODE.md` - Complete documentation
- `README.md` - Project overview
- `START_HERE.md` - Hardware and porting guide
- `PERSISTENT_STORAGE.md` - Previous storage implementation

---
**Version**: Barebones  
**Date**: October 2025  
**Target**: OrangePi RV2
