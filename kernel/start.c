#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "sbi.h"

void main();

// entry.S needs one stack per CPU.
__attribute__ ((aligned (16))) char stack0[4096 * NCPU];

// entry.S jumps here.
// On OrangePi RV2, we enter in S-mode (Supervisor mode) because
// OpenSBI firmware is running in M-mode and has already done
// the M-mode setup for us.
void
start()
{
  // We're already in S-mode thanks to OpenSBI!
  // No need for M-mode CSR access (mstatus, mepc, mret, etc.)
  
  // disable paging initially
  w_satp(0);

  // set up S-mode trap vector
  w_stvec((uint64)kernelvec);

  // enable S-mode interrupts
  // SIE_SEIE: external interrupts (from PLIC)
  // SIE_STIE: timer interrupts (from SBI/sstc)
  // SIE_SSIE: software interrupts (IPIs)
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

  // enable interrupts in sstatus
  w_sstatus(r_sstatus() | SSTATUS_SIE);

  // ask for timer interrupts via SBI
  // The Ky X1 SoC has sstc extension, but we'll use SBI for portability
  sbi_set_timer(r_time() + 10000000);

  // keep each CPU's hartid in its tp register, for cpuid()
  int id = r_mhartid();
  w_tp(id);

  // Jump to main() - we're already in S-mode, so just call it
  main();
}

// Request timer interrupt via SBI
void
timerinit()
{
  // Use SBI to set next timer interrupt
  // This will cause a timer interrupt in S-mode
  sbi_set_timer(r_time() + 10000000);
}
