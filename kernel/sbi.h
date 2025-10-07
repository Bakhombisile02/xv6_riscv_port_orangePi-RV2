// OpenSBI (Supervisor Binary Interface) support for xv6
// SBI provides services to S-mode kernels from M-mode firmware

#ifndef SBI_H
#define SBI_H

// SBI Extension IDs (EID)
#define SBI_EXT_BASE                0x10
#define SBI_EXT_TIME                0x54494D45
#define SBI_EXT_IPI                 0x735049
#define SBI_EXT_RFENCE              0x52464E43
#define SBI_EXT_HSM                 0x48534D
#define SBI_EXT_SRST                0x53525354

// Legacy SBI Extension IDs (still supported)
#define SBI_EXT_0_1_SET_TIMER       0x0
#define SBI_EXT_0_1_CONSOLE_PUTCHAR 0x1
#define SBI_EXT_0_1_CONSOLE_GETCHAR 0x2
#define SBI_EXT_0_1_SHUTDOWN        0x8

// SBI function return structure
struct sbiret {
  long error;
  long value;
};

// SBI error codes
#define SBI_SUCCESS               0
#define SBI_ERR_FAILED           -1
#define SBI_ERR_NOT_SUPPORTED    -2
#define SBI_ERR_INVALID_PARAM    -3
#define SBI_ERR_DENIED           -4
#define SBI_ERR_INVALID_ADDRESS  -5
#define SBI_ERR_ALREADY_AVAILABLE -6
#define SBI_ERR_ALREADY_STARTED  -7
#define SBI_ERR_ALREADY_STOPPED  -8

// Core SBI functions
struct sbiret sbi_ecall(int ext, int fid, 
                        unsigned long arg0, unsigned long arg1,
                        unsigned long arg2, unsigned long arg3,
                        unsigned long arg4, unsigned long arg5);

// Legacy console functions (for early debug)
void sbi_console_putchar(int ch);
int sbi_console_getchar(void);

// Timer extension
void sbi_set_timer(uint64 stime_value);

// System reset
void sbi_shutdown(void);

#endif // SBI_H
