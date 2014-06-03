#ifndef __HARDWARE_H
#define __HARDWARE_H

#include <msp430.h>  

//define identifiers for serial periferals
#define SER_INTF_UCA0  3
#define SER_INTF_UCA1  4
#define SER_INTF_UCB0  5
#define SER_INTF_UCB1  6 

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

 
#if SPI_SER_INTF ==  SER_INTF_UCA1                     
                                    
// SPI port definitions   UCA1    
#define MMC_PxSEL1        P3SEL      
#define MMC_PxOUT1        P3OUT
#define MMC_PxREN1        P3REN
#define MMC_PxDIR1        P3DIR
  
#define MMC_PxSEL0        P5SEL    
#define MMC_PxOUT0        P5OUT
#define MMC_PxREN0        P5REN
#define MMC_PxDIR0        P5DIR
  
#define MMC_SIMO          BIT6       //P3.6
#define MMC_SOMI          BIT7       //P3.7
#define MMC_UCLK          BIT0       //P5.0

// Chip Select
#define MMC_CS_PxOUT      P3OUT
#define MMC_CS_PxDIR      P3DIR
#define MMC_CS            BIT3      //use P3.3 for CS


//define SPI registers for UCA1
 #define SPIRXBUF        UCA1RXBUF
 #define SPITXBUF        UCA1TXBUF
 #define SPI_SEND(x)     (UCA1TXBUF=x)
 #define SPITXREADY      (UC1IFG&UCA1TXIFG)               /* Wait for TX to be ready */
 #define SPITXDONE       (!(UCA1STAT&UCBUSY))            /* Wait for TX to finish */
 #define SPIRXREADY      (UC1IFG&UCA1RXIFG)              /* Wait for TX to be ready */
 #define SPIRXFG_CLR     (UC1IFG &= ~UCA1RXIFG)
 #define SPIFG_CLR       (UC1IFG&=~(UCA1RXIFG|UCA1TXIFG))
 #define SPI_PxIN        SPI_USART1_PxIN
 #define SPI_SOMI        SPI_USART1_SOMI

#elif  SPI_SER_INTF == SER_INTF_UCB1

  // SPI port definitions   UCB1     
  #define MMC_PxSEL1        P5SEL    
  #define MMC_PxOUT1        P5OUT
  #define MMC_PxREN1        P5REN
  #define MMC_PxDIR1        P5DIR
  //two seperate ports needed because UCA1 is split across two ports
  //becasue UCB1 is all on one port the above deffinitions are repeated here
  #define MMC_PxSEL0        P5SEL    
  #define MMC_PxOUT0        P5OUT
  #define MMC_PxREN0        P5REN
  #define MMC_PxDIR0        P5DIR

  #define MMC_SIMO          BIT1  //P5.1
  #define MMC_SOMI          BIT2  //P5.2
  #define MMC_UCLK          BIT3  //P5.3

  // Chip Select
  #define MMC_CS_PxOUT      P3OUT
  #define MMC_CS_PxDIR      P3DIR
  #define MMC_CS            BIT3  //use P3.3 for CS

  //define SPI registers for UCA1
  #define SPIRXBUF        UCB1RXBUF
  #define SPITXBUF        UCB1TXBUF 
  #define SPI_SEND(x)     (UCB1TXBUF=x)         
  #define SPITXREADY      (UC1IFG&UCB1TXIFG)        /* Wait for TX to be ready */
  #define SPITXDONE       (!(UCB1STAT&UCBUSY))      /* Wait for TX to finish */
  #define SPIRXREADY      (UC1IFG&UCB1RXIFG)        /* Wait for TX to be ready */
  #define SPIRXFG_CLR     (UC1IFG&=~UCB1RXIFG)      
  #define SPIFG_CLR       (UC1IFG&=~(UCB1RXIFG|UCB1TXIFG))
  #define SPI_PxIN        SPI_USART1_PxIN  
  #define SPI_SOMI        SPI_USART1_SOMI      
  
  #ifdef withDMA
    #error there is no DMA channel for UCB1 so withDMA can not be used
  #endif
  
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
