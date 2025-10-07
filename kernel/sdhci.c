// SDHCI (SD Host Controller Interface) driver for OrangePi RV2
// Implements SD card block device interface for xv6

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"
#include "sdhci.h"

// SDHCI register access macros
#define SDHCI_READ(reg)         (*((volatile uint32*)(SDHCI_BASE + (reg))))
#define SDHCI_WRITE(reg, val)   (*((volatile uint32*)(SDHCI_BASE + (reg))) = (val))
#define SDHCI_READ16(reg)       (*((volatile uint16*)(SDHCI_BASE + (reg))))
#define SDHCI_WRITE16(reg, val) (*((volatile uint16*)(SDHCI_BASE + (reg))) = (val))
#define SDHCI_READ8(reg)        (*((volatile uint8*)(SDHCI_BASE + (reg))))
#define SDHCI_WRITE8(reg, val)  (*((volatile uint8*)(SDHCI_BASE + (reg))) = (val))

// SD card state
static struct {
  struct spinlock lock;
  int present;              // Card is present
  int initialized;          // Card is initialized
  uint32 rca;              // Relative Card Address
  uint32 capacity;         // Card capacity in blocks
  int high_capacity;       // SDHC/SDXC card
} sdcard;

// Simple delay function (approximate)
static void
sdhci_delay_ms(int ms)
{
  for(int i = 0; i < ms; i++) {
    for(volatile int j = 0; j < 10000; j++)
      ;
  }
}

// Wait for condition with timeout
static int
sdhci_wait_for(uint32 reg, uint32 mask, uint32 value, int timeout_ms)
{
  int count = timeout_ms * 10; // rough approximation
  while(count > 0) {
    if((SDHCI_READ(reg) & mask) == value)
      return 0;
    sdhci_delay_ms(1);
    count--;
  }
  return -1; // timeout
}

// Send command to SD card
static int
sdhci_send_cmd(uint8 cmd, uint32 arg, uint8 resp_type, uint32 *response)
{
  uint16 cmd_reg = 0;
  uint32 flags = 0;
  
  // Wait for CMD and DATA lines to be ready
  if(sdhci_wait_for(SDHCI_PRESENT_STATE, 
                    SDHCI_CMD_INHIBIT | SDHCI_DATA_INHIBIT, 
                    0, SDHCI_CMD_TIMEOUT) < 0) {
    printf("sdhci: timeout waiting for cmd/data ready\n");
    return -1;
  }
  
  // Clear interrupt status
  SDHCI_WRITE(SDHCI_INT_STATUS, 0xFFFFFFFF);
  
  // Set argument
  SDHCI_WRITE(SDHCI_ARGUMENT, arg);
  
  // Build command register value
  cmd_reg = (cmd << 8);
  
  // Set response type
  switch(resp_type) {
    case SD_RSP_NONE:
      flags = SDHCI_CMD_RESP_NONE;
      break;
    case SD_RSP_R1:
    case SD_RSP_R6:
    case SD_RSP_R7:
      flags = SDHCI_CMD_RESP_SHORT | SDHCI_CMD_CRC | SDHCI_CMD_INDEX;
      break;
    case SD_RSP_R2:
      flags = SDHCI_CMD_RESP_LONG | SDHCI_CMD_CRC;
      break;
    case SD_RSP_R3:
      flags = SDHCI_CMD_RESP_SHORT;
      break;
    default:
      flags = SDHCI_CMD_RESP_NONE;
  }
  
  cmd_reg |= flags;
  
  // Send command
  SDHCI_WRITE16(SDHCI_COMMAND, cmd_reg);
  
  // Wait for command complete
  if(sdhci_wait_for(SDHCI_INT_STATUS, SDHCI_INT_RESPONSE, 
                    SDHCI_INT_RESPONSE, SDHCI_CMD_TIMEOUT) < 0) {
    printf("sdhci: cmd%d timeout\n", cmd);
    return -1;
  }
  
  // Check for errors
  uint32 status = SDHCI_READ(SDHCI_INT_STATUS);
  if(status & SDHCI_INT_ERROR_MASK) {
    printf("sdhci: cmd%d error: status=0x%x\n", cmd, status);
    return -1;
  }
  
  // Read response if requested
  if(response && resp_type != SD_RSP_NONE) {
    if(resp_type == SD_RSP_R2) {
      // Long response (128 bits)
      response[0] = SDHCI_READ(SDHCI_RESPONSE + 0);
      response[1] = SDHCI_READ(SDHCI_RESPONSE + 4);
      response[2] = SDHCI_READ(SDHCI_RESPONSE + 8);
      response[3] = SDHCI_READ(SDHCI_RESPONSE + 12);
    } else {
      // Short response (32 bits)
      response[0] = SDHCI_READ(SDHCI_RESPONSE);
    }
  }
  
  // Clear status
  SDHCI_WRITE(SDHCI_INT_STATUS, status);
  
  return 0;
}

// Send application command (CMD55 + ACMD)
static int
sdhci_send_acmd(uint8 acmd, uint32 arg, uint8 resp_type, uint32 *response)
{
  // First send CMD55 (APP_CMD)
  if(sdhci_send_cmd(SD_CMD_APP_CMD, sdcard.rca << 16, SD_RSP_R1, 0) < 0)
    return -1;
  
  // Then send the actual application command
  return sdhci_send_cmd(acmd, arg, resp_type, response);
}

// Reset SDHCI controller
static int
sdhci_reset(uint8 mask)
{
  SDHCI_WRITE8(SDHCI_SOFTWARE_RESET, mask);
  
  // Wait for reset to complete
  int timeout = 100;
  while((SDHCI_READ8(SDHCI_SOFTWARE_RESET) & mask) && timeout > 0) {
    sdhci_delay_ms(1);
    timeout--;
  }
  
  if(timeout == 0) {
    printf("sdhci: reset timeout\n");
    return -1;
  }
  
  return 0;
}

// Set SDHCI clock
static int
sdhci_set_clock(uint32 clock_hz)
{
  uint16 clk = 0;
  uint32 base_clock = 50000000; // 50MHz base clock (approximate)
  uint32 div;
  
  // Disable clock
  SDHCI_WRITE16(SDHCI_CLOCK_CONTROL, 0);
  
  // Calculate divider
  // Target: 400KHz for initialization, 25MHz for normal operation
  if(clock_hz >= base_clock)
    div = 1;
  else
    div = base_clock / clock_hz;
  
  if(div > 256) div = 256;
  
  // Set divider and enable internal clock
  clk = ((div & 0xFF) << SDHCI_DIVIDER_SHIFT) | SDHCI_CLOCK_INT_EN;
  SDHCI_WRITE16(SDHCI_CLOCK_CONTROL, clk);
  
  // Wait for clock to stabilize
  if(sdhci_wait_for(SDHCI_CLOCK_CONTROL, SDHCI_CLOCK_INT_STABLE,
                    SDHCI_CLOCK_INT_STABLE, 10) < 0) {
    printf("sdhci: clock not stable\n");
    return -1;
  }
  
  // Enable SD clock
  clk |= SDHCI_CLOCK_CARD_EN;
  SDHCI_WRITE16(SDHCI_CLOCK_CONTROL, clk);
  sdhci_delay_ms(10);
  
  return 0;
}

// Initialize SD card
static int
sdhci_card_init(void)
{
  uint32 response[4];
  int retries;
  
  printf("sdhci: initializing SD card...\n");
  
  // CMD0: GO_IDLE_STATE
  if(sdhci_send_cmd(SD_CMD_GO_IDLE_STATE, 0, SD_RSP_NONE, 0) < 0) {
    printf("sdhci: CMD0 failed\n");
    return -1;
  }
  sdhci_delay_ms(10);
  
  // CMD8: SEND_IF_COND (check voltage and card type)
  if(sdhci_send_cmd(SD_CMD_SEND_IF_COND, 0x1AA, SD_RSP_R7, response) == 0) {
    // SD 2.0 or later card
    if((response[0] & 0xFF) != 0xAA) {
      printf("sdhci: CMD8 bad echo\n");
      return -1;
    }
  }
  
  // ACMD41: SD_SEND_OP_COND (initialize card)
  retries = 100;
  while(retries > 0) {
    uint32 ocr_arg = 0x40FF8000; // HCS=1, 3.0-3.6V
    if(sdhci_send_acmd(SD_ACMD_SD_SEND_OP_COND, ocr_arg, SD_RSP_R3, response) < 0) {
      printf("sdhci: ACMD41 failed\n");
      return -1;
    }
    
    // Check if card is ready
    if(response[0] & SD_OCR_BUSY) {
      sdcard.high_capacity = (response[0] & SD_OCR_CCS) ? 1 : 0;
      printf("sdhci: card ready, %s capacity\n", 
             sdcard.high_capacity ? "high" : "standard");
      break;
    }
    
    sdhci_delay_ms(10);
    retries--;
  }
  
  if(retries == 0) {
    printf("sdhci: card init timeout\n");
    return -1;
  }
  
  // CMD2: ALL_SEND_CID
  if(sdhci_send_cmd(SD_CMD_ALL_SEND_CID, 0, SD_RSP_R2, response) < 0) {
    printf("sdhci: CMD2 failed\n");
    return -1;
  }
  
  // CMD3: SEND_RELATIVE_ADDR
  if(sdhci_send_cmd(SD_CMD_SEND_RELATIVE_ADDR, 0, SD_RSP_R6, response) < 0) {
    printf("sdhci: CMD3 failed\n");
    return -1;
  }
  sdcard.rca = response[0] >> 16;
  printf("sdhci: RCA = 0x%x\n", sdcard.rca);
  
  // CMD9: SEND_CSD (get card capacity)
  if(sdhci_send_cmd(SD_CMD_SEND_CSD, sdcard.rca << 16, SD_RSP_R2, response) < 0) {
    printf("sdhci: CMD9 failed\n");
    return -1;
  }
  
  // Calculate capacity (simplified)
  if(sdcard.high_capacity) {
    uint32 c_size = ((response[1] & 0x3F) << 16) | (response[2] >> 16);
    sdcard.capacity = (c_size + 1) * 1024; // in 512-byte blocks
  } else {
    // Standard capacity calculation - simplified
    sdcard.capacity = 0; // will use block addressing anyway
  }
  
  printf("sdhci: capacity = %d blocks (%.1d MB)\n", 
         sdcard.capacity, (sdcard.capacity / 2) / 1024);
  
  // CMD7: SELECT_CARD
  if(sdhci_send_cmd(SD_CMD_SELECT_CARD, sdcard.rca << 16, SD_RSP_R1, 0) < 0) {
    printf("sdhci: CMD7 failed\n");
    return -1;
  }
  
  // CMD16: SET_BLOCKLEN (set block size to 512)
  if(sdhci_send_cmd(SD_CMD_SET_BLOCKLEN, 512, SD_RSP_R1, 0) < 0) {
    printf("sdhci: CMD16 failed\n");
    return -1;
  }
  
  // Increase clock speed for normal operations
  sdhci_set_clock(25000000); // 25MHz
  
  printf("sdhci: card initialized successfully\n");
  sdcard.initialized = 1;
  return 0;
}

// Initialize SDHCI controller
void
sdhci_init(void)
{
  printf("sdhci: initializing controller at 0x%lx\n", SDHCI_BASE);
  
  initlock(&sdcard.lock, "sdcard");
  sdcard.present = 0;
  sdcard.initialized = 0;
  sdcard.rca = 0;
  
  // Reset controller
  if(sdhci_reset(SDHCI_RESET_ALL) < 0) {
    printf("sdhci: controller reset failed\n");
    return;
  }
  
  // Check card presence
  uint32 present_state = SDHCI_READ(SDHCI_PRESENT_STATE);
  if(!(present_state & SDHCI_CARD_PRESENT)) {
    printf("sdhci: no card detected\n");
    return;
  }
  sdcard.present = 1;
  printf("sdhci: card detected\n");
  
  // Set power to 3.3V
  SDHCI_WRITE8(SDHCI_POWER_CONTROL, SDHCI_POWER_330 | SDHCI_POWER_ON);
  sdhci_delay_ms(10);
  
  // Set timeout to maximum
  SDHCI_WRITE8(SDHCI_TIMEOUT_CONTROL, 0x0E);
  
  // Enable interrupts (for status checking, not interrupt-driven yet)
  SDHCI_WRITE(SDHCI_INT_ENABLE, 0xFFFFFFFF);
  SDHCI_WRITE(SDHCI_SIGNAL_ENABLE, 0); // No interrupt signals for now
  
  // Set initial clock (400KHz for card identification)
  if(sdhci_set_clock(400000) < 0) {
    printf("sdhci: clock setup failed\n");
    return;
  }
  
  // Initialize SD card
  if(sdhci_card_init() < 0) {
    printf("sdhci: card initialization failed\n");
    return;
  }
  
  printf("sdhci: ready\n");
}

// Read a single block from SD card using PIO mode
int
sdhci_read_block(uint32 block, void *buf)
{
  uint32 *data = (uint32*)buf;
  uint32 status;
  
  if(!sdcard.initialized) {
    printf("sdhci: card not initialized\n");
    return -1;
  }
  
  acquire(&sdcard.lock);
  
  // Set block size and count
  SDHCI_WRITE16(SDHCI_BLOCK_SIZE, 512);
  SDHCI_WRITE16(SDHCI_BLOCK_COUNT, 1);
  
  // Set transfer mode (read, single block)
  SDHCI_WRITE16(SDHCI_TRANSFER_MODE, SDHCI_TRNS_READ);
  
  // Send READ_SINGLE_BLOCK command
  if(sdhci_send_cmd(SD_CMD_READ_SINGLE_BLOCK, block, SD_RSP_R1, 0) < 0) {
    release(&sdcard.lock);
    return -1;
  }
  
  // Read data using PIO (Programmed I/O)
  for(int i = 0; i < 512 / 4; i++) {
    // Wait for buffer to have data
    if(sdhci_wait_for(SDHCI_PRESENT_STATE, SDHCI_DATA_AVAILABLE,
                      SDHCI_DATA_AVAILABLE, SDHCI_DATA_TIMEOUT) < 0) {
      printf("sdhci: read timeout at word %d\n", i);
      release(&sdcard.lock);
      return -1;
    }
    
    // Read 32-bit word from buffer
    data[i] = SDHCI_READ(SDHCI_BUFFER);
  }
  
  // Wait for transfer complete
  if(sdhci_wait_for(SDHCI_INT_STATUS, SDHCI_INT_DATA_END,
                    SDHCI_INT_DATA_END, SDHCI_DATA_TIMEOUT) < 0) {
    printf("sdhci: read transfer not complete\n");
    release(&sdcard.lock);
    return -1;
  }
  
  // Check for errors
  status = SDHCI_READ(SDHCI_INT_STATUS);
  if(status & SDHCI_INT_ERROR_MASK) {
    printf("sdhci: read error: status=0x%x\n", status);
    release(&sdcard.lock);
    return -1;
  }
  
  // Clear status
  SDHCI_WRITE(SDHCI_INT_STATUS, status);
  
  release(&sdcard.lock);
  return 0;
}

// Write a single block to SD card using PIO mode
int
sdhci_write_block(uint32 block, void *buf)
{
  uint32 *data = (uint32*)buf;
  uint32 status;
  
  if(!sdcard.initialized) {
    printf("sdhci: card not initialized\n");
    return -1;
  }
  
  acquire(&sdcard.lock);
  
  // Set block size and count
  SDHCI_WRITE16(SDHCI_BLOCK_SIZE, 512);
  SDHCI_WRITE16(SDHCI_BLOCK_COUNT, 1);
  
  // Set transfer mode (write, single block)
  SDHCI_WRITE16(SDHCI_TRANSFER_MODE, 0); // Write mode
  
  // Send WRITE_BLOCK command
  if(sdhci_send_cmd(SD_CMD_WRITE_BLOCK, block, SD_RSP_R1, 0) < 0) {
    release(&sdcard.lock);
    return -1;
  }
  
  // Write data using PIO (Programmed I/O)
  for(int i = 0; i < 512 / 4; i++) {
    // Wait for buffer to be ready
    if(sdhci_wait_for(SDHCI_PRESENT_STATE, SDHCI_SPACE_AVAILABLE,
                      SDHCI_SPACE_AVAILABLE, SDHCI_DATA_TIMEOUT) < 0) {
      printf("sdhci: write timeout at word %d\n", i);
      release(&sdcard.lock);
      return -1;
    }
    
    // Write 32-bit word to buffer
    SDHCI_WRITE(SDHCI_BUFFER, data[i]);
  }
  
  // Wait for transfer complete
  if(sdhci_wait_for(SDHCI_INT_STATUS, SDHCI_INT_DATA_END,
                    SDHCI_INT_DATA_END, SDHCI_DATA_TIMEOUT) < 0) {
    printf("sdhci: write transfer not complete\n");
    release(&sdcard.lock);
    return -1;
  }
  
  // Check for errors
  status = SDHCI_READ(SDHCI_INT_STATUS);
  if(status & SDHCI_INT_ERROR_MASK) {
    printf("sdhci: write error: status=0x%x\n", status);
    release(&sdcard.lock);
    return -1;
  }
  
  // Clear status
  SDHCI_WRITE(SDHCI_INT_STATUS, status);
  
  release(&sdcard.lock);
  return 0;
}

// xv6 virtio_disk interface compatibility wrappers
void
virtio_disk_init(void)
{
  sdhci_init();
}

void
virtio_disk_rw(struct buf *b, int write)
{
  int ret;
  
  if(write)
    ret = sdhci_write_block(b->blockno, b->data);
  else
    ret = sdhci_read_block(b->blockno, b->data);
  
  if(ret < 0) {
    panic("sdhci: I/O error");
  }
}

void
virtio_disk_intr(void)
{
  // No interrupts in this implementation (polling mode)
}
