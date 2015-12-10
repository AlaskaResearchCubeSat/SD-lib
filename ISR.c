#include <msp430.h>
#include "hardware.h"

//dummy ISR just in case and to cause conflicts if 
void mmc_ISR(void) __ctl_interrupt[SPI_VECTOR]{
  int tmp;
  //dummy read to clear flag(s)
  tmp=SPI_REG_W(UCAxIV_OFFSET);
}

