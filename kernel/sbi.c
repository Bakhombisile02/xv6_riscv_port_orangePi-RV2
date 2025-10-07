// OpenSBI interface implementation
#include "types.h"
#include "sbi.h"

// Perform SBI ecall
// The SBI calling convention uses:
// a7: Extension ID (EID)
// a6: Function ID (FID)  
// a0-a5: Arguments
// Return: a0=error, a1=value
struct sbiret sbi_ecall(int ext, int fid,
                        unsigned long arg0, unsigned long arg1,
                        unsigned long arg2, unsigned long arg3,
                        unsigned long arg4, unsigned long arg5)
{
  register unsigned long a0 asm("a0") = arg0;
  register unsigned long a1 asm("a1") = arg1;
  register unsigned long a2 asm("a2") = arg2;
  register unsigned long a3 asm("a3") = arg3;
  register unsigned long a4 asm("a4") = arg4;
  register unsigned long a5 asm("a5") = arg5;
  register unsigned long a6 asm("a6") = fid;
  register unsigned long a7 asm("a7") = ext;

  asm volatile("ecall"
               : "+r"(a0), "+r"(a1)
               : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
               : "memory");

  struct sbiret ret;
  ret.error = a0;
  ret.value = a1;
  return ret;
}

// Legacy console putchar (SBI v0.1)
void sbi_console_putchar(int ch)
{
  sbi_ecall(SBI_EXT_0_1_CONSOLE_PUTCHAR, 0, ch, 0, 0, 0, 0, 0);
}

// Legacy console getchar (SBI v0.1)
int sbi_console_getchar(void)
{
  struct sbiret ret = sbi_ecall(SBI_EXT_0_1_CONSOLE_GETCHAR, 0, 0, 0, 0, 0, 0, 0);
  return ret.error;
}

// Set timer for next interrupt
void sbi_set_timer(uint64 stime_value)
{
  sbi_ecall(SBI_EXT_0_1_SET_TIMER, 0, stime_value, 0, 0, 0, 0, 0);
}

// Shutdown the system
void sbi_shutdown(void)
{
  sbi_ecall(SBI_EXT_0_1_SHUTDOWN, 0, 0, 0, 0, 0, 0, 0);
}
