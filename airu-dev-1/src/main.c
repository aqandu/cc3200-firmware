// Standard includes
#include <simplelink_if.h>
#include <stdlib.h>
#include <string.h>
#include "stdio.h"

// Simplelink includes
#include "simplelink.h"
#include "netcfg.h"

// Driverlib includes
#include "hw_ints.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "utils.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "pin.h"
#include "timer.h"

// OS includes
#include "osi.h"
#include "flc_api.h"
#include "ota_api.h"
#include "freertos.h"
#include "task.h"

// Common interface includes
#include "common.h"
#include "gpio_if.h"
#include "uart_if.h"
#include "i2c_if.h"
#include "network_if.h"
#include "timer_if.h"

// App Includes
#include "hooks.h"
#include "handlers.h"
#include "sdhost.h"
#include "ff.h"
#include "HDC1080.h"
#include "OPT3001.h"
#include "pms_if.h"

//#include "handlers.h"
#include "device_status.h"
#include "smartconfig.h"
#include "pinmux.h"

#define APPLICATION_VERSION              "1.1.0"
#define APP_NAME                         "AirU-Firmware"

#define FILE         "test.txt"

// NTP Time #defines
#define TIME2013                3565987200u      /* 113 years + 28 days(leap) */
#define YEAR2013                2013
#define TIME2017                3692217600u      /* 117 years + 29 leap days since 1900 */
#define YEAR2017                2017
#define SEC_IN_MIN              60
#define SEC_IN_HOUR             3600
#define SEC_IN_DAY              86400
#define SERVER_RESPONSE_TIMEOUT 10
#define GMT_DIFF_TIME_HRS       6
#define GMT_DIFF_TIME_MINS      0

// Network Configuration #defines
#define NETWORK_CONFIG_TASK_PRIORITY     3
#define DATAGATHER_TASK_PRIORITY		 2
#define DATAUPLOAD_TASK_PRIORITY         1
#define SPAWN_TASK_PRIORITY              9

#define OSI_STACK_SIZE                  2048

#define AP_SSID_LEN_MAX                 32
#define SH_GPIO_3                       3       /* P58 - Device Mode */
#define AUTO_CONNECTION_TIMEOUT_COUNT   50      /* 5 Sec */
#define SL_STOP_TIMEOUT                 200

// task wait #defines
#define DATA_UPLOAD_SLEEP               15000   /* 15 seconds */
#define DATA_GATHER_SLEEP               15000   /* 15 seconds */
#define SD_CYCLES_PER_WRITE             5       /* data gather cycles per write to SD card
                                                   So writes every (DATA_GATHER_SLEEP * SD_CYCLES_PER_WRITE) seconds */
#define NTP_GET_TIME_SLEEP              5000    /* 5 seconds */
#define NETWORK_CONFIG_SLEEP            5000    /* 5 seconds */

//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
void GetNTPTimeTask(void *pvParameters);
static long GetSNTPTime(unsigned char ucGmtDiffHr, unsigned char ucGmtDiffMins);
static void RebootMCU();

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

FIL fp;
FATFS fs;
FRESULT res;
DIR dir;
UINT Size;

//!    ######################### list of SNTP servers ##################################
//!    ##
//!    ##          hostname         |        IP       |       location
//!    ## -----------------------------------------------------------------------------
//!    ##   nist1-nj2.ustiming.org  | 165.193.126.229 |  Weehawken, NJ
//!    ##   nist1-pa.ustiming.org   | 206.246.122.250 |  Hatfield, PA
//!    ##   time-a.nist.gov         | 129.6.15.28     |  NIST, Gaithersburg, Maryland
//!    ##   time-b.nist.gov         | 129.6.15.29     |  NIST, Gaithersburg, Maryland
//!    ##   time-c.nist.gov         | 129.6.15.30     |  NIST, Gaithersburg, Maryland
//!    ##   ntp-nist.ldsbc.edu      | 198.60.73.8     |  LDSBC, Salt Lake City, Utah
//!    ##   nist1-macon.macon.ga.us | 98.175.203.200  |  Macon, Georgia
//!
//!    ##   For more SNTP server link visit 'http://tf.nist.gov/tf-cgi/servers.cgi'
//!    ###################################################################################
const char g_acSNTPserver[30] = "wwv.nist.gov"; //Add any one of the above servers

// Tuesday is the 1st day in 2013 - the relative year
const char g_acDaysOfWeek2013[7][3] = {{"Tue"},
                                       {"Wed"},
                                       {"Thu"},
                                       {"Fri"},
                                       {"Sat"},
                                       {"Sun"},
                                       {"Mon"}};

const char g_acMonthOfYear[12][3] = {{"Jan"},
                                     {"Feb"},
                                     {"Mar"},
                                     {"Apr"},
                                     {"May"},
                                     {"Jun"},
                                     {"Jul"},
                                     {"Aug"},
                                     {"Sep"},
                                     {"Oct"},
                                     {"Nov"},
                                     {"Dec"}};

const char g_acNumOfDaysPerMonth[12] = {31, 28, 31, 30, 31, 30,
                                        31, 31, 30, 31, 30, 31};

SlSockAddr_t sAddr;
SlSockAddrIn_t sLocalAddr;

struct
{
    unsigned long ulDestinationIP;
    int iSockID;
    unsigned long ulElapsedSec;
    short isGeneralVar;
    unsigned long ulGeneralVar;
    unsigned long ulGeneralVar1;
    char acTimeStore[30];
    char *pcCCPtr;
    unsigned short uisCCLen;
}g_sAppData;

typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    SERVER_GET_TIME_FAILED = -0x7D0,
    DNS_LOOPUP_FAILED = SERVER_GET_TIME_FAILED  -1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

unsigned short g_usTimerInts;

// END: Get Time variables

typedef enum {
	LED_OFF = 0,
	LED_ON,
	LED_BLINK
} eLEDStatus;

static eLEDStatus g_ucLEDStatus;

static OtaOptServerInfo_t g_otaOptServerInfo;
static void *pvOtaApp;

static const char pcDigits[] = "0123456789";
static unsigned char POST_token[] = "__SL_P_ULD";
static unsigned char GET_token_TEMP[] = "__SL_G_UTP";
static unsigned char GET_token_ACC[] = "__SL_G_UAC";
static unsigned char GET_token_UIC[] = "__SL_G_UIC";
static unsigned int g_uiDeviceModeConfig = ROLE_STA; //default is STA mode
static unsigned long g_ulStatus = 0; //SimpleLink Status
static unsigned char g_ucConnectionSSID[SSID_LEN_MAX + 1]; //Connection SSID
static unsigned char g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID

// Global flags
static int g_iInternetAccess = -1;  // TODO: does this ever get set back to -1 on loss of internet?

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

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
//! Network_IF_GetHostIP
//!
//! \brief  This function obtains the server IP address using a DNS lookup
//!
//! \param[in]  pcHostName        The server hostname
//! \param[out] pDestinationIP    This parameter is filled with host IP address.
//!
//! \return On success, +ve value is returned. On error, -ve value is returned
//!
//
//*****************************************************************************
long Network_IF_GetHostIP( char* pcHostName,unsigned long * pDestinationIP )
{
    long lStatus = 0;

    lStatus = sl_NetAppDnsGetHostByName((signed char *) pcHostName,
                                            strlen(pcHostName),
                                            pDestinationIP, SL_AF_INET);
    ASSERT_ON_ERROR(lStatus);

    UART_PRINT("Get Host IP succeeded.\n\rHost: %s IP: %d.%d.%d.%d \n\r\n\r",
                    pcHostName, SL_IPV4_BYTE(*pDestinationIP,3),
                    SL_IPV4_BYTE(*pDestinationIP,2),
                    SL_IPV4_BYTE(*pDestinationIP,1),
                    SL_IPV4_BYTE(*pDestinationIP,0));
    return lStatus;

}

//*****************************************************************************
//
//! Gets the current time from the selected SNTP server
//!
//! \brief  This function obtains the NTP time from the server.
//!
//! \param  GmtDiffHr is the GMT Time Zone difference in hours
//! \param  GmtDiffMins is the GMT Time Zone difference in minutes
//!
//! \return 0 : success, -ve : failure
//!
//
//*****************************************************************************
long GetSNTPTime(unsigned char ucGmtDiffHr, unsigned char ucGmtDiffMins)
{

/*
                            NTP Packet Header:


       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9  0  1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |LI | VN  |Mode |    Stratum    |     Poll      |   Precision    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          Root  Delay                           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Root  Dispersion                         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     Reference Identifier                       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Reference Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Originate Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Receive Timestamp (64)                     |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Transmit Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                 Key Identifier (optional) (32)                 |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                                                                |
      |                 Message Digest (optional) (128)                |
      |                                                                |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
    char cDataBuf[48];
    long lRetVal = 0;
    int iAddrSize;

    //
    // Send a query ? to the NTP server to get the NTP time
    //
    memset(cDataBuf, 0, sizeof(cDataBuf));
    cDataBuf[0] = '\x1b';

    sAddr.sa_family = AF_INET;

    // the source port
    sAddr.sa_data[0] = 0x00;
    sAddr.sa_data[1] = 0x7B;    // UDP port number for NTP is 123
    sAddr.sa_data[2] = (char)((g_sAppData.ulDestinationIP>>24)&0xff);
    sAddr.sa_data[3] = (char)((g_sAppData.ulDestinationIP>>16)&0xff);
    sAddr.sa_data[4] = (char)((g_sAppData.ulDestinationIP>>8)&0xff);
    sAddr.sa_data[5] = (char)(g_sAppData.ulDestinationIP&0xff);

    lRetVal = sl_SendTo(g_sAppData.iSockID,
                     cDataBuf,
                     sizeof(cDataBuf), 0,
                     &sAddr, sizeof(sAddr));
    if (lRetVal != sizeof(cDataBuf))
    {
        // could not send SNTP request
        ASSERT_ON_ERROR(SERVER_GET_TIME_FAILED);
    }

    //
    // Wait to receive the NTP time from the server
    //
    sLocalAddr.sin_family = SL_AF_INET;
    sLocalAddr.sin_port = 0;
    sLocalAddr.sin_addr.s_addr = 0;
    if(g_sAppData.ulElapsedSec == 0)
    {
        lRetVal = sl_Bind(g_sAppData.iSockID,
                (SlSockAddr_t *)&sLocalAddr,
                sizeof(SlSockAddrIn_t));
    }

    iAddrSize = sizeof(SlSockAddrIn_t);

    lRetVal = sl_RecvFrom(g_sAppData.iSockID,
                       cDataBuf, sizeof(cDataBuf), 0,
                       (SlSockAddr_t *)&sLocalAddr,
                       (SlSocklen_t*)&iAddrSize);
    ASSERT_ON_ERROR(lRetVal);

    //
    // Confirm that the MODE is 4 --> server
    //
    if ((cDataBuf[0] & 0x7) != 4)    // expect only server response
    {
         ASSERT_ON_ERROR(SERVER_GET_TIME_FAILED);  // MODE is not server, abort
    }
    else
    {
        unsigned char iIndex;

        //
        // Getting the data from the Transmit Timestamp (seconds) field
        // This is the time at which the reply departed the
        // server for the client
        //
        g_sAppData.ulElapsedSec = cDataBuf[40];
        g_sAppData.ulElapsedSec <<= 8;
        g_sAppData.ulElapsedSec += cDataBuf[41];
        g_sAppData.ulElapsedSec <<= 8;
        g_sAppData.ulElapsedSec += cDataBuf[42];
        g_sAppData.ulElapsedSec <<= 8;
        g_sAppData.ulElapsedSec += cDataBuf[43];

        //
        // seconds are relative to 0h on 1 January 1900
        //
        g_sAppData.ulElapsedSec -= TIME2017;

        //
        // in order to correct the timezone
        //
        g_sAppData.ulElapsedSec -= (ucGmtDiffHr * SEC_IN_HOUR);
        g_sAppData.ulElapsedSec += (ucGmtDiffMins * SEC_IN_MIN);

        g_sAppData.pcCCPtr = &g_sAppData.acTimeStore[0];

        //
        // day, number of days since beginning of 2017
        //
        g_sAppData.isGeneralVar = g_sAppData.ulElapsedSec/SEC_IN_DAY;
        memcpy(g_sAppData.pcCCPtr,
               g_acDaysOfWeek2013[g_sAppData.isGeneralVar%7], 3);
        g_sAppData.pcCCPtr += 3;
        *g_sAppData.pcCCPtr++ = '\x20';

        //
        // month
        //
        g_sAppData.isGeneralVar %= 365;
        for (iIndex = 0; iIndex < 12; iIndex++)
        {
            g_sAppData.isGeneralVar -= g_acNumOfDaysPerMonth[iIndex];
            if (g_sAppData.isGeneralVar < 0)
                    break;
        }
        if(iIndex == 12)
        {
            iIndex = 0;
        }
        memcpy(g_sAppData.pcCCPtr, g_acMonthOfYear[iIndex], 3);
        g_sAppData.pcCCPtr += 3;
        *g_sAppData.pcCCPtr++ = '\x20';

        //
        // date
        // restore the day in current month
        //
        g_sAppData.isGeneralVar += g_acNumOfDaysPerMonth[iIndex];
        g_sAppData.uisCCLen = itoa(g_sAppData.isGeneralVar + 1,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = '\x20';

        //
        // time
        //
        g_sAppData.ulGeneralVar = g_sAppData.ulElapsedSec%SEC_IN_DAY;

        // number of seconds per hour
        g_sAppData.ulGeneralVar1 = g_sAppData.ulGeneralVar%SEC_IN_HOUR;

        // number of hours
        g_sAppData.ulGeneralVar /= SEC_IN_HOUR;
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = ':';

        // number of minutes per hour
        g_sAppData.ulGeneralVar = g_sAppData.ulGeneralVar1/SEC_IN_MIN;

        // number of seconds per minute
        g_sAppData.ulGeneralVar1 %= SEC_IN_MIN;
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = ':';
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar1,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = '\x20';

        //
        // year
        // number of days since beginning of 2017
        //
        g_sAppData.ulGeneralVar = g_sAppData.ulElapsedSec/SEC_IN_DAY;
        g_sAppData.ulGeneralVar /= 365;
        g_sAppData.uisCCLen = itoa(YEAR2017 + g_sAppData.ulGeneralVar,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;

        *g_sAppData.pcCCPtr++ = '\0';

        UART_PRINT("response from server: ");
        UART_PRINT((char *)g_acSNTPserver);
        UART_PRINT("\n\r");
        UART_PRINT(g_sAppData.acTimeStore);
        UART_PRINT("\n\r\n\r");
    }
    return SUCCESS;
}

//*****************************************************************************
//
//! Periodic Timer Interrupt Handler
//!
//! \param None
//!
//! \return None
//
//*****************************************************************************
void
TimerPeriodicIntHandler(void)
{
    unsigned long ulInts;

    //
    // Clear all pending interrupts from the timer we are
    // currently using.
    //
    ulInts = MAP_TimerIntStatus(TIMERA0_BASE, true);
    MAP_TimerIntClear(TIMERA0_BASE, ulInts);

    //
    // Increment our interrupt counter.
    //
    g_usTimerInts++;
//    if(!(g_usTimerInts & 0x1))
//    {
//        //
//        // Off Led
//        //
//        GPIO_IF_LedOff(MCU_RED_LED_GPIO);
//    }
//    else
//    {
//        //
//        // On Led
//        //
//        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
//    }
}

//****************************************************************************
//
//! Function to configure and start timer to blink the LED while device is
//! trying to connect to an AP
//!
//! \param none
//!
//! return none
//
//****************************************************************************
void LedTimerConfigNStart()
{
    //
    // Configure Timer for blinking the LED for IP acquisition
    //
    Timer_IF_Init(PRCM_TIMERA0,TIMERA0_BASE,TIMER_CFG_PERIODIC,TIMER_A,0);
    Timer_IF_IntSetup(TIMERA0_BASE,TIMER_A,TimerPeriodicIntHandler);
    Timer_IF_Start(TIMERA0_BASE,TIMER_A,100);  // time is in mSec
}

//****************************************************************************
//
//! Disable the LED blinking Timer as Device is connected to AP
//!
//! \param none
//!
//! return none
//
//****************************************************************************
void LedTimerDeinitStop()
{
    //
    // Disable the LED blinking Timer as Device is connected to AP
    //
    Timer_IF_Stop(TIMERA0_BASE,TIMER_A);
    Timer_IF_DeInit(TIMERA0_BASE,TIMER_A);

}

//****************************************************************************
//
//! Task function implementing the gettime functionality using an NTP server
//!
//! \param none
//!
//! This function
//!    1. Initializes the required peripherals
//!    2. Initializes network driver and connects to the default AP
//!    3. Creates a UDP socket, gets the NTP server IP address using DNS
//!    4. Periodically gets the NTP time and displays the time
//!
//! \return None.
//
//****************************************************************************
void GetNTPTimeTask(void *pvParameters)
{
    int iSocketDesc;
    long lRetVal = -1;

    while(g_iInternetAccess < 0)
    {
        Message("Waiting on internet connection...\n\r");
        osi_Sleep(1000);
    }
    // wait 10 seconds before beginning test

    UART_PRINT("GET_TIME: Test Begin\n\r");

    // Commented out code to connect to network
//    //
//    // Reset The state of the machine
//    //
//    Network_IF_ResetMCUStateMachine();
//
//    //
//    // Start the driver
//    //
//    lRetVal = Network_IF_InitDriver(ROLE_STA);
//    if(lRetVal < 0)
//    {
//       UART_PRINT("Failed to start SimpleLink Device\n\r",lRetVal);
//       LOOP_FOREVER();
//    }
//
//    // switch on Green LED to indicate Simplelink is properly up
//    GPIO_IF_LedOn(MCU_ON_IND);
//
//    // Start Timer to blink Red LED till AP connection
//    LedTimerConfigNStart();
//
//    // Initialize AP security params
//    SecurityParams.Key = (signed char *)SECURITY_KEY;
//    SecurityParams.KeyLen = strlen(SECURITY_KEY);
//    SecurityParams.Type = SECURITY_TYPE;
//
//    //
//    // Connect to the Access Point
//    //
//    lRetVal = Network_IF_ConnectAP(SSID_NAME, SecurityParams);
//    if(lRetVal < 0)
//    {
//       UART_PRINT("Connection to an AP failed\n\r");
//       LOOP_FOREVER();
//    }
//
//    //
//    // Disable the LED blinking Timer as Device is connected to AP
//    //
//    LedTimerDeinitStop();
//
//    //
//    // Switch ON RED LED to indicate that Device acquired an IP
//    //
//    GPIO_IF_LedOn(MCU_IP_ALLOC_IND);

//------------------------------------------------------------------------------------
    //
    // Create UDP socket
    //
    iSocketDesc = sl_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(iSocketDesc < 0)
    {
        ERR_PRINT(iSocketDesc);
        goto end;
    }
    g_sAppData.iSockID = iSocketDesc;

    UART_PRINT("Socket created\n\r");


    //
    // Get the NTP server host IP address using the DNS lookup
    //
    lRetVal = Network_IF_GetHostIP((char*)g_acSNTPserver, \
                                    &g_sAppData.ulDestinationIP);

    if( lRetVal >= 0)
    {

        struct SlTimeval_t timeVal;
        timeVal.tv_sec =  SERVER_RESPONSE_TIMEOUT;    // Seconds
        timeVal.tv_usec = 0;     // Microseconds. 10000 microseconds resolution
        lRetVal = sl_SetSockOpt(g_sAppData.iSockID,SL_SOL_SOCKET,SL_SO_RCVTIMEO,\
                        (unsigned char*)&timeVal, sizeof(timeVal));
        if(lRetVal < 0)
        {
           ERR_PRINT(lRetVal);
           LOOP_FOREVER();
        }

        while(1)
        {
            //
            // Sit here if internet connection goes out
            //
            while(g_iInternetAccess < 0)
            {
                Message("Waiting on internet connection...\n\r");
                osi_Sleep(1000);
            }

            //
            // Get the NTP time and display the time
            //
            lRetVal = GetSNTPTime(GMT_DIFF_TIME_HRS, GMT_DIFF_TIME_MINS);
            if(lRetVal < 0)
            {
                UART_PRINT("Server Get Time failed\n\r");
                break;
            }

            //
            // Wait a while before resuming
            //
            osi_Sleep(5000);
        }
    }
    else
    {
        UART_PRINT("DNS lookup failed. \n\r");
    }

    //
    // Close the socket
    //
    close(iSocketDesc);
    UART_PRINT("Socket closed\n\r");
//------------------------------------------------------------------------------------

end:
    // Commented out Network Connectivity code
//    //
//    // Stop the driver
//    //
//    lRetVal = Network_IF_DeInitDriver();
//    if(lRetVal < 0)
//    {
//       UART_PRINT("Failed to stop SimpleLink Device\n\r");
//       LOOP_FOREVER();
//    }

    UART_PRINT("GET_TIME: Test Complete\n\r");

    //
    // Loop here
    //
    LOOP_FOREVER();
}

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void DisplayBanner(char * AppName) {
	UART_PRINT("\n\n\n\r");
	UART_PRINT("\t\t *************************************************\n\r");
	UART_PRINT("\t\t      CC3200 %s Application       \n\r", AppName);
	UART_PRINT("\t\t *************************************************\n\r");
	UART_PRINT("\n\n\n\r");
}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************

//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent) {
	if (pWlanEvent == NULL) {
		UART_PRINT("Null pointer\n\r");
		LOOP_FOREVER()
		;
	}
	switch (pWlanEvent->Event) {
	case SL_WLAN_CONNECT_EVENT: {
		SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

		//
		// Information about the connected AP (like name, MAC etc) will be
		// available in 'slWlanConnectAsyncResponse_t'
		// Applications can use it if required
		//
		//  slWlanConnectAsyncResponse_t *pEventData = NULL;
		// pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
		//

		// Copy new connection SSID and BSSID to global parameters
		memcpy(g_ucConnectionSSID,
				pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_name,
				pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
		memcpy(g_ucConnectionBSSID,
				pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
				SL_BSSID_LENGTH);

		UART_PRINT("[WLAN EVENT] Device Connected to the AP: %s , "
				"BSSID: %x:%x:%x:%x:%x:%x\n\r", g_ucConnectionSSID,
				g_ucConnectionBSSID[0], g_ucConnectionBSSID[1],
				g_ucConnectionBSSID[2], g_ucConnectionBSSID[3],
				g_ucConnectionBSSID[4], g_ucConnectionBSSID[5]);
	}
		break;

	case SL_WLAN_DISCONNECT_EVENT: {
		slWlanConnectAsyncResponse_t* pEventData = NULL;

		CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
		CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

		pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

		// If the user has initiated 'Disconnect' request,
		//'reason_code' is SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION
		if (SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION
				== pEventData->reason_code) {
			UART_PRINT("[WLAN EVENT] Device disconnected from the AP: %s, "
					"BSSID: %x:%x:%x:%x:%x:%x on application's "
					"request \n\r", g_ucConnectionSSID, g_ucConnectionBSSID[0],
					g_ucConnectionBSSID[1], g_ucConnectionBSSID[2],
					g_ucConnectionBSSID[3], g_ucConnectionBSSID[4],
					g_ucConnectionBSSID[5]);
		} else {
			UART_PRINT("[WLAN ERROR] Device disconnected from the AP AP: %s, "
					"BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
					g_ucConnectionSSID, g_ucConnectionBSSID[0],
					g_ucConnectionBSSID[1], g_ucConnectionBSSID[2],
					g_ucConnectionBSSID[3], g_ucConnectionBSSID[4],
					g_ucConnectionBSSID[5]);
		}
		memset(g_ucConnectionSSID, 0, sizeof(g_ucConnectionSSID));
		memset(g_ucConnectionBSSID, 0, sizeof(g_ucConnectionBSSID));
	}
		break;

	case SL_WLAN_STA_CONNECTED_EVENT: {
		// when device is in AP mode and any client connects to device cc3xxx
		//SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
		//CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION_FAILED);

		//
		// Information about the connected client (like SSID, MAC etc) will
		// be available in 'slPeerInfoAsyncResponse_t' - Applications
		// can use it if required
		//
		// slPeerInfoAsyncResponse_t *pEventData = NULL;
		// pEventData = &pSlWlanEvent->EventData.APModeStaConnected;
		//

		UART_PRINT("[WLAN EVENT] Station connected to device\n\r");
	}
		break;

	case SL_WLAN_STA_DISCONNECTED_EVENT: {
		// when client disconnects from device (AP)
		//CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
		//CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

		//
		// Information about the connected client (like SSID, MAC etc) will
		// be available in 'slPeerInfoAsyncResponse_t' - Applications
		// can use it if required
		//
		// slPeerInfoAsyncResponse_t *pEventData = NULL;
		// pEventData = &pSlWlanEvent->EventData.APModestaDisconnected;
		//
		UART_PRINT("[WLAN EVENT] Station disconnected from device\n\r");
	}
		break;

	case SL_WLAN_SMART_CONFIG_COMPLETE_EVENT: {
		//SET_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

		//
		// Information about the SmartConfig details (like Status, SSID,
		// Token etc) will be available in 'slSmartConfigStartAsyncResponse_t'
		// - Applications can use it if required
		//
		//  slSmartConfigStartAsyncResponse_t *pEventData = NULL;
		//  pEventData = &pSlWlanEvent->EventData.smartConfigStartResponse;
		//

	}
		break;

	case SL_WLAN_SMART_CONFIG_STOP_EVENT: {
		// SmartConfig operation finished
		//CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

		//
		// Information about the SmartConfig details (like Status, padding
		// etc) will be available in 'slSmartConfigStopAsyncResponse_t' -
		// Applications can use it if required
		//
		// slSmartConfigStopAsyncResponse_t *pEventData = NULL;
		// pEventData = &pSlWlanEvent->EventData.smartConfigStopResponse;
		//
	}
		break;

	default: {
		UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
				pWlanEvent->Event);
	}
		break;
	}
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent) {
	if (pNetAppEvent == NULL) {
		UART_PRINT("Null pointer\n\r");
		LOOP_FOREVER()
		;
	}

	switch (pNetAppEvent->Event) {
	case SL_NETAPP_IPV4_IPACQUIRED_EVENT: {
		SlIpV4AcquiredAsync_t *pEventData = NULL;

		SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

		//Ip Acquired Event Data
		pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

		UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
				"Gateway=%d.%d.%d.%d\n\r",
				SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip, 3),
				SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip, 2),
				SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip, 1),
				SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip, 0),
				SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway, 3),
				SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway, 2),
				SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway, 1),
				SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway, 0));

		UNUSED(pEventData);
	}
		break;

	case SL_NETAPP_IP_LEASED_EVENT: {
		SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

		//
		// Information about the IP-Leased details(like IP-Leased,lease-time,
		// mac etc) will be available in 'SlIpLeasedAsync_t' - Applications
		// can use it if required
		//
		// SlIpLeasedAsync_t *pEventData = NULL;
		// pEventData = &pNetAppEvent->EventData.ipLeased;
		//

	}
		break;

	case SL_NETAPP_IP_RELEASED_EVENT: {
		CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

		//
		// Information about the IP-Released details (like IP-address, mac
		// etc) will be available in 'SlIpReleasedAsync_t' - Applications
		// can use it if required
		//
		// SlIpReleasedAsync_t *pEventData = NULL;
		// pEventData = &pNetAppEvent->EventData.ipReleased;
		//
	}
		break;

	default: {
		UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
				pNetAppEvent->Event);
	}
		break;
	}
}

//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent,
		SlHttpServerResponse_t *pSlHttpServerResponse) {
	switch (pSlHttpServerEvent->Event) {
	case SL_NETAPP_HTTPGETTOKENVALUE_EVENT: {
		unsigned char *ptr;

		ptr = pSlHttpServerResponse->ResponseData.token_value.data;
		pSlHttpServerResponse->ResponseData.token_value.len = 0;
		if (memcmp(pSlHttpServerEvent->EventData.httpTokenName.data,
				GET_token_TEMP, strlen((const char *) GET_token_TEMP)) == 0) {

		}

		if (memcmp(pSlHttpServerEvent->EventData.httpTokenName.data,
				GET_token_UIC, strlen((const char *) GET_token_UIC)) == 0) {
			if (g_iInternetAccess == 0)
				strcpy(
						(char*) pSlHttpServerResponse->ResponseData.token_value.data,
						"1");
			else
				strcpy(
						(char*) pSlHttpServerResponse->ResponseData.token_value.data,
						"0");
			pSlHttpServerResponse->ResponseData.token_value.len = 1;
		}

		if (memcmp(pSlHttpServerEvent->EventData.httpTokenName.data,
				GET_token_ACC, strlen((const char *) GET_token_ACC)) == 0) {

			// read sensor
//			if (g_ucDryerRunning) {
//				strcpy(
//						(char*) pSlHttpServerResponse->ResponseData.token_value.data,
//						"Running");
//				pSlHttpServerResponse->ResponseData.token_value.len += strlen(
//						"Running");
//			} else {
//				strcpy(
//						(char*) pSlHttpServerResponse->ResponseData.token_value.data,
//						"Stopped");
//				pSlHttpServerResponse->ResponseData.token_value.len += strlen(
//						"Stopped");
//			}
		}

	}
		break;

	case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT: {
		unsigned char led;
		unsigned char *ptr =
				pSlHttpServerEvent->EventData.httpPostData.token_name.data;

		if (memcmp(ptr, POST_token, strlen((const char *) POST_token)) == 0) {
			ptr = pSlHttpServerEvent->EventData.httpPostData.token_value.data;
			if (memcmp(ptr, "LED", 3) != 0)
				break;
			ptr += 3;
			led = *ptr;
			ptr += 2;
			if (led == '1') {
				if (memcmp(ptr, "ON", 2) == 0) {
					GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
					g_ucLEDStatus = LED_ON;

				} else if (memcmp(ptr, "Blink", 5) == 0) {
					GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
					g_ucLEDStatus = LED_BLINK;
				} else {
					GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
					g_ucLEDStatus = LED_OFF;
				}
			} else if (led == '2') {
				if (memcmp(ptr, "ON", 2) == 0) {
					GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);
				} else if (memcmp(ptr, "Blink", 5) == 0) {
					GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);
					g_ucLEDStatus = 1;
				} else {
					GPIO_IF_LedOff(MCU_STAT_2_LED_GPIO);
				}
			}

		}
	}
		break;
	default:
		break;
	}
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent) {
	if (pDevEvent == NULL) {
		UART_PRINT("Null pointer\n\r");
		LOOP_FOREVER()
		;
	}

	//
	// Most of the general errors that are not FATAL are to be handled
	// appropriately by the application
	//
	UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
			pDevEvent->EventData.deviceEvent.status,
			pDevEvent->EventData.deviceEvent.sender);
}

//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock) {
	if (pSock == NULL) {
		UART_PRINT("Null pointer\n\r");
		LOOP_FOREVER()
		;
	}
	//
	// This application doesn't work w/ socket - Events are not expected
	//
	switch (pSock->Event) {
	case SL_SOCKET_TX_FAILED_EVENT:
		switch (pSock->socketAsyncEvent.SockTxFailData.status) {
		case SL_ECLOSE:
			UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
					"failed to transmit all queued packets\n\n",
					pSock->socketAsyncEvent.SockTxFailData.sd);
			break;
		default:
			UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
					"(%d) \n\n", pSock->socketAsyncEvent.SockTxFailData.sd,
					pSock->socketAsyncEvent.SockTxFailData.status);
			break;
		}
		break;

	default:
		UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n", pSock->Event);
		break;
	}
}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************

//*****************************************************************************
//
//! \brief This function initializes the application variables
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
static void InitializeAppVariables() {
	g_ulStatus = 0;
	memset(g_ucConnectionSSID, 0, sizeof(g_ucConnectionSSID));
	memset(g_ucConnectionBSSID, 0, sizeof(g_ucConnectionBSSID));
	g_iInternetAccess = -1;
	g_uiDeviceModeConfig = ROLE_STA; //default is STA mode
}

//****************************************************************************
//
//! Confgiures the mode in which the device will work
//!
//! \param iMode is the current mode of the device
//!
//!
//! \return   SlWlanMode_t
//!                        
//
//****************************************************************************
static int ConfigureMode(int iMode) {
	long lRetVal = -1;

	lRetVal = sl_WlanSetMode(iMode);
	ASSERT_ON_ERROR(lRetVal);

	// Restart Network processor
	lRetVal = sl_Stop(SL_STOP_TIMEOUT);

	// Reset status bits
	CLR_STATUS_BIT_ALL(g_ulStatus);

	return sl_Start(NULL, NULL, NULL);
}

//****************************************************************************
//
//!    \brief Sets up the device in AP or STA Mode, depending on the presence
//!           and connectivity of stored WLAN profiles and the AP jumper pin.
//!
//! On startup, if the AP jumper pin is present or if no Wireless LAN (WLAN) profiles are present,
//! the station is put into Access Point (AP) mode, allowing the user to set WLAN profiles for
//! the device.
//!
//! Otherwise one or more WLAN profiles are present on the device. In this later case, the
//! station is put into station (STA) mode and connects to the highest signal strength
//! WLAN profile present, falling back to the other WLAN profiles if LAN connectivity to the
//! higher priority profile(s) is not possible. If LAN connectivity cannot be established for
//! any of the saved profiles, then the device is put into AP mode.
//!
//! Once successfully connected to an access point in station mode, if at any point LAN
//! connectivity to the access point is lost, the device trys to connect to the other present
//! WLAN profiles based on their priority.
//!
//! \return  0 - Success
//!         -1 - Failure
//
//****************************************************************************
long ConnectToNetwork() {

	long lRetVal = -1;
	unsigned int uiConnectTimeoutCnt = 0;

	// Start Simplelink
	lRetVal = sl_Start(NULL, NULL, NULL);
	ASSERT_ON_ERROR(lRetVal);

//	lRetVal = setDeviceName();
//	ASSERT_ON_ERROR(lRetVal);

//	lRetVal = setApDomainName();
//	ASSERT_ON_ERROR(lRetVal);

//	char str[33] = getDeviceName()+ "-" + getMacAddress();
//	lRetVal = setSsidName("airu-123");
//	ASSERT_ON_ERROR(lRetVal);

//	unsigned char ssid[15] = "airu-321";
//	unsigned short ssid_len = strlen((const char *) ssid);
//	sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, ssid_len, ssid);

//	unsigned char url[32] = "myairu.edu";
//	unsigned char url_len = strlen((const char *) url);
//	lRetVal = sl_NetAppSet(SL_NET_APP_DEVICE_CONFIG_ID,
//			NETAPP_SET_GET_DEV_CONF_OPT_DOMAIN_NAME, url_len,
//			(const unsigned char *) url);

//	int e;
//	e = setSsidName();
//	UART_PRINT("%d error setting ssid name\r\n", e);
//	UART_PRINT();

	if (g_uiDeviceModeConfig == ROLE_AP) {
		UART_PRINT("Force AP Jumper is Connected.\n\r");

		if (lRetVal != ROLE_AP) {
			// Put the device into AP mode.
			lRetVal = ConfigureMode(ROLE_AP);
			ASSERT_ON_ERROR(lRetVal);
		}

		// Now the device is in AP mode, we need to wait for this event
		// before doing anything
		while (!IS_IP_ACQUIRED(g_ulStatus)) {
#ifndef SL_PLATFORM_MULTI_THREADED
			_SlNonOsMainLoopTask();
#endif
		}

		// Stop Internal HTTP Server
		lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
		ASSERT_ON_ERROR(lRetVal);

		// Start Internal HTTP Server
		lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
		ASSERT_ON_ERROR(lRetVal);


		char ssid[32];
		strcpy(ssid, getSsidName());
//		unsigned short len = 32;
//		unsigned short config_opt = WLAN_AP_OPT_SSID;
//		sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len, (unsigned char*) ssid);
		UART_PRINT("\n\r Connect to : \'%s\'\n\r\n\r", ssid);
	} else {
		if (lRetVal == ROLE_AP) {
			UART_PRINT(
					"Device is in AP Mode and Force AP Jumper is not Connected.\n\r");

			// If the device is in AP mode, we need to wait for this event
			// before doing anything
			while (!IS_IP_ACQUIRED(g_ulStatus)) {
#ifndef SL_PLATFORM_MULTI_THREADED
				_SlNonOsMainLoopTask();
#endif
			}
		}

//		Report("Registering mDNS. \r\n");
//		lRetVal = registerMdnsService();
//		ASSERT_ON_ERROR(lRetVal);

		// Switch to STA Mode
		lRetVal = ConfigureMode(ROLE_STA);
		ASSERT_ON_ERROR(lRetVal);

		UART_PRINT("Device has been put into STA Mode.\n\r");

		lRetVal = registerMdnsService();
		ASSERT_ON_ERROR(lRetVal);

		// Stop Internal HTTP Server
		lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
		ASSERT_ON_ERROR(lRetVal);

		// Start Internal HTTP Server
		lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
		ASSERT_ON_ERROR(lRetVal);

		// Waiting for the device to Auto Connect
		UART_PRINT("Trying to Auto Connect to Existing Profiles.\n\r");

		while (uiConnectTimeoutCnt < AUTO_CONNECTION_TIMEOUT_COUNT
				&& ((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))) {
			// Turn STAT 1 LED On
			GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
			osi_Sleep(50);

			// Turn STAT 1 LED Off
			GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
			osi_Sleep(50);

			uiConnectTimeoutCnt++;
		}

		// Couldn't connect Using Auto Profile
		if (uiConnectTimeoutCnt == AUTO_CONNECTION_TIMEOUT_COUNT) {
			UART_PRINT("Couldn't connect Using Auto Profile.\n\r");

			// Blink STAT1 LEd to Indicate Connection Error
			GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);

			CLR_STATUS_BIT_ALL(g_ulStatus);

			// Put the station into AP mode
			lRetVal = ConfigureMode(ROLE_AP);
			ASSERT_ON_ERROR(lRetVal);
			UART_PRINT("Device has been put into AP Mode.\n\r");

			// Waiting for the AP to acquire IP address from Internal DHCP Server
			// If the device is in AP mode, we need to wait for this event
			// before doing anything
			while (!IS_IP_ACQUIRED(g_ulStatus)) {
#ifndef SL_PLATFORM_MULTI_THREADED
				_SlNonOsMainLoopTask();
#endif
			}

			char cCount = 0;

			// Blink LED 3 times to Indicate AP Mode
			for (cCount = 0; cCount < 3; cCount++) {
				// Turn STAT1 LED On
				GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
				osi_Sleep(400);

				// Turn STAT1 LED Off
				GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
				osi_Sleep(400);
			}

			char ssid[32];
			strcpy(ssid, getSsidName());
//			unsigned short len = 32;
//			unsigned short config_opt = WLAN_AP_OPT_SSID;
//			sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len,
//					(unsigned char*) ssid);
			UART_PRINT("\n\r Connect to : \'%s\'\n\r\n\r", ssid);
		}

		// Turn STAT1 LED Off
		GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);

		g_iInternetAccess = ConnectionTest();

	}

	return SUCCESS;
}

//****************************************************************************
//
//!    \brief Read Force AP GPIO and Configure Mode - 1(Access Point Mode)
//!                                                  - 0 (Station Mode)
//!
//! \return                        None
//
//****************************************************************************
static void ReadDeviceConfiguration() {
	unsigned int uiGPIOPort;
	unsigned char pucGPIOPin;
	unsigned char ucPinValue;

	// Read GPIO
	GPIO_IF_GetPortNPin(SH_GPIO_3, &uiGPIOPort, &pucGPIOPin);
	ucPinValue = GPIO_IF_Get(SH_GPIO_3, uiGPIOPort, pucGPIOPin);

	// If Connected to VCC, Mode is AP
	if (ucPinValue == 1) {
		// AP Mode
		g_uiDeviceModeConfig = ROLE_AP;

	} else {
		// STA Mode
		g_uiDeviceModeConfig = ROLE_STA;

	}

}

//****************************************************************************
//
//!    \brief HTTP Server Application Main Task - Initializes SimpleLink Driver and
//!                                              Handles HTTP Requests
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void NetworkConfigTask(void *pvParameters) {
	long lRetVal = -1;

	ReadDeviceConfiguration();

	lRetVal = ConnectToNetwork();
	if (lRetVal < 0) {
		while(1){
		    Message("Could not connect to network...\n\r");
		    osi_Sleep(1000);
		}
	}

	while(1)
	{
	    Message("NetworkConfigTask finished...\n\r");
	    osi_Sleep(NETWORK_CONFIG_SLEEP);
	}

}

//****************************************************************************
//
//!    \brief DataGather Application Task - Samples the sensors every 1 minute.
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//  TODO: Currently writing single data measurement every ? seconds to SD
//          Want multiple data samples to get uploaded instead
//
//****************************************************************************
static void DataGatherTask(void *pvParameters)
{
    unsigned long lRetVal = 0;
    static unsigned short pm_buf[3] = {0};
    static unsigned long sample = 0;
    float temperature;
    float humidity;
    float light;
    unsigned long tempErrorCode = 0;

    unsigned char LED_ON = 0;

    while (1)   // Enter the task loop
    {
        // reset variables
        tempErrorCode = 0; // reset the error code

        // Sleep for 15 seconds
        //osi_Sleep(DATA_GATHER_SLEEP);
        osi_Sleep(1000);

/*************************************************************************
 * PMS DATA ACQUISITION
 **************************************************************************/
        tempErrorCode |= PMSSampleGet(pm_buf);

        if(tempErrorCode)       // for debugging
        {
            //Report("PMS return val: %lu\n\r",lRetVal);
        }

/*************************************************************************
 * TEMPERATURE & HUMIDITY DATA ACQUISITION
 **************************************************************************/
        GPIO_IF_LedOn(MCU_STAT_3_LED_GPIO);
        I2C_IF_Open(I2C_MASTER_MODE_FST);
        temperature = GetTemperature();
        humidity = GetHumidity();
        I2C_IF_Close(I2C_MASTER_MODE_FST);
        GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);

/*************************************************************************
 * AMBIENT LIGHT DATA ACQUISITION | TODO: get light module working
 **************************************************************************/

/*************************************************************************
 * GAS SENSOR DATA ACQUISITION | TODO: gas sensor/ADC
 **************************************************************************/

/*************************************************************************
 * GPS DATA ACQUISITION | TODO: GPS lat, lon, alt, time
 **************************************************************************/

/*************************************************************************
 * GET TIME | TODO: time: ntp server? GPS? both?
 * NOTE: this needs to come last, and right next to GPS so times sync up
 **************************************************************************/

/*************************************************************************
 * WRITE ERROR CODE | TODO: write error code from all the sensors
 **************************************************************************/

/*************************************************************************
 * WRITE TO FILE
 **************************************************************************/
        char* csv = (char*)malloc(250); /* Allocate 250 bytes: 'sprintf' will clean up extra.
                                         * malloc() for heap allocation, instead of stack */

        sprintf(csv, "%i,%i,%i,%i,%f,%f,%lu\n\r", sample++, pm_buf[0], pm_buf[1], pm_buf[2],temperature,humidity,tempErrorCode);

        //Report("%s\n\r",csv);

        int btw = strlen(csv);

        f_mount(&fs, "", 0);    /* mount the sD card */

        /* Open or create a log file and ready to append
         * TODO create file for each new day using GPS
         * This will propose a problem though if date doesn't get set
         */
        res = f_append(&fp, "logfile.txt");
        if (res == FR_OK)
        {
            res = f_write(&fp,csv,btw,&Size);   /* Append a line */
            res = f_close(&fp);                 /* Close the file */

            if(LED_ON){
                LED_ON = 0;
                GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
            }
            else{
                LED_ON = 1;
                GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
            }
        }

        free(csv);  /* free the data pointer */



    }
}

//****************************************************************************
//
//! \brief DataUpload Application Task - Uploads collected data every 90 minutes.
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void DataUploadTask(void *pvParameters)
{

	int i = 0;
	while (1)
	{
		osi_Sleep(DATA_UPLOAD_SLEEP);

		// Write over UART
		UART_PRINT("Upload Task %d\r\n", i++);
	}
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void BoardInit(void) {

	// Set vector table base
#if defined(ccs) || defined(gcc)
	MAP_IntVTableBaseSet((unsigned long) &g_pfnVectors[0]);
#endif
#if defined(ewarm)
	MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif

	// Enable Processor
	MAP_IntMasterEnable();
	MAP_IntEnable(FAULT_SYSTICK);

	PRCMCC3200MCUInit();
}

static void SDInit() {
	//
	// Set the SD card clock as output pin
	//
	MAP_PinDirModeSet(PIN_07, PIN_DIR_MODE_OUT);

	//
	// Enable Pull up on data
	//
	MAP_PinConfigSet(PIN_06, PIN_STRENGTH_4MA, PIN_TYPE_STD_PU);

	//
	// Enable Pull up on CMD
	//
	MAP_PinConfigSet(PIN_08, PIN_STRENGTH_4MA, PIN_TYPE_STD_PU);

	//
	// Enable MMCHS
	//
	MAP_PRCMPeripheralClkEnable(PRCM_SDHOST, PRCM_RUN_MODE_CLK);

	//
	// Reset MMCHS
	//
	MAP_PRCMPeripheralReset(PRCM_SDHOST);

	//
	// Configure MMCHS
	//
	MAP_SDHostInit(SDHOST_BASE);

	//
	// Configure card clock
	//
	MAP_SDHostSetExpClk(SDHOST_BASE, MAP_PRCMPeripheralClockGet(PRCM_SDHOST),
			15000000);

}

//****************************************************************************
//
//! Reboot the MCU by requesting hibernate for a short duration
//!
//! \return None
//
//****************************************************************************
static void RebootMCU() {

	//
	// Configure hibernate RTC wakeup
	//
	PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);

	//
	// Delay loop
	//
	MAP_UtilsDelay(8000000);

	//
	// Set wake up time
	//
	PRCMHibernateIntervalSet(330);

	//
	// Request hibernate
	//
	PRCMHibernateEnter();

	//
	// Control should never reach here
	//
	while (1);
}

static void BlinkTask(void *pvParameters)
{
    unsigned char LED_ON = 0;
    unsigned char LED = 0;
    while(1)
    {
        switch(LED){
        case 0:
            GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
            GPIO_IF_LedOff(MCU_STAT_2_LED_GPIO);
            GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);
            break;
        case 1:
            GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
            GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);
            GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);
            break;
        case 2:
            GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
            GPIO_IF_LedOff(MCU_STAT_2_LED_GPIO);
            GPIO_IF_LedOn(MCU_STAT_3_LED_GPIO);
            break;
        case 3:
            GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
            GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);
            GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);
            break;
        }

        LED = ++LED % 4;
        osi_Sleep(500);
    }
//    while(1)
//    {
//        if(LED_ON){
//            LED_ON = 0;
//            GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
//            GPIO_IF_LedOff(MCU_STAT_2_LED_GPIO);
//            GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);
//        }
//        else{
//            LED_ON = 1;
//            GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
//            GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);
//            GPIO_IF_LedOn(MCU_STAT_3_LED_GPIO);
//        }
//        osi_Sleep(1000);
//    }
}
void main() {

	long lRetVal = -1;

	// Board Initialization
	BoardInit();

	// Configure the pinmux settings for the peripherals exercised
	PinMuxConfig();

	PinConfigSet(PIN_58, PIN_STRENGTH_2MA | PIN_STRENGTH_4MA, PIN_TYPE_STD_PD);

	// Initialize Global Variables
	InitializeAppVariables();

	InitPMS();

	// uSD Init
	SDInit();

	DisplayBanner("airu");

	// LED Init
	GPIO_IF_LedConfigure(LED1);
	GPIO_IF_LedConfigure(LED2);
	GPIO_IF_LedConfigure(LED3);

	// Turn Off the LEDs
	GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
	GPIO_IF_LedOff(MCU_STAT_2_LED_GPIO);
	GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);

	g_ucLEDStatus = LED_OFF;
	//GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);

	// Simplelink Spawn Task
//	lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
//	if (lRetVal < 0) {
//		ERR_PRINT(lRetVal);
//		LOOP_FOREVER()
//		;
//	}

	//
	// Network Configuration Task
	//
//	osi_TaskCreate(NetworkConfigTask, (signed char*) "NetworkConfigTask",
//	               OSI_STACK_SIZE, NULL,
//	               NETWORK_CONFIG_TASK_PRIORITY, NULL);

    //
    // Start the GetNTPTime task
    //
//    lRetVal = osi_TaskCreate(GetNTPTimeTask,
//                    (const signed char *)"Get NTP Time",
//                    OSI_STACK_SIZE,
//                    NULL,
//                    1,
//                    NULL );
//
//    if(lRetVal < 0)
//    {
//        ERR_PRINT(lRetVal);
//        LOOP_FOREVER();
//    }

	//
	// Create the DataGather Task
	//
    lRetVal = osi_TaskCreate(DataGatherTask, (signed char*)"DataGatherTask",
    		OSI_STACK_SIZE, NULL, DATAGATHER_TASK_PRIORITY, NULL);
        if(lRetVal < 0)
        {
            ERR_PRINT(lRetVal);
            LOOP_FOREVER();
        }

	//
	// Create the DataUpload Task
	//
//    osi_TaskCreate(DataUploadTask, (signed char*)"DataUploadTask",
//        						 OSI_STACK_SIZE, NULL, DATAUPLOAD_TASK_PRIORITY, NULL);


//    lRetVal = osi_TaskCreate(BlinkTask, (signed char*)"BlinkTask",
//                   OSI_STACK_SIZE, NULL, 4, NULL);
//        if(lRetVal < 0)
//        {
//            ERR_PRINT(lRetVal);
//            LOOP_FOREVER();
//        }

	//
	// Start OS Scheduler
	//
	osi_start();

	//
	// Should never reach here
	//
	while (1){
	}

}
