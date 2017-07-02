/*
 * gps_if.c
 *
 *  Created on: Jun 5, 2017
 *      Author: Tom
 */

#include "gps_if.h"
#include "utils.h"
#include "uart_if.h"
#include "app_utils.h"

//*****************************************************************************

// how long are max NMEA lines to parse?
#define MAXLINELENGTH 120
#define TIME2017                3692217600u      /* 117 years + 29 leap days since 1900 */
#define YEAR2017                2017
#define HOUR_IN_DAY
#define SEC_IN_MIN              60
#define SEC_IN_HOUR             3600
#define SEC_IN_DAY              SEC_IN_HOUR*HOUR_IN_DAY

//*****************************************************************************
//
//! Local functions declaration
//
//*****************************************************************************
static void GPSIntHandler(void);
static void GPSSetTimeSeconds(void);

unsigned char g_ucGPSRingBuf[MAXLINELENGTH] = {0};
char packet[MAXLINELENGTH];
struct sGPGGA_data GPGGA_data;
static const struct sGPGGA_data GPGGA_empty;
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
//! Interrupt handler for UART interupt
//!
//! \param  None
//!
//! \return None
//!
//! TODO: How to handle multiple good transmissions in the ring buffer?
//!         Right now it will only parse the LATEST good transmission.
//!         We would need a queue of pointers to the '$' and a function
//!             that's always parsing them (and a big ring buffer)
//!
//*****************************************************************************
static void GPSIntHandler()
{
    static unsigned char ucRingBufIndex = 0;
    static char bFoundMoney = 0;
    static unsigned int ucTransStart = 0;
    long lRetVal = -1;
    char cChar;
    //
    // Clear the UART Interrupt
    //
    MAP_UARTIntClear(GPS,UART_INT_RX);

    cChar = MAP_UARTCharGetNonBlocking(GPS);
    g_ucGPSRingBuf[ucRingBufIndex] = cChar;

    // if we found the start of a transmission then mark its index
    // TODO: Do I want to add ( && !bFoundMoney) -- probably not
    if(cChar=='$')
    {
        bFoundMoney = 1;
        ucTransStart = (int)ucRingBufIndex;
    }

    // if we found the start and end of a transmission parse the message
    if(cChar==0x0A && bFoundMoney) // '\n'
    {
        bFoundMoney = 0;
        lRetVal = getPacket(packet,ucTransStart);
        if (!lRetVal) parse(packet);
    }

    ucRingBufIndex++;
    if (ucRingBufIndex == MAXLINELENGTH)
    {
        // TODO: these should be rolled into a state machine
        //    --> found '$' --> found '\r\n' --> parse -->
        ucRingBufIndex = 0;
    }


}

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
void InitGPS(int iGPS_NO_INT){
    //
    // Configure GPS clock (9600)
    //
    MAP_UARTConfigSetExpClk(GPS,MAP_PRCMPeripheralClockGet(GPS_PERIPH),
                    UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                     UART_CONFIG_PAR_NONE));

    if(!iGPS_NO_INT){
        //
        // Register interrupt handler for UART0
        //
        MAP_UARTIntRegister(GPS, GPSIntHandler);
        //
        // Enable UART0 Rx not empty interrupt
        //
        MAP_UARTIntEnable(GPS,UART_INT_RX);
    }
    //
    // Configure the UART0 Tx and Rx FIFO level to 1/8 i.e 2 characters
    //
    UARTFIFODisable(GPS);
    //
    // Turn off RESET
    //
    GPIO_IF_Set(GPS_nRST_PIN,GPIOA1_BASE,0x4,1);
    //
    // Wait for GPS to respond
    //
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
        //UtilsDelay(8000000);
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
    Report("Getting packet...\n\r");
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
}

//! bad packet: -1
// \param i: ring buffer pointer (start of transmission '$')
long getPacket(char *packet,int i)
{
    //static int i = 0;   // ring buffer pointer
    int max = 0;        // distance to travel in one search
    int j = 0;          // packet pointer
    int k;              // used in for loop
    char cChar = '$';   // character from ring buffer

    // seek start of packet
    // we start where last packet left off
    // and try MAXLINELENGTH steps
    while(g_ucGPSRingBuf[i++] != '$')
    {
        if(i==MAXLINELENGTH) i=0;
        if(++max==MAXLINELENGTH){
            return -1;
        }
    }

    // copy packet up to '*'
    while(cChar != '*')
    {
        packet[j++] = cChar;
        if (i==MAXLINELENGTH) i=0;          // loop back to start of buffer
        if (j==MAXLINELENGTH) return -1;    // filled up packet array, it's not good
        cChar = g_ucGPSRingBuf[i++];
    }

    // copy checksum
    packet[j++] = '*';
    for(k=0;k<2;k++)
    {
        packet[j++] = g_ucGPSRingBuf[i++];
        if (i==MAXLINELENGTH) i=0;
    }

    // add <CR><LF> because apparently they don't always show up...
    packet[j++] = '\r';
    packet[j++] = '\n';
    packet[j] = '\0';

    // move pointer to the end of this transmission
    i += j;
    if(i>=MAXLINELENGTH) i -= MAXLINELENGTH;

    //Report("Packet:\n\r\t%s\n\r",packet);

    //memset(g_ucGPSRingBuf,0,sizeof(g_ucGPSRingBuf));
    return 0;

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
        //Report("Bad checksum\n\r");
        return lRetVal;
    }

    char coord_buff[8];
    GPGGA_data = GPGGA_empty;

    //
    // $GPGGA:
    //     time hhmmss.sss |  lat  |      long    |pos|sats|HDOP|altit|geo-sep| cs |
    // $GPGGA,064951.000,2307.1256,N,12016.4438,E,  1,  8,  0.95,39.9,M,17.8,M,,*65
    //
    if (strstr(nmea, "$GPGGA")) {
        //Report("Found $GPGGA Header...\n\r");
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
        //Report("Found $GPRMC Header...\n\r");
        return -1;
    }

    //
    // $PMTK535 (RTC UTC Time)
    // ex) $PMTK535,2017,7,1,23,7,59*09\r\n
    //
    else if(strstr(nmea, "$PMTK535")){
        //Report("Found $PMTK535 Header...\n\r");

        char *p = nmea;
        // seek first comma
        p = strchr(p, ',')+1;

        // get year
        int year = atoi(p);
        if(year<2017||year>3017) return -1;
        GPGGA_data.date.year = year;

        // get month
        p = strchr(p, ',')+1;
        int month = atoi(p);
        if(month<0||month>12) return -1;
        GPGGA_data.date.month = month;

        // get day
        p = strchr(p, ',')+1;
        int day = atoi(p);
        if(day<0||day>31) return -1;
        GPGGA_data.date.day = day;

        // get hour
        p = strchr(p, ',')+1;
        int hour = atoi(p);
        if(hour<0||hour>23) return -1;
        GPGGA_data.time.hour = (unsigned char) hour;

        // get minute
        p = strchr(p, ',')+1;
        int minute = atoi(p);
        if(minute<0||minute>59) return -1;
        GPGGA_data.time.minute = (unsigned char) minute;

        // get second
        p = strchr(p, ',')+1;
        int sec = atoi(p);
        if(sec<0||sec>59) return -1;
        GPGGA_data.time.seconds = (unsigned char) sec;

        DEBUGprintDateTime();

        return 0;
    }

    //
    // didn't find good header
    //
    else{
        //Report("No GPS header found...\n\r");
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
void spit(){
     Report("%c",GPSGetChar());
}
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

// return the timestamp
//  YEAR_MONTH_DAY_HOUR_MINUTE_SECOND
void GPS_GetDateTime(char *uc_timestamp) {
    unsigned short us_CCLen;

    // Year
    us_CCLen = itoa(GPGGA_data.date.year, uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Month
    us_CCLen = itoa(GPGGA_data.date.month, uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Day
    us_CCLen = itoa(GPGGA_data.date.day, uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Hour
    us_CCLen = itoa(GPGGA_data.time.hour, uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Minute
    us_CCLen = itoa(GPGGA_data.time.minute, uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Second
    us_CCLen = itoa(GPGGA_data.time.seconds, uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp = '\0';

}

void DEBUGprintDateTime(){
    char cTimeStamp[20];

//    sprintf(dt,"%i/%i/%i %i:%i:%i", GPGGA_data.date.month, \
//                                    GPGGA_data.date.day,   \
//                                    GPGGA_data.date.year,  \
//                                    GPGGA_data.time.hour,  \
//                                    GPGGA_data.time.minute,\
//                                    GPGGA_data.time.seconds);

    GPS_GetDateTime(cTimeStamp);
    //Report("\r%s",cTimeStamp);
}

// the whole enchilada. Asks GPS RTC the current date/time
// and sets cDate and cTime strings
long GPSGetDateAndTime(char *cDate, char *cTime)
{
    char cTimestamp[25];

    sendCommand(PMTK_SET_NMEA_OUTPUT_OFF);
    UtilsDelay(8000000);
    sendCommand(PMTK_API_GET_RTC_TIME);
    UtilsDelay(8000000);
    GPS_GetDateTime(cTimestamp);

    // split them into date and time
    char *p = cTimestamp;
    p = strchr(p, '_')+1; // find year_
    p = strchr(p, '_')+1; // find month_
    p = strchr(p, '_')+1; // find day_

    // now pointing to '_' between day and hour
    memcpy(cDate,cTimestamp,p-cTimestamp); // copy date
    cDate[p-cTimestamp-1] = '\0';

    strcpy(cTime,p); // copy time

    return 0;

}

void GPS_GetGlobalCoords(float *coords)
{
    sendCommand(PMTK_SET_NMEA_OUTPUT_GGA);
    UtilsDelay(8000000);
    coords[0] = (float)GPGGA_data.coordinates.latitude_degrees;
    coords[1] = GPGGA_data.coordinates.latitude_minutes;
    coords[2] = (float)GPGGA_data.coordinates.longitude_degrees;
    coords[3] = GPGGA_data.coordinates.longitude_minutes;
    coords[4] = GPGGA_data.coordinates.altitude_meters;
}

void GPSSetTimeSeconds(void)
{
}



