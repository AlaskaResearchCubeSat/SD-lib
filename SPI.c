//----------------------------------------------------------------------------
//  This file contains functions that allow the MSP430 device to access the
//  SPI interface.  There are multiple instances of each function; 
//  the one to be compiled is selected by the system variable
//  SPI_SER_INTF, defined in "hal_hardware_board.h".
//----------------------------------------------------------------------------

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

//---------------------------------------------------------------
#include "SPI.h"
#include "hardware.h"
#include <msp430.h>
#include <ctl.h>
//needed for error definitions
#include "SDlib.h"

// SPI port functions

#if SPI_SER_INTF == SER_INTF_UCB1

//setup SPI with 200kHz clock
void SPISetup(void){
  UCB1CTLW0|= UCSWRST;
  UCB1CTLW0 =UCMST|UCCKPL|UCMSB|UCSYNC|UCSSEL_2|UCSWRST;              // 3-pin, 8-bit SPI master, SMCLK
  UCB1BRW   =80;
  UCB1CTLW0&=~UCSWRST;                     //pull interface out of reset state
}

//set SPI clock speed to 4MHz
void SPI_fast(void){
  UCB1CTL1|=UCSWRST;                      //put interface into reset state
  UCB1BRW  =4;
  UCB1CTL1&=~UCSWRST;                     //pull interface out of reset state
}

//set SPI clock speed to 2kHz
void SPI_slow(void){
  UCB1CTL1|=UCSWRST;                       //put interface into reset state
  UCB1BRW  =80;
  UCB1CTL1&=~UCSWRST;                     //pull interface out of reset state
}

void SPIShutdown(void){
  //put peripheral in reset state
  UCB1CTL1 =UCSWRST;
}

#else

//setup SPI with 200kHz clock
void SPISetup(void){
  SPI_BASE[UCAxCTLW0_OFFSET] |= UCSWRST;
  SPI_BASE[UCAxCTLW0_OFFSET]  =UCMST|UCCKPL|UCMSB|UCSYNC|UCSSEL_2|UCSWRST;     // 3-pin, 8-bit SPI master, SMCLK
  SPI_BASE[UCAxBRW_OFFSET]    =80;    
  SPI_BASE[UCAxCTLW0_OFFSET] &=~UCSWRST;                      //Initialize state machine
}

//set SPI clock speed to 4MHz
void SPI_fast(void){
  SPI_BASE[UCAxCTLW0_OFFSET]|=UCSSEL_2;                     //put interface into reset state
  SPI_BASE[UCAxBRW_OFFSET]   =4;                            //set clock rate
  SPI_BASE[UCAxCTLW0_OFFSET]&=~UCSWRST;                     //pull interface out of reset state
}

//set SPI clock speed to 200kHz
void SPI_slow(void){
  SPI_BASE[UCAxCTLW0_OFFSET]|= UCSWRST;                     //put interface into reset state
  SPI_BASE[UCAxBRW_OFFSET]   =80;                           //set clock rate
  SPI_BASE[UCAxCTLW0_OFFSET]&=~UCSWRST;                     //pull interface out of reset state
}

//shutdown SPI peripheral
void SPIShutdown(void){
  //put peripheral in reset state
  SPI_BASE[UCAxCTLW0_OFFSET]|=UCSWRST;
}

#endif

//Send one byte via SPI
unsigned char spiSendByte(const unsigned char data){
  while(!SPITXREADY);    // wait while not ready for TX
  SPI_SEND(data);            // write
  while(!SPIRXREADY);    // wait for RX buffer (full) 
  return (SPIRXBUF);
}

//Send out 8 dummy clocks
void spiDummyClk(void){
  char d;
  while (SPITXREADY ==0);    // wait while not ready for TX
  SPI_SEND(DUMMY_CHAR);      // write dummy char
  while(!SPITXDONE);         //wait for completion
  d=SPIRXBUF;                //dummy read to clear flags
}

//Read a frame of bytes via SPI
unsigned char spiReadFrame(unsigned char* pBuffer, unsigned int size)
{
#ifndef withDMA
  unsigned long i = 0;
  // clock the actual data transfer and receive the bytes; spi_read automatically finds the Data Block
  for (i = 0; i < size; i++){
    while(!SPITXREADY);   // wait while not ready for TX
    SPI_SEND(DUMMY_CHAR);     // dummy write
    while(!SPIRXREADY);   // wait for RX buffer (full)
    pBuffer[i] = SPIRXBUF;
  }
#else
    unsigned int e;
    //set DMA flags for spi tx and rx
    DMACTL0 &=~(DMA1TSEL_15|DMA2TSEL_15);
    #if SPI_SER_INTF ==  SER_INTF_UCA1
      DMACTL0 |= DMA1TSEL__USCIA1RX|DMA2TSEL__USCIA1TX;
    #elif SPI_SER_INTF ==  SER_INTF_UCA2
      DMACTL0 |= DMA1TSEL__USCIA2RX|DMA2TSEL__USCIA2TX;
    #elif SPI_SER_INTF ==  SER_INTF_UCA3
      DMACTL0 |= DMA2TSEL__USCIA3RX|DMA2TSEL__USCIA3TX;
    #elif SPI_SER_INTF ==  SER_INTF_UCB1
      DMACTL0 |= DMA1TSEL__USCIB1RX|DMA2TSEL__USCIB1TX;
    #endif
    DMACTL0 |= (DMA1TSEL_9 |DMA2TSEL_10);
    // Source DMA address: receive register.
    DMA1SA = (unsigned int)(&SPIRXBUF);
    // Destination DMA address: the user data buffer.
    DMA1DA = (unsigned short)pBuffer;
    // The size of the block to be transferred
    DMA1SZ = size;
    // Configure the DMA transfer. single byte transfer with destination increment. enable interrupt on completion
    DMA1CTL = DMADT_0|DMASBDB|DMAEN|DMADSTINCR_3|DMAIE;
    // Source DMA address: constant 0xFF (don't increment)
    DMA2SA = (unsigned int)(&SPITXBUF);
    // Destination DMA address: the transmit buffer. 
    DMA2DA = (unsigned int)(&SPITXBUF);
    // Increment the destination address sta
    // The size of the block to be transferred 
    DMA2SZ = size-1;
    // Configure DMA transfer. single byte transfer with no increment
    DMA2CTL = DMADT_0|DMASBDB|DMAEN;
    // Kick off the transfer by sending the first byte
    SPI_SEND(DUMMY_CHAR);
    //wait for event to complete
    //TODO: improve timeout
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&DMA_events,DMA_EV_SD_SPI,CTL_TIMEOUT_DELAY,1024);
    //check to see that event happened
    if(!(e&DMA_EV_SD_SPI)){
      //event did not happen, return error
      return MMC_DMA_TIMEOUT_ERROR;
    }
#endif
  return 0;
}


//Send a frame of bytes via SPI
unsigned char spiSendFrame(const unsigned char* pBuffer, unsigned int size)
{
#ifndef withDMA
  unsigned long i = 0;
  unsigned char tmp;
  // clock the actual data transfer and receive the bytes; spi_read automatically finds the Data Block
  for (i = 0; i < size; i++){
    while(!SPITXREADY);   // wait while not ready for TX
    SPI_SEND(pBuffer[i]);     // write data
    while(!SPIRXREADY);   // wait for RX buffer (full)
    tmp = SPIRXBUF;        //dummy read
  }
#else
      unsigned int e;
      //TODO: is there a better way??
      while(!SPITXDONE);
      // DMA trigger is SPI send
      DMACTL0 &= ~(DMA1TSEL_15);
      #if SPI_SER_INTF ==  SER_INTF_UCA1
        DMACTL0 |= DMA1TSEL__USCIA1TX;
      #elif SPI_SER_INTF ==  SER_INTF_UCA2
        DMACTL0 |= DMA1TSEL__USCIA2TX;
      #elif SPI_SER_INTF ==  SER_INTF_UCA3
        DMACTL0 |= DMA1TSEL__USCIA3TX;
      #elif SPI_SER_INTF ==  SER_INTF_UCB1
        DMACTL0 |= DMA1TSEL__USCIB1TX;
      #endif
      // Source DMA address: the data buffer.
      DMA1SA = (unsigned short)(&pBuffer[1]);
      // Destination DMA address: the SPI send register.
      DMA1DA = (unsigned int)(&SPITXBUF);
      // The size of the block to be transferred, the first byte is transfered manualy so don't count it
      DMA1SZ = size-1;
      // Configure the DMA transfer. single byte transfer with destination increment. enable interrupt on completion
      DMA1CTL = DMADT_0|DMASBDB|DMAEN|DMASRCINCR_3|DMAIE;
      //Kick off the transfer by sending the first byte
      SPI_SEND(pBuffer[0]);
      //wait for transaction to complete
      //TODO: improve timeout
      e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&DMA_events,DMA_EV_SD_SPI,CTL_TIMEOUT_DELAY,1024);         
      //clear SPI rx flag. Needed because RX buffer is not read
      SPIRXFG_CLR;
      //check to see that event happened
      if(!(e&DMA_EV_SD_SPI)){
        //event did not happen, return error
        return MMC_DMA_TIMEOUT_ERROR;
      }
#endif
  return 0;
}

