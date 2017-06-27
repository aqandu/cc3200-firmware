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
#define NETWORK_CONFIG_TASK_PRIORITY     3
#define DATAGATHER_TASK_PRIORITY         2
#define DATAUPLOAD_TASK_PRIORITY         1
#define SPAWN_TASK_PRIORITY              9

// Task Sleeps
#define DATA_UPLOAD_SLEEP               15000   /* 15 seconds */
#define DATA_GATHER_SLEEP               15000   /* 15 seconds */
#define NTP_GET_TIME_SLEEP              5000    /* 5 seconds */
#define NETWORK_CONFIG_SLEEP            5000    /* 5 seconds */

#define SD_CYCLES_PER_WRITE             5       /* data gather cycles per write to SD card */

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

typedef enum {
    LED_OFF = 0,
    LED_ON,
    LED_BLINK
} eLEDStatus;

static eLEDStatus g_ucLEDStatus;

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
static void NetworkConfigTask(void *pvParameters) {
    long lRetVal = -1;

    lRetVal = Wlan_Connect();
    if (lRetVal < 0) {
        while(1){
            Message("Could not connect to network...\n\r");
        }
    }
    while(1){
        osi_Sleep(15000);
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
    long lRetVal = 0;
    static unsigned short pm_buf[3] = {0};
    static unsigned long sample = 0;
    static unsigned char firstTime = 1;
    float temperature = 0;
    float humidity = 0;
    float last_temp = 0;
    float last_hum = 0;
    unsigned long tempErrorCode = 0;

    char* header = "Sample,PM_01,PM_2.5,PM_10,Temp,Humidity\n\r";
    int btw_header = strlen(header);

    unsigned char LED_ON = 0;

    while (1)   // Enter the task loop
    {
        // reset variables
        tempErrorCode = 0; // reset the error code

        // Sleep for 15 seconds
        //osi_Sleep(DATA_GATHER_SLEEP);
        osi_Sleep(5000);

if (SUCCESS == Wlan_IsInternetAccess()){
/*************************************************************************
 * PMS DATA ACQUISITION
 **************************************************************************/
        PMSSampleGet(pm_buf);
//
//        if(tempErrorCode)       // for debugging
//        {
//            //Report("PMS return val: %lu\n\r",lRetVal);
//        }

/*************************************************************************
 * TEMPERATURE & HUMIDITY DATA ACQUISITION
 **************************************************************************/
        I2C_IF_Open(I2C_MASTER_MODE_STD);

        // Get temperature
        lRetVal = GetTemperature(&temperature);
        if(SUCCESS != lRetVal)  {temperature = last_temp;}
        else                    {last_temp = temperature;}

        // Get humidity
        lRetVal = GetHumidity(&humidity);
        if(SUCCESS != lRetVal)  {humidity = last_hum;}
        else                    {last_hum = humidity;}

        I2C_IF_Close(I2C_MASTER_MODE_STD);

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
        lRetVal = NTP_GetServerTime();

/*************************************************************************
 * WRITE ERROR CODE | TODO: write error code from all the sensors
 **************************************************************************/

/*************************************************************************
 * WRITE TO FILE
 **************************************************************************/

        char* csv = (char*)malloc(250); /* Allocate 250 bytes: 'sprintf' will clean up extra.
                                         * malloc() for heap allocation, instead of stack */
        char cTimestamp[9];
        char cDatestamp[11];
        NTP_GetTime(&cTimestamp[0]);

        sprintf(csv, "%s,%i,%i,%i,%f,%f\n\r", cTimestamp, pm_buf[0], pm_buf[1], pm_buf[2],temperature,humidity);
        //sprintf(csv,"%i temp: %f\thumi: %f",sample++,temperature,humidity);
        //Report("%s\n\r",csv);

        int btw = strlen(csv);

        f_mount(&fs, "", 0);    /* mount the sD card */

        /* Open or create a log file and ready to append
         * TODO create file for each new day using GPS
         * This will propose a problem though if date doesn't get set
         */
        NTP_GetDate(&cDatestamp[0]);
        res = f_append(&fp, cDatestamp);
        if (res == FR_OK)
        {
            if(firstTime){
                res = f_write(&fp,header,btw_header,&Size);
                firstTime = 0;
            }

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
} // end if (internet access)
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
        if(SUCCESS == Wlan_IsInternetAccess() && 1 == OTA_getOTAUpdateFlag()){

            osi_EnterCritical();
            OTA_setOTAUpdateFlag(0);
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
                osi_ExitCritical(1);
                RebootMCU();
            }
        }
        osi_Sleep(1000);
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
    long lRetVal = 0;
    while(1){
        if(Wlan_IsInternetAccess() == 0){
            UART_PRINT("DataUploadTask...\n\r");

            lRetVal = HTTP_InitInfluxHTTPClient(&InfluxDBhttpClient);
            if(lRetVal<0){
                UART_PRINT("Couldn't connect to HTTP Client\n\r\t");
                ERR_PRINT(lRetVal);
            }
            lRetVal = HTTP_POSTInfluxDBDataPoint(&InfluxDBhttpClient, dataPoint);
            if(lRetVal<0){
                UART_PRINT("Couldn't POST to server\n\r\t");
                ERR_PRINT(lRetVal);
            }

        }
        else{
            UART_PRINT("DataUploadTask waiting on internet access...\n\r");
        }
        osi_Sleep(5000);
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

/*****************************************************************************
 * Initialization Section
 ****************************************************************************/

    // Board Initialization
    BoardInit();

    // Configure the pinmux settings for the peripherals exercised
    PinMuxConfig();

    PinConfigSet(PIN_58, PIN_STRENGTH_2MA | PIN_STRENGTH_4MA, PIN_TYPE_STD_PD);

    InitPMS();

    InitializeAppVariables();

    // uSD Init
    //SDInit();

    DisplayBanner("AIRU NTP Test");

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

    VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);

    osi_TaskCreate(NetworkConfigTask, (const signed char*) "NetworkConfigTask",
                   OSI_STACK_SIZE, NULL, NETWORK_CONFIG_TASK_PRIORITY, NULL);
//
//    osi_TaskCreate(vOTATask, (const signed char*) "OTATask",
//                   OSI_STACK_SIZE, NULL,NETWORK_CONFIG_TASK_PRIORITY, NULL);
//
    osi_TaskCreate(vGetNTPTimeTask,(const signed char *)"GetNTPTimeTASK",
                   OSI_STACK_SIZE,NULL,1,NULL );
//
//    osi_TaskCreate(DataGatherTask, (const signed char*)"DataGatherTask",
//                   OSI_STACK_SIZE, NULL, DATAGATHER_TASK_PRIORITY, NULL);
//
//    osi_TaskCreate(vDataUploadTask, (const signed char*)"DataUploadTask",
//                   OSI_STACK_SIZE, NULL, DATAUPLOAD_TASK_PRIORITY, NULL);

    //
    // Start OS Scheduler
    //
    osi_start();

}
