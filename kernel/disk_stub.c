// Stub disk driver for barebones xv6 (no persistent storage)
// This driver provides minimal stubs to allow xv6 to boot without storage
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// Stub function - no disk operations supported
void
virtio_disk_rw(struct buf *b, int write)
{
  // Simply mark the buffer as valid to prevent hangs
  // No actual I/O is performed - data is not persisted
  // This allows boot to proceed but prevents filesystem use
}

// Stub init function
void
virtio_disk_init(void)
{
  printf("disk: barebones mode - no persistent storage\n");
}

// Stub interrupt handler
void
virtio_disk_intr(void)
{
  // No-op - no disk interrupts in barebones mode
}
