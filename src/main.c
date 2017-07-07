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
#include "internet_if.h"
#include "timer_if.h"

// App Includes
#include "hooks.h"
#include "handlers.h"
#include "sdhost.h"
#include "ff.h"
#include "HDC1080.h"
#include "OPT3001.h"
#include "pms_if.h"
#include "gps_if.h"
#include "app_utils.h"

//#include "handlers.h"
#include "device_status.h"
#include "smartconfig.h"
#include "pinmux.h"

// HTTP Client lib
#include "http/client/httpcli.h"
#include "http/client/common.h"

#define OSI_STACK_SIZE                  2048

// Task Priorities
#define NETWORK_CONFIG_TASK_PRIORITY    8
#define DATAGATHER_TASK_PRIORITY        2
#define DATAUPLOAD_TASK_PRIORITY        5
#define SPAWN_TASK_PRIORITY             9
#define BLINK_TASK_PRIORITY             7

// Task Sleeps
#define DATA_UPLOAD_SLEEP               60*1000
#define DATA_GATHER_SLEEP               10*1000
#define NETWORK_CONFIG_SLEEP            63*1000
#define OTA_SLEEP                        1*1000

#define NUM_SAMPLES     4

// GPIO stuff
#define MCU_STAT_1_LED_GPIO 9
#define MCU_STAT_2_LED_GPIO 10
#define MCU_STAT_3_LED_GPIO 11

//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
static void RebootMCU();

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

FIL fp;
FATFS fs;
FRESULT res;
DIR dir;
UINT Size;
HTTPCli_Struct InfluxDBhttpClient;
HTTPCli_Struct OTAhttpClient;
influxDBDataPoint dataPoint;
unsigned short g_usTimerInts;

unsigned char ucHasPreviouslyConnectedToNetwork = 0;
unsigned long g_ulErrorCodes = 0;

// Data collection variables
static float g_fTemp[NUM_SAMPLES]  = {0};
static float g_fHumi[NUM_SAMPLES]  = {0};
static unsigned int g_uiPM01[NUM_SAMPLES]  = {0};                 // PM data
static unsigned int g_uiPM2_5[NUM_SAMPLES] = {0};
static unsigned int g_uiPM10[NUM_SAMPLES]  = {0};

static char g_cTimestamp[20];
static char g_cDatestampFN[20];          // file name date stamp

typedef enum {
    LED_OFF = 0,
    LED_ON,
    LED_BLINK
} eLEDStatus;

static eLEDStatus g_ucLEDStatus;

unsigned char bInitialConnectionFinished = 0;

static OtaOptServerInfo_t g_otaOptServerInfo;
static void *pvOtaApp;

// Global flags //
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
void vGetNTPTimeTask(void *pvParameters)
{
    int iSocketDesc = 0;
    long lRetVal = -1;

    while(1){
        if(SUCCESS==Wlan_IsInternetAccess()){
            SKT_CloseSocket(iSocketDesc); // close the old one just in case
            iSocketDesc = SKT_OpenUDPSocket();
            lRetVal = GetSNTPTime();
            SKT_CloseSocket(iSocketDesc);

        }
        else{
            UART_PRINT("\tWaiting on internet access...\n\r");
        }
        osi_Sleep(5000);
    }
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
    UART_PRINT("\t\t       %s       \n\r", AppName);
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\n\n\n\r");
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
static void vNetworkConfigTask(void *pvParameters) {
    long lRetVal = -1;

    UART_PRINT("Network Config Task...\n\r");
    lRetVal = ConnectToNetwork();

    if (lRetVal < 0) {
        while(1){
            UART_PRINT("\tCould not connect to network...\n\r");
            osi_Sleep(NETWORK_CONFIG_SLEEP);
        }
    }
    else if(lRetVal == SUCCESS){
        bInitialConnectionFinished = 1;
        ucHasPreviouslyConnectedToNetwork = 1;
    }

    while(1){
        osi_Sleep(NETWORK_CONFIG_SLEEP);
        UART_PRINT("Network Config Task...\n\r");
        if(Wlan_NetworkTest() != SUCCESS && ucHasPreviouslyConnectedToNetwork){
            UART_PRINT("\t[WLAN] Bad Connection... Attempting to reconnect...\n\r");
            lRetVal = ConnectToNetwork();
            if (lRetVal < 0) {
                UART_PRINT("\tCould not connect to network...\n\r");
            }
        }
    }
}

//static void vGPSTestTask(void *pvParameters)
//{
//    char cDate[20],cTime[20];
//
//    sendCommand(PMTK_SET_NMEA_OUTPUT_OFF);
//    osi_Sleep(100);
//
//    while(1){
//        osi_Sleep(1000);
//        GPSGetDateAndTime(cDate, cTime);
//        UART_PRINT("Date: %s\n\rTime: %s\033[F",cDate,cTime);
//    }
//}

//****************************************************************************
//
//!    \brief DataGather Application Task - Samples the sensors every 1 minute.
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//
//  TODO: Store directly into 'dataPoint' variable
//
//****************************************************************************
static void vDataGatherTask(void *pvParameters)
{
    long lRetVal = 0;
    int i;
    static unsigned short pm_buf[3] = {0};
    static unsigned char firstTime = 1;
    static unsigned int uiSampleCount = 0;

    float temperature = 0;
    float humidity = 0;
    float last_temp = 0;
    float last_hum = 0;
    float fGPSCoords[5] = {0};

    while (1)   // Enter the task loop
    {
        // let this one marinate for a bit
        sendCommand(PMTK_SET_NMEA_OUTPUT_GGA);

        osi_Sleep(DATA_GATHER_SLEEP);

        UART_PRINT("Data Gather Task...\n\r");

/*************************************************************************
 * PMS DATA ACQUISITION
 **************************************************************************/
        UART_PRINT("\tCollecting PMS Data...\n\r");
        lRetVal = PMSSampleGet(g_uiPM01[uiSampleCount],g_uiPM2_5[uiSampleCount],g_uiPM10[uiSampleCount]);

        if     (lRetVal==-1) g_ulErrorCodes |= ERROR_PMS_BAD_HEADER;
        else if(lRetVal==-2) g_ulErrorCodes |= ERROR_PMS_BAD_CS;

/*************************************************************************
 * TEMPERATURE & HUMIDITY DATA ACQUISITION
 **************************************************************************/
        UART_PRINT("\tCollecting I2C Data...\n\r");
        I2C_IF_Open(I2C_MASTER_MODE_STD);

        // Get temperature
        lRetVal = GetTemperature(&temperature);
        if(SUCCESS != lRetVal)
        {
            temperature = last_temp;
            g_ulErrorCodes |= ERROR_TEMP_BAD;
        }
        else {last_temp = temperature;}

        // Get humidity
        lRetVal = GetHumidity(&humidity);
        if(SUCCESS != lRetVal)
        {
            humidity = last_hum;
            g_ulErrorCodes |= ERROR_HUM_BAD;

        }
        else {last_hum = humidity;}

        I2C_IF_Close(I2C_MASTER_MODE_STD);

        g_fTemp[uiSampleCount] = temperature;
        g_fHumi[uiSampleCount] = humidity;

/*************************************************************************
 * AMBIENT LIGHT DATA ACQUISITION | TODO: get light module working
 **************************************************************************/

/*************************************************************************
 * GAS SENSOR DATA ACQUISITION | TODO: gas sensor/ADC
 **************************************************************************/

/*************************************************************************
 * GPS DATA ACQUISITION | TODO: time
 **************************************************************************/
        UART_PRINT("\tCollecting GPS Data...\n\r");
        GPS_GetGlobalCoords(fGPSCoords);

        // lat & lon are hhmm.mmm
        if (fGPSCoords[4]!=0)
        {
            dataPoint.altitude  = fGPSCoords[4];
            dataPoint.latitude  = fGPSCoords[0]*100+fGPSCoords[1];
            dataPoint.longitude = fGPSCoords[2]*100+fGPSCoords[3];
        }

/*************************************************************************
 * GET TIME | TODO: time: ntp server? GPS? both?
 * NOTE: this needs to come last, and right next to GPS so times sync up
 **************************************************************************/
        UART_PRINT("\tGetting Timestamp...\n\r");
        if (SUCCESS == Wlan_IsInternetAccess()){    // wait for internet access
            lRetVal = NTP_GetServerTime();
            if(SUCCESS==lRetVal)
            {
                NTP_GetTime(g_cTimestamp);
                NTP_GetDate(g_cDatestampFN);
            }
            else
            {
                g_ulErrorCodes |= ERROR_NTP_SERVER;
            }
        }
        else{
            // TODO: GPS when error
            sprintf(g_cTimestamp,"NoTime");
            sprintf(g_cDatestampFN,"NoDate");
            g_ulErrorCodes |= ERROR_INTERNET;
        }

        uiSampleCount = (uiSampleCount+1) % NUM_SAMPLES;

/*************************************************************************
 * WRITE ERROR CODE | TODO: write error code from all the sensors
 **************************************************************************/

/*************************************************************************
 * WRITE TO FILE
 **************************************************************************/
//
//        char* csv = (char*)malloc(250); /* Allocate 250 bytes: 'sprintf' will clean up extra.
//                                         * malloc() for heap allocation, instead of stack */
//
//        sprintf(csv, "%s,%i,%i,%i,%.2f,%.2f,%i%.3f,%i%.3f,%.2f\n\r",    cTimestamp, pm_buf[0], pm_buf[1], pm_buf[2],\
//                                                                        temperature,humidity,                           \
//                                                                        (int)fGPSCoords[0],fGPSCoords[1],               \
//                                                                        (int)fGPSCoords[2],fGPSCoords[3],               \
//                                                                        fGPSCoords[4]);
//
//
//        //sprintf(csv,"%i temp: %f\thumi: %f",sample++,temperature,humidity);
//        //UART_PRINT("\tStored Data:\n\r\t\t%s\n\r",csv);
//        UART_PRINT("\t%i | %i | %i | %.2f | %.2f | %.2f |\n\r",pm_buf[0],pm_buf[1],pm_buf[2],\
//                   dataPoint.temperature,dataPoint.humidity,dataPoint.altitude);
//
//        f_mount(&fs, "", 0);    /* mount the sD card */
//
//        /* Open or create a log file and ready to append
//         * TODO create file for each new day using GPS
//         * This will propose a problem though if date doesn't get set
//         */
////        NTP_GetDate(cDatestampFN);
//        strcat(cDatestampFN,".csv");
//        UART_PRINT("\n\rFilename: %s\n\r",cDatestampFN);
//        res = f_append(&fp, cDatestampFN);
//        UART_PRINT("Res: [%i]\n\r",res);
//        if (res == FR_OK)
//        {
//            UART_PRINT("\tWriting...\n\r");
//            if(firstTime){
//                res = f_write(&fp,header,btw_header,&Size);
//                firstTime = 0;
//            }
//
//            res = f_write(&fp,csv,strlen(csv),&Size);   /* Append a line */
//
//            res = f_close(&fp);                         /* Close the file */
//        }
//        else{
//            UART_PRINT("Couldn't open file...\n\r");
//        }
//
//        free(csv);  /* free the data pointer */
    } // end while(1)
}

//****************************************************************************
//
//! \brief DataUpload Application Task - Uploads collected data every so often.
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void vOTATask(void *pvParameters)
{
    long lRetVal = 0;

    while(1){

        osi_Sleep(OTA_SLEEP);

        if(SUCCESS == Wlan_IsInternetAccess() && 1 == OTA_getOTAUpdateFlag()){

            OTA_setOTAUpdateFlag(0);
            UART_PRINT("Connecting to HTTP Client\n\r");
            lRetVal = HTTP_InitOTAHTTPClient(&OTAhttpClient);
            if(lRetVal<0){
                UART_PRINT("Couldn't connect to OTA Client\n\r\t");
                ERR_PRINT(lRetVal);
            }

            lRetVal = HTTP_ProcessOTASession(&OTAhttpClient);

            if(lRetVal<0){
                UART_PRINT("Couldn't GET from server\n\r\t");
                ERR_PRINT(lRetVal);
            }
            else{
                UART_PRINT("File download complete, rebooting...\n\r");
                RebootMCU();
            }
        }
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
static void vDataUploadTask(void *pvParameters)
{
    long lRetval = -1;
    int i;
    static unsigned char firstTime = 0;
    const char* header = "Timestamp,PM_01,PM_2.5,PM_10,Temp,Humidity,Latitude,Longitude,Altitude,Error Code\n\r";
    int btw_header = strlen(header);

    while(1){
        osi_Sleep(DATA_UPLOAD_SLEEP);
        UART_PRINT("DataUploadTask...\n\r");
        UART_PRINT("---------------------------------------\n\r");

        // Average the samples
        dataPoint.temperature = 0;
        dataPoint.humidity    = 0;
        dataPoint.pm1         = 0;
        dataPoint.pm2_5       = 0;
        dataPoint.pm10        = 0;

        // avg
        for(i=0;i<NUM_SAMPLES;i++)
        {
            dataPoint.temperature += g_fTemp[i];
            dataPoint.humidity    += g_fHumi[i];
            dataPoint.pm1         += g_uiPM01[i];
            dataPoint.pm2_5       += g_uiPM2_5[i];
            dataPoint.pm10        += g_uiPM10[i];
        }

        // avg
        dataPoint.temperature /= NUM_SAMPLES;
        dataPoint.humidity    /= NUM_SAMPLES;
        dataPoint.pm1         /= NUM_SAMPLES;
        dataPoint.pm2_5       /= NUM_SAMPLES;
        dataPoint.pm10        /= NUM_SAMPLES;

/*************************************************************************
 * WRITE TO FILE
 **************************************************************************/

        char* csv = (char*)malloc(250); /* Allocate 250 bytes: 'sprintf' will clean up extra.
                                         * malloc() for heap allocation, instead of stack */

        sprintf(csv, "%s,%i,%i,%i,%.2f,%.2f,%.3f,%.3f,%.2f,%lu\n\r",g_cTimestamp, dataPoint.pm1, dataPoint.pm2_5, dataPoint.pm10,    \
                                                                    dataPoint.temperature,dataPoint.humidity,                        \
                                                                    dataPoint.latitude, dataPoint.longitude,dataPoint.altitude,      \
                                                                    g_ulErrorCodes);


        UART_PRINT("\t%i | %i | %i | %.2f | %.2f | %.2f |\n\r",dataPoint.pm1,dataPoint.pm2_5,dataPoint.pm10,\
                                                               dataPoint.temperature,dataPoint.humidity,dataPoint.altitude);

        f_mount(&fs, "", 0);    /* mount the sD card */

        /* Open or create a log file and ready to append
         * TODO create file for each new day using GPS
         * This will propose a problem though if date doesn't get set
         */
//        NTP_GetDate(cDatestampFN);
        strcat(g_cDatestampFN,".csv");
        UART_PRINT("\n\rFilename: %s\n\r",g_cDatestampFN);
        res = f_append(&fp, g_cDatestampFN);
        UART_PRINT("Res: [%i]\n\r",res);
        if (res == FR_OK)
        {
            UART_PRINT("\tWriting...\n\r");
            if(firstTime){
                res = f_write(&fp,header,btw_header,&Size);
                firstTime = 0;
            }

            res = f_write(&fp,csv,strlen(csv),&Size);   /* Append a line */

            res = f_close(&fp);                         /* Close the file */
        }
        else{
            UART_PRINT("Couldn't open file...\n\r");
        }

        free(csv);  /* free the data pointer */

/*************************************************************************
 * UPLOAD TO SERVER
 **************************************************************************/
        if(Wlan_IsInternetAccess() == SUCCESS){

            lRetval = HTTP_InitInfluxHTTPClient(&InfluxDBhttpClient);
            if(lRetval<0){
                UART_PRINT("Couldn't connect to HTTP Client\n\r\t");
                ERR_PRINT(lRetval);
            }
            else{
                lRetval = HTTP_POSTInfluxDBDataPoint(&InfluxDBhttpClient, dataPoint);
                if(lRetval<0){
                    UART_PRINT("Couldn't POST to server\n\r\t");
                    ERR_PRINT(lRetval);
                }
            }
            HTTPCli_disconnect(&InfluxDBhttpClient);

        }
        else{
            UART_PRINT("DataUploadTask waiting on internet access...\n\r");
        }
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

static void vBlinkTask(void *pvParameters)
{
    unsigned char LED_ON = 0;
    unsigned char LED = 0;
    while(1)
    {
        GPIO_IF_LedOn(MCU_STAT_3_LED_GPIO);
        osi_Sleep(500);
        GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);
        osi_Sleep(500);
    }
//        switch(LED){
//        case 0:
//            GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
//            GPIO_IF_LedOff(MCU_STAT_2_LED_GPIO);
//            GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);
//            break;
//        case 1:
//            GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
//            GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);
//            GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);
//            break;
//        case 2:
//            GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
//            GPIO_IF_LedOff(MCU_STAT_2_LED_GPIO);
//            GPIO_IF_LedOn(MCU_STAT_3_LED_GPIO);
//            break;
//        case 3:
//            GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
//            GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);
//            GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);
//            break;
//        }
//
//        LED = ++LED % 4;
//        osi_Sleep(500);
//    }

}
void main() {

    unsigned int puiGPIOPort;
    unsigned char pucGPIOPin;
    char packet[120] = {0};
    long lRetVal = -1;
    char line[30];
    memset(line,'-',sizeof(line));
    line[29] = '\0';

/*****************************************************************************
 * Initialization Section
 ****************************************************************************/

    // Board Initialization
    BoardInit();

    // Configure the pinmux settings for the peripherals exercised
    PinMuxConfig();

    PinConfigSet(PIN_58, PIN_STRENGTH_2MA | PIN_STRENGTH_4MA, PIN_TYPE_STD_PD);

    InitPMS();
    InitGPS(0);
    SDInit();

    InitializeAppVariables();

    DisplayBanner("AIRU Test");

    // LED Init
    GPIO_IF_LedConfigure(0x1);
    GPIO_IF_LedConfigure(0x2);
    GPIO_IF_LedConfigure(0x4);

    // Turn Off the LEDs
    GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
    GPIO_IF_LedOff(MCU_STAT_2_LED_GPIO);
    GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);
    g_ucLEDStatus = LED_OFF;
    //GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);



/*****************************************************************************
 * RTOS Task Section
 ****************************************************************************/
//
    VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);

    osi_TaskCreate(vNetworkConfigTask, (const signed char*) "NetworkConfigTask",
                   OSI_STACK_SIZE, NULL, NETWORK_CONFIG_TASK_PRIORITY, NULL);
//
    osi_TaskCreate(vOTATask, (const signed char*) "OTATask",
                   2*OSI_STACK_SIZE, NULL,NETWORK_CONFIG_TASK_PRIORITY-1, NULL);
////
////    osi_TaskCreate(vGetNTPTimeTask,(const signed char *)"GetNTPTimeTASK",
////                   OSI_STACK_SIZE,NULL,1,NULL );
////
    osi_TaskCreate(vDataGatherTask, (const signed char*)"DataGatherTask",
                   OSI_STACK_SIZE, NULL, DATAGATHER_TASK_PRIORITY, NULL);
//
    osi_TaskCreate(vDataUploadTask, (const signed char*)"DataUploadTask",
                   OSI_STACK_SIZE, NULL, DATAUPLOAD_TASK_PRIORITY, NULL);
//
    osi_TaskCreate(vBlinkTask, (const signed char*)"BlinkTask",
                       OSI_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, NULL);
    //
    // Start OS Scheduler
    //
    osi_start();
}
