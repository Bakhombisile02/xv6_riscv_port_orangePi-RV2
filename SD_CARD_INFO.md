# SD Card Information for xv6 Port

## Hardware Discovery

### SD Card Details
- **Device**: `/dev/mmcblk0` 
- **Capacity**: 29.7 GB (32GB card)
- **Model**: SE32G
- **Type**: SDHC (SDR104 - Ultra High Speed)
- **Partition**: `/dev/mmcblk0p1` (29.4 GB, ext4)
- **Label**: opi_root
- **Currently**: Unmounted (available for use!)

### SD/MMC Controller (SDHCI)
- **Base Address**: `0xd4280000`
- **Compatible**: "ky,x1-sdhci"
- **Interrupt**: IRQ 99 (0x63)
- **Driver**: sdhci-ky (SDHCI platform driver)
- **DMA Mode**: ADMA (Advanced DMA)
- **Status**: Working (successfully detected card)

### Additional MMC Controllers
The OrangePi RV2 has 3 SD/MMC controllers:
- **mmc0**: 0xd4280000 (our target - external SD card slot)
- **mmc1**: 0xd4280800 (possibly eMMC or another slot)
- **mmc2**: 0xd4281000 (possibly eMMC or another slot)

## Device Tree Information

```dts
sdh@d4280000 {
    compatible = "ky,x1-sdhci";
    reg = <0x00 0xd4280000 0x00 0x200>;
    interrupts = <0x63>;          // IRQ 99
    interrupt-parent = <0x1e>;     // PLIC
    resets = <0x1d 0x47 0x1d 0x48>;
    reset-names = "sdh_axi", "sdh0";
    clocks = <0x03 0x89 0x03 0x88 0x03 0x64>;
    // ... more properties
};
```

## Current Linux Driver

From dmesg:
```
[3.859213] mmc0: SDHCI controller on d4280000.sdh [d4280000.sdh] using ADMA
[132.143242] mmc0: set tx_delaycode: 159
[132.153187] mmc0: new ultra high speed SDR104 SDHC card at address aaaa
[132.154137] mmcblk0: mmc0:aaaa SE32G 29.7 GiB
```

## Implications for xv6

### Good News ✅
1. **SD card is available** - 32GB of storage ready to use
2. **Standard SDHCI controller** - Well-documented interface
3. **Working in Linux** - We can reference the Linux driver
4. **Dedicated controller** - Won't interfere with system disk

### Implementation Options

#### Option 1: Full SDHCI Driver (Recommended)
Implement a proper SDHCI driver for xv6:
- **Pros**: 
  - Full functionality
  - Can use the entire SD card
  - Learn about real hardware
  - Educational value
- **Cons**:
  - Complex (SDHCI spec is large)
  - Time-consuming
  - Need to handle DMA, clocks, resets
  
**Estimated effort**: 1-2 weeks of development

#### Option 2: Simple Block Device Driver
Implement a simplified driver:
- **Pros**:
  - Faster to implement
  - Just need basic read/write
  - Good enough for xv6
- **Cons**:
  - Less feature-complete
  - May be slow (no DMA initially)
  
**Estimated effort**: 2-3 days

#### Option 3: Ramdisk (Quick Start)
Use RAM as a disk:
- **Pros**:
  - Very fast
  - No hardware driver needed
  - Can test file system immediately
- **Cons**:
  - Loses data on reboot
  - Limited by RAM size
  
**Estimated effort**: Few hours

### Recommended Approach

**Phase 1: Boot without disk** (✅ DONE)
- Get kernel booting
- Console output working
- Basic functionality

**Phase 2: Ramdisk** (Quick win)
- Implement simple ramdisk
- Test file system
- Get shell running
- **Status**: Can do this now!

**Phase 3: SD Card Driver** (Future)
- Implement SDHCI driver
- Replace ramdisk with real storage
- Full persistence
- **Status**: Future work

## Ramdisk Implementation

For quick testing, we can create a ramdisk:

```c
// kernel/ramdisk.c
#define RAMDISK_SIZE (4 * 1024 * 1024) // 4MB
static char ramdisk[RAMDISK_SIZE];

void ramdisk_rw(struct buf *b, int write) {
  uint64 offset = b->blockno * BSIZE;
  if (offset + BSIZE > RAMDISK_SIZE)
    panic("ramdisk: out of bounds");
  
  if (write)
    memmove(ramdisk + offset, b->data, BSIZE);
  else
    memmove(b->data, ramdisk + offset, BSIZE);
}
```

This would replace `disk_stub.c` and allow xv6 to:
- Boot completely
- Run the shell
- Execute user programs
- Test all OS features
- **But**: Lose data on reboot

## SD Card Driver Skeleton

For future reference, here's what's needed:

### 1. Register Definitions
```c
#define SDHCI_BASE 0xd4280000L
#define SDHCI_IRQ  99

// SDHCI Standard Registers
#define SDHCI_DMA_ADDRESS           0x00
#define SDHCI_BLOCK_SIZE            0x04
#define SDHCI_BLOCK_COUNT           0x06
#define SDHCI_ARGUMENT              0x08
#define SDHCI_TRANSFER_MODE         0x0C
#define SDHCI_COMMAND               0x0E
#define SDHCI_RESPONSE              0x10
#define SDHCI_BUFFER                0x20
#define SDHCI_PRESENT_STATE         0x24
#define SDHCI_HOST_CONTROL          0x28
// ... many more registers
```

### 2. Initialization Sequence
1. Reset controller
2. Set clock frequency
3. Enable power
4. Initialize card (CMD0, CMD8, ACMD41)
5. Get card info (CMD2, CMD3, CMD9)
6. Select card (CMD7)
7. Set block size (CMD16)
8. Ready for I/O

### 3. Read/Write Operations
- CMD17: Read single block
- CMD24: Write single block
- CMD18/25: Multi-block transfers
- Use ADMA for DMA transfers

### 4. Interrupt Handling
- Register IRQ 99 with PLIC
- Handle command complete
- Handle transfer complete
- Handle errors

## Resources

### SDHCI Specification
- SD Host Controller Simplified Specification
- Available from SD Association
- ~200 pages of detailed register descriptions

### Linux Driver Reference
- `drivers/mmc/host/sdhci-of-spacemit.c` (likely similar)
- `drivers/mmc/host/sdhci.c` (core SDHCI)
- Can study initialization and command sequences

### xv6 Integration Points
- Replace `disk_stub.c` with real driver
- Update `memlayout.h` with SDHCI address
- Add SDHCI IRQ to PLIC initialization
- Implement block device interface

## Next Actions

### Immediate (for testing)
1. Implement ramdisk to replace disk_stub
2. Test full xv6 boot with ramdisk
3. Verify all OS features work

### Short-term (for persistence)
1. Study SDHCI specification
2. Implement basic SDHCI initialization
3. Implement single-block read/write
4. Test with real SD card

### Long-term (for full functionality)
1. Implement DMA transfers
2. Add multi-block operations
3. Handle all error cases
4. Performance optimization

---

**Current Status**: SD card detected and available at 0xd4280000
**Recommendation**: Start with ramdisk for quick testing, then implement SD driver
**Storage Available**: 29.7 GB on `/dev/mmcblk0`
