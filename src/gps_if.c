/*
 * gps_if.c
 *
 *  Created on: Jun 5, 2017
 *      Author: Tom
 */

#include "gps_if.h"
#include "utils.h"

//*****************************************************************************

// how long are max NMEA lines to parse?
#define MAXLINELENGTH 120

// we double buffer: read one line in and leave one for the main program
//volatile char line1[MAXLINELENGTH];
//volatile char line2[MAXLINELENGTH];

// our index into filling the current line
//volatile int lineidx = 0;

// pointers to the double buffers
//volatile char *currentline;
//volatile char *lastline;
//volatile int recvdflag;
//volatile int inStandbyMode;

//*****************************************************************************

//*****************************************************************************
//
//!    Initialize the GPS
//!
//! This function
//!         1. itializes UART0 at 9600 baud
//!         2. short delay while GPS warms up
//!         3. TODO: Interrupt
//!
//! \return none
//
//*****************************************************************************
void InitGPS(void){
    MAP_UARTConfigSetExpClk(GPS,MAP_PRCMPeripheralClockGet(GPS_PERIPH),
                    UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                     UART_CONFIG_PAR_NONE));
    UtilsDelay(8000000);
}

//*****************************************************************************
//
//!    Sends a NMEA Sentence to configure GPS
//!
//! This function
//!         1. sends an NMEA sentence over UART0 (Blocking)
//!         2. issues a short delay so sentence can be received
//!
//!     TODO: Make nonblocking perhaps?
//!
//! \return none
//
//*****************************************************************************
long
sendCommand(const char *packet){
    if(packet != NULL)
    {
        while(*packet!='\0')
        {
            MAP_UARTCharPut(GPS,*packet++);
        }
        UtilsDelay(8000000);
        return 0;
    }
    else
    {
        return -1;
    }

}

//*****************************************************************************
//
//!    Receives an NMEA sentence
//!
//! This function
//!     1. receives an NMEA sentence from the GPS over UART0
//!
//!     TODO: this function should be wrapped into an interrupt and ring buffer
//!
//! \return none
//
//*****************************************************************************
void receivePacket(void){
    char response[250];

    while(GPSGetChar() != '$');
    response[0] = '$';
    int i;
    for(i = 1; i < 150; i++)
    {
        response[i] = GPSGetChar();
        if(response[i]==0x0A) // <CR>
        {
            break;
        }
    }

    DEBUGprintPacket(i,response);
    parse(response);
    return;
}

//*****************************************************************************
//
//!    Calculates the NMEA Sentence Checksum
//!
//! Checksum is all bytes XOR'd together
//!
//! \return true (1): checksum matches | false (0): checksum doesn't match
//
//*****************************************************************************
int checksum(char *nmea){
    if (nmea[strlen(nmea)-5] == '*'){
        unsigned char sum = parseHex(nmea[strlen(nmea)-4]) * 16;
        sum += parseHex(nmea[strlen(nmea)-3]);

        // check checksum
        int i;
        for (i = 1; i < (strlen(nmea)-5); i++){
            sum ^= nmea[i];
        }
        return !sum;
    }
    else{
        return false;
    }
}

//*****************************************************************************
//
//!    Parse the NMEA Sentence
//!
//! This function
//!         1. Can parse $GPGGA Sentences
//!         2. CANNOT parse any other sentence
//!             Needs functionality to parse $GPRMC sentences if date is needed
//!             However, RMCGGA command is giving bad RMC data at the moment
//!                 (5/30/17)
//!
//!     TODO: $GPRMC parsing functionality
//!
//! \return lRetVal 0: no error | -1: error
//
//*****************************************************************************
long parse(char *nmea) {

    long lRetVal = -1;

    //
    // Bad checksum
    //
    if (!checksum(nmea))
    {
        printf("Bad checksum\n\r");
        return lRetVal;
    }

    char coord_buff[8];
    GPGGA_data = GPGGA_empty;

    //
    // $GPGGA
    //
    if (strstr(nmea, "$GPGGA")) {
        printf("Found $GPGGA Header...\n\r");
        // found GGA
        char *p = nmea;
        // get time
        p = strchr(p, ',')+1;
        float timef = atof(p);
        int time = timef;
        GPGGA_data.time.hour = time / 10000;
        GPGGA_data.time.minute = (time % 10000) / 100;
        GPGGA_data.time.seconds = (time % 100);

        // parse out latitude (DDmm.mmmm)
        p = strchr(p, ',')+1;

        // make sure the data exists
        if (',' != *p){
            // parse latitude degrees (DD)
            strncpy(coord_buff, p, 2);
            coord_buff[2] = '\0';
            GPGGA_data.coordinates.latitude_degrees = atoi(coord_buff);

            // parse latitude minutes (mm.mmmm)
            p += 2;
            strncpy(coord_buff,p,7);
            coord_buff[7] = '\0';
            GPGGA_data.coordinates.latitude_minutes = atof(coord_buff);
        }

        // get latitude N/S
        p = strchr(p, ',')+1;
        if (',' != *p){
            if (p[0] == 'S') GPGGA_data.coordinates.latitude_degrees *= -1;
            else if (p[0] == 'N');
            else return -1;
        }

        // parse out longitude (DDDmm.mmmm)
        p = strchr(p, ',')+1;
        if (',' != *p){
            // parse longitude (DDD)
            strncpy(coord_buff, p, 3);
            coord_buff[3] = '\0';
            GPGGA_data.coordinates.longitude_degrees = atoi(coord_buff);

            // parse longitude minutes (mm.mmmm)
            p += 3;
            strncpy(coord_buff, p, 7); // minutes
            coord_buff[7] = '\0';
            GPGGA_data.coordinates.longitude_minutes = atof(coord_buff);
        }

        // longitude E/W
        p = strchr(p, ',')+1;
        if (',' != *p){
            if (p[0] == 'W') GPGGA_data.coordinates.longitude_degrees *= -1;
            else if (p[0] == 'E');
            else return -1;
        }

        // Position Indicator
        //      0: Fix not available
        //      1: GPS fix
        //      2: Differential GPS fix
        p = strchr(p, ',')+1;
        if (',' != *p){
            GPGGA_data.pos_ind = atoi(p);
        }

        // Satellites used
        p = strchr(p, ',')+1;
        if (',' != *p){
            GPGGA_data.satellites = atoi(p);
        }

        // Horizontal dilution of precision
        p = strchr(p, ',')+1;
        if (',' != *p){
            GPGGA_data.HDOP = atof(p);
        }

        // Altitude (meters)
        p = strchr(p, ',')+1;
        if (',' != *p){
            GPGGA_data.coordinates.altitude_meters = atof(p);
        }

        // Geoidal Separation (meters)
        p = strchr(p, ',')+1;
        p = strchr(p, ',')+1;
        if (',' != *p){
            GPGGA_data.geoidheight = atof(p);
        }
        return 0;
    }

    //
    // $GPRMC
    //
    else if (strstr(nmea, "$GPRMC")) {
        return -1;
    }

    //
    // didn't find good header
    //
    else{
        return -1;
    }
}





// Initialization code used by all constructor types
//void common_init(void) {
//  recvdflag   = false;
//  paused      = false;
//  lineidx     = 0;
//  currentline = line1;
//  lastline    = line2;
//
//  hour = minute = seconds = year = month = day =
//    fixquality = satellites = 0; // uint8_t
//  lat = lon = mag = 0; // char
//  fix = false; // boolean
//  milliseconds = 0; // uint16_t
//  latitude = longitude = geoidheight = altitude =
//    speed = angle = magvariation = HDOP = 0.0; // float
//}



//int newNMEAreceived(void) {
//  return recvdflag;
//}
//
//void pause(int p) {
//  paused = p;
//}
//
//char *lastNMEA(void) {
//  recvdflag = false;
//  return (char *)lastline;
//}

// read a Hex value and return the decimal equivalent
int parseHex(char c) {
    if (c < '0')
      return 0;
    if (c <= '9')
      return c - '0';
    if (c < 'A')
       return 0;
    if (c <= 'F')
       return (c - 'A')+10;
    // if (c > 'F')
    return 0;
}

//int waitForSentence(const char *wait4me) {
//  int max = MAXWAITSENTENCE;
//  char str[20];
//
//  int i=0;
//  while (i < max) {
//    if (newNMEAreceived()) {
//      char *nmea = lastNMEA();
//      strncpy(str, nmea, 20);
//      str[19] = 0;
//      i++;
//
//      if (strstr(str, wait4me))
//    return true;
//    }
//  }
//
//  return false;
//}



//// Standby Mode Switches
//int standby(void) {
//  if (inStandbyMode) {
//    return false;  // Returns false if already in standby mode, so that you do not wake it up by sending commands to GPS
//  }
//  else {
//    inStandbyMode = true;
//    sendCommand(PMTK_STANDBY);
//    //return waitForSentence(PMTK_STANDBY_SUCCESS);  // don't seem to be fast enough to catch the message, or something else just is not working
//    return true;
//  }
//}
//
//int wakeup(void) {
//  if (inStandbyMode) {
//   inStandbyMode = false;
//    sendCommand("");  // send byte to wake it up
//    return waitForSentence(PMTK_AWAKE);
//  }
//  else {
//      return false;  // Returns false if not in standby mode, nothing to wakeup
//  }
//}

void
DEBUGprintPacket(int i, const char *packet)
{
    int j;
    for (j=0;j<i;j++)
    {
        Report("%c",packet[j]);
    }
    Message("\n\r");
}



