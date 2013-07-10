// ***********************************************************
// File: mmc.c 
// Description: Library to access a MultiMediaCard 
//              functions: init, read, write ...
//  C. Speck / S. Schauer
//  Texas Instruments, Inc
//  June 2005
//
// Version 1.1
//   corrected comments about connection the MMC to the MSP430
//   increased timeout in mmcGetXXResponse
//
// ***********************************************************
// MMC Lib
// ***********************************************************


/* ***********************************************************
* THIS PROGRAM IS PROVIDED "AS IS". TI MAKES NO WARRANTIES OR
* REPRESENTATIONS, EITHER EXPRESS, IMPLIED OR STATUTORY, 
* INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS 
* FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR 
* COMPLETENESS OF RESPONSES, RESULTS AND LACK OF NEGLIGENCE. 
* TI DISCLAIMS ANY WARRANTY OF TITLE, QUIET ENJOYMENT, QUIET 
* POSSESSION, AND NON-INFRINGEMENT OF ANY THIRD PARTY 
* INTELLECTUAL PROPERTY RIGHTS WITH REGARD TO THE PROGRAM OR 
* YOUR USE OF THE PROGRAM.
*
* IN NO EVENT SHALL TI BE LIABLE FOR ANY SPECIAL, INCIDENTAL, 
* CONSEQUENTIAL OR INDIRECT DAMAGES, HOWEVER CAUSED, ON ANY 
* THEORY OF LIABILITY AND WHETHER OR NOT TI HAS BEEN ADVISED 
* OF THE POSSIBILITY OF SUCH DAMAGES, ARISING IN ANY WAY OUT 
* OF THIS AGREEMENT, THE PROGRAM, OR YOUR USE OF THE PROGRAM. 
* EXCLUDED DAMAGES INCLUDE, BUT ARE NOT LIMITED TO, COST OF 
* REMOVAL OR REINSTALLATION, COMPUTER TIME, LABOR COSTS, LOSS 
* OF GOODWILL, LOSS OF PROFITS, LOSS OF SAVINGS, OR LOSS OF 
* USE OR INTERRUPTION OF BUSINESS. IN NO EVENT WILL TI'S 
* AGGREGATE LIABILITY UNDER THIS AGREEMENT OR ARISING OUT OF 
* YOUR USE OF THE PROGRAM EXCEED FIVE HUNDRED DOLLARS 
* (U.S.$500).
*
* Unless otherwise stated, the Program written and copyrighted 
* by Texas Instruments is distributed as "freeware".  You may, 
* only under TI's copyright in the Program, use and modify the 
* Program without any charge or restriction.  You may 
* distribute to third parties, provided that you transfer a 
* copy of this license to the third party and the third party 
* agrees to these terms by its first use of the Program. You 
* must reproduce the copyright notice and any other legend of 
* ownership on each copy or partial copy, of the Program.
*
* You acknowledge and agree that the Program contains 
* copyrighted material, trade secrets and other TI proprietary 
* information and is protected by copyright laws, 
* international copyright treaties, and trade secret laws, as 
* well as other intellectual property laws.  To protect TI's 
* rights in the Program, you agree not to decompile, reverse 
* engineer, disassemble or otherwise translate any object code 
* versions of the Program to a human-readable form.  You agree 
* that in no event will you alter, remove or destroy any 
* copyright notice included in the Program.  TI reserves all 
* rights not specifically granted under this license. Except 
* as specifically provided herein, nothing in this agreement 
* shall be construed as conferring by implication, estoppel, 
* or otherwise, upon you, any license or other right under any 
* TI patents, copyrights or trade secrets.
*
* You may not use the Program in non-TI devices.
* ********************************************************* */


#ifndef _MMCLIB_C
#define _MMCLIB_C
//
//---------------------------------------------------------------

#include "MMC.h"
#include "SPI.h"
#include "hardware.h"
#include "SDlib.h"

#include <stdio.h>

//used to keep track of card type to calculate block address
char is_SDSC=0;

int mmc_R1(void);

//==============================================================
//              [MMC init functions]
//==============================================================

// Initialize ports and pins for MMC opperation
void mmcInit_msp(void){
  // Port x Function           Dir       On/Off
  //         mmcCS         Out       0 - Active 1 - none Active
  //         Dout          Out       0 - off    1 - On -> init in SPI_Init
  //         Din           Inp       0 - off    1 - On -> init in SPI_Init
  //         Clk           Out       -                 -> init in SPI_Init

  
  //deselect card (bring line high)
  CS_HIGH();
  // Chip Select
  MMC_CS_PxDIR |= MMC_CS;
  
  // Init SPI Module
  SPISetup();

  //setup SPI PINS for SPI function
  MMC_PxSEL1|=MMC_SIMO|MMC_SOMI;
  MMC_PxSEL0|=MMC_UCLK;
  //enable pullup on SOMI pin
  MMC_PxOUT1|=MMC_SOMI;
  MMC_PxREN1|=MMC_SOMI;
}

// Initialize mmc card
int mmcInit_card(void){
  int i;
  int resp;
  //set SPI to slow speed
  SPI_slow();
  //initialization sequence on PowerUp
  for(i=0;i<=9;i++)
    spiSendByte(DUMMY_CHAR);
  
  resp=mmcGoIdle();
  //check if init worked
  if(resp==MMC_SUCCESS){
    //set SPI to fast speed
    SPI_fast();
    //set block length of card
    if(mmcSetBlockLength(512) != MMC_SUCCESS){
      return MMC_INIT_ERR_BLOCK_SIZE;
    }
  }
  //return status
  return resp;
}

// set MMC in Idle mode
int mmcGoIdle(void){
  int resp;
  //extra response bytes for R3 and R7
  unsigned char extresp[4];
  int i;

  //select card
  CS_LOW();
  //Send Command 0 to put MMC in SPI mode
  mmcSendCmd(MMC_GO_IDLE_STATE,0,0x95);
  //Now wait for READY RESPONSE
  resp=mmc_R1();
  //error occurred
  CS_HIGH();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //check response
  if(((char)resp)!=MMC_R1_IDLE){
    return MMC_INIT_ERR_GO_IDLE;
  }

  //start new transaction
  CS_LOW();
  //send operating voltage and check pattern
  mmcSendCmd(MMC_SEND_IF_COND,MMC_VHS_27_36|0xAA,0x87);
  //response starts with R1
  resp=mmc_R1();
  //check for affermitave ir response
  if((((char)resp)&(~MMC_R1_IDLE))==MMC_SUCCESS){
    //get R7 response bits
    spiReadFrame(extresp,4);
  }
  //end transaction
  CS_HIGH();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //check for illegal command
  if(resp&MMC_R1_ILLEGAL_CMD){
    //older card so must be SDSC
    is_SDSC=1;
    //TODO: maybe send CMD58 to check voltage range
    for(i=0,resp=MMC_R1_IDLE;i<50 && resp==MMC_R1_IDLE;i++){
      //start transaction
      CS_LOW();
      //next command is application spesific command
      mmcSendCmd(MMC_APP_CMD,0,0xff);
      //get response
      resp=mmc_R1();
      //check for error
      if((resp&(~MMC_R1_IDLE))==MMC_SUCCESS){
        //send host capacity information
        mmcSendCmd(MMC_SD_SEND_OP_COND,MMC_HCS,0xFF);
        //get response
        resp=mmc_R1();
      }
      //Transaction done
      CS_HIGH();
      // Send 8 Clock pulses of delay.
      spiDummyClk();
      //wait 20 or so ms
      ctl_timeout_wait(ctl_get_current_time()+20);
    }
    //check if card has exited the idle state
    if(resp!=MMC_SUCCESS){
      return MMC_INIT_ERR_TIMEOUT;
    }
    //SUCCESS!!!!!!!!!
    return MMC_SUCCESS;
  }else{
    //make sure that check pattern matches
    if(extresp[3]!=0xAA){
      return MMC_INIT_ERR_CHECK_PATTERN;
    }
    //check voltage range
    if((extresp[2]&0x0F)!=(MMC_VHS_27_36>>8)){
      return MMC_INIT_ERR_VOLTAGE;
    }
    //TODO: maybe send CMD58 to check voltage range
    for(i=0,resp=MMC_R1_IDLE;i<50 && ((char)resp)==MMC_R1_IDLE;i++){
      //start transaction
      CS_LOW();
      //next command is application spesific command
      mmcSendCmd(MMC_APP_CMD,0,0xff);
      //get response
      resp=mmc_R1();
      //check for error
      if((((char)resp)&(~MMC_R1_IDLE))==MMC_SUCCESS){
        //send host capacity information
        mmcSendCmd(MMC_SD_SEND_OP_COND,MMC_HCS,0xFF);
        //get response
        resp=mmc_R1();
      }
      //Transaction done
      CS_HIGH();
      // Send 8 Clock pulses of delay.
      spiDummyClk();
      //wait 20 or so ms
      ctl_timeout_wait(ctl_get_current_time()+20);
    }
    //check if card has exited the idle state or error occurred
    if(resp!=MMC_SUCCESS){
      return MMC_INIT_ERR_TIMEOUT;
    }
    //start transaction
    CS_LOW();
    //get response
    resp=mmc_R1();
    //read OCR to check CCS bit
    mmcSendCmd(MMC_READ_OCR,0,0xFF);
    //get response
    resp=mmc_R1();
    //check response
    if(resp==MMC_SUCCESS){
      //get R3 response bits
      spiReadFrame(extresp,4);
    }
    //Transaction done
    CS_HIGH();
    // Send 8 Clock pulses of delay.
    spiDummyClk();

    //check if command was successfull
    if(resp!=MMC_SUCCESS){
      return MMC_INIT_ERR_READ_OCR;
    }

    //check if card is standard capacity or high capacity
    if(extresp[0]&(MMC_OCR_CCS>>24)){
      //high capacity
      is_SDSC=0;
    }else{
      //standard capacity
      is_SDSC=1;
    }
    //SUCCESS!!!!!!!!!
    return MMC_SUCCESS;
  }
}

//==============================================================
//              [MMC response functions]
//==============================================================

//get R1 response from SD card
int mmc_R1(void){
  int i;
  unsigned char resp;
  for(i=0;i<100;i++){
    resp=spiSendByte(DUMMY_CHAR);
    //check for R1 response start bit
    if(!(resp&0x80)){
        //response found, return response
        return (resp==MMC_SUCCESS)?MMC_SUCCESS:(MMC_R1_RESPONSE|resp);
    }
    //wait a bit
    ctl_timeout_wait(ctl_get_current_time()+2);
  }
  //response not found
  return MMC_TIMEOUT_ERROR;
}

//wait for busy signal to go away
int mmc_busy(void){
  unsigned char resp;
  int i;
  //not really sure on this one but it seems to need a dummy read
  resp=spiSendByte(DUMMY_CHAR);
  //wait for busy signal to go away
  for(i=0;i<100;i++){
    //get byte
    resp=spiSendByte(DUMMY_CHAR);
    //check if busy
    if(resp!=0x00){
      return MMC_SUCCESS;
    }
    //wait a bit
    ctl_timeout_wait(ctl_get_current_time()+2);
  }
  //response not found
  return MMC_BUSY_TIMEOUT_ERROR;
}

//get R1 response followed by a busy signal
int mmc_R1b(void){
  int i;
  unsigned char resp,rt;
  //get response
  rt=mmc_R1();
  //check if there was an error
  if(resp==MMC_SUCCESS){
    //wait for busy signal to go away
    for(i=0;i<100;i++){
      //get byte
      resp=spiSendByte(DUMMY_CHAR);
      //check if busy
      if(resp!=0x00)
        return rt;
      //wait a bit
      ctl_timeout_wait(ctl_get_current_time()+2);
    }
  }
  //card still busy
  return MMC_BUSY_TIMEOUT_ERROR;
}

//get data response used when writing data
int mmc_dat_resp(void){
  int i;
  unsigned int resp,rt;
  for(i=0,rt=MMC_BUSY_TIMEOUT_ERROR;i<100;i++){
    resp=spiSendByte(DUMMY_CHAR);
    //check for data response
    switch(resp&0x1F){
       case MMC_DAT_ACCEPTED:
       case MMC_DAT_CRC:
       case MMC_DAT_WRITE_ERR:
        rt=(resp&0x1F)|MMC_DATA_RESPONSE;
       break;
    }
    //if response found, return
    if(rt!=MMC_BUSY_TIMEOUT_ERROR){
      break;
    }
    //wait a bit
    ctl_timeout_wait(ctl_get_current_time()+2);
  }
  //check if there was an error
  if(((char)rt)== MMC_DAT_ACCEPTED){
    //wait for busy signal to go away
    for(i=0;i<150;i++){
      //get byte
      resp=spiSendByte(DUMMY_CHAR);
      //check if busy
      if(resp!=0x00){
        return rt;
      }
      //wait a bit
      ctl_timeout_wait(ctl_get_current_time()+2);
    }
    //busy timeout
    return MMC_BUSY_TIMEOUT_ERROR;
  }
  //return response
  return rt;
}

//wait for token response
int mmc_token(void){
  int i;
  unsigned char resp;
  for(i=0;i<=100;i++){
    //get byte
    resp=spiSendByte(DUMMY_CHAR);
    //check for data start token
    if(resp==MMC_START_DATA_BLOCK_TOKEN){
      return resp|MMC_DATA_TOKEN_RESP;
    }
    //check for Data Error Token
    if((resp&0xF0)==0x00){
      return resp|MMC_DATA_TOKEN_RESP;
    }
    //wait a bit before checking again
    ctl_timeout_wait(ctl_get_current_time()+2);
  }
  return MMC_TIMEOUT_ERROR;
}

//==============================================================
//              [MMC read functions]
//==============================================================

// read a block beginning at the given address.
int mmcReadBlock(unsigned long addr, unsigned char *pBuffer){
  int rvalue;
  //check if SDSC card
  if(is_SDSC==1){
    //address is block address, not byte address
    addr*=512;
  }
  // CS = LOW (on)
  CS_LOW ();
  // send read command MMC_READ_SINGLE_BLOCK=CMD17
  mmcSendCmd(MMC_READ_SINGLE_BLOCK,addr, 0xFF);
  //Get R1 response
  rvalue=mmc_R1();
  if(rvalue==MMC_SUCCESS){
    //look for the data token to signify the start of the data
    if(((char)(rvalue=mmc_token()))==MMC_START_DATA_BLOCK_TOKEN){
      // clock the actual data transfer and receive the bytes; spi_read automatically finds the Data Block
      rvalue = spiReadFrame(pBuffer,512);
      // get CRC bytes (not really needed by us, but required by MMC)
      spiSendByte(DUMMY_CHAR);
      spiSendByte(DUMMY_CHAR);
    }
  }
  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  return rvalue;
}// mmc_read_block

// read out multiple blocks at once
int mmcReadBlocks(unsigned long addr,unsigned long count, unsigned char *pBuffer){
  unsigned short i;
  int rvalue,rt;
  //check if SDSC card
  if(is_SDSC==1){
    //address is block address, not byte address
    addr*=512;
  }
  // CS = LOW (on)
  CS_LOW ();
  // send read command MMC_READ_SINGLE_BLOCK=CMD17
  mmcSendCmd(MMC_READ_MULTIPLE_BLOCK,addr, 0xFF);
  //Get R1 response from card
  if ((rvalue=mmc_R1())==MMC_SUCCESS){
    //get data blocks
    for(i=0;i<count;i++){
      // look for the data token to signify the start of the data
      if(((char)(rvalue=mmc_token()))==MMC_START_DATA_BLOCK_TOKEN){
        // clock the actual data transfer and receive the bytes
        rvalue = spiReadFrame(pBuffer+i*512,512);
        // get CRC bytes (not really needed by us, but required by MMC)
        spiSendByte(DUMMY_CHAR);
        spiSendByte(DUMMY_CHAR);
      }else{
        // the data token was never received
        //abort
        break;
      }
    }
    //send stop transmission command
    mmcSendCmd(MMC_STOP_TRANSMISSION,0,0xFF);
    //get R1 response with busy signal
    rt=mmc_R1b(); 
    //check if an error occurred before
    if(((char)rvalue)==MMC_START_DATA_BLOCK_TOKEN){
      //return response to MMC_STOP_TRANSMISSION
      rvalue=rt;
    }
  }
  //CS = HIGH (off)
  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //return result
  return rvalue;
}// mmc_read_block

//==============================================================
//              [MMC write functions]
//==============================================================

//write one data block on the SD card
int mmcWriteBlock(unsigned long addr,const unsigned char *pBuffer){
  int rvalue,result;
  //check if SDSC card
  if(is_SDSC==1){
    //address is block address, not byte address
    addr*=512;
  }
  // CS = LOW (on)
  CS_LOW ();
  // send write command
  mmcSendCmd(MMC_WRITE_BLOCK,addr, 0xFF);

  //check R1 response for no errors
  if((rvalue=mmc_R1())==MMC_SUCCESS){
    // send the data token to signify the start of the data
    spiSendByte(MMC_START_DATA_BLOCK_WRITE);
    // clock the actual data transfer and transmit the bytes
    result=spiSendFrame(pBuffer,512);

    // put CRC bytes (not really needed by us, but required by MMC)
    spiSendByte(DUMMY_CHAR);
    spiSendByte(DUMMY_CHAR);
    //get data response
    rvalue=mmc_dat_resp(); 
    //check if data was accepted CRC not used so ignore CRC error
    if(((char)rvalue)==MMC_DAT_ACCEPTED){
      rvalue=MMC_SUCCESS;
    }
    //check if spiSendFrame was succussfull
    if(result){
      //return error from spiSendFrame
      rvalue=result;
    }
  }

  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  return rvalue;
} // mmc_write_block

/* This version uses MMC_SET_WR_BLK_ERASE_COUNT which seems to have no effect on write time 
//write mutiple blocks of data fist block # is given as start
char mmcWriteMultiBlock(unsigned long addr, const unsigned char *pBuffer,unsigned short blocks){
  char rvalue = MMC_SUCCESS;
  unsigned char resp;
  unsigned short i;
  //check if SDSC card
  if(is_SDSC==1){
    //address is block address, not byte address
    addr*=512;
  }
  // CS = LOW (on)
  CS_LOW ();
  mmcSendCmd(MMC_APP_CMD,0,0xFF);
  resp=mmcGetResponse();
  // send write number of write blocks command
  mmcSendCmd(MMC_SET_WR_BLK_ERASE_COUNT,blocks, 0xFF);

  if(mmc_R1()==MMC_SUCCESS){
    // send write command
    mmcSendCmd(MMC_WRITE_MULTIPLE_BLOCK,addr, 0xFF);
      //check for errors
    if (mmc_R1() == MMC_SUCCESS){ 
      //loop over blocks to write
      for(i=0;i<blocks;i++){
        // send the data token to signify the start of the data
        spiSendByte(MMC_START_DATA_MULTIPLE_BLOCK_WRITE);
        // clock the actual data transfer and transmit the bytes
        
        spiSendFrame(pBuffer+i*512,512);
        
        // put CRC bytes (not really needed by us, but required by MMC)
        spiSendByte(DUMMY_CHAR);
        spiSendByte(DUMMY_CHAR);
        //get data response
        rvalue=mmc_dat_resp(); 
        //TODO : handle errors accordingly
      
      }
      //send stop tran token
      spiSendByte(MMC_STOP_DATA_MULTIPLE_BLOCK_WRITE);
      //wait for completion
      rvalue=(mmcGetXXResponse(MMC_R1_RESPONSE)==MMC_R1_RESPONSE)?MMC_SUCCESS:MMC_RESPONSE_ERROR;
    }else{
      // the MMC never acknowledge the write block count command
      rvalue = MMC_RESPONSE_ERROR;   // 2
    }

  }else{
    // the MMC never acknowledge the write block count command
    rvalue = MMC_RESPONSE_ERROR;   // 2
  }
  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  return rvalue;
} // mmc_write_block*/


//write mutiple blocks of data fist block # is given as start
int mmcWriteMultiBlock(unsigned long addr,const unsigned char *pBuffer,unsigned short blocks){
  int rvalue;
  unsigned char resp;
  unsigned short i;
  //check if SDSC card
  if(is_SDSC==1){
    //address is block address, not byte address
    addr*=512;
  }
  // CS = LOW (on)
  CS_LOW ();
  
  // send write command
  mmcSendCmd(MMC_WRITE_MULTIPLE_BLOCK,addr, 0xFF);
    //check for errors
  if((rvalue=mmc_R1())==MMC_SUCCESS){ 
    //loop over blocks to write
    for(i=0;i<blocks;i++){
      // send the data token to signify the start of the data
      spiSendByte(MMC_START_DATA_MULTIPLE_BLOCK_WRITE);
      // clock the actual data transfer and transmit the bytes
      
      spiSendFrame(pBuffer+i*512,512);
      
      // put CRC bytes (not really needed by us, but required by MMC)
      spiSendByte(DUMMY_CHAR);
      spiSendByte(DUMMY_CHAR);
      //get data response
      rvalue=mmc_dat_resp(); 
      //an error occurred, abort transmission
      if(((char)rvalue)!=MMC_DAT_ACCEPTED){
        //send stop transmission command
        mmcSendCmd(MMC_STOP_TRANSMISSION,0,0xFF);
        //get R1 response with busy signal
        //keep the response that generated the error
        mmc_R1b(); 
        break;
      }
    }
    if(((char)rvalue)==MMC_DAT_ACCEPTED){
      //send stop transfer token
      spiSendByte(MMC_STOP_DATA_MULTIPLE_BLOCK_WRITE);
      //wait for completion
      rvalue=mmc_busy();
    }
  }

  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  return rvalue;
} // mmc_write_block

//==============================================================
//              [other MMC functions]
//==============================================================

// send command to MMC
void mmcSendCmd (char cmd, unsigned long data,char crc)
{
  unsigned char frame[6];
  char temp;
  int i;
  frame[0]=(cmd|MMC_TX_BIT);
  frame[1]=(data>>(24));
  frame[2]=(data>>(16));
  frame[3]=(data>>(8));
  frame[4]=(data);
  frame[5]=(crc);
  spiSendFrame(frame,6);
}


//--------------- set blocklength 2^n ------------------------------------------------------
//TODO: determine if this is usefull and delete
//block size should be kept at the default of 512 for compatibility reasons
int mmcSetBlockLength(unsigned long blocklength){
  int rt;
  // CS = LOW (on)
  CS_LOW ();
  // Set the block length to read
  mmcSendCmd(MMC_SET_BLOCKLEN,blocklength,0xFF);
  //get R1 response
  rt=mmc_R1();
  // CS = HIGH (off)
  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  return rt;
} // Set block_length

//erase blocks from start to end
int mmcErase(unsigned long start,unsigned long end){
  int rvalue;
  //check if SDSC card
  if(is_SDSC==1){
    //address is block address, not byte address
    start*=512;
    end*=512;
  }
  // CS = LOW (on)
  CS_LOW ();
  
  //send erase block start
  mmcSendCmd(MMC_ERASE_WR_BLK_START,start,0xFF);
  //check for correct response
  if((rvalue=mmc_R1())==MMC_SUCCESS){
    //send erase block end
    mmcSendCmd(MMC_ERASE_WR_BLK_END,end,0xFF);
    //check for correct response
    if((rvalue=mmc_R1())==MMC_SUCCESS){
      //send erase command
      mmcSendCmd(MMC_EREASE,0,0xFF);
      //get R1b response
      rvalue=mmc_R1b();
    }
  }
  
  //end transaction
  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();

  return rvalue;
}

//read CID or CSD register reg should be MMC_SEND_CSD or MMC_SEND_CID depending on which register is to be read
//data is stored in buffer which should be at least 16 bytes long
int mmcReadReg(unsigned char reg,unsigned char *buffer){
  int rvalue;
  //select
  CS_LOW ();
  //send read CSD
  mmcSendCmd(reg,0,0xFF);
  //check response
  if((rvalue=mmc_R1())==MMC_SUCCESS){
    if(((char)(rvalue=mmc_token()))==MMC_START_DATA_BLOCK_TOKEN){
      //get CSD data
      rvalue=spiReadFrame(buffer,16);
      // put CRC bytes (not really needed by us, but required by MMC)
      spiSendByte(DUMMY_CHAR);
      spiSendByte(DUMMY_CHAR);
    }
  }
  //deselect card
  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //return error value
  return rvalue;
}

//return size in KB from CSD structure
unsigned long mmcGetCardSize(unsigned char *CSD){
  unsigned long Csize;
  unsigned short mult,blocklen;
  //check CSD version
  switch(CSD[0]>>6){
    case 0:
      //version 1.0
      //get C_SIZE          bits 62-73
      Csize =(CSD[8]&0xC0>>6);          //CSD bits 62-63
      Csize|= CSD[7]<<2;                //CSD bits 64-71
      Csize|=(CSD[6]&0x03)<<10;         //CSD bits 64-71
      //get C_SIZE_MULT     bits 49-47
      mult = CSD[10]>>7;                //CSD bit  47
      mult|=(CSD[9]&0x03)<<1;           //CSD bits 48-49
      //calculate multiplier
      mult=1<<(mult+2);
      //get READ_BL_LEN
      blocklen=CSD[5]&0x0F;             //CSD bits 80-83
      //calculate block length
      blocklen=1<<blocklen;
      //compute size in KB
      //return ((Csize+1)*mult*blocklen)/1024;
      //compute size in Bytes
      return ((Csize+1)*mult*blocklen)/1024;
    case 1:
      //version 2.0
      //get C_SIZE field
      Csize =CSD[9];                               //CSD bits 48-55
      Csize|=CSD[8]<<8;                            //CSD bits 56-63
      Csize|=((unsigned long)CSD[7]&0x3F)<<16;     //CSD bits 64-69
      //compute size in KB
      //return (Csize+1)512;
      //size in bytes
      return (Csize+1)*512*1024;
    default:
      //error unknown version
      return 0;
  }
}

#ifndef withDMA
  int SD_DMA_is_enabled(void){
    return 0;
  }
#else
  int SD_DMA_is_enabled(void){
    return 1;
  }
#endif


//---------------------------------------------------------------------
#endif /* _MMCLIB_C */
