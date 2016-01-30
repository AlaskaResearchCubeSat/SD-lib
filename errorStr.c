#include "SDlib.h"

//return string representation of initialization errors
//this is good for mmcInit_card and mmcGoIdle functions
/*const char* SD_init_error_str(int error){
  switch(error){
    case MMC_SUCCESS:
      return "SUCCESS";
    case MMC_INIT_ERR_CHECK_PATTERN:
      return "ERROR CHECK PATTERN MISMATCH";
    case MMC_INIT_ERR_VOLTAGE:
      return "ERROR VOLTAGE RANGE MISMATCH";
    case MMC_INIT_ERR_GO_IDLE:
      return "ERROR GO IDLE COMMAND FALED";
    case MMC_INIT_ERR_TIMEOUT:
      return "ERROR TIMEOUT";
    case MMC_INIT_ERR_READ_OCR:
      return "ERROR READ OCR FAILED";
    case MMC_INIT_ERR_BLOCK_SIZE:
      return "ERROR SET BLOCK SIZE FAILED";
    default:
      return "UNKNOWN ERROR";
  }
}*/

//return string representation of SD card function errors
const char * SD_error_str(int error){
  switch(error){
    //Success
    case MMC_SUCCESS:
      return "SUCCESS";
    //software defined errors
    case MMC_TIMEOUT_ERROR:
      return "ERROR TIMEOUT";
    case MMC_DMA_TIMEOUT_ERROR:
      return "ERROR DMA TIMEOUT";
    case MMC_BUSY_TIMEOUT_ERROR:
      return "ERROR CARD BUSY TIMEOUT";
    case MMC_LOCK_TIMEOUT_ERROR:
      return "ERROR LOCK TIMEOUT";
    case MMC_INVALID_CARD_SIZE:
      return "ERROR INVALID CARD SIZE";
    case MMC_CARD_UNINIT_ERROR:
      return "ERROR CARD NOT INITIALIZED";
    case MMC_MSP_UNINIT_ERROR:
      return "ERROR PERIPHERALS NOT INITIALIZED";
    case MMC_INIT_ERR_CHECK_PATTERN:
      return "ERROR CHECK PATTERN MISMATCH";
    case MMC_INIT_ERR_VOLTAGE:
      return "ERROR VOLTAGE RANGE MISMATCH";
    case MMC_INIT_ERR_GO_IDLE:
      return "ERROR GO IDLE COMMAND FALED";
    case MMC_INIT_ERR_TIMEOUT:
      return "ERROR INITIALIZATION TIMEOUT";
    case MMC_INIT_ERR_READ_OCR:
      return "ERROR READ OCR FAILED";
    case MMC_INIT_ERR_BLOCK_SIZE:
      return "ERROR SET BLOCK SIZE FAILED";
    case MMC_CRC_FAIL_ERROR:
      return "ERROR CRC FAILED";
    case MMC_DMA_RX_TIMEOUT_ERROR:
      return "ERROR DMA RX TIMEOUT";
    case MMC_TOKEN_TIMEOUT_ERROR:
      return "ERROR TOKEN TIMEOUT";
    case MMC_IDLE_TIMEOUT_ERROR:
      return "ERROR IDLE TIMEOUT";
    case MMC_INTERNAL_ERROR:
      return "INTERNAL ERROR";
    //Other errors, Probably from SD card
    default:
      //check for errors from the card
      if(error>0){
        //first figure out which function returned the error
        switch(error&MMC_RC_MASK){
          //response of R1 type
          case MMC_R1_RESPONSE:
            switch(error&MMC_CARD_MASK){
              case MMC_R1_IDLE:
                return "IDLE";
              case MMC_R1_ERASE_RESET:
                return "ERROR ERASE RESET";
              case MMC_R1_ILLEGAL_CMD:
                return "ILLEAGL COMMAND";
              case MMC_R1_CRC_ERROR:
                return "COMMAND CRC ERROR";
              case MMC_R1_ERASE_SEQ_ERROR:
                return "ERASE SEQUENCE ERROR";
              case MMC_R1_ADDRESS_ERROR:
                return "ERROR BAD ADDRESS";
              case MMC_R1_PARAMETER_ERROR:
                return "ERROR BAD PARAMETER";
            }
          break;
          case MMC_DATA_RESPONSE:
            switch(error&MMC_CARD_MASK){
              case MMC_DAT_ACCEPTED:
                return "DATA_ACCEPTED";
              case MMC_DAT_CRC:
                return "DATA CRC ERROR";
              case MMC_DAT_WRITE_ERR:
                return "DATA WRITE ERROR";
            }
          break;
          case MMC_DATA_TOKEN_RESP:
            switch(error&MMC_CARD_MASK){
              case MMC_ERR_TOKEN_UNKNOWN:
                return "UNKNOWN CARD ERROR";
              case MMC_ERR_TOKEN_CC:
                return "CARD CONTROLLER ERROR";
              case MMC_ERR_TOKEN_ECC_FAIL:
                return "ERROR CORRECTION CODE FAILED";
              case MMC_ERR_TOKEN_OUT_OF_RANGE:
                return "ERROR OUT OF RANGE";
            }
          break;
          case MMC_IDLE_RESP:
            switch(error&MMC_CARD_MASK){
              case MMC_R1_IDLE:
                //code should never get here
                return "INTERNAL INIT ERROR";
              case MMC_R1_ERASE_RESET:
                return "GO IDLE ERROR ERASE RESET";
              case MMC_R1_ILLEGAL_CMD:
                return "GO IDLE ILLEAGL COMMAND";
              case MMC_R1_CRC_ERROR:
                return "GO IDLE COMMAND CRC ERROR";
              case MMC_R1_ERASE_SEQ_ERROR:
                return "GO IDLE ERASE SEQUENCE ERROR";
              case MMC_R1_ADDRESS_ERROR:
                return "GO IDLE ERROR BAD ADDRESS";
              case MMC_R1_PARAMETER_ERROR:
                return "GO IDLE ERROR BAD PARAMETER";
            }
          break;
        }
      }
      //no match found, unknown
      return "UNKNOWN ERROR";
  }
}
