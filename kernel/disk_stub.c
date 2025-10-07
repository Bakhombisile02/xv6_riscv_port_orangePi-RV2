// Stub disk driver for OrangePi RV2
// TODO: Implement SD/MMC driver
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// Stub function - no disk available yet
void
virtio_disk_rw(struct buf *b, int write)
{
  // For now, just mark buffer as invalid to prevent hangs
  // This will cause file system operations to fail gracefully
  panic("virtio_disk_rw: no disk driver available");
}

// Stub init function
void
virtio_disk_init(void)
{
  // No-op for now
  printf("disk_stub: no disk driver loaded (SD/MMC support needed)\n");
}

// Stub interrupt handler
void
virtio_disk_intr(void)
{
  // No-op
}
