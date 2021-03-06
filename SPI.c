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

//setup SPI with ~200kHz clock
void SPISetup(void){
  SPI_REG_W(UCAxCTLW0_OFFSET) |= UCSWRST;
  SPI_REG_W(UCAxCTLW0_OFFSET)  =UCMST|UCCKPL|UCMSB|UCSYNC|UCSSEL_2|UCSWRST;     // 3-pin, 8-bit SPI master, SMCLK
  SPI_REG_W(UCAxBRW_OFFSET)    = 100;    
  SPI_REG_W(UCAxCTLW0_OFFSET) &=~UCSWRST;                      //Initialize state machine
}

//set SPI clock speed to 4MHz
void SPI_fast(void){
  SPI_REG_W(UCAxCTLW0_OFFSET)|=UCSSEL_2;                     //put interface into reset state
  //SPI_REG_W(UCAxBRW_OFFSET)   =4;                            //set clock rate
  SPI_REG_W(UCAxBRW_OFFSET)   =5;                            //set clock rate
  SPI_REG_W(UCAxCTLW0_OFFSET)&=~UCSWRST;                     //pull interface out of reset state
}

//set SPI clock speed to 200kHz
void SPI_slow(void){
  SPI_REG_W(UCAxCTLW0_OFFSET)|= UCSWRST;                     //put interface into reset state
  SPI_REG_W(UCAxBRW_OFFSET)   = 100;                         //set clock rate
  SPI_REG_W(UCAxCTLW0_OFFSET)&=~UCSWRST;                     //pull interface out of reset state
}

//shutdown SPI peripheral
void SPIShutdown(void){
  //put peripheral in reset state
  SPI_REG_W(UCAxCTLW0_OFFSET)|=UCSWRST;
}


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
int spiReadFrame(unsigned char* pBuffer, unsigned int size)
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
    //disable DMA
    DMA0CTL&=~DMAEN;
    DMA1CTL&=~DMAEN;
    DMA2CTL&=~DMAEN;
    //set DMA flags for spi tx and rx
    DMACTL0 &=~(DMA1TSEL_31|DMA0TSEL_31);
    DMACTL1 &=~(DMA2TSEL_31);
    #if SPI_SER_INTF ==  SER_INTF_UCA1
      DMACTL0 |= DMA1TSEL__USCIA1RX|DMA0TSEL__USCIA1TX;
      DMACTL1 |= DMA2TSEL__USCIA1RX;
      //setup dummy channel: read and write from unused space in the SPI registers
      *((unsigned int*)&DMA2SA) = EUSCI_A1_BASE + 0x02;
      *((unsigned int*)&DMA2DA) = EUSCI_A1_BASE + 0x04;
    #elif SPI_SER_INTF ==  SER_INTF_UCA2
      DMACTL0 |= DMA1TSEL__USCIA2RX|DMA0TSEL__USCIA2TX;
      DMACTL1 |= DMA2TSEL__USCIA2RX;
      //setup dummy channel: read and write from unused space in the SPI registers
      *((unsigned int*)&DMA2SA) = EUSCI_A2_BASE + 0x02;
      *((unsigned int*)&DMA2DA) = EUSCI_A2_BASE + 0x04;
    #elif SPI_SER_INTF ==  SER_INTF_UCA3
      //DMACTL0 |= DMA2TSEL__USCIA3RX|DMA0TSEL__USCIA3TX;   
      DMACTL0 |= DMA1TSEL__USCIA3RX|DMA0TSEL_26;   //bug in header, DMA2TSEL__USCIA3TX not defined    
      DMACTL1 |= DMA2TSEL__USCIA3RX; 
      //setup dummy channel: read and write from unused space in the SPI registers
      *((unsigned int*)&DMA2SA) = EUSCI_A3_BASE + 0x02;
      *((unsigned int*)&DMA2DA) = EUSCI_A3_BASE + 0x04;
    #elif SPI_SER_INTF ==  SER_INTF_UCB1
      DMACTL0 |= DMA1TSEL__USCIB1RX|DMA0TSEL__USCIB1TX;
      DMACTL1 |= DMA2TSEL__USCIB1RX;
      //setup dummy channel: read and write from unused space in the SPI registers
      *((unsigned int*)&DMA2SA) = EUSCI_B1_BASE + 0x02;
      *((unsigned int*)&DMA2DA) = EUSCI_B1_BASE + 0x04;
    #endif
    //Setup dummy channel for DMA9 workaround
    // only one byte
    DMA2SZ = 1;
    // Configure the DMA transfer, repeated byte transfer with no increment
    DMA2CTL = DMADT_4|DMASBDB|DMAEN|DMASRCINCR_0|DMADSTINCR_0;

    // Source DMA address: receive register.
    DMA1SA = (unsigned int)(&SPIRXBUF);
    // Destination DMA address: the user data buffer.
    DMA1DA = (unsigned short)pBuffer;
    // The size of the block to be transferred
    DMA1SZ = size;
    // Configure the DMA transfer. single byte transfer with destination increment. enable interrupt on completion
    DMA1CTL = DMADT_0|DMASBDB|DMAEN|DMADSTINCR_3|DMAIE;
    // Source DMA address: constant 0xFF (don't increment)
    DMA0SA = (unsigned int)(&SPITXBUF);
    // Destination DMA address: the transmit buffer. 
    DMA0DA = (unsigned int)(&SPITXBUF);
    // Increment the destination address sta
    // The size of the block to be transferred 
    DMA0SZ = size-1;
    // Configure DMA transfer. single byte transfer with no increment
    DMA0CTL = DMADT_0|DMASBDB|DMAEN;
    // Kick off the transfer by sending the first byte
    SPI_SEND(DUMMY_CHAR);
    //wait for event to complete
    //TODO: improve timeout
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&DMA_events,DMA_EV_SD_SPI,CTL_TIMEOUT_DELAY,1024);
    //disable DMA
    DMA0CTL&=~DMAEN;
    DMA1CTL&=~DMAEN;
    DMA2CTL&=~DMAEN;
    //check to see that event happened
    if(!(e&DMA_EV_SD_SPI)){
      //event did not happen, return error
      return MMC_DMA_RX_TIMEOUT_ERROR;
    }
#endif
  return MMC_SUCCESS;
}


//Send a frame of bytes via SPI
int spiSendFrame(const unsigned char* pBuffer, unsigned int size)
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
      int i;
      //disable DMA
      DMA1CTL&=~DMAEN;
      DMA2CTL&=~DMAEN;
      // DMA trigger is SPI send
      DMACTL0 &= ~(DMA1TSEL_31);
      DMACTL1 &= ~(DMA2TSEL_31);
      #if SPI_SER_INTF ==  SER_INTF_UCA1
        DMACTL0 |= DMA1TSEL__USCIA1TX;
        DMACTL1 |= DMA2TSEL__USCIA1TX;
        //setup dummy channel: read and write from unused space in the SPI registers
        *((unsigned int*)&DMA2SA) = EUSCI_A1_BASE + 0x02;
        *((unsigned int*)&DMA2DA) = EUSCI_A1_BASE + 0x04;
      #elif SPI_SER_INTF ==  SER_INTF_UCA2
        DMACTL0 |= DMA1TSEL__USCIA2TX;
        DMACTL1 |= DMA2TSEL__USCIA2TX;
        //setup dummy channel: read and write from unused space in the SPI registers
        *((unsigned int*)&DMA2SA) = EUSCI_A2_BASE + 0x02;
        *((unsigned int*)&DMA2DA) = EUSCI_A2_BASE + 0x04;
      #elif SPI_SER_INTF ==  SER_INTF_UCA3
        //DMACTL0 |= DMA1TSEL__USCIA3TX;
        //DMACTL1 |= DMA2TSEL__USCIA3TX;
        DMACTL0 |= DMA1TSEL_26;      //bug in header DMA2TSEL__USCIA3TX not defined
        DMACTL1 |= DMA2TSEL_26;
        //setup dummy channel: read and write from unused space in the SPI registers
        *((unsigned int*)&DMA2SA) = EUSCI_A3_BASE + 0x02;
        *((unsigned int*)&DMA2DA) = EUSCI_A3_BASE + 0x04; 
      #elif SPI_SER_INTF ==  SER_INTF_UCB1
        DMACTL0 |= DMA1TSEL__USCIB1TX;
        DMACTL1 |= DMA2TSEL__USCIB1TX;
        //setup dummy channel: read and write from unused space in the SPI registers
        *((unsigned int*)&DMA2SA) = EUSCI_B1_BASE + 0x02;
        *((unsigned int*)&DMA2DA) = EUSCI_B1_BASE + 0x04;
      #endif
      //Setup dummy channel for DMA9 workaround
      // only one byte
      DMA2SZ = 1;
      // Configure the DMA transfer, repeated byte transfer with no increment
      DMA2CTL = DMADT_4|DMASBDB|DMAEN|DMASRCINCR_0|DMADSTINCR_0;

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
      //disable dummy DMA
      DMA2CTL&=~DMAEN;
      //clear RX flag and set TX flag
      SPI_REG_W(UCAxIFG_OFFSET)=UCTXIFG;
      //wait while busy bit is set. Loop escape because of USCI41
      for(i=0;i<60 && SPI_BUSY;i++);
      //check to see that event happened
      if(!(e&DMA_EV_SD_SPI)){
        //event did not happen, return error
        return MMC_DMA_TX_TIMEOUT_ERROR;
      }
#endif
  return MMC_SUCCESS;
}

