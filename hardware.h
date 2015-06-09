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
#define UCAxBRW_OFFSET        0X03
#define UCAxSTATW_OFFSET      0X05
#define UCAxRXBUF_OFFSET      0x06
#define UCAxTXBUF_OFFSET      0x07
#define UCAxIE_OFFSET         0x0D 
#define UCAxIFG_OFFSET        0x0E 
#define UCAxIV_OFFSET         0x0F 
 

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

#define SPI_BASE          (&UCA1CTLW0)

  //port mapping values
  #define MMC_PM_SIMO     PM_UCA1SIMO
  #define MMC_PM_SOMI     PM_UCA1SOMI
  #define MMC_PM_UCLK     PM_UCA1CLK

  //define SPI registers for UCA1
 #define SPIRXBUF        UCA1RXBUF
 #define SPITXBUF        UCA1TXBUF
 #define SPI_SEND(x)     (UCA1TXBUF=x)
 #define SPITXREADY      (UCA1IFG&UCTXIFG)               /* Wait for TX to be ready */
 #define SPITXDONE       (!(UCA1STATW&UCBUSY))            /* Wait for TX to finish */
 #define SPIRXREADY      (UCA1IFG&UCRXIFG)              /* Wait for TX to be ready */
 #define SPIRXFG_CLR     (UCA1IFG &= ~UCRXIFG)
 #define SPIFG_CLR       (UCA1IFG&=~(UCRXIFG|UCTXIFG))

  //define ISR name
  #define SPI_VECTOR      USCI_A1_VECTOR

#elif  SPI_SER_INTF == SER_INTF_UCB1

  //port mapping values
  #define MMC_PM_SIMO     PM_UCB1SIMO
  #define MMC_PM_SOMI     PM_UCB1SOMI
  #define MMC_PM_UCLK     PM_UCB1CLK

  //define SPI registers for UCB1
  #define SPIRXBUF        UCB1RXBUF
  #define SPITXBUF        UCB1TXBUF 
  #define SPI_SEND(x)     (UCB1TXBUF=x)         
  #define SPITXREADY      (UCB1IFG&UCTXIFG)        /* Wait for TX to be ready */
  #define SPITXDONE       (!(UCB1STATW&UCBUSY))      /* Wait for TX to finish */
  #define SPIRXREADY      (UCB1IFG&UCRXIFG)        /* Wait for TX to be ready */
  #define SPIRXFG_CLR     (UCB1IFG&=~UCRXIFG)      
  #define SPIFG_CLR       (UCB1IFG&=~(UCRXIFG|UCTXIFG))  

  //define ISR name
  #define SPI_VECTOR      USCI_B1_VECTOR
  
#elif  SPI_SER_INTF == SER_INTF_UCA2

  //port mapping values
  #define MMC_PM_SIMO     PM_UCA2SIMO
  #define MMC_PM_SOMI     PM_UCA2SOMI
  #define MMC_PM_UCLK     PM_UCA2CLK

  #define SPI_BASE          (&UCA2CTLW0)

  //define SPI registers for UCA2
  #define SPIRXBUF        UCA2RXBUF
  #define SPITXBUF        UCB1TXBUF 
  #define SPI_SEND(x)     (UCA2TXBUF=x)         
  #define SPITXREADY      (UCA2IFG&UCTXIFG)        /* Wait for TX to be ready */
  #define SPITXDONE       (!(UCA2STATW&UCBUSY))      /* Wait for TX to finish */
  #define SPIRXREADY      (UCA2IFG&UCRXIFG)        /* Wait for TX to be ready */
  #define SPIRXFG_CLR     (UCA2IFG&=~UCRXIFG)      
  #define SPIFG_CLR       (UCA2IFG&=~(UCRXIFG|UCTXIFG))    

  //define ISR name
  #define SPI_VECTOR      USCI_A2_VECTOR
  
#elif  SPI_SER_INTF == SER_INTF_UCA3

  //port mapping values
  #define MMC_PM_SIMO     PM_UCA3SIMO
  #define MMC_PM_SOMI     PM_UCA3SOMI
  #define MMC_PM_UCLK     PM_UCA3CLK
  
  #define SPI_BASE          (&UCA3CTLW0)

  //define SPI registers for UCA3
  #define SPIRXBUF        UCA3RXBUF
  #define SPITXBUF        UCA3TXBUF 
  #define SPI_SEND(x)     (UCA3TXBUF=x)         
  #define SPITXREADY      (UCA3IFG&UCTXIFG)        /* Wait for TX to be ready */
  #define SPITXDONE       (!(UCA3STATW&UCBUSY))      /* Wait for TX to finish */
  #define SPIRXREADY      (UCA3IFG&UCRXIFG)        /* Wait for TX to be ready */
  #define SPIRXFG_CLR     (UCA3IFG&=~UCRXIFG)      
  #define SPIFG_CLR       (UCA3IFG&=~(UCRXIFG|UCTXIFG))    
  
  //define ISR name
  #define SPI_VECTOR      USCI_A3_VECTOR

  
#else
  #ifndef SPI_SER_INTF
    #error SPI_SER_INTF not defined pleases pesify SPI Port to use
  #else
    #error SPI_SER_INTF not valid please spesify SPI Port to use
  #endif
#endif


//definitions for chip select pin
#define CS_LOW()    (MMC_CS_PxOUT &= ~MMC_CS)               // Card Select
//TODO: this seems like bad practice, fix
#define CS_HIGH()   while(!SPITXDONE); MMC_CS_PxOUT |= MMC_CS  // Card Deselect

#endif
