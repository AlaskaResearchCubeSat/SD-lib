#ifndef __HARDWARE_H
#define __HARDWARE_H

#include <msp430.h>  

//define identifiers for serial periferals
#define SER_INTF_UCA0  3
#define SER_INTF_UCA1  4
#define SER_INTF_UCB0  5
#define SER_INTF_UCB1  6
#define SER_INTF_UCA2  7
#define SER_INTF_UCA3  8 

#ifdef SER_USE_UCA0
  #define SPI_SER_INTF  SER_INTF_UCA0
#endif

#ifdef SER_USE_UCB0
  #define SPI_SER_INTF  SER_INTF_UCB0
#endif

#ifdef SER_USE_UCA1
  #define SPI_SER_INTF  SER_INTF_UCA1
#endif

#ifdef SER_USE_UCB1
  #define SPI_SER_INTF  SER_INTF_UCB1
#endif

#ifdef SER_USE_UCA2
  #define SPI_SER_INTF  SER_INTF_UCA2
#endif

#ifdef SER_USE_UCA3
  #define SPI_SER_INTF  SER_INTF_UCA3
#endif

#define UCAxCTLW0_OFFSET      0X00
#define UCAxBRW_OFFSET        0X06
#define UCAxSTATW_OFFSET      0X0A
#define UCAxRXBUF_OFFSET      0x0C
#define UCAxTXBUF_OFFSET      0x0E
#define UCAxIE_OFFSET         0x1A 
#define UCAxIFG_OFFSET        0x1C 
#define UCAxIV_OFFSET         0x1E 
 

// SPI pin definitions due to portmaping all variants use the same pins
#define MMC_PxSEL        P4SEL0    
#define MMC_PxOUT        P4OUT
#define MMC_PxREN        P4REN
#define MMC_PxDIR        P4DIR 

#define MMC_SIMO          BIT7  //P4.7
#define MMC_SOMI          BIT5  //P4.5
#define MMC_UCLK          BIT6  //P4.6

#define MMC_PMAP_SIMO     P4MAP7
#define MMC_PMAP_SOMI     P4MAP5
#define MMC_PMAP_UCLK     P4MAP6

// Chip Select
#define MMC_CS_PxOUT      P5OUT
#define MMC_CS_PxDIR      P5DIR
#define MMC_CS            BIT0  //use P5.0 for CS

#if SPI_SER_INTF ==  SER_INTF_UCA1                     

#define SPI_BASE          (__MSP430_BASEADDRESS_EUSCI_A1__)

  //port mapping values
  #define MMC_PM_SIMO     PM_UCA1SIMO
  #define MMC_PM_SOMI     PM_UCA1SOMI
  #define MMC_PM_UCLK     PM_UCA1CLK

  //define SPI registers for UCA1
 #define SPI_SEND(x)     do{int t;UCA1TXBUF=x;t=UCA1RXBUF;}while(0)
 #define SPITXDONE       ((SPIRXREADY))          /* Wait for TX to finish */
 #define SPIRXFG_RST     {int t;t=UCA1RXBUF;}

  //define ISR name
  #define SPI_VECTOR      USCI_A1_VECTOR

#elif  SPI_SER_INTF == SER_INTF_UCB1

  //port mapping values
  #define MMC_PM_SIMO     PM_UCB1SIMO
  #define MMC_PM_SOMI     PM_UCB1SOMI
  #define MMC_PM_UCLK     PM_UCB1CLK

  #define SPI_BASE          (__MSP430_BASEADDRESS_EUSCI_B1__)

  //define SPI registers for UCB1
  #define SPI_SEND(x)     (UCB1TXBUF=x)         
  #define SPITXDONE       (!(UCB1STATW&UCBUSY))      /* Wait for TX to finish */
  #define SPIRXFG_RST     {int t;t=UCB1RXBUF;}     

  //define ISR name
  #define SPI_VECTOR      USCI_B1_VECTOR
  
#elif  SPI_SER_INTF == SER_INTF_UCA2

  //port mapping values
  #define MMC_PM_SIMO     PM_UCA2SIMO
  #define MMC_PM_SOMI     PM_UCA2SOMI
  #define MMC_PM_UCLK     PM_UCA2CLK

  #define SPI_BASE          (__MSP430_BASEADDRESS_EUSCI_A2__)

  //define SPI registers for UCA2 
  #define SPI_SEND(x)     do{int t;UCA2TXBUF=x;t=UCA2RXBUF;}while(0)        
  #define SPITXDONE       ((SPIRXREADY))          /* Wait for TX to finish */
  #define SPIRXFG_RST     {int t;t=UCA2RXBUF;}     

  //define ISR name
  #define SPI_VECTOR      USCI_A2_VECTOR
  
#elif  SPI_SER_INTF == SER_INTF_UCA3

  //port mapping values
  #define MMC_PM_SIMO     PM_UCA3SIMO
  #define MMC_PM_SOMI     PM_UCA3SOMI
  #define MMC_PM_UCLK     PM_UCA3CLK
  
  #define SPI_BASE          (__MSP430_BASEADDRESS_EUSCI_A3__)

  //define SPI registers for UCA3
  #define SPI_SEND(x)     do{int t;t=UCA3RXBUF;UCA3TXBUF=x;}while(0)
  #define SPITXDONE       ((SPIRXREADY))          /* Wait for TX to finish */
  #define SPIRXFG_RST     {int t;t=UCA3RXBUF;}     
  
  //define ISR name
  #define SPI_VECTOR      USCI_A3_VECTOR

  
#else
  #ifndef SPI_SER_INTF
    #error SPI_SER_INTF not defined pleases pesify SPI Port to use
  #else
    #error SPI_SER_INTF not valid please spesify SPI Port to use
  #endif
#endif

#define SPI_REG_W(offset)     (*(volatile unsigned short*)(SPI_BASE+(offset)))
#define SPI_REG_B(offset)     (*(volatile unsigned char*)(SPI_BASE+(offset)))

#define SPI_BUSY        (SPI_REG_W(UCAxSTATW_OFFSET)&UCBUSY) 
#define SPIRXBUF        (SPI_REG_W(UCAxRXBUF_OFFSET))
#define SPITXBUF        (SPI_REG_W(UCAxTXBUF_OFFSET))
#define SPIRXREADY      (SPI_REG_W(UCAxIFG_OFFSET)&UCRXIFG)        /* Wait for TX to be ready */
#define SPITXREADY      (SPI_REG_W(UCAxIFG_OFFSET)&UCTXIFG)        /* Wait for TX to be ready */

//definitions for chip select pin
#define CS_LOW()    (MMC_CS_PxOUT &= ~MMC_CS)                   // Card Select
#define CS_HIGH()   (MMC_CS_PxOUT |= MMC_CS)                    // Card Deselect

//definitions for card select and deselect
#define SD_SEL()    CS_LOW()                        // Card Select
//TODO: this seems like bad practice, fix
#define SD_DESEL()   CS_HIGH()  // Card Deselect

#endif
