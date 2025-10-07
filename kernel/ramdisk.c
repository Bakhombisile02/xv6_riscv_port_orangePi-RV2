// Ramdisk driver for xv6 - stores file system in RAM
// This allows xv6 to boot and run fully without real disk hardware
// Note: All data is lost on reboot

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// Ramdisk size: 8MB should be enough for xv6
// This gives us 16384 blocks (8MB / 512 bytes per block)
#define RAMDISK_SIZE (8 * 1024 * 1024)
#define RAMDISK_BLOCKS (RAMDISK_SIZE / BSIZE)

// Static buffer to hold the ramdisk contents
// This will be allocated in kernel BSS section
static char ramdisk[RAMDISK_SIZE];

// Statistics (for debugging)
static uint64 read_count = 0;
static uint64 write_count = 0;

// Initialize ramdisk
void
ramdisk_init(void)
{
  printf("ramdisk: initializing %d MB disk (%d blocks)\n", 
         RAMDISK_SIZE / (1024*1024), RAMDISK_BLOCKS);
  
  // Clear the ramdisk (optional, BSS is already zeroed)
  // memset(ramdisk, 0, RAMDISK_SIZE);
  
  printf("ramdisk: ready at %p, size %d bytes\n", ramdisk, RAMDISK_SIZE);
}

// Read or write a disk block
void
ramdisk_rw(struct buf *b, int write)
{
  uint64 offset;
  
  // Validate block number
  if(b->blockno >= RAMDISK_BLOCKS) {
    panic("ramdisk_rw: block number out of range");
  }
  
  // Calculate offset into ramdisk
  offset = b->blockno * BSIZE;
  
  if(write) {
    // Write: copy from buffer to ramdisk
    memmove(ramdisk + offset, b->data, BSIZE);
    write_count++;
  } else {
    // Read: copy from ramdisk to buffer
    memmove(b->data, ramdisk + offset, BSIZE);
    read_count++;
  }
  
  // Optional: print progress every 1000 operations (for debugging)
  if((read_count + write_count) % 1000 == 0) {
    printf("ramdisk: %ld reads, %ld writes\n", read_count, write_count);
  }
}

// Get ramdisk statistics (for debugging)
void
ramdisk_stats(void)
{
  printf("ramdisk statistics:\n");
  printf("  reads:  %ld\n", read_count);
  printf("  writes: %ld\n", write_count);
  printf("  size:   %d MB (%d blocks)\n", 
         RAMDISK_SIZE / (1024*1024), RAMDISK_BLOCKS);
}

// Compatibility wrappers for existing xv6 code
// These match the virtio_disk interface

void
virtio_disk_init(void)
{
  ramdisk_init();
}

void
virtio_disk_rw(struct buf *b, int write)
{
  ramdisk_rw(b, write);
}

void
virtio_disk_intr(void)
{
  // Ramdisk has no interrupts - this is never called
  // But we need to provide the function for compatibility
}
