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
#define GPS_nRST_PIN    10

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
#define PMTK_API_SET_RTC_TIME "$PMTK335,2007,1,1,0,0,0*02\r\n"
//get RTC UTC time
#define PMTK_API_GET_RTC_TIME "$PMTK435*30\r\n"

typedef struct
{
    unsigned long time_in_seconds;
    unsigned char hour;
    unsigned char minute;
    unsigned char seconds;
}sGPSTime;

typedef struct
{
    int year;
    int month;
    int day;
}sGPSDate;

typedef struct
{
    int     latitude_degrees;
    float   latitude_minutes;
    int     longitude_degrees;
    float   longitude_minutes;
    float   altitude_meters;
}sGlobalPosition;

struct sGPGGA_data
{
    sGlobalPosition coordinates;
    unsigned char satellites;
    sGPSTime time;
    sGPSDate date;
    unsigned long ulDateTime;
    unsigned char pos_ind;
    float HDOP;
    float geoidheight;
};

extern void InitGPS(int);
extern long sendCommand(const char*);
extern void receivePacket(void);
extern long getPacket(char*,int);
extern long parse(char *);
extern void spit(void);
//extern void common_init(void);
//extern int newNMEAreceived();
//extern char *lastNMEA(void);
extern int parseHex(char c);
//extern int waitForSentence(const char *);
//extern int standby(void);
//extern int wakeup(void);
extern void DEBUGprintPacket(int,const char *);
extern void GPS_GetDateTime(char *uc_timestamp);
extern void DEBUGprintDateTime(void);
extern long GPSGetDateAndTime(char *cDate, char *cTime);
void GPS_GetGlobalCoords(float *coords);






#endif /* GPS_H_ */

