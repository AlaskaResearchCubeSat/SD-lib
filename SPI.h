//SPI.h SPI functions

#ifndef _SPI_H
#define _SPI_H

#include "MMC.h"
#include <ctl.h>

#ifdef withDMA
  //DMA events from ARCbus
  #include <DMA.h>
  #include <ARCbus.h>
#endif

#define DUMMY 0xff
#define DUMMY_CHAR 0xFF

// Function Prototypes
void SPISetup (void);
void SPI_fast(void);
void SPI_slow(void);
unsigned char spiSendByte(const unsigned char data);
int spiReadFrame(unsigned char* pBuffer, unsigned int size);
int spiSendFrame(const unsigned char* pBuffer, unsigned int size);
void spiDummyClk(void);
void SPIShutdown(void);

#endif


