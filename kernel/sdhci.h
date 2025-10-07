// SDHCI (SD Host Controller Interface) driver for OrangePi RV2
// Based on SDHCI Simplified Specification Version 3.00

#ifndef SDHCI_H
#define SDHCI_H

// SDHCI Controller Base Address (from device tree)
#define SDHCI_BASE 0xd4280000L
#define SDHCI_IRQ  99

// SDHCI Standard Register Offsets
#define SDHCI_DMA_ADDRESS           0x00
#define SDHCI_BLOCK_SIZE            0x04
#define SDHCI_BLOCK_COUNT           0x06
#define SDHCI_ARGUMENT              0x08
#define SDHCI_TRANSFER_MODE         0x0C
#define SDHCI_COMMAND               0x0E
#define SDHCI_RESPONSE              0x10  // 0x10-0x1C (4 x 32-bit)
#define SDHCI_BUFFER                0x20
#define SDHCI_PRESENT_STATE         0x24
#define SDHCI_HOST_CONTROL          0x28
#define SDHCI_POWER_CONTROL         0x29
#define SDHCI_BLOCK_GAP_CONTROL     0x2A
#define SDHCI_WAKE_UP_CONTROL       0x2B
#define SDHCI_CLOCK_CONTROL         0x2C
#define SDHCI_TIMEOUT_CONTROL       0x2E
#define SDHCI_SOFTWARE_RESET        0x2F
#define SDHCI_INT_STATUS            0x30
#define SDHCI_INT_ENABLE            0x34
#define SDHCI_SIGNAL_ENABLE         0x38
#define SDHCI_ACMD12_ERR            0x3C
#define SDHCI_HOST_CONTROL2         0x3E
#define SDHCI_CAPABILITIES          0x40
#define SDHCI_CAPABILITIES_1        0x44
#define SDHCI_MAX_CURRENT           0x48
#define SDHCI_SLOT_INT_STATUS       0xFC
#define SDHCI_HOST_VERSION          0xFE

// Transfer Mode Register Bits
#define SDHCI_TRNS_DMA              0x01
#define SDHCI_TRNS_BLK_CNT_EN       0x02
#define SDHCI_TRNS_ACMD12           0x04
#define SDHCI_TRNS_READ             0x10
#define SDHCI_TRNS_MULTI            0x20

// Command Register Bits
#define SDHCI_CMD_RESP_MASK         0x03
#define SDHCI_CMD_CRC               0x08
#define SDHCI_CMD_INDEX             0x10
#define SDHCI_CMD_DATA              0x20
#define SDHCI_CMD_ABORTCMD          0xC0

#define SDHCI_CMD_RESP_NONE         0x00
#define SDHCI_CMD_RESP_LONG         0x01
#define SDHCI_CMD_RESP_SHORT        0x02
#define SDHCI_CMD_RESP_SHORT_BUSY   0x03

// Present State Register Bits
#define SDHCI_CMD_INHIBIT           0x00000001
#define SDHCI_DATA_INHIBIT          0x00000002
#define SDHCI_DOING_WRITE           0x00000100
#define SDHCI_DOING_READ            0x00000200
#define SDHCI_SPACE_AVAILABLE       0x00000400
#define SDHCI_DATA_AVAILABLE        0x00000800
#define SDHCI_CARD_PRESENT          0x00010000
#define SDHCI_WRITE_PROTECT         0x00080000
#define SDHCI_DATA_LVL_MASK         0x00F00000

// Host Control Register Bits
#define SDHCI_CTRL_LED              0x01
#define SDHCI_CTRL_4BITBUS          0x02
#define SDHCI_CTRL_HISPD            0x04
#define SDHCI_CTRL_DMA_MASK         0x18
#define SDHCI_CTRL_8BITBUS          0x20

// Power Control Register Bits
#define SDHCI_POWER_ON              0x01
#define SDHCI_POWER_180             0x0A
#define SDHCI_POWER_300             0x0C
#define SDHCI_POWER_330             0x0E

// Clock Control Register Bits
#define SDHCI_DIVIDER_SHIFT         8
#define SDHCI_CLOCK_CARD_EN         0x0004
#define SDHCI_CLOCK_INT_STABLE      0x0002
#define SDHCI_CLOCK_INT_EN          0x0001

// Software Reset Register Bits
#define SDHCI_RESET_ALL             0x01
#define SDHCI_RESET_CMD             0x02
#define SDHCI_RESET_DATA            0x04

// Interrupt Status/Enable Register Bits
#define SDHCI_INT_RESPONSE          0x00000001
#define SDHCI_INT_DATA_END          0x00000002
#define SDHCI_INT_DMA_END           0x00000008
#define SDHCI_INT_SPACE_AVAIL       0x00000010
#define SDHCI_INT_DATA_AVAIL        0x00000020
#define SDHCI_INT_CARD_INSERT       0x00000040
#define SDHCI_INT_CARD_REMOVE       0x00000080
#define SDHCI_INT_TIMEOUT           0x00010000
#define SDHCI_INT_CRC               0x00020000
#define SDHCI_INT_END_BIT           0x00040000
#define SDHCI_INT_INDEX             0x00080000
#define SDHCI_INT_DATA_TIMEOUT      0x00100000
#define SDHCI_INT_DATA_CRC          0x00200000
#define SDHCI_INT_DATA_END_BIT      0x00400000
#define SDHCI_INT_BUS_POWER         0x00800000
#define SDHCI_INT_ACMD12ERR         0x01000000
#define SDHCI_INT_ADMA_ERROR        0x02000000

#define SDHCI_INT_NORMAL_MASK       0x00007FFF
#define SDHCI_INT_ERROR_MASK        0xFFFF8000

#define SDHCI_INT_CMD_MASK  (SDHCI_INT_RESPONSE | SDHCI_INT_TIMEOUT | \
                             SDHCI_INT_CRC | SDHCI_INT_END_BIT | SDHCI_INT_INDEX)
#define SDHCI_INT_DATA_MASK (SDHCI_INT_DATA_END | SDHCI_INT_DMA_END | \
                             SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL | \
                             SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_DATA_CRC | \
                             SDHCI_INT_DATA_END_BIT | SDHCI_INT_ADMA_ERROR)

// SD Commands
#define SD_CMD_GO_IDLE_STATE        0
#define SD_CMD_SEND_OP_COND         1
#define SD_CMD_ALL_SEND_CID         2
#define SD_CMD_SEND_RELATIVE_ADDR   3
#define SD_CMD_SET_DSR              4
#define SD_CMD_SWITCH_FUNC          6
#define SD_CMD_SELECT_CARD          7
#define SD_CMD_SEND_IF_COND         8
#define SD_CMD_SEND_CSD             9
#define SD_CMD_SEND_CID             10
#define SD_CMD_STOP_TRANSMISSION    12
#define SD_CMD_SEND_STATUS          13
#define SD_CMD_SET_BLOCKLEN         16
#define SD_CMD_READ_SINGLE_BLOCK    17
#define SD_CMD_READ_MULTIPLE_BLOCK  18
#define SD_CMD_WRITE_BLOCK          24
#define SD_CMD_WRITE_MULTIPLE_BLOCK 25
#define SD_CMD_APP_CMD              55

// SD Application Commands (preceded by CMD55)
#define SD_ACMD_SET_BUS_WIDTH       6
#define SD_ACMD_SD_STATUS           13
#define SD_ACMD_SEND_NUM_WR_BLOCKS  22
#define SD_ACMD_SET_WR_BLK_ERASE_COUNT 23
#define SD_ACMD_SD_SEND_OP_COND     41
#define SD_ACMD_SET_CLR_CARD_DETECT 42
#define SD_ACMD_SEND_SCR            51

// SD Card Response Types
#define SD_RSP_NONE                 0
#define SD_RSP_R1                   1
#define SD_RSP_R2                   2
#define SD_RSP_R3                   3
#define SD_RSP_R6                   6
#define SD_RSP_R7                   7

// OCR (Operating Conditions Register) bits
#define SD_OCR_CCS                  0x40000000  // Card Capacity Status
#define SD_OCR_BUSY                 0x80000000  // Card power up status

// Timeouts
#define SDHCI_TIMEOUT_MS            1000
#define SDHCI_CMD_TIMEOUT           100
#define SDHCI_DATA_TIMEOUT          1000

// Block size
#define SDHCI_BLOCK_SIZE_512        512

// Function declarations
void sdhci_init(void);
int sdhci_read_block(uint32 block, void *buf);
int sdhci_write_block(uint32 block, void *buf);

#endif // SDHCI_H
