//MMC.h - definitions for SD card interface

#ifndef __MMC_H
#define __MMC_H

              
// Tokens (necessary  because at NPO/IDLE (and CS active) only 0xff is on the data/command line)
#define MMC_START_DATA_BLOCK_TOKEN          0xFe   // Data token start byte, Start Single Block Read
#define MMC_START_DATA_MULTIPLE_BLOCK_READ  0xFe   // Data token start byte, Start Multiple Block Read
#define MMC_START_DATA_BLOCK_WRITE          0xFe   // Data token start byte, Start Single Block Write
#define MMC_START_DATA_MULTIPLE_BLOCK_WRITE 0xFc   // Data token start byte, Start Multiple Block Write
#define MMC_STOP_DATA_MULTIPLE_BLOCK_WRITE  0xFd   // Data toke stop byte, Stop Multiple Block Write

//transmission bit for commands
#define MMC_TX_BIT                0x40

// commands: first bit 0 (start bit), second 1 (transmission bit); CMD-number + 0ffsett 0x40
#define MMC_GO_IDLE_STATE          (MMC_TX_BIT| 0)     //CMD0
#define MMC_SEND_OP_COND           (MMC_TX_BIT| 1)     //CMD1
#define MMC_SEND_IF_COND           (MMC_TX_BIT| 8)     //CMD8
#define MMC_SEND_CSD               (MMC_TX_BIT| 9)     //CMD9
#define MMC_SEND_CID               (MMC_TX_BIT|10)     //CMD10
#define MMC_STOP_TRANSMISSION      (MMC_TX_BIT|12)     //CMD12
#define MMC_SEND_STATUS            (MMC_TX_BIT|13)     //ACMD13
#define MMC_SET_BLOCKLEN           (MMC_TX_BIT|16)     //CMD16 Set block length for next read/write
#define MMC_READ_SINGLE_BLOCK      (MMC_TX_BIT|17)     //CMD17 Read block from memory
#define MMC_READ_MULTIPLE_BLOCK    (MMC_TX_BIT|18)     //CMD18
#define MMC_CMD_WRITEBLOCK         (MMC_TX_BIT|20)     //CMD20 Write block to memory
#define MMC_SET_WR_BLK_ERASE_COUNT (MMC_TX_BIT|23)     //ACMD23 
#define MMC_WRITE_BLOCK            (MMC_TX_BIT|24)     //CMD24
#define MMC_WRITE_MULTIPLE_BLOCK   (MMC_TX_BIT|25)     //CMD25
#define MMC_PROGRAM_CSD            (MMC_TX_BIT|27)     //CMD27 PROGRAM_CSD
#define MMC_SET_WRITE_PROT         (MMC_TX_BIT|28)     //CMD28
#define MMC_CLR_WRITE_PROT         (MMC_TX_BIT|29)     //CMD29
#define MMC_SEND_WRITE_PROT        (MMC_TX_BIT|30)     //CMD30
#define MMC_ERASE_WR_BLK_START     (MMC_TX_BIT|32)     //CMD32
#define MMC_ERASE_WR_BLK_END       (MMC_TX_BIT|33)     //CMD33
#define MMC_EREASE                 (MMC_TX_BIT|38)     //CMD38
#define MMC_SD_SEND_OP_COND        (MMC_TX_BIT|41)     //ACMD41
#define MMC_APP_CMD                (MMC_TX_BIT|55)     //CMD55
#define MMC_READ_OCR               (MMC_TX_BIT|58)     //CMD58
#define MMC_CRC_ON_OFF             (MMC_TX_BIT|59)     //CMD59


//values for VHS bits of CMD8
#define MMC_VHS_27_36              ((0x01)<<8)    //2.7-3.6V supply voltage
#define MMC_VHS_LV                 ((0x02)<<8)    //Reserved for Low Voltage Range
#define MMC_VHS_RES1               ((0x04)<<8)    //Reserved
#define MMC_VHS_RES2               ((0x08)<<8)    //Reserved

//values for ACMD41
#define MMC_HCS                    (1ul<<30)        //HCS bit

//values for OCR
#define MMC_OCR_VDD_27_28          (1ul<<15)
#define MMC_OCR_VDD_28_29          (1ul<<16)
#define MMC_OCR_VDD_29_30          (1ul<<17)
#define MMC_OCR_VDD_30_31          (1ul<<18)
#define MMC_OCR_VDD_31_32          (1ul<<19)
#define MMC_OCR_VDD_32_33          (1ul<<20)
#define MMC_OCR_VDD_33_34          (1ul<<21)
#define MMC_OCR_VDD_34_35          (1ul<<22)
#define MMC_OCR_VDD_35_36          (1ul<<23)
#define MMC_OCR_S18A               (1ul<<24)        //Switching to 1.8V accepted
#define MMC_OCR_UHSII              (1ul<<30)        //UHS-II card
#define MMC_OCR_CCS                (1ul<<30)        //Card Capacity Status, set if SDHC or SDXC
#define MMC_OCR_STAT               (1ul<<31)        //Card power up status


#endif /* __MMC_H */
