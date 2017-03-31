/*
 * pms3003.c
 *
 *  Created on: Jan 31, 2017
 *      Author: Tom
 */

// Standard includes
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "pin.h"
#include "uart.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"

#if defined(USE_FREERTOS) || defined(USE_TI_RTOS)
#include "osi.h"
#endif

#include "PMS3003.h"

#define UartGetChar()       MAP_UARTCharGet(PMS)
#define BUFFER_LENGTH       24
#define PM01_HIGH           4
#define PM01_LOW            5
#define PM2_5_HIGH          6
#define PM2_5_LOW           7
#define PM10_HIGH           8
#define PM10_LOW            9

//*****************************************************************************
//
//! Initialization
//!
//! This function
//!        1. Configures the UART to be used.
//!
//! \return none
//
//*****************************************************************************
void
InitPMS()
{
  MAP_UARTConfigSetExpClk(PMS,MAP_PRCMPeripheralClockGet(PMS_PERIPH),
                    UART1_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                     UART_CONFIG_PAR_NONE));
}

void
FillBuffer(unsigned char *buf)
{

    int i;

    // Wait for start of new transmission
   while (UartGetChar() != 'B');

    // Insert first character artificially
    buf[0] ='B';

    // Get the rest of the characters
    for(i=1;i<BUFFER_LENGTH;i++){
        buf[i] = UartGetChar();
    }
}

unsigned int
CheckSum(unsigned char *buf)
{
    unsigned int check_sum = (buf[BUFFER_LENGTH-2]<<8)|(buf[BUFFER_LENGTH-1]);
    unsigned int sum = 0;
    int i;

    for(i=0;i<BUFFER_LENGTH-2;i++){
        sum += buf[i];
    }

    if (sum==check_sum)
        return 1;
    else
        return 0;

}

unsigned int
GetPM01(unsigned char *buf)
{
    unsigned int PM = 0x0000;
    PM = (buf[PM01_HIGH]<<8)|(buf[PM01_LOW]);
    return PM;
}

unsigned int
GetPM2_5(unsigned char *buf)
{
    unsigned int PM = 0x0000;
    PM = (buf[PM2_5_HIGH]<<8)|(buf[PM2_5_LOW]);
    return PM;
}

unsigned int
GetPM10(unsigned char *buf)
{
    unsigned int PM = 0x0000;
    PM = (buf[PM10_HIGH]<<8)|(buf[PM10_LOW]);
    return PM;
}
