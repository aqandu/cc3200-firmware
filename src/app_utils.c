/*
 * app_utils.c
 *
 *  Created on: Jun 22, 2017
 *      Author: Tom
 */

#include "app_utils.h"

const char     pcDigits[] = "0123456789"; /* variable used by itoa function */

//*****************************************************************************
//
//! itoa
//!
//!    @brief  Convert integer to ASCII in decimal base
//!
//!     @param  cNum is input integer number to convert
//!     @param  cString is output string
//!
//!     @return number of ASCII parameters
//!
//!
//
//*****************************************************************************
unsigned short itoa(short cNum, char *cString)
{
    char* ptr;
    short uTemp = cNum;
    unsigned short length;

    // single digit: append zero
    if (cNum < 10)
    {
        length = 2;
        *cString++ = '0';
        *cString = pcDigits[cNum % 10];
        return length;
    }


    // Find out the length of the number, in decimal base
    length = 0;
    while (uTemp > 0)
    {
        uTemp /= 10;
        length++;
    }

    // Do the actual formatting, right to left
    ptr = cString + length;

    uTemp = cNum;

    while (uTemp > 0)
    {
        --ptr;
        *ptr = pcDigits[uTemp % 10];
        uTemp /= 10;
    }

    return length;
}

//*****************************************************************************
//
//! stripChar
//!
//!    \brief   Removes all occurances of a character from a string
//!
//!     \param  cStringDst is the destination string to write to
//!     \param  cStringSrc is the source string to pull from
//!
//!     \return N/A
//!
//*****************************************************************************
void stripChar(char *cStringDst, char *cStringSrc, char c){
    unsigned char *pr = cStringSrc;
    unsigned char *pw = cStringDst;

    while(*pr){
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}
