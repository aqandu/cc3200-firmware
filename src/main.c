// Standard includes
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


// App Includes
#include "hooks.h"
#include "handlers.h"
#include "sdhost.h"
#include "ff.h"
#include "HDC1080.h"
#include "OPT3001.h"
#include "PMS3003.h"


//#include "handlers.h"
#include "device_status.h"
#include "smartconfig.h"
#include "pinmux.h"

#define APPLICATION_VERSION              "1.1.0"
#define APP_NAME                         "AirU-Firmware"

#define FILE         "test.txt"
FIL fp;
FATFS fs;
FRESULT res;
DIR dir;
UINT Size;

#define OTA_TASK_PRIORITY                3
#define DATAGATHER_TASK_PRIORITY		 2
#define DATAUPLOAD_TASK_PRIORITY         1
#define SPAWN_TASK_PRIORITY              9

#define OSI_STACK_SIZE                  2048

#define AP_SSID_LEN_MAX                 32
#define SH_GPIO_3                       3       /* P58 - Device Mode */
#define AUTO_CONNECTION_TIMEOUT_COUNT   50      /* 5 Sec */
#define SL_STOP_TIMEOUT                 200

typedef enum
{
  LED_OFF = 0,
  LED_ON,
  LED_BLINK
} eLEDStatus;

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

#define OTA_SERVER_NAME                 "api.dropbox.com"
#define OTA_SERVER_IP_ADDRESS           0x00000000
#define OTA_SERVER_SECURED              1
#define OTA_SERVER_REST_UPDATE_CHK      "/2/files/get_metadata/" // returns files/folder list
#define OTA_SERVER_REST_RSRC_METADATA   "/2/files/get_temporary_link/"     // returns A url that serves the media directly
#define OTA_SERVER_REST_HDR             "Authorization: Bearer "
#define OTA_SERVER_REST_HDR_VAL         "NtJxfXoFAzAAAAAAAAAACYbDIBby7z7Nuh-N-0pG9l-z2hFkjygHDPCV7TgaNgJ1"
#define LOG_SERVER_NAME                 "api-content.dropbox.com"
#define OTA_SERVER_REST_FILES_PUT       "/2/files/upload"
#define OTA_VENDOR_STRING               "Vid01_Pid00_Ver00"

int OTAServerInfoSet(void **pvOtaApp, char *vendorStr);
static void RebootMCU();

static OtaOptServerInfo_t g_otaOptServerInfo;
static void *pvOtaApp;

static const char pcDigits[] = "0123456789";
static unsigned char POST_token[] = "__SL_P_ULD";
static unsigned char GET_token_TEMP[]  = "__SL_G_UTP";
static unsigned char GET_token_ACC[]  = "__SL_G_UAC";
static unsigned char GET_token_UIC[]  = "__SL_G_UIC";
static int g_iInternetAccess = -1;
static unsigned char g_ucDryerRunning = 0;
static unsigned int g_uiDeviceModeConfig = ROLE_STA; //default is STA mode
static unsigned char g_ucLEDStatus = LED_OFF;
static unsigned long  g_ulStatus = 0;//SimpleLink Status
static unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
static unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID


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
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{
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
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    if(pWlanEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
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
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] Device Connected to the AP: %s , "
                       "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                      g_ucConnectionSSID,g_ucConnectionBSSID[0],
                      g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                      g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                      g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION
            if(SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT] Device disconnected from the AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on application's "
                           "request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else
            {
                UART_PRINT("[WLAN ERROR] Device disconnected from the AP AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        case SL_WLAN_STA_CONNECTED_EVENT:
        {
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

        case SL_WLAN_STA_DISCONNECTED_EVENT:
        {
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

        case SL_WLAN_SMART_CONFIG_COMPLETE_EVENT:
        {
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

        case SL_WLAN_SMART_CONFIG_STOP_EVENT:
        {
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

        default:
        {
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
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(pNetAppEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }

    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
                       "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));

            UNUSED(pEventData);
        }
        break;

        case SL_NETAPP_IP_LEASED_EVENT:
        {
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

        case SL_NETAPP_IP_RELEASED_EVENT:
        {
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

        default:
        {
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
                                SlHttpServerResponse_t *pSlHttpServerResponse)
{
    switch (pSlHttpServerEvent->Event)
    {
        case SL_NETAPP_HTTPGETTOKENVALUE_EVENT:
        {
            unsigned char *ptr;

            ptr = pSlHttpServerResponse->ResponseData.token_value.data;
            pSlHttpServerResponse->ResponseData.token_value.len = 0;
            if(memcmp(pSlHttpServerEvent->EventData.httpTokenName.data,
                    GET_token_TEMP, strlen((const char *)GET_token_TEMP)) == 0)
            {
//                float fCurrentTemp;
//                TMP006DrvGetTemp(&fCurrentTemp);
//                char cTemp = (char)fCurrentTemp;
//                short sTempLen = itoa(cTemp,(char*)ptr);
//                ptr[sTempLen++] = ' ';
//                ptr[sTempLen] = 'F';
//                pSlHttpServerResponse->ResponseData.token_value.len += sTempLen;

            }

            if(memcmp(pSlHttpServerEvent->EventData.httpTokenName.data,
                      GET_token_UIC, strlen((const char *)GET_token_UIC)) == 0)
            {
                if(g_iInternetAccess==0)
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"1");
                else
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"0");
                pSlHttpServerResponse->ResponseData.token_value.len =  1;
            }

            if(memcmp(pSlHttpServerEvent->EventData.httpTokenName.data,
                       GET_token_ACC, strlen((const char *)GET_token_ACC)) == 0)
            {

                // read sensor
                if(g_ucDryerRunning)
                {
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"Running");
                    pSlHttpServerResponse->ResponseData.token_value.len += strlen("Running");
                }
                else
                {
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"Stopped");
                    pSlHttpServerResponse->ResponseData.token_value.len += strlen("Stopped");
                }
            }



        }
            break;

        case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT:
        {
            unsigned char led;
            unsigned char *ptr = pSlHttpServerEvent->EventData.httpPostData.token_name.data;

            g_ucLEDStatus = 0;
            if(memcmp(ptr, POST_token, strlen((const char *)POST_token)) == 0)
            {
                ptr = pSlHttpServerEvent->EventData.httpPostData.token_value.data;
                if(memcmp(ptr, "LED", 3) != 0)
                    break;
                ptr += 3;
                led = *ptr;
                ptr += 2;
                if(led == '1')
                {
                    if(memcmp(ptr, "ON", 2) == 0)
                    {
                        GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
                                                g_ucLEDStatus = LED_ON;

                    }
                    else if(memcmp(ptr, "Blink", 5) == 0)
                    {
                        GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
                        g_ucLEDStatus = LED_BLINK;
                    }
                    else
                    {
                        GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
                        g_ucLEDStatus = LED_OFF;
                    }
                }
                else if(led == '2')
                {
                    if(memcmp(ptr, "ON", 2) == 0)
                    {
                        GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);
                    }
                    else if(memcmp(ptr, "Blink", 5) == 0)
                    {
                        GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);
                        g_ucLEDStatus = 1;
                    }
                    else
                    {
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
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    if(pDevEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }

    //
    // Most of the general errors are not FATAL are are to be handled
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
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    if(pSock == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }
    //
    // This application doesn't work w/ socket - Events are not expected
    //
    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status)
            {
                case SL_ECLOSE:
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                                "failed to transmit all queued packets\n\n",
                                    pSock->socketAsyncEvent.SockTxFailData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
                                "(%d) \n\n",
                                pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
                  break;
            }
            break;

        default:
        	UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
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
static void InitializeAppVariables()
{
    g_ulStatus = 0;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
    g_iInternetAccess = -1;
    g_ucDryerRunning = 0;
    g_uiDeviceModeConfig = ROLE_STA; //default is STA mode
    g_ucLEDStatus = LED_OFF;
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
static int ConfigureMode(int iMode)
{
    long   lRetVal = -1;

    lRetVal = sl_WlanSetMode(iMode);
    ASSERT_ON_ERROR(lRetVal);

    // Restart Network processor
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);

    // Reset status bits
    CLR_STATUS_BIT_ALL(g_ulStatus);

    return sl_Start(NULL,NULL,NULL);
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
long ConnectToNetwork()
{
    long lRetVal = -1;
    unsigned int uiConnectTimeoutCnt = 0;

    // Start Simplelink
    lRetVal =  sl_Start(NULL,NULL,NULL);
    ASSERT_ON_ERROR( lRetVal);

    unsigned char ssid[15] = "airu-1";
    unsigned short ssid_len = strlen((const char *) ssid);
    sl_WlanSet(SL_WLAN_CFG_AP_ID,WLAN_AP_OPT_SSID, ssid_len, ssid);

    unsigned char url[32] = "myairu.edu";
    unsigned char url_len = strlen((const char *) url);
    lRetVal = sl_NetAppSet(SL_NET_APP_DEVICE_CONFIG_ID, NETAPP_SET_GET_DEV_CONF_OPT_DOMAIN_NAME, url_len, (const char *)url);




    if ( g_uiDeviceModeConfig == ROLE_AP )
    {
    	UART_PRINT("Force AP Jumper is Connected.\n\r");

    	if ( lRetVal != ROLE_AP )
    	{
    		// Put the device into AP mode.
    		lRetVal = ConfigureMode(ROLE_AP);
    		ASSERT_ON_ERROR(lRetVal);
    	}

		// Now the device is in AP mode, we need to wait for this event
		// before doing anything
		while(!IS_IP_ACQUIRED(g_ulStatus))
		{
		#ifndef SL_PLATFORM_MULTI_THREADED
		  _SlNonOsMainLoopTask();
		#endif
		}

    	// Stop Internal HTTP Server
		lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
		ASSERT_ON_ERROR( lRetVal);

		// Start Internal HTTP Server
		lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
		ASSERT_ON_ERROR( lRetVal);

	    char ssid[32];
	    unsigned short len = 32;
	    unsigned short config_opt = WLAN_AP_OPT_SSID;
	    sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt , &len, (unsigned char* )ssid);
	    UART_PRINT("\n\r Connect to : \'%s\'\n\r\n\r",ssid);
    }
    else
    {
    	if ( lRetVal == ROLE_AP )
    	{
    		UART_PRINT("Device is in AP Mode and Force AP Jumper is not Connected.\n\r");

            // If the device is in AP mode, we need to wait for this event
            // before doing anything
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
            #ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
            #endif
            }
    	}

        // Switch to STA Mode
        lRetVal = ConfigureMode(ROLE_STA);
        ASSERT_ON_ERROR( lRetVal);

        UART_PRINT("Device has been put into STA Mode.\n\r");

        // Stop Internal HTTP Server
		lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
		ASSERT_ON_ERROR( lRetVal);

		// Start Internal HTTP Server
		lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
		ASSERT_ON_ERROR( lRetVal);

		// Waiting for the device to Auto Connect
		UART_PRINT("Trying to Auto Connect to Existing Profiles.\n\r");

		while(uiConnectTimeoutCnt < AUTO_CONNECTION_TIMEOUT_COUNT &&
			((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus))))
		{
			// Turn STAT 1 LED On
			GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
			osi_Sleep(50);

			// Turn STAT 1 LED Off
			GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
			osi_Sleep(50);

			uiConnectTimeoutCnt++;
		}

		// Couldn't connect Using Auto Profile
		if(uiConnectTimeoutCnt == AUTO_CONNECTION_TIMEOUT_COUNT)
		{
			UART_PRINT("Couldn't connect Using Auto Profile.\n\r");

			// Blink STAT1 LEd to Indicate Connection Error
			GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);

			CLR_STATUS_BIT_ALL(g_ulStatus);

			// Put the station into AP mode
			lRetVal = ConfigureMode(ROLE_AP);
			ASSERT_ON_ERROR( lRetVal);
			UART_PRINT("Device has been put into AP Mode.\n\r");


			// Waiting for the AP to acquire IP address from Internal DHCP Server
			// If the device is in AP mode, we need to wait for this event
			// before doing anything
			while(!IS_IP_ACQUIRED(g_ulStatus))
			{
			#ifndef SL_PLATFORM_MULTI_THREADED
				_SlNonOsMainLoopTask();
			#endif
			}

			char cCount=0;

			// Blink LED 3 times to Indicate AP Mode
			for(cCount=0; cCount<3; cCount++)
			{
				// Turn STAT1 LED On
				GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
				osi_Sleep(400);

				// Turn STAT1 LED Off
				GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
				osi_Sleep(400);
			}

		    char ssid[32];
		    unsigned short len = 32;
		    unsigned short config_opt = WLAN_AP_OPT_SSID;
		    sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt , &len, (unsigned char* )ssid);
		    UART_PRINT("\n\r Connect to : \'%s\'\n\r\n\r",ssid);
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
static void ReadDeviceConfiguration()
{
    unsigned int uiGPIOPort;
    unsigned char pucGPIOPin;
    unsigned char ucPinValue;
        
    // Read GPIO
    GPIO_IF_GetPortNPin(SH_GPIO_3,&uiGPIOPort,&pucGPIOPin);
    ucPinValue = GPIO_IF_Get(SH_GPIO_3,uiGPIOPort,pucGPIOPin);
        
    // If Connected to VCC, Mode is AP
    if(ucPinValue == 1)
    {
        // AP Mode
        g_uiDeviceModeConfig = ROLE_AP;

    }
    else
    {
        // STA Mode
        g_uiDeviceModeConfig = ROLE_STA;

    }

}

//****************************************************************************
//
//!    \brief OTA Application Main Task - Initializes SimpleLink Driver and
//!                                              Handles HTTP Requests
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void OTATask(void *pvParameters)
{
	long lRetVal = -1;

	long OptionLen;
	unsigned char OptionVal;
	int SetCommitInt;
	unsigned char ucVendorStr[20];

	//TODO

	//Read Device Mode Configuration

	ReadDeviceConfiguration();


	lRetVal = ConnectToNetwork();
	if (lRetVal < 0){
		GPIO_IF_LedOn(MCU_STAT_3_LED_GPIO);
		osi_Sleep(500);
		GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);
	}

	//
	// Initialize OTA
	//
	pvOtaApp = sl_extLib_OtaInit(RUN_MODE_NONE_OS | RUN_MODE_BLOCKING,0);

	strcpy((char *)ucVendorStr, OTA_VENDOR_STRING);

	OTAServerInfoSet(&pvOtaApp, (char *)ucVendorStr);


	//
	// Check if this image is booted in test mode
	//


	sl_extLib_OtaGet(pvOtaApp, EXTLIB_OTA_GET_OPT_IS_PENDING_COMMIT, &OptionLen, (_u8 *)&OptionVal);


	UART_PRINT("EXTLIB_OTA_GET_OPT_IS_PENDING_COMMIT? %d \n\r", OptionVal);


	if (OptionVal == true)  {
	    UART_PRINT("OTA: PENDING COMMIT & WLAN OK ==> PERFORM COMMIT \n\r");


	    SetCommitInt = OTA_ACTION_IMAGE_COMMITED;
	    sl_extLib_OtaSet(pvOtaApp, EXTLIB_OTA_SET_OPT_IMAGE_COMMIT, sizeof(int), (_u8 *)&SetCommitInt);
	}
	else    {
	    UART_PRINT("Starting OTA\n\r");
	    lRetVal = 0;


	    while (!lRetVal)    {
	        lRetVal = sl_extLib_OtaRun(pvOtaApp);
	    }


	    UART_PRINT("OTA run = %d\n\r", lRetVal);
	    if (lRetVal < 0) {
	        UART_PRINT("OTA: Error with OTA server\n\r");
	    }
	    else if (lRetVal == RUN_STAT_NO_UPDATES)    {
	        UART_PRINT("OTA: RUN_STAT_NO_UPDATES\n\r");
	    }
	    else if (lRetVal && RUN_STAT_DOWNLOAD_DONE) {
	        // Set OTA File for testing


	        lRetVal = sl_extLib_OtaSet(pvOtaApp, EXTLIB_OTA_SET_OPT_IMAGE_TEST, sizeof(int), (_u8 *)&SetCommitInt);


	        UART_PRINT("OTA: NEW IMAGE DOWNLOAD COMPLETE\n\r");


	        UART_PRINT("Rebooting...\n\r");
	        RebootMCU();
	    }


	}

}

//****************************************************************************
//
//!    \brief DataGather Application Task - Samples the sensors every 1 minute.
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void DataGatherTask(void *pvParameters)
{
	long   lRetVal = -1;

	TickType_t xLastWakeTime;
	const TickType_t xFreq = 10000; // 60 seconds

	xLastWakeTime = xTaskGetTickCount();

	float temperature;
	float humidity;
	float light;
	int pm01;
	int pm2_5;
	int pm10;
	unsigned char pm_buf[24];




	int i = 0;
	while (1)
	{
		GPIO_IF_LedOn(MCU_STAT_1_LED_GPIO);
		vTaskDelayUntil(&xLastWakeTime, xFreq);
		GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);

		// Gather i2c sensor data
		I2C_IF_Open(I2C_MASTER_MODE_FST);
		temperature = GetTemperature();
		humidity = GetHumidity();
		ConfigureOPT3001Mode();
		light = GetOPT3001Result();

		//TODO CO/NO2
		I2C_IF_Close(I2C_MASTER_MODE_FST);
		GetPMS3003Result(pm_buf);

		// Gather uart sensor data
		//TODO GPS

		pm01 = GetPM01(pm_buf);
		pm2_5 =GetPM2_5(pm_buf);
		pm10 = GetPM10(pm_buf);


		char *csv = (char*)malloc(50* sizeof(char));
		sprintf(csv, "%d,%f,%f,%f,%d,%d,%d\r\n", i++, temperature, humidity, light, pm01, pm2_5, pm10);
		int btw = strlen(csv);

		//TODO create file for each new day using GPS
		// this will propose a problem though if date doesnt get set
		f_mount(&fs, "", 0);
		/* Open or create a log file and ready to append */
		res = f_append(&fp, "logfile.txt");
		if (res == FR_OK) {
			/* Append a line */
			res = f_write(&fp,csv,btw,&Size);
			/* Close the file */
			res = f_close(&fp);

		}
		else
		{
			//Message("Failed to create a new file\n\r");
		}
		free(csv);

	}
}

//****************************************************************************
//
//!    \brief DataUpload Application Task - Uploads collected data every 90 minutes.
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
//static void DataUploadTask(void *pvParameters)
//{
//	TickType_t xLastWakeTime;
//	const TickType_t xFreq = 60000; // 1 minute
//
//	xLastWakeTime = xTaskGetTickCount();
//
//	int i = 0;
//	while (1)
//	{
//		vTaskDelayUntil(&xLastWakeTime, xFreq);
//
//		// Write over UART
//		UART_PRINT("%d\r\n", i++);
//	}
//}




//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void BoardInit(void)
{

    // Set vector table base
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
    
    // Enable Processor
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

static void SDInit(){
	//
	// Set the SD card clock as output pin
	//
	MAP_PinDirModeSet(PIN_07,PIN_DIR_MODE_OUT);

	//
	// Enable Pull up on data
	//
	MAP_PinConfigSet(PIN_06,PIN_STRENGTH_4MA, PIN_TYPE_STD_PU);

	//
	// Enable Pull up on CMD
	//
	MAP_PinConfigSet(PIN_08,PIN_STRENGTH_4MA, PIN_TYPE_STD_PU);

	//
	// Enable MMCHS
	//
	MAP_PRCMPeripheralClkEnable(PRCM_SDHOST,PRCM_RUN_MODE_CLK);

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
	MAP_SDHostSetExpClk(SDHOST_BASE, MAP_PRCMPeripheralClockGet(PRCM_SDHOST),15000000);


}

//****************************************************************************
//
//! Sets the OTA server info and vendor ID
//!
//! \param pvOtaApp pointer to OtaApp handler
//! \param ucVendorStr vendor string
//! \param pfnOTACallBack is  pointer to callback function
//!
//! This function sets the OTA server info and vendor ID.
//!
//! \return None.
//
//****************************************************************************
int OTAServerInfoSet(void **pvOtaApp, char *vendorStr)
{

    unsigned char macAddressLen = SL_MAC_ADDR_LEN;

    //
    // Set OTA server info
    //
    g_otaOptServerInfo.ip_address = OTA_SERVER_IP_ADDRESS;
    g_otaOptServerInfo.secured_connection = OTA_SERVER_SECURED;
    strcpy((char *)g_otaOptServerInfo.server_domain, OTA_SERVER_NAME);
    strcpy((char *)g_otaOptServerInfo.rest_update_chk, OTA_SERVER_REST_UPDATE_CHK);
    strcpy((char *)g_otaOptServerInfo.rest_rsrc_metadata, OTA_SERVER_REST_RSRC_METADATA);
    strcpy((char *)g_otaOptServerInfo.rest_hdr, OTA_SERVER_REST_HDR);
    strcpy((char *)g_otaOptServerInfo.rest_hdr_val, OTA_SERVER_REST_HDR_VAL);
    strcpy((char *)g_otaOptServerInfo.log_server_name, LOG_SERVER_NAME);
    strcpy((char *)g_otaOptServerInfo.rest_files_put, OTA_SERVER_REST_FILES_PUT);
    sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &macAddressLen, (_u8 *)g_otaOptServerInfo.log_mac_address);

    //
    // Set OTA server Info
    //
    sl_extLib_OtaSet(*pvOtaApp, EXTLIB_OTA_SET_OPT_SERVER_INFO,
                     sizeof(g_otaOptServerInfo), (_u8 *)&g_otaOptServerInfo);

    //
    // Set vendor ID.
    //
    sl_extLib_OtaSet(*pvOtaApp, EXTLIB_OTA_SET_OPT_VENDOR_ID, strlen(vendorStr),
                     (_u8 *)vendorStr);

    //
    // Return ok status
    //
    return RUN_STAT_OK;
}

//****************************************************************************
//
//! Reboot the MCU by requesting hibernate for a short duration
//!
//! \return None
//
//****************************************************************************
static void RebootMCU()
{

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
  while(1)
  {

  }
}

void main()
{

	long   lRetVal = -1;

    // Board Initialization
    BoardInit();
    
    // Configure the pinmux settings for the peripherals exercised
    PinMuxConfig();

    PinConfigSet(PIN_58,PIN_STRENGTH_2MA|PIN_STRENGTH_4MA,PIN_TYPE_STD_PD);

    // Initialize Global Variables
    InitializeAppVariables();
    

    InitTerm();
    //InitPMS();

    // uSD Init
    SDInit();


    DisplayBanner(OTA_VENDOR_STRING);


    // LED Init
    GPIO_IF_LedConfigure(LED1);
    GPIO_IF_LedConfigure(LED2);
    GPIO_IF_LedConfigure(LED3);
      
    // Turn Off the LEDs
    GPIO_IF_LedOff(MCU_STAT_1_LED_GPIO);
    GPIO_IF_LedOff(MCU_STAT_2_LED_GPIO);
    GPIO_IF_LedOff(MCU_STAT_3_LED_GPIO);
    //GPIO_IF_LedOn(MCU_STAT_2_LED_GPIO);

    // Simplelink Spawn Task
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0)
    {
    	ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }
    
    // Create OTA Task
    osi_TaskCreate(OTATask, (signed char*)"OTATask", \
                                OSI_STACK_SIZE, NULL, \
                                OTA_TASK_PRIORITY, NULL );

    // Create the DataGather Task
//    osi_TaskCreate(DataGatherTask, (signed char*)"DataGatherTask",
//    		OSI_STACK_SIZE, NULL, DATAGATHER_TASK_PRIORITY, NULL);

//    // Create the DataUpload Task
//    osi_TaskCreate(DataUploadTask, (signed char*)"DataUploadTask",
//        						 OSI_STACK_SIZE, NULL, DATAUPLOAD_TASK_PRIORITY, NULL);

    // Start OS Scheduler
    osi_start();

    while (1)
    {

    }

}
