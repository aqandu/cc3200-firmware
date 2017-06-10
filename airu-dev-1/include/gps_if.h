/*
 * gps_if.h
 *
 *  Created on: Jun 5, 2017
 *      Author: Kara
 */

#ifndef GPS_H_
#define GPS_H_

#include "pinmux.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_gpio.h"
#include "pin.h"
#include "rom.h"
#include "rom_map.h"
#include "gpio.h"
#include "prcm.h"
#include "uart.h"

#include "uart_if.h"
#include "gpio_if.h"

//Can I use these??
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

#define GPS             UARTA0_BASE
#define GPS_PERIPH      PRCM_UARTA0

//
#define ConsoleGetChar()     MAP_UARTCharGet(CONSOLE)
#define ConsolePutChar(c)    MAP_UARTCharPut(CONSOLE,c)
#define GPSGetChar()         MAP_UARTCharGet(GPS)
#define GPSPutChar(c)        MAP_UARTCharPut(GPS,c)

// how long to wait when we're looking for a response
#define MAXWAITSENTENCE 5
//Set GLL output frequency to be outputting once every 1 position fix and RMC to be outputting once every 1 position fix
#define PMTK_API_SET_NMEA_OUTPUT "$PMTK314,1,1,1,1,1,5,0,0,0,0,0,0,0,0,0,0,0,1,0*2D\r\n"
// turn on only the second sentence (GPRMC)
#define PMTK_SET_NMEA_OUTPUT_RMCONLY "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n"
// turn on GPRMC and GGA
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n"
// turn on GPGGA
#define PMTK_SET_NMEA_OUTPUT_GGA "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n"
// turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n"
// turn off output
#define PMTK_SET_NMEA_OUTPUT_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n"
// standby command & boot successful message
#define PMTK_STANDBY "$PMTK161,0*28\r\n"
#define PMTK_AWAKE "$PMTK010,002*2D\r\n"
//set RTC UTC time
#define PMTK_API_SET_RTC_TIME " $PMTK335,2007,1,1,0,0,0*02\r\n"

typedef struct
{
    unsigned long time_in_seconds;
    unsigned char hour;
    unsigned char minute;
    unsigned char seconds;
}gps_world_time;

typedef struct
{
    int     latitude_degrees;
    float   latitude_minutes;
    int     longitude_degrees;
    float   longitude_minutes;
    float   altitude_meters;
}global_position;

struct GPGGA_data_struct
{
    global_position coordinates;
    unsigned char satellites;
    gps_world_time time;
    unsigned long date;
    unsigned char pos_ind;
    float HDOP;
    float geoidheight;
};

struct GPGGA_data_struct GPGGA_data;
static const struct GPGGA_data_struct GPGGA_empty;

extern void InitGPS(void);
extern long sendCommand(const char*);
extern void receivePacket(void);
extern long parse(char *);
//extern void common_init(void);
//extern int newNMEAreceived();
//extern char *lastNMEA(void);
extern int parseHex(char c);
//extern int waitForSentence(const char *);
//extern int standby(void);
//extern int wakeup(void);
extern void DEBUGprintPacket(int,const char *);


 int hour, minute, seconds, year, month, day;
 long milliseconds;
 // Floating point latitude and longitude value in degrees.
 float latitude, longitude;
 // Fixed point latitude and longitude value with degrees stored in units of 1/100000 degrees,
 // and minutes stored in units of 1/100000 degrees.  See pull #13 for more details:
 //   https://github.com/adafruit/Adafruit-GPS-Library/pull/13
 long latitude_fixed, longitude_fixed;
 float latitudeDegrees, longitudeDegrees;
 float geoidheight, altitude;
 float speed, angle, magvariation, HDOP;
 char lat, lon, mag;
 int fix;
 int paused;
 int fixquality, satellites;



#endif /* GPS_H_ */

