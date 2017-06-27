/*
 * pms3003.c
 *
 *  Created on: Jan 31, 2017
 *      Author: Tom Becnel <thomas.becnel@utah.edu>
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
#include "interrupt.h"

#define DEBUG_MODE 1

#ifdef DEBUG_MODE
#include "uart_if.h"
#include "gpio_if.h"
#endif

#if defined(USE_FREERTOS) || defined(USE_TI_RTOS)
#include "osi.h"
#endif

// Local includes
#include "pms_if.h"

#define PMS_DATA_LENGTH     24
#define RING_BUFFER_LENGTH  24
#define PM01_HIGH           4
#define PM01_LOW            5
#define PM2_5_HIGH          6
#define PM2_5_LOW           7
#define PM10_HIGH           8
#define PM10_LOW            9

// PMS error messages | TODO: errors should come from top level error enum
#define ERROR_NO_ERROR          0UL
#define ERROR_PMS_BAD_HEADER    1UL
#define ERROR_PMS_BAD_CS        2UL

// GPIO stuff
#define MCU_STAT_1_LED_GPIO 9
#define MCU_STAT_2_LED_GPIO 10
#define MCU_STAT_3_LED_GPIO 11

//*****************************************************************************
//
//! Begin Global Parameters
//
//*****************************************************************************
unsigned char g_ucpmsRingBuf[RING_BUFFER_LENGTH] = {0};     // Ring buffer to hold data from PMS output
unsigned int g_uiPM01  = 0;                 // PM data
unsigned int g_uiPM2_5 = 0;
unsigned int g_uiPM10  = 0;
//*****************************************************************************
//
//! End Global Parameters
//
//*****************************************************************************

//*****************************************************************************
//
//! Interrupt handler for UART interupt
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
static void PMSIntHandler()
{
    static unsigned char ringBufIndex = 0;
    int i;

    //
    // Clear the UART Interrupt
    //
    MAP_UARTIntClear(PMS,UART_INT_RX);

//  GPIO_IF_LedToggle(MCU_STAT_2_LED_GPIO);
    GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);
//  GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);
//  GPIO_IF_LedOn(MCU_STAT_3_LED_GPIO);

    /* Rx FIFO 7/8 */
//  for (i=0;i<8;i++)
//  {
//      g_ucpmsRingBuf[ringBufIndex] = MAP_UARTCharGetNonBlocking(PMS);
//      ringBufIndex++;
//      if (ringBufIndex == RING_BUFFER_LENGTH)
//      {
//          ringBufIndex = 0;
//      }
//  }

    /* Rx No FIFO */
    g_ucpmsRingBuf[ringBufIndex] = MAP_UARTCharGetNonBlocking(PMS);
    ringBufIndex++;
    if (ringBufIndex == RING_BUFFER_LENGTH)
    {
        ringBufIndex = 0;
    }
    GPIO_IF_LedOff(MCU_STAT_2_LED_GPIO);
}

//*****************************************************************************
//
//! Initialization
//!
//! This function
//!         1. Configures the UART1 to be used.
//!         2. Sets up UART1 Interrupt and ISR
//!         3. Configures UART1 FIFO
//!
//! \return none
//
//*****************************************************************************
void
InitPMS()
{
    //
    // Configure Core Clock for UART1 (PMS)
    MAP_UARTConfigSetExpClk(PMS,MAP_PRCMPeripheralClockGet(PMS_PERIPH),
                     UART1_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                     UART_CONFIG_PAR_NONE));


    //
    // Register interrupt handler for UART
    //
    MAP_UARTIntRegister(PMS,PMSIntHandler);
    //
    // Enable UART Rx not empty interrupt
    //
    MAP_UARTIntEnable(PMS,UART_INT_RX);
    //
    // Configure the UART Tx and Rx FIFO level to 1/8 i.e 2 characters
    //
    //UARTFIFOLevelSet(PMS,UART_FIFO_TX1_8,UART_FIFO_RX7_8);//TODO
    UARTFIFODisable(PMS);

}

//*****************************************************************************
//
//! Retrieve sample from ring buffer
//!
//! This function
//!         1. retrieves data from ring buffer
//!         2. parses data to get PM 01, PM 2.5, and PM 10 measurements
//!
//! \return ERROR_NUMBER
//
//*****************************************************************************
unsigned long
PMSSampleGet(unsigned short *pmsDataBuf)
{
    unsigned long lRetVal = 0;
    int ringBufIndex = 0;
    int pmsDataIndex = 0;

    unsigned char pmsSample[PMS_DATA_LENGTH] = {0};

    // Find the start of the PMS transmission in the buffer
    ringBufIndex = 0;
    while (g_ucpmsRingBuf[ringBufIndex] != 'B')
    {
        if (++ringBufIndex == RING_BUFFER_LENGTH)
        {
            break;
        }
    }

    // if 'B' was not found, go to bottom
    if (ringBufIndex == RING_BUFFER_LENGTH) // there was an error
    {
        lRetVal = ERROR_PMS_BAD_HEADER;
//      Message("'B' not received...\n\r");
    }

    else    // no error -- check for 'M', then get data
    {
        if (g_ucpmsRingBuf[ringBufIndex+1] != 'M')  // there was an error
        {
            lRetVal = ERROR_PMS_BAD_HEADER;
//          Message("'M' not received...\n\r");
        }

        else    // no header error -- retreive the data
        {
            // if header ('BM') matches, retrieve the sample from the ring buffer
            for(pmsDataIndex=0;pmsDataIndex<PMS_DATA_LENGTH;pmsDataIndex++)
            {
                pmsSample[pmsDataIndex] = g_ucpmsRingBuf[(pmsDataIndex+ringBufIndex)%RING_BUFFER_LENGTH];
            }

            // Checksum
            /*      NOTE: Most checksum errors occur if this function is called while ring buffer is being written to.
                          Since it happens so infrequently, we can just keep previous data and tell sender there was
                          a checksum error.
             */
            if (PMSSampleCheckSum(pmsSample))   // no checksum error
            {
                // assigned to globals in case they want to be read in other functions
                g_uiPM01  = PMSGetPM01(pmsSample);
                g_uiPM2_5 = PMSGetPM2_5(pmsSample);
                g_uiPM10  = PMSGetPM10(pmsSample);

                // and fill the caller's data buffer
                pmsDataBuf[0] = g_uiPM01;
                pmsDataBuf[1] = g_uiPM2_5;
                pmsDataBuf[2] = g_uiPM2_5;

            }

            else    // Checksum error -- use old data
            {
                // fill with old data
                pmsDataBuf[0] = g_uiPM01;
                pmsDataBuf[1] = g_uiPM2_5;
                pmsDataBuf[2] = g_uiPM2_5;

                lRetVal = ERROR_PMS_BAD_CS;

//              Message("Checksum error...\n\r");
            }
        }
    }

//  Report("Error: %lu\n\r",lRetVal);
//  Report("PM 01:  %i\n\r",pmsDataBuf[0]);
//  Report("PM 2.5: %i\n\r",pmsDataBuf[1]);
//  Report("PM 10:  %i\n\n\r",pmsDataBuf[2]);

    return lRetVal;

}

//*****************************************************************************
//
//! Do Checksum on PMS data
//!
//! Checksum is:
//!     byte 0 + ... + byte 21 == [byte 22, byte 21]
//!
//! \return No Error: 1
//!            Error: 0
//
//*****************************************************************************
unsigned char
PMSSampleCheckSum(unsigned char *buf)
{
    unsigned int check_sum = (buf[PMS_DATA_LENGTH-2]*256)+(buf[PMS_DATA_LENGTH-1]);
    unsigned int sum = 0;
    unsigned int i;

    for(i=0;i<PMS_DATA_LENGTH-2;i++){
        sum += buf[i];
    }

    if (sum == check_sum)
        return 1;
    else
        return 0;

}

//*****************************************************************************
//
//! Retrieve PM 01 ug/cm3 measurement from buffer
//!
//! \return PM measurement
//
//*****************************************************************************
unsigned short
PMSGetPM01(unsigned char *buf)
{
    return (buf[PM01_HIGH]*256)+(buf[PM01_LOW]);
}

//*****************************************************************************
//
//! Retrieve PM 2.5 ug/cm3 measurement from buffer
//!
//! \return PM measurement
//
//*****************************************************************************
unsigned short
PMSGetPM2_5(unsigned char *buf)
{
    return (buf[PM2_5_HIGH]*256)+(buf[PM2_5_LOW]);
}

//*****************************************************************************
//
//! Retrieve PM 10 ug/cm3 measurement from buffer
//!
//! \return PM measurement
//
//*****************************************************************************
unsigned short
PMSGetPM10(unsigned char *buf)
{
    return (buf[PM10_HIGH]*256)+(buf[PM10_LOW]);
}
