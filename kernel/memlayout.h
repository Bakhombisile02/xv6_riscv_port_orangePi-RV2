// Physical memory layout

// OrangePi RV2 (Ky X1 SoC) memory layout
// Based on device tree and hardware documentation
//
// Memory regions:
//   Region 1: 0x00000000 - 0x7FFFFFFF (2 GB)
//   Region 2: 0x100000000 - 0x17FFFFFFF (2 GB, above 4GB)
//
// Boot process:
//   U-Boot -> OpenSBI (M-mode) -> Kernel (S-mode)
//
// Hardware addresses:
//   0xd4017000 -- UART0 (console, PXA UART)
//   0xe0000000 -- PLIC (Platform Level Interrupt Controller)
//   (No CLINT - uses sstc timer extension via SBI)

// the kernel uses physical memory thus:
// 40000000 -- kernel entry point (loaded by bootloader)
// 40000000 -- entry.S, then kernel text and data
// end -- start of kernel page allocation area
// PHYSTOP -- end RAM used by the kernel

// OrangePi RV2 UART0 (console)
#define UART0 0xd4017000L
#define UART0_IRQ 74

// OrangePi RV2 SDHCI (SD card controller)
#define SDHCI0 0xd4280000L
#define SDHCI0_IRQ 99

// No VirtIO disk on real hardware (will add SD/MMC later)
// #define VIRTIO0 0x10001000
// #define VIRTIO0_IRQ 1

// OrangePi RV2 PLIC configuration
// Base address: 0xe0000000
// 159 interrupt sources, 16 contexts (2 per hart: M+S mode)
#define PLIC 0xe0000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)

// S-mode context offsets (context = hart*2 + 1 for S-mode)
// Each context has enable bits at different offsets
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + ((hart)*2 + 1)*0x80)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + ((hart)*2 + 1)*0x1000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + ((hart)*2 + 1)*0x1000)

// No CLINT on OrangePi RV2 - timer handled via SBI/sstc
// #define CLINT 0x2000000L
// #define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
// #define CLINT_MTIME (CLINT + 0xBFF8)

// Kernel memory layout
// Start at 1GB mark (common for embedded systems)
// Use first 512MB for kernel (conservative estimate)
#define KERNBASE 0x40000000L
#define PHYSTOP (KERNBASE + 512*1024*1024)

// Alternative: Start at 2GB mark (like QEMU)
// Uncomment below and comment above if kernel loads there
// #define KERNBASE 0x80000000L
// #define PHYSTOP (KERNBASE + 128*1024*1024)

// map the trampoline page to the highest address,
// in both user and kernel space.
#define TRAMPOLINE (MAXVA - PGSIZE)

// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
#define KSTACK(p) (TRAMPOLINE - ((p)+1)* 2*PGSIZE)

// User memory layout.
// Address zero first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
//   ...
//   TRAPFRAME (p->trapframe, used by the trampoline)
//   TRAMPOLINE (the same page as in the kernel)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)
