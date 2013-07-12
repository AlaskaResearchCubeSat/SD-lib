//SDlib.h - include file for library interface
#ifndef __SDLIB_H
#define __SDLIB_H

#include <msp430.h>  

//"response classes" to determine which function returned the response
#define MMC_R1_RESPONSE         0x0100
#define MMC_DATA_RESPONSE       0x0200
#define MMC_DATA_TOKEN_RESP     0x0300

//mask to get response class
#define MMC_RC_MASK             0xFF00
//mask to get card response
#define MMC_CARD_MASK           0x00FF

//MMC R1 Response Codes as returned by mmc_r1 and mmc_r1b
#define MMC_R1_IDLE                     0x01      //Card in idle state for init process
#define MMC_R1_ERASE_RESET              0x02      //Erase Sequence Cleared because out of erase sequence command
#define MMC_R1_ILLEGAL_CMD              0x04      //illegal command
#define MMC_R1_CRC_ERROR                0x08      //Communication CRC check failed
#define MMC_R1_ERASE_SEQ_ERROR          0x10      //Erase command out of sequence
#define MMC_R1_ADDRESS_ERROR            0x20      //Address misalignment
#define MMC_R1_PARAMETER_ERROR          0x40      //bad argument

//MMC data error token bits
#define MMC_ERR_TOKEN_UNKNOWN           0x01      //unknown error
#define MMC_ERR_TOKEN_CC                0x02      //card controller error
#define MMC_ERR_TOKEN_ECC_FAIL          0x04      //error correction error
#define MMC_ERR_TOKEN_OUT_OF_RANGE      0x08      //command argument out of range


//return values from data response
#define MMC_DAT_ACCEPTED                0x05      //Data accepted
#define MMC_DAT_CRC                     0x0B      //Data rejected due to a CRC error
#define MMC_DAT_WRITE_ERR               0x0D      //Data rejected due to a Write Error

//software defined return values
enum{MMC_SUCCESS=0,MMC_TIMEOUT_ERROR=-1,MMC_DMA_TIMEOUT_ERROR=-3,MMC_BUSY_TIMEOUT_ERROR=-4,
     MMC_LOCK_TIMEOUT_ERROR=-5,MMC_INVALID_CARD_SIZE=-6,MMC_CARD_UNINIT_ERROR=-7,MMC_MSP_UNINIT_ERROR=-8,
     MMC_INIT_ERR_CHECK_PATTERN=-9,MMC_INIT_ERR_VOLTAGE=-10,MMC_INIT_ERR_GO_IDLE=-11,MMC_INIT_ERR_TIMEOUT=-12,
     MMC_INIT_ERR_READ_OCR=-13,MMC_INIT_ERR_BLOCK_SIZE=-14};

enum{MMC_SIZE_UNKNOWN=-1,MMC_SIZE_SDHC=1,MMC_SIZE_SDSC=2};

//TODO: figure out witch functions are needed by subsystems and remove the rest

//initialize ports for the MSP
void mmcInit_msp(void);

//initialize the card for reading and writing
int mmcInit_card(void);

// send command to MMC
void mmcSendCmd (char cmd, unsigned long data, char crc);

// set MMC in Idle mode
int mmcGoIdle(void);

// set MMC block length of count=2^n Byte
int mmcSetBlockLength (unsigned long blocklength);

//read a whole block from the card given a block number
int mmcReadBlock(unsigned long addr, unsigned char *pBuffer);

// write a 512 Byte block given by sector address
int mmcWriteBlock(unsigned long addr,const unsigned char *pBuffer);

//write mutiple blocks at a time
int mmcWriteMultiBlock(unsigned long addr,const unsigned char *pBuffer,unsigned short blocks);

//read multiple blocks at a time
int mmcReadBlocks(unsigned long addr,unsigned long count, unsigned char *pBuffer);

// Read CID or CSD Register into buffer
int mmcReadReg(unsigned char reg,unsigned char *buffer);

//erase blocks from start to end
int mmcErase(unsigned long start,unsigned long end);

//check if DMA is enabled
int SD_DMA_is_enabled(void);

//return string representation of initialization errors
//const char* SD_init_error_str(int error);

//return string representation of SD card function errors
const char * SD_error_str(int error);

//return size in KB from CSD structure
unsigned long mmcGetCardSize(unsigned char *CSD);

//check if the SD card is initialized
int mmc_is_init(void);

//get the size of the SD card
int mmc_size_class(void);

//lock SD card so that other tasks can't access it
int mmcLock(void);

//unlock SD card so that other tasks can access it
void mmcUnlock(void);

#endif
