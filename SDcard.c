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

//include crc stuff from ARClib
#include <crc.h>

MMC_STAT_t mmcStat={0,0};

int mmc_R1(int num);

//==============================================================
//              [MMC locking functions]
//==============================================================

//return size of the SD card 
int mmc_size_class(void){
  //mask out size bit
  switch(mmcStat.flags&MMC_FLAG_SIZE_BITS){
    case MMC_FLAG_SDHC:
      return MMC_SIZE_SDHC;
    case MMC_FLAG_SDSC:
      return MMC_SIZE_SDSC;
    break;
    default: 
      return MMC_SIZE_UNKNOWN;
  }
}

//checks if card and periphal have been initialized 
int mmc_is_init(void){
  //check peripheral
  if(!(mmcStat.flags&MMC_FLAG_INIT_MSP)){
    return MMC_MSP_UNINIT_ERROR;
  }
  //check card
  if(!(mmcStat.flags&MMC_FLAG_INIT_CARD)){
    return MMC_CARD_UNINIT_ERROR;
  }
  //success
  return MMC_SUCCESS;
}

//==============================================================
//              [MMC locking functions]
//==============================================================

//unlock mutex
void mmcUnlock(void){
  //unlock mutex, always succeeds 
  ctl_mutex_unlock(&mmcStat.mutex);
}

//lock mutex, internal version that does not check if card is initialized
//this is used for init functions where card is usually not initialized
static int _mmcLock(CTL_TIMEOUT_t t,CTL_TIME_t timeout){
  //check if peripherals have been initialized
  if(!(mmcStat.flags&MMC_FLAG_INIT_MSP)){
    return MMC_MSP_UNINIT_ERROR;
  }
  //try to lock mutex
  if(0==ctl_mutex_lock(&mmcStat.mutex,t,timeout)){
    //return error
    return MMC_LOCK_TIMEOUT_ERROR;
  }
  //return success
  return MMC_SUCCESS; 
}

//lock mutex and check if card is initialized
int mmcLock(CTL_TIMEOUT_t t,CTL_TIME_t timeout){
  int resp;
  //lock card
  resp=_mmcLock(t,timeout);
  //check for error
  if(resp){
    //return error
    return resp;
  }
  //check if card is initialized
  resp=mmc_is_init();
  //check card
  if(!(mmcStat.flags&MMC_FLAG_INIT_CARD)){
    //unlock mutex
    mmcUnlock();
    return MMC_CARD_UNINIT_ERROR;
  }
  //return success
  return MMC_SUCCESS; 
}

//==============================================================
//              [MMC init functions]
//==============================================================


// Initialize ports and pins for MMC opperation
void mmc_pins_on(void){
  //init flags
  mmcStat.flags=MMC_FLAG_INIT_MSP;
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

//init msp ports and pins and setup MUTEX
//because this initializes the MUTEX it should only be called ONCE (usually in main)
void mmcInit_msp(void){
  // Port x Function           Dir       On/Off
  //         mmcCS         Out       0 - Active 1 - none Active
  //         Dout          Out       0 - off    1 - On -> init in SPI_Init
  //         Din           Inp       0 - off    1 - On -> init in SPI_Init
  //         Clk           Out       -                 -> init in SPI_Init

  //init mutex
  ctl_mutex_init(&mmcStat.mutex);
  //setup everything else
  mmc_pins_on();
}

void _mmc_pins_off(void){
    //init flags
  mmcStat.flags=0;
  //set chip select low
  CS_LOW();
  // Chip Select direction
  MMC_CS_PxDIR |= MMC_CS;
  
  // Init SPI Module
  SPIShutdown();

  //setup SPI PINS to drive low
  MMC_PxOUT1&=~(MMC_SIMO|MMC_SOMI);
  MMC_PxOUT0&=~(MMC_UCLK);
  //setup SPI PINS to outputs
  MMC_PxDIR1|=MMC_SIMO|MMC_SOMI;
  MMC_PxDIR0|=MMC_UCLK;
  //setup SPI PINS for GPIO function
  MMC_PxSEL1&=~(MMC_SIMO|MMC_SOMI);
  MMC_PxSEL0&=~(MMC_UCLK);
}

void mmc_pins_off(void){
  //wait for card to not be in use
  _mmcLock(CTL_TIMEOUT_NONE,0);
  //turn off MMC pins
  _mmc_pins_off();
  //done with SD card
  mmcUnlock();
}

void mmcInit_msp_off(void){
  //init mutex
  ctl_mutex_init(&mmcStat.mutex);
  //setup pins with outputs low
  _mmc_pins_off();
}



//Force initialization of SD card
int mmcReInit_card(void){
  int i;
  int resp;
  //check if MSP430 has been initialized
  if(!(mmcStat.flags&MMC_FLAG_INIT_MSP)){
    return MMC_MSP_UNINIT_ERROR;
  }
  //TODO: perhaps it would be nice to check if multiple tasks are running here and bail if they are not
  //get a lock on the card
  if(resp=_mmcLock(CTL_TIMEOUT_DELAY,10)){
    return resp;
  }
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
  }
  //unlock card
  mmcUnlock();
  //return status
  return resp;
}

// Initialize mmc card
int mmcInit_card(void){
  //check if card has already been initialized
  if(mmc_is_init()==MMC_SUCCESS){
    //nothing to do
    return MMC_SUCCESS;
  }else{
    //Initialize card
    return mmcReInit_card();
  }
}

// set MMC in Idle mode
int mmcGoIdle(void){
  int resp;
  //extra response bytes for R3 and R7
  unsigned char extresp[4];
  int i;

  //get a lock on the card
  if(resp=_mmcLock(CTL_TIMEOUT_DELAY,10)){
    return resp;
  }
  //select card
  CS_LOW();
  //Send Command 0 to put MMC in SPI mode
  resp=mmcSendCmd(MMC_GO_IDLE_STATE,0,0x95);
  //check for error (probably DMA timeout)
  if(resp!=MMC_SUCCESS){
    //unlock card
    mmcUnlock();
    //return error
    return resp;
  }
  //Now wait for READY RESPONSE
  resp=mmc_R1(100);
  //error occurred
  CS_HIGH();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //check response
  if(resp==MMC_SUCCESS){
    //unlock card
    mmcUnlock();
    //idle command failed to put card in idle state, return error
    return MMC_INIT_ERR_GO_IDLE;
  }else if(resp==MMC_TIMEOUT_ERROR){
    //unlock card
    mmcUnlock();
    //idle command timeout. card likely not in place
    return MMC_IDLE_TIMEOUT_ERROR;
  }else if((resp&MMC_RC_MASK)==MMC_R1_RESPONSE){
    //check if idle response was given
    if((resp&MMC_CARD_MASK)!=MMC_R1_IDLE){
      //unlock card
      mmcUnlock();
      //return a go idle response instead of a R1 response
      return (resp&MMC_CARD_MASK)|MMC_IDLE_RESP;
    }
    //yes then fall through and continue
  }else{
    //unlock card
    mmcUnlock();
    //unexpected response, return internal error
    return MMC_INTERNAL_ERROR;
  }

  //start new transaction
  CS_LOW();
  //Turn on CRC
  mmcSendCmd(MMC_CRC_ON_OFF,1,0xFF);
  //response starts with R1
  resp=mmc_R1(100);
  //TOOD: check response
  //end transaction
  CS_HIGH();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  
  //start new transaction
  CS_LOW();
  //send operating voltage and check pattern
  resp=mmcSendCmd(MMC_SEND_IF_COND,MMC_VHS_27_36|0xAA,0x87);
  //check for error (probably DMA timeout)
  if(resp!=MMC_SUCCESS){
    //deselect card
    SD_DESEL();
    //unlock card
    mmcUnlock();
    //return error
    return resp;
  }
  //response starts with R1
  resp=mmc_R1(100);
  //check for affermitave ir response
  if((((char)resp)&(~MMC_R1_IDLE))==MMC_SUCCESS){
    //get R7 response bits
    if(resp=spiReadFrame(extresp,4)){
      //For some reason (DMA9?) there was an error with readframe
      //deselect card
      SD_DESEL();
      //unlock card
      mmcUnlock();
      return resp;
    }
  }
  //end transaction
  CS_HIGH();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //check for illegal command
  if(resp&MMC_R1_ILLEGAL_CMD){
    //older card so must be SDSC
    //TODO: maybe send CMD58 to check voltage range
    for(i=0,resp=MMC_R1_IDLE;i<50 && resp==MMC_R1_IDLE;i++){
      //start transaction
      CS_LOW();
      //next command is application spesific command
      resp=mmcSendCmd(MMC_APP_CMD,0,0xff);
      //check for error (probably DMA timeout)
      if(resp!=MMC_SUCCESS){
        //deselect card
        SD_DESEL();
        //unlock card
        mmcUnlock();
        //return error
        return resp;
      }
      //get response
      resp=mmc_R1(100);
      //check for error
      if((resp&(~MMC_R1_IDLE))==MMC_SUCCESS){
        //send host capacity information
        resp=mmcSendCmd(MMC_SD_SEND_OP_COND,MMC_HCS,0xFF);
        //check for error (probably DMA timeout)
        if(resp!=MMC_SUCCESS){
          //deselect card
          SD_DESEL();
          //unlock card
          mmcUnlock();
          //return error
          return resp;
        }
        //get response
        resp=mmc_R1(100);
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
      //unlock card
      mmcUnlock();
      return MMC_INIT_ERR_TIMEOUT;
    }
    
    //set flags
    mmcStat.flags|=MMC_FLAG_INIT_CARD|MMC_FLAG_SDSC;
    mmcStat.flags&=~MMC_FLAG_SDHC;
    //SUCCESS!!!!!!!!!
    return MMC_SUCCESS;
  }else{
    //make sure that check pattern matches
    if(extresp[3]!=0xAA){
      //unlock card
      mmcUnlock();
      return MMC_INIT_ERR_CHECK_PATTERN;
    }
    //check voltage range
    if((extresp[2]&0x0F)!=(MMC_VHS_27_36>>8)){
      //unlock card
      mmcUnlock();
      return MMC_INIT_ERR_VOLTAGE;
    }
    //TODO: maybe send CMD58 to check voltage range
    for(i=0,resp=MMC_R1_IDLE;i<50 && ((char)resp)==MMC_R1_IDLE;i++){
      //start transaction
      CS_LOW();
      //next command is application spesific command
      resp=mmcSendCmd(MMC_APP_CMD,0,0xff);
      //check for error (probably DMA timeout)
      if(resp!=MMC_SUCCESS){
        //deselect card
        SD_DESEL();
        //unlock card
        mmcUnlock();
        //return error
        return resp;
      }
      //get response
      resp=mmc_R1(100);
      //check for error
      if((((char)resp)&(~MMC_R1_IDLE))==MMC_SUCCESS){
        //send host capacity information
        resp=mmcSendCmd(MMC_SD_SEND_OP_COND,MMC_HCS,0xFF);        
        //check for error (probably DMA timeout)
        if(resp!=MMC_SUCCESS){
          //deselect card
          SD_DESEL();
          //unlock card
          mmcUnlock();
          //return error
          return resp;
        }
        //get response
        resp=mmc_R1(100);
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
      //unlock card
      mmcUnlock();
      return MMC_INIT_ERR_TIMEOUT;
    }
    //start transaction
    CS_LOW();
    //get response
    resp=mmc_R1(100);
    //read OCR to check CCS bit
    resp=mmcSendCmd(MMC_READ_OCR,0,0xFF);
    //check for error (probably DMA timeout)
    if(resp!=MMC_SUCCESS){
      //deselect card
      SD_DESEL();
      //unlock card
      mmcUnlock();
      //return error
      return resp;
    }
    //get response
    resp=mmc_R1(100);
    //check response
    if(resp==MMC_SUCCESS){
      //get R3 response bits
      if(resp=spiReadFrame(extresp,4)){
        //deselect card
        SD_DESEL();
        //unlock card
        mmcUnlock();
        return resp;
      }  
    }
    //Transaction done
    CS_HIGH();
    // Send 8 Clock pulses of delay.
    spiDummyClk();

    //check if command was successful
    if(resp!=MMC_SUCCESS){
      //unlock card
      mmcUnlock();
      return MMC_INIT_ERR_READ_OCR;
    }

    //check if card is standard capacity or high capacity
    if(extresp[0]&(MMC_OCR_CCS>>24)){
      //high capacity
      mmcStat.flags&=~MMC_FLAG_SDSC;
      mmcStat.flags|=MMC_FLAG_SDHC;
    }else{
      //standard capacity      
      mmcStat.flags|=MMC_FLAG_SDSC;
      mmcStat.flags&=~MMC_FLAG_SDHC;
    }
    //set init flag
    mmcStat.flags|=MMC_FLAG_INIT_CARD;
    //unlock card
    mmcUnlock();
    //SUCCESS!!!!!!!!!
    return MMC_SUCCESS;
  }
}

//==============================================================
//              [MMC response functions]
//==============================================================

//get R1 response from SD card
int mmc_R1(int num){
  int i;
  unsigned char resp;
  for(i=0;i<num;i++){
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

//get R2 response from SD card
int mmc_R2(void){
  int i;
  unsigned char resp,rb2;
  for(i=0;i<100;i++){
    resp=spiSendByte(DUMMY_CHAR);
    //check for R1 response start bit
    if(!(resp&0x80)){
        //get byte #2 of response
        rb2=spiSendByte(DUMMY_CHAR);
        return (((unsigned short)resp)<<8)|rb2;
    }
    //wait a bit
    ctl_timeout_wait(ctl_get_current_time()+2);
  }
  //response not found
  return MMC_TIMEOUT_ERROR;
}

//wait for busy signal to go away
int mmc_busy(int num){
  unsigned char resp;
  int i;
  //not really sure on this one but it seems to need a dummy read
  resp=spiSendByte(DUMMY_CHAR);
  //wait for busy signal to go away
  for(i=0;i<num;i++){
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
int mmc_R1b(int r1_num,int busy_num){
  int i;
  unsigned char resp,rt;
  //get response
  rt=mmc_R1(r1_num);
  //check if there was an error
  if(rt==MMC_SUCCESS){
    //wait for busy signal to go away
    for(i=0;i<busy_num;i++){
      //get byte
      resp=spiSendByte(DUMMY_CHAR);
      //check if busy
      if(resp!=0x00){
        return rt;
      }
      //wait a bit
      ctl_timeout_wait(ctl_get_current_time()+2);
    }
  }else{
    return rt;
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
int mmc_token(int num){
  int i;
  unsigned char resp;
  for(i=0;i<num;i++){
    //get byte
    resp=spiSendByte(DUMMY_CHAR);
    //check for data start token
    if(resp!=0xFF){
      return resp|MMC_DATA_TOKEN_RESP;
    }
    //wait a bit before checking again
    ctl_timeout_wait(ctl_get_current_time()+2);
  }
  return MMC_TOKEN_TIMEOUT_ERROR;
}

//==============================================================
//              [MMC read functions]
//==============================================================

// read a block beginning at the given address.
int mmcReadBlock(SD_block_addr addr,void *pBuffer){
  int rvalue,resp,size;
  unsigned short crc_calc,crc;
  unsigned char crc1,crc2;
  //get a lock on the card
  if(resp=mmcLock(CTL_TIMEOUT_DELAY,10)){
    return resp;
  }
  //check if SDSC card
  size=mmc_check_size(mmcStat);
  //check if SDSC card
  if(size==MMC_FLAG_SDSC){
    //address is block address, not byte address
    addr*=512;
  //SDHC card falls through
  }else if(size!=MMC_FLAG_SDHC){
    //unknown card size
    //unlock card
    mmcUnlock();
    return MMC_INVALID_CARD_SIZE;
  }  
  // CS = LOW (on)
  CS_LOW ();
  // send read command MMC_READ_SINGLE_BLOCK=CMD17
  rvalue=mmcSendCmd(MMC_READ_SINGLE_BLOCK,addr, 0xFF);
  //check for error (probably DMA timeout)
  if(rvalue==MMC_SUCCESS){
    //Get R1 response
    rvalue=mmc_R1(100);
    if(rvalue==MMC_SUCCESS){
      //look for the data token to signify the start of the data
      if((rvalue=mmc_token(100))==(MMC_DATA_TOKEN_RESP|MMC_START_DATA_BLOCK_TOKEN)){
        // clock the actual data transfer and receive the bytes; spi_read automatically finds the Data Block
        rvalue = spiReadFrame(pBuffer,512);
          
        // get CRC bytes
        crc1=spiSendByte(DUMMY_CHAR);
        crc2=spiSendByte(DUMMY_CHAR);
      
        //only check CRC if transmission was successful
        if(!rvalue){
            //assemble CRC from SD card
            crc=(((unsigned short)crc1)<<8)|((unsigned short)crc2);
            //calculate CRC for buffer
            crc_calc=crc16(pBuffer,512);
            //check CRC's
            if(crc!=crc_calc){
                //CRC mismatch, return error
                rvalue=MMC_CRC_FAIL_ERROR;
            }
        }
      }
    }
  }
  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //unlock card
  mmcUnlock();
  return rvalue;
}// mmc_read_block

// read out multiple blocks at once
int mmcReadBlocks(SD_block_addr addr,unsigned short count,void *pBuffer){
  unsigned short i;
  int rvalue,rt,resp,size;
  unsigned short crc_calc,crc;
  unsigned char crc1,crc2;
  //get a lock on the card
  if(resp=mmcLock(CTL_TIMEOUT_DELAY,10)){
    return resp;
  }
  //check if SDSC card
  size=mmc_check_size(mmcStat);
  //check if SDSC card
  if(size==MMC_FLAG_SDSC){
    //address is block address, not byte address
    addr*=512;
  //SDHC card falls through
  }else if(size!=MMC_FLAG_SDHC){
    //unknown card size
    //unlock card
    mmcUnlock();
    return MMC_INVALID_CARD_SIZE;
  }
  // CS = LOW (on)
  CS_LOW ();
  // send read command MMC_READ_SINGLE_BLOCK=CMD17
  rvalue=mmcSendCmd(MMC_READ_MULTIPLE_BLOCK,addr, 0xFF);
  //check response (probably DMA timeout)
  if(rvalue==MMC_SUCCESS){
    //Get R1 response from card
    if ((rvalue=mmc_R1(100))==MMC_SUCCESS){
      //get data blocks
      for(i=0;i<count;i++){
        // look for the data token to signify the start of the data
        if((rvalue=mmc_token(100))==(MMC_DATA_TOKEN_RESP|MMC_START_DATA_BLOCK_TOKEN)){
          // clock the actual data transfer and receive the bytes
          resp = spiReadFrame(((unsigned char*)pBuffer)+i*512,512);
    
          // get CRC bytes
          crc1=spiSendByte(DUMMY_CHAR);
          crc2=spiSendByte(DUMMY_CHAR);
          //only check CRC if transmission was successful
          if(!rvalue){
              //assemble CRC from SD card
              crc=(((unsigned short)crc1)<<8)|((unsigned short)crc2);
              //calculate CRC for buffer
              crc_calc=crc16(pBuffer,512);
              //check CRC's
              if(crc!=crc_calc){
                  //CRC mismatch, return error
                  rvalue=MMC_CRC_FAIL_ERROR;
                  //stop transmission
                  break;
              }
          }else{
              // the data token was never received
              //abort
              break;
          }
        }
      }
      //send stop transmission command
      rt=mmcSendCmd(MMC_STOP_TRANSMISSION,0,0xFF);
      //check for error (probably DMA error)
      if(rt==MMC_SUCCESS){
        //check return value
        //get R1 response with busy signal
        rt=mmc_R1b(100,4000); 
        //check if an error occurred before
        if(((char)rvalue)==MMC_START_DATA_BLOCK_TOKEN){
          //return response to MMC_STOP_TRANSMISSION
          rvalue=rt;
        }
        //check if there was an error writing blocks
        if(resp != MMC_SUCCESS){
          //return previous error
          rvalue=resp;
        }
      }else{
        //return the first error received
        rvalue=(resp==MMC_SUCCESS)?rt:resp;
      }
    }
  }
  //CS = HIGH (off)
  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //unlock card
  mmcUnlock();
  //return result
  return rvalue;
}// mmc_read_block

//==============================================================
//              [MMC write functions]
//==============================================================

//write one data block on the SD card
int mmcWriteBlock(SD_block_addr addr,const void *pBuffer){
  int rvalue,result,resp,size;
  unsigned short crc;
  //get a lock on the card
  if(resp=mmcLock(CTL_TIMEOUT_DELAY,10)){
    return resp;
  }
  //check if SDSC card
  size=mmc_check_size(mmcStat);
  //check if SDSC card
  if(size==MMC_FLAG_SDSC){
    //address is block address, not byte address
    addr*=512;
  //SDHC card falls through
  }else if(size!=MMC_FLAG_SDHC){
    //unknown card size
    //unlock card
    mmcUnlock();
    return MMC_INVALID_CARD_SIZE;
  }
  // CS = LOW (on)
  CS_LOW ();
  // send write command
  rvalue=mmcSendCmd(MMC_WRITE_BLOCK,addr, 0xFF);
  //check for error (probably DMA timeout)
  if(rvalue == MMC_SUCCESS){
    //check R1 response for no errors
    if((rvalue=mmc_R1(100))==MMC_SUCCESS){
      // send the data token to signify the start of the data
      spiSendByte(MMC_START_DATA_BLOCK_WRITE);
      // clock the actual data transfer and transmit the bytes
      result=spiSendFrame(pBuffer,512);
    
      crc=crc16(pBuffer,512);

      // put CRC bytes 
      spiSendByte(crc>>8);        //MSB
      spiSendByte(crc);           //LSB
      //get data response
      rvalue=mmc_dat_resp(); 
      //check if data was accepted CRC not used so ignore CRC error
      if(((char)rvalue)==MMC_DAT_ACCEPTED){
        rvalue=MMC_SUCCESS;
      }else if(((char)rvalue)==MMC_DAT_CRC){
        //CRC error, read status to clear error
        CS_HIGH ();
        // Send 8 Clock pulses of delay.
        spiDummyClk();
        // CS = LOW (on)
        CS_LOW ();
        //CRC error, read status to clear
        mmcSendCmd(MMC_SEND_STATUS,0,0xFF);
        //get response
        mmc_R2();
      }
      //check if spiSendFrame was successful
      if(result){
        //return error from spiSendFrame
        rvalue=result;
      }
    }
  }

  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //unlock card
  mmcUnlock();
  return rvalue;
} // mmc_write_block

//write mutiple blocks of data fist block # is given as start
int mmcWriteMultiBlock(SD_block_addr addr,const void *pBuffer,unsigned short blocks){
  int rvalue,size;
  int resp;
  unsigned short crc;
  unsigned int i;
  //get a lock on the card
  if(resp=mmcLock(CTL_TIMEOUT_DELAY,10)){
    return resp;
  }
  //check if SDSC card
  size=mmc_check_size(mmcStat);
  //check if SDSC card
  if(size==MMC_FLAG_SDSC){
    //address is block address, not byte address
    addr*=512;
  //SDHC card falls through
  }else if(size!=MMC_FLAG_SDHC){
    //unknown card size
    //unlock card
    mmcUnlock();
    return MMC_INVALID_CARD_SIZE;
  }
  // CS = LOW (on)
  CS_LOW ();
  
  // send write command
  rvalue=mmcSendCmd(MMC_WRITE_MULTIPLE_BLOCK,addr, 0xFF);
  //check for errors
  if(rvalue==MMC_SUCCESS){ 
    //check for errors
    if((rvalue=mmc_R1(100))==MMC_SUCCESS){ 
      //loop over blocks to write
      for(i=0;i<blocks;i++){
        // send the data token to signify the start of the data
        spiSendByte(MMC_START_DATA_MULTIPLE_BLOCK_WRITE);
        // clock the actual data transfer and transmit the bytes
      
        spiSendFrame(((unsigned char*)pBuffer)+i*512,512);
      
        crc=crc16(((unsigned char*)pBuffer)+i*512,512);

        // put CRC bytes 
        spiSendByte(crc>>8);        //MSB
        spiSendByte(crc);           //LSB
        //get data response
        rvalue=mmc_dat_resp(); 
        //an error occurred, abort transmission
        if(((char)rvalue)!=MMC_DAT_ACCEPTED){
          //send stop transmission command
          mmcSendCmd(MMC_STOP_TRANSMISSION,0,0xFF);
          //get R1 response with busy signal
          //keep the response that generated the error
          mmc_R1b(100,400); 
          break;
        }
      }
      if(((char)rvalue)==MMC_DAT_ACCEPTED){
        //send stop transfer token
        spiSendByte(MMC_STOP_DATA_MULTIPLE_BLOCK_WRITE);
        //wait for completion
        rvalue=mmc_busy(400);
      }else if(((char)rvalue)==MMC_DAT_CRC){
        //CRC error, read status to clear error
        CS_HIGH ();
        // Send 8 Clock pulses of delay.
        spiDummyClk();
        // CS = LOW (on)
        CS_LOW ();
        //CRC error, read status to clear
        mmcSendCmd(MMC_SEND_STATUS,0,0xFF);
        //get response
        mmc_R2();
      }
    }
  }

  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //unlock card
  mmcUnlock();
  //return resp if an error occurred sending data
  return resp==MMC_SUCCESS?rvalue:resp;
} // mmc_write_block

//==============================================================
//              [other MMC functions]
//==============================================================

// send command to MMC
int mmcSendCmd (char cmd, unsigned long data,char crc)
{
  unsigned char frame[6];
  char temp;
  int i;
  frame[0]=(cmd|MMC_TX_BIT);
  frame[1]=(data>>(24));
  frame[2]=(data>>(16));
  frame[3]=(data>>(8));
  frame[4]=(data);
  frame[5]=crc7(frame,5);
  return spiSendFrame(frame,6);
}

//erase blocks from start to end
int mmcErase(SD_block_addr start,SD_block_addr end){
  int rvalue,resp,size;
  //get a lock on the card
  if(resp=mmcLock(CTL_TIMEOUT_DELAY,10)){
    return resp;
  }
  //check if SDSC card
  size=mmc_check_size(mmcStat);
  //check if SDSC card
  if(size==MMC_FLAG_SDSC){
    //address is block address, not byte address
    start*=512;
    end*=512;
  //SDHC card falls through
  }else if(size!=MMC_FLAG_SDHC){
    //unknown card size
    //unlock card
    mmcUnlock();
    return MMC_INVALID_CARD_SIZE;
  }
  // CS = LOW (on)
  CS_LOW ();
  
  //send erase block start
  rvalue=mmcSendCmd(MMC_ERASE_WR_BLK_START,start,0xFF);
  //check for error (probably DMA timeout)
  if(rvalue==MMC_SUCCESS){
    //check for correct response
    if((rvalue=mmc_R1(100))==MMC_SUCCESS){
      //send erase block end
      rvalue=mmcSendCmd(MMC_ERASE_WR_BLK_END,end,0xFF);
      //check for error (probably DMA timeout)
      if(rvalue==MMC_SUCCESS){
        //check for correct response
        if((rvalue=mmc_R1(100))==MMC_SUCCESS){
          //send erase command
          rvalue=mmcSendCmd(MMC_EREASE,0,0xFF);          
          //check for error (probably DMA timeout)
          if(rvalue==MMC_SUCCESS){
            //get R1b response
            rvalue=mmc_R1b(100,4000);
          }
        }
      }
    }
  }
  //end transaction
  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //unlock card
  mmcUnlock();
  return rvalue;
}

//read CID or CSD register reg should be MMC_SEND_CSD or MMC_SEND_CID depending on which register is to be read
//data is stored in buffer which should be at least 16 bytes long
int mmcReadReg(unsigned char reg,unsigned char *buffer){
  int rvalue,resp;
  //get a lock on the card
  if(resp=mmcLock(CTL_TIMEOUT_DELAY,10)){
    return resp;
  }
  //select
  CS_LOW ();
  //send read CSD
  rvalue=mmcSendCmd(reg,0,0xFF);
  //check for error (probably DMA timeout)
  if(rvalue==MMC_SUCCESS){
    //check response
    if((rvalue=mmc_R1(100))==MMC_SUCCESS){
      if((rvalue=mmc_token(100))==(MMC_DATA_TOKEN_RESP|MMC_START_DATA_BLOCK_TOKEN)){
        //get CSD data
        rvalue=spiReadFrame(buffer,16);
        // put CRC bytes (not really needed by us, but required by MMC)
        spiSendByte(DUMMY_CHAR);
        spiSendByte(DUMMY_CHAR);
      }
    }
  }
  //deselect card
  CS_HIGH ();
  // Send 8 Clock pulses of delay.
  spiDummyClk();
  //unlock card
  mmcUnlock();
  //return error value
  return rvalue;
}


//define for number of bytes in a megabyte
#define SD_B_IN_MB      (1024lu*1024lu)

//return size in MB from CSD structure
//MB is used because it results in smaller numbers
unsigned long mmcGetCardSize(unsigned char *CSD){
  unsigned long Csize,tmp;
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
      //compute size in MB
      //return ((Csize+1)*mult*blocklen)/1024;
      //calculate multiplication value
      tmp=mult*blocklen;
      //check if value is less than 
      if(tmp<SD_B_IN_MB){
        return ((Csize+1)*tmp)/SD_B_IN_MB;
      }else{
        return (Csize+1)/(SD_B_IN_MB/tmp);
      }
    case 1:
      //version 2.0
      //get C_SIZE field
      Csize =CSD[9];                               //CSD bits 48-55
      Csize|=CSD[8]<<8;                            //CSD bits 56-63
      Csize|=((unsigned long)CSD[7]&0x3F)<<16;     //CSD bits 64-69
      //size in MB
      //return (Csize+1)*512*1024/SD_B_IN_MB;
      return (Csize+1)/2;
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
