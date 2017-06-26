//*****************************************************************************
//*****************************************************************************

#include <string.h>
#include <stdlib.h>

// Simplelink includes
#include "simplelink.h"
#include "simplelink_if.h"

// driverlib includes
#include "hw_types.h"
#include "timer.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "osi.h"
#include "device_status.h"
#include "flc.h"
#include "fs.h"

// common interface includes
#include "internet_if.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "gpio_if.h"
#include "timer_if.h"
#include "common.h"

// application specific includes
#include "app_utils.h"
//#include "internet_if.h"
#include "influxdb.h"
#include "ota.h"

// HTTP Client lib
#include "http/client/httpcli.h"
#include "http/client/common.h"

// JSON Parser
#include "jsmn.h"

#define AUTO_CONNECTION_TIMEOUT_COUNT   50      /* 5 Sec */
#define SL_STOP_TIMEOUT                 200
#define SH_GPIO_3                       3       /* P58 - Device Mode */
#define READ_SIZE                       1450
#define MAX_BUFF_SIZE                   1460

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

// globals
static int g_iInternetAccess = -1;     // TODO: does this ever get set back to -1 on loss of internet?
static unsigned long g_ulAirUIP;
static unsigned long g_ulOTAIP;
static unsigned int g_uiDeviceModeConfig = ROLE_STA;    //default is STA mode
static unsigned char g_ucMacAddress[6*2+5+1];           // (#pairs*pair + colons + \0)
static unsigned char g_ucUniqueID[6*2+1];               // like ^^^ but no colons
static unsigned char g_httpResponseBuff[MAX_BUFF_SIZE+1];
static unsigned char g_ucUsrUpdateFWRequest = 0;

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

// Sunday is the 1st day in 2017 - the relative year
const char g_acDaysOfWeek2017[7][3]  = {{"Sun"},{"Mon"},{"Tue"},{"Wed"},{"Thu"},{"Fri"},{"Sat"}};
const char g_acMonthOfYear[12][3]    = {{"Jan"},{"Feb"},{"Mar"},{"Apr"},{"May"},{"Jun"},
                                        {"Jul"},{"Aug"},{"Sep"},{"Oct"},{"Nov"},{"Dec"}};
const char g_acNumOfDaysPerMonth[12] =  {31, 28, 31, 30, 31, 30,31, 31, 30, 31, 30, 31};
// socket variables
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
    unsigned short uisYear;
    unsigned short uisMonth;
    unsigned short uisDay;
    unsigned short uisHour;
    unsigned short uisMinute;
    unsigned short uisSecond;
}g_sSNTPData;

// define the tokens
static const unsigned char POST_token[] =       "__SL_P_ULD";
static const unsigned char GET_token_TEMP[] =   "__SL_G_UTP";
static const unsigned char GET_token_ACC[] =    "__SL_G_UAC";
static const unsigned char GET_token_UIC[] =    "__SL_G_UIC";

// Network App specific status/error codes which are used only in this file
typedef enum{
     // Choosing this number to avoid overlap w/ host-driver's error codes
    DEVICE_NOT_IN_STATION_MODE  = -0x7F0,
    DEVICE_NOT_IN_AP_MODE       = DEVICE_NOT_IN_STATION_MODE - 1,
    DEVICE_NOT_IN_P2P_MODE      = DEVICE_NOT_IN_AP_MODE - 1,
    DEVICE_START_FAILED         = DEVICE_NOT_IN_STATION_MODE - 1,
    INVALID_HEX_STRING          = DEVICE_START_FAILED - 1,
    TCP_RECV_ERROR              = INVALID_HEX_STRING - 1,
    TCP_SEND_ERROR              = TCP_RECV_ERROR - 1,
    FILE_NOT_FOUND_ERROR        = TCP_SEND_ERROR - 1,
    INVALID_SERVER_RESPONSE     = FILE_NOT_FOUND_ERROR - 1,
    FORMAT_NOT_SUPPORTED        = INVALID_SERVER_RESPONSE - 1,
    FILE_OPEN_FAILED            = FORMAT_NOT_SUPPORTED - 1,
    FILE_WRITE_ERROR            = FILE_OPEN_FAILED - 1,
    INVALID_FILE                = FILE_WRITE_ERROR - 1,
    SERVER_CONNECTION_FAILED    = INVALID_FILE - 1,
    GET_HOST_IP_FAILED          = SERVER_CONNECTION_FAILED  - 1,
    SERVER_GET_TIME_FAILED      = GET_HOST_IP_FAILED -1,

    STATUS_CODE_MAX = -0xBB8
}e_NetAppStatusCodes;

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile unsigned long  g_ulStatus = 0;   /* SimpleLink Status */
unsigned long  g_ulStaIp = 0;    /* Station IP address */
unsigned long  g_ulGatewayIP = 0; /* Network Gateway IP address */
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; /* Connection SSID */
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; /* Connection BSSID */
volatile unsigned short g_usConnectIndex; /* Connection time delay index */
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************


//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pSlWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent)
{
    if(pSlWlanEvent == NULL)
    {
        return;
    }

     switch(((SlWlanEvent_t*)pSlWlanEvent)->Event)
     {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION_FAILED);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'-Applications
            // can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pSlWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pSlWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                    pSlWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s , BSSID: "
                        "%x:%x:%x:%x:%x:%x\n\r", g_ucConnectionSSID,
                        g_ucConnectionBSSID[0], g_ucConnectionBSSID[1],
                        g_ucConnectionBSSID[2], g_ucConnectionBSSID[3],
                        g_ucConnectionBSSID[4], g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pSlWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION
            if(SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on application's request "
                           "\n\r", g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else
            {
                UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s,"
                           " BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
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
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION_FAILED);

            //
            // Information about the connected client (like SSID, MAC etc) will
            // be available in 'slPeerInfoAsyncResponse_t' - Applications
            // can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.APModeStaConnected;
            //

        }
        break;

        case SL_WLAN_STA_DISCONNECTED_EVENT:
        {
            // when client disconnects from device (AP)
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the connected client (like SSID, MAC etc) will
            // be available in 'slPeerInfoAsyncResponse_t' - Applications
            // can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.APModestaDisconnected;
            //
        }
        break;

        case SL_WLAN_SMART_CONFIG_COMPLETE_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

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
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

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

        case SL_WLAN_P2P_DEV_FOUND_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_P2P_DEV_FOUND);

            //
            // Information about P2P config details (like Peer device name, own
            // SSID etc) will be available in 'slPeerInfoAsyncResponse_t' -
            // Applications can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.P2PModeDevFound;
            //
        }
        break;

        case SL_WLAN_P2P_NEG_REQ_RECEIVED_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_P2P_REQ_RECEIVED);

            //
            // Information about P2P Negotiation req details (like Peer device
            // name, own SSID etc) will be available in 'slPeerInfoAsyncResponse_t'
            //  - Applications can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.P2PModeNegReqReceived;
            //
        }
        break;

        case SL_WLAN_CONNECTION_FAILED_EVENT:
        {
            // If device gets any connection failed event
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION_FAILED);
        }
        break;

        default:
        {
            UART_PRINT("[WLAN EVENT] Unexpected event \n\r");
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
    UART_PRINT("NetAppEventHandler Called\n\r");
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
        return;
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
        return;
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

        case SL_SOCKET_ASYNC_EVENT:

             switch(pSock->socketAsyncEvent.SockAsyncData.type)
             {
             case SSL_ACCEPT:/*accept failed due to ssl issue ( tcp pass)*/
                 UART_PRINT("[SOCK ERROR] - close socket (%d) operation"
                             "accept failed due to ssl issue\n\r",
                             pSock->socketAsyncEvent.SockAsyncData.sd);
                 break;
             case RX_FRAGMENTATION_TOO_BIG:
                 UART_PRINT("[SOCK ERROR] -close scoket (%d) operation"
                             "connection less mode, rx packet fragmentation\n\r"
                             "> 16K, packet is being released",
                             pSock->socketAsyncEvent.SockAsyncData.sd);
                 break;
             case OTHER_SIDE_CLOSE_SSL_DATA_NOT_ENCRYPTED:
                 UART_PRINT("[SOCK ERROR] -close socket (%d) operation"
                             "remote side down from secure to unsecure\n\r",
                            pSock->socketAsyncEvent.SockAsyncData.sd);
                 break;
             default:
                 UART_PRINT("unknown sock async event: %d\n\r",
                             pSock->socketAsyncEvent.SockAsyncData.type);
             }
            break;
        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
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
    unsigned char *ptr;
    switch (pSlHttpServerEvent->Event) {
    case SL_NETAPP_HTTPGETTOKENVALUE_EVENT: {

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
//          if (g_ucDryerRunning) {
//              strcpy(
//                      (char*) pSlHttpServerResponse->ResponseData.token_value.data,
//                      "Running");
//              pSlHttpServerResponse->ResponseData.token_value.len += strlen(
//                      "Running");
//          } else {
//              strcpy(
//                      (char*) pSlHttpServerResponse->ResponseData.token_value.data,
//                      "Stopped");
//              pSlHttpServerResponse->ResponseData.token_value.len += strlen(
//                      "Stopped");
//          }
        }

    }
        break;

    case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT: {
        unsigned char led;
        unsigned char *ptr =
                pSlHttpServerEvent->EventData.httpPostData.token_name.data;

        if (memcmp(ptr, POST_token, strlen((const char *) POST_token)) == 0) {
            ptr = pSlHttpServerEvent->EventData.httpPostData.token_value.data;

            // if it's an OTA update button click
            if (memcmp(ptr, "update", 6) == 0){
                g_ucUsrUpdateFWRequest = 1;
            }
        }
    }
        break;
    default:
        break;
    }
}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************


//****************************************************************************
//
//!    \brief This function initializes the application variables
//!
//!    \param[in]  None
//!
//!    \return     0 on success, negative error-code on error
//
//****************************************************************************
void InitializeAppVariables()
{
    g_ulStatus = 0;
    g_ulStaIp = 0;
    g_ulGatewayIP = 0;
    g_uiDeviceModeConfig = ROLE_STA;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));

    // set MAC Address and UniqueID
    getMacAddress(g_ucMacAddress);
    stripChar(g_ucUniqueID, g_ucMacAddress, ':');

    g_sSNTPData.ulDestinationIP = 0;

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
static int Wlan_ConfigureMode(int iMode) {
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
//!    \brief Read Force AP GPIO and Configure Mode - 1(Access Point Mode)
//!                                                  - 0 (Station Mode)
//!
//! \return                        None
//
//****************************************************************************
static void Wlan_ReadDeviceConfiguration() {
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
        Message("Device configured in AP Mode...\n\r");

    } else {
        // STA Mode
        g_uiDeviceModeConfig = ROLE_STA;
        Message("Device configured in STA Mode...\n\r");

    }

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
long Wlan_Connect() {

    long lRetVal = -1;
    unsigned int uiConnectTimeoutCnt = 0;

    // Start Simplelink
    lRetVal = sl_Start(NULL, NULL, NULL);
    ASSERT_ON_ERROR(lRetVal);

    if (g_uiDeviceModeConfig == ROLE_AP) {
        UART_PRINT("Force AP Jumper is Connected.\n\r");

        if (lRetVal != ROLE_AP) {
            // Put the device into AP mode.
            lRetVal = Wlan_ConfigureMode(ROLE_AP);
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
        getSsidName(ssid);
//      unsigned short len = 32;
//      unsigned short config_opt = WLAN_AP_OPT_SSID;
//      sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len, (unsigned char*) ssid);
        UART_PRINT("\n\r Connect to : \'%s\'\n\r\n\r", ssid);
    }
    else {
        if (lRetVal == ROLE_AP) {
            UART_PRINT("Device is in AP Mode and Force AP Jumper is not Connected.\n\r");

            // If the device is in AP mode, we need to wait for this event
            // before doing anything
            while (!IS_IP_ACQUIRED(g_ulStatus)) {
#ifndef SL_PLATFORM_MULTI_THREADED
                _SlNonOsMainLoopTask();
#endif
            }
        }

//      Report("Registering mDNS. \r\n");
//      lRetVal = registerMdnsService();
//      ASSERT_ON_ERROR(lRetVal);

        // Switch to STA Mode
        lRetVal = Wlan_ConfigureMode(ROLE_STA);
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
            osi_Sleep(100);
            uiConnectTimeoutCnt++;
        }

        // Couldn't connect Using Auto Profile
        if (uiConnectTimeoutCnt == AUTO_CONNECTION_TIMEOUT_COUNT) {
            UART_PRINT("Couldn't connect Using Auto Profile.\n\r");

            CLR_STATUS_BIT_ALL(g_ulStatus);

            // Put the station into AP mode
            lRetVal = Wlan_ConfigureMode(ROLE_AP);
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

            char ssid[32];
            getSsidName(ssid);
//          unsigned short len = 32;
//          unsigned short config_opt = WLAN_AP_OPT_SSID;
//          sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len,
//                  (unsigned char*) ssid);
            UART_PRINT("\n\r Connect to : \'%s\'\n\r\n\r", ssid);
        }

        g_iInternetAccess =0;// ConnectionTest();

    }

    return SUCCESS;
}

//*****************************************************************************
//
//! Disconnect  Disconnects from an Access Point
//!
//! \param  none
//!
//! \return 0 disconnected done, other already disconnected
//
//*****************************************************************************
long Wlan_Disconnect()
{
    long lRetVal = 0;

    if (IS_CONNECTED(g_ulStatus))
    {
        lRetVal = sl_WlanDisconnect();
        if(0 == lRetVal)
        {
            // Wait
            while(IS_CONNECTED(g_ulStatus))
            {
    #ifndef SL_PLATFORM_MULTI_THREADED
                  _SlNonOsMainLoopTask();
    #else
                  osi_Sleep(1);
    #endif
            }
            return lRetVal;
        }
        else
        {
            return lRetVal;
        }
    }
    else
    {
        return lRetVal;
    }

}

//*****************************************************************************
//
//! Wlan_IsInternetAccess
//!
//! \brief  This function returns the local variable g_iInternetAccess
//!
//! \return internet access (0) or error (-1,-2)
//!
//
//*****************************************************************************
int Wlan_IsInternetAccess(){
    return g_iInternetAccess;
}

//*****************************************************************************
//
//! Network_IF_IpConfigGet  Get the IP Address of the device.
//!
//! \param  pulIP IP Address of Device
//! \param  pulSubnetMask Subnetmask of Device
//! \param  pulDefaultGateway Default Gateway value
//! \param  pulDNSServer DNS Server
//!
//! \return On success, zero is returned. On error, -1 is returned
//
//*****************************************************************************
long Net_IpConfigGet(unsigned long *pulIP, unsigned long *pulSubnetMask,
                unsigned long *pulDefaultGateway, unsigned long *pulDNSServer)
{
    unsigned char isDhcp;
    unsigned char len = sizeof(SlNetCfgIpV4Args_t);
    long lRetVal = -1;
    SlNetCfgIpV4Args_t ipV4 = {0};

    lRetVal = sl_NetCfgGet(SL_IPV4_STA_P2P_CL_GET_INFO,&isDhcp,&len,
                                  (unsigned char *)&ipV4);
    ASSERT_ON_ERROR(lRetVal);

    *pulIP=ipV4.ipV4;
    *pulSubnetMask=ipV4.ipV4Mask;
    *pulDefaultGateway=ipV4.ipV4Gateway;
    *pulDefaultGateway=ipV4.ipV4DnsServer;

    return lRetVal;
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
long Net_GetHostIP( char* pcHostName,unsigned long * pDestinationIP )
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

int SKT_OpenUDPSocket(){
    int iSocketDesc;
    long lRetVal = -1;
    //
    // Create UDP socket
    //
    iSocketDesc = sl_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(iSocketDesc < 0){ASSERT_ON_ERROR(iSocketDesc);}

    g_sSNTPData.iSockID = iSocketDesc;

    // look up the IP from dns if it hasn't been done previously
    if(g_sSNTPData.ulDestinationIP == 0){
        //
        // Get the NTP server host IP address using the DNS lookup
        //
        lRetVal = Net_GetHostIP((char*)g_acSNTPserver, \
                                       &g_sSNTPData.ulDestinationIP);
        if(lRetVal<0){ASSERT_ON_ERROR(lRetVal);}}
    else{ lRetVal = 0; }

    struct SlTimeval_t timeVal;
    timeVal.tv_sec =  SERVER_RESPONSE_TIMEOUT;    // Seconds
    timeVal.tv_usec = 0;                          // Microseconds. 10000 microseconds resolution

    lRetVal = sl_SetSockOpt(g_sSNTPData.iSockID,SL_SOL_SOCKET,SL_SO_RCVTIMEO,\
                    (unsigned char*)&timeVal, sizeof(timeVal));
    if(lRetVal < 0){ASSERT_ON_ERROR(lRetVal);}

    return iSocketDesc;

}

void SKT_CloseSocket(int iSocketDesc){
    sl_Close(iSocketDesc);
}

//*****************************************************************************
//
//! \brief This function initializes the device to be an HTTP client. First
//!        the device connects to an access point, then it connects to the
//!        HTTP server
//!
//! \param   none
//! \return  On success, zero is returned. On error, negative is returned
//! \warning if failure to connect to AP - Loops forever
//
//*****************************************************************************
long HTTP_InitInfluxHTTPClient(HTTPCli_Handle httpClient){
    long lRetVal = 0;
    if (g_iInternetAccess==0){
        lRetVal = HTTP_ConnectToInfluxServer(httpClient);
        return lRetVal;
    }
    else{
        return (long) SERVER_CONNECTION_FAILED;
    }


}

//*****************************************************************************
//
//! \brief This function initializes the device to be an HTTP client.
//!
//! \param   none
//! \return  On success, zero is returned. On error, negative is returned
//! \warning if failure to connect to AP - Loops forever
//
//*****************************************************************************
long HTTP_InitOTAHTTPClient(HTTPCli_Handle httpClient){
    long lRetVal = 0;
    if (g_iInternetAccess==0){
        lRetVal = HTTP_ConnectToOTAServer(httpClient);
        return lRetVal;
    }
    else{
        return (long) SERVER_CONNECTION_FAILED;
    }


}

long HTTP_GETDownloadFile(HTTPCli_Handle httpClient, unsigned char *fn){
    long lRetVal = 0;
    long fileHandle = -1;
    unsigned int bytesReceived = 0;
    unsigned long Token = 0;
    int id;
    int len = 0;
    bool moreFlag = 0;
    char *ptr;
    unsigned long fileSize = 0;

    HTTPCli_Field fields[3] = {
                                {HTTPCli_FIELD_NAME_HOST, OTA_DNS_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "text/html, application/xhtml+xml, */*"},
                                {NULL, NULL}
                            };
    const char *ids[4] = {
                              HTTPCli_FIELD_NAME_CONTENT_LENGTH,
                              HTTPCli_FIELD_NAME_TRANSFER_ENCODING,
                              HTTPCli_FIELD_NAME_CONNECTION,
                              NULL
                          };

    UART_PRINT("Start downloading the file...\n\r");

    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    memset(g_httpResponseBuff, 0, sizeof(g_httpResponseBuff));

    // Make HTTP 1.1 GET request
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, OTA_BIN_SERVER_DIR, 0);
    if (lRetVal<0){
        DBG_PRINT("Couldn't make GET request\n\r\t");
        ERR_PRINT(lRetVal);
    }

    // Get response status
    lRetVal = HTTPCli_getResponseStatus(httpClient);
    if (lRetVal != 200){
        HTTP_FlushHTTPResponse(httpClient);
        DBG_PRINT("ERROR on http client response\n\r\t");
        ERR_PRINT(lRetVal);
    }

    // Read Response Headers
    HTTPCli_setResponseFields(httpClient, ids);
    while((id=HTTPCli_getResponseField(httpClient, (char*)g_httpResponseBuff, sizeof(g_httpResponseBuff), &moreFlag))
            != HTTPCli_FIELD_ID_END){

        if(id==0){
            UART_PRINT("Content length: %s\n\r", g_httpResponseBuff);
            fileSize = strtoul((const char*)g_httpResponseBuff,&ptr,10);
        }
        else if(id==1){
            if(!strncmp((const char*)g_httpResponseBuff, "chunked", sizeof("chunked"))){
                UART_PRINT("Chunked transfer encoding\n\r");
            }
        }
        else if(id==2){
            if(!strncmp((const char*)g_httpResponseBuff, "close", sizeof("close"))){
                ERR_PRINT(lRetVal);
            }
        }
    }

    // Open file to save the downloaded file
    lRetVal = sl_FsOpen((_u8 *)fn, FS_MODE_OPEN_WRITE, &Token, &fileHandle);
    if(lRetVal < 0)
    {
        // File Doesn't exit create a new one with size fileSize
        lRetVal = sl_FsOpen((unsigned char *)OTA_BIN_FS_NAME, \
                           FS_MODE_OPEN_CREATE(fileSize, \
                           _FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE),
                           &Token, &fileHandle);
        ASSERT_ON_ERROR(lRetVal);

    }

    while(1)
    {
        len = HTTPCli_readResponseBody(httpClient, (char *)g_httpResponseBuff, sizeof(g_httpResponseBuff) - 1, &moreFlag);
        if(len < 0)
        {
            // Close file without saving
            lRetVal = sl_FsClose(fileHandle, 0, (unsigned char*) "A", 1);
            return lRetVal;
        }

        lRetVal = sl_FsWrite(fileHandle, bytesReceived,
                                (unsigned char *)g_httpResponseBuff, len);

        if(lRetVal < len)
        {
            UART_PRINT("Failed during writing the file, Error-code: %d\r\nlen: %d\r\n", \
                         lRetVal,len);
            // Close file without saving
            lRetVal = sl_FsClose(fileHandle, 0, (unsigned char*) "A", 1);
            return lRetVal;
        }
        bytesReceived +=len;

        if ((len - 2) >= 0 && g_httpResponseBuff[len-2] == '\r' && g_httpResponseBuff[len-1] == '\n'){
            break;
        }

        if(!moreFlag)
        {
            break;
        }
    }

    //
    // If user file has checksum which can be used to verify the temporary
    // file then file should be verified
    // In case of invalid file (FILE_NAME) should be closed without saving to
    // recover the previous version of file
    //

    // Save and close file
    UART_PRINT("Total bytes received: %d\n\r", bytesReceived);
    lRetVal = sl_FsClose(fileHandle, 0, 0, 0);
    ASSERT_ON_ERROR(lRetVal);

    return SUCCESS;
}
//*****************************************************************************
//
//! Function to connect to Influx server
//!
//! \param  httpClient - Pointer to HTTP Client instance
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************

int HTTP_ConnectToInfluxServer(HTTPCli_Handle httpClient)
{
    long lRetVal = -1;
    struct sockaddr_in addr;


#ifdef USE_PROXY
    struct sockaddr_in paddr;
    paddr.sin_family = AF_INET;
    paddr.sin_port = htons(PROXY_PORT);
    paddr.sin_addr.s_addr = sl_Htonl(PROXY_IP);
    HTTPCli_setProxy((struct sockaddr *)&paddr);
#endif

    /* Resolve HOST NAME/IP */
        lRetVal = sl_NetAppDnsGetHostByName((signed char *)INFLUXDB_DNS_NAME,
                                              strlen((const char *)INFLUXDB_DNS_NAME),
                                              &g_ulAirUIP,SL_AF_INET);

        DBG_PRINT("AirU IP aquired from DNS\n\r");

        if(lRetVal < 0)
        {
            ASSERT_ON_ERROR(GET_HOST_IP_FAILED);
        }



    /* Set up the input parameters for HTTP Connection */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(INFLUXDB_PORT);
    addr.sin_addr.s_addr = sl_Htonl(g_ulAirUIP);

    /* Testing HTTPCli open call: handle, address params only */
    HTTPCli_construct(httpClient);
    lRetVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, 0, NULL);
    if (lRetVal < 0)
    {
        UART_PRINT("Connection to server failed. error(%d)\n\r", lRetVal);
        ASSERT_ON_ERROR(SERVER_CONNECTION_FAILED);
    }
    else
    {
        UART_PRINT("Connection to server created successfully\r\n");
    }

    return 0;
}

//*****************************************************************************
//
//! Function to connect to OTA server
//!
//! \param  httpClient - Pointer to HTTP Client instance
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************

int HTTP_ConnectToOTAServer(HTTPCli_Handle httpClient)
{
    long lRetVal = -1;
    struct sockaddr_in addr;


#ifdef USE_PROXY
    struct sockaddr_in paddr;
    paddr.sin_family = AF_INET;
    paddr.sin_port = htons(PROXY_PORT);
    paddr.sin_addr.s_addr = sl_Htonl(PROXY_IP);
    HTTPCli_setProxy((struct sockaddr *)&paddr);
#endif

    /* Resolve HOST NAME/IP */
        lRetVal = sl_NetAppDnsGetHostByName((signed char *)OTA_DNS_NAME,
                                              strlen((const char *)OTA_DNS_NAME),
                                              &g_ulOTAIP,SL_AF_INET);

        DBG_PRINT("OTA Server IP aquired from DNS\n\r");

        if(lRetVal < 0)
        {
            ASSERT_ON_ERROR(GET_HOST_IP_FAILED);
        }



    /* Set up the input parameters for HTTP Connection */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(OTA_PORT);
    addr.sin_addr.s_addr = sl_Htonl(g_ulOTAIP);

    /* Testing HTTPCli open call: handle, address params only */
    HTTPCli_construct(httpClient);
    lRetVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, 0, NULL);
    if (lRetVal < 0)
    {
        UART_PRINT("Connection to server failed. error(%d)\n\r", lRetVal);
        ASSERT_ON_ERROR(SERVER_CONNECTION_FAILED);
    }
    else
    {
        UART_PRINT("Connection to server created successfully\r\n");
    }

    return 0;
}

//*****************************************************************************
//
//! \brief Flush response body.
//!
//! \param[in]  httpClient - Pointer to HTTP Client instance
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int HTTP_FlushHTTPResponse(HTTPCli_Handle httpClient)
{
    const char *ids[2] = {
                            HTTPCli_FIELD_NAME_CONNECTION, /* App will get connection header value. all others will skip by lib */
                            NULL
                         };
    char buf[128];
    int id;
    int len = 1;
    bool moreFlag = 0;
    char ** prevRespFilelds = NULL;


    /* Store previosly store array if any */
    prevRespFilelds = HTTPCli_setResponseFields(httpClient, ids);

    /* Read response headers */
    while ((id = HTTPCli_getResponseField(httpClient, buf, sizeof(buf), &moreFlag))
            != HTTPCli_FIELD_ID_END)
    {

        if(id == 0)
        {
            if(!strncmp(buf, "close", sizeof("close")))
            {
                UART_PRINT("Connection terminated by server\n\r");
            }
        }

    }

    /* Restore previosuly store array if any */
    HTTPCli_setResponseFields(httpClient, (const char **)prevRespFilelds);

    while(1)
    {
        /* Read response data/body */
        /* Note:
                moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
                data is available Or in other words content length > length of buffer.
                The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
                Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
                for more information.
        */
        HTTPCli_readResponseBody(httpClient, buf, sizeof(buf) - 1, &moreFlag);
        ASSERT_ON_ERROR(len);

        if ((len - 2) >= 0 && buf[len - 2] == '\r' && buf [len - 1] == '\n'){
            break;
        }

        if(!moreFlag)
        {
            /* There no more data. break the loop. */
            break;
        }
    }
    return 0;
}

//*****************************************************************************
//
//! \brief HTTP POST Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int HTTP_PostMethod(HTTPCli_Handle httpClient, influxDBDataPoint data)
{
    bool moreFlags = 1;
    bool lastFlag = 1;
    char tmpBuf[4];
    long lRetVal = 0;
    HTTPCli_Field fields[4] = {
                                {HTTPCli_FIELD_NAME_HOST, INFLUXDB_DNS_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                {HTTPCli_FIELD_NAME_CONTENT_TYPE, "application/x-www-form-urlencoded"},
                                {NULL, NULL}
                            };


    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send POST method request. */
    /* Here we are setting moreFlags = 1 as there are some more header fields need to send
       other than setted in previous call HTTPCli_setRequestFields() at later stage.
       Please refer HTTP Library API documentation @ref HTTPCli_sendRequest for more information.
    */
    moreFlags = 1;
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_POST, POST_REQUEST_URI, moreFlags);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP POST request header.\n\r");
        return lRetVal;
    }

    // Construct HTTP POST Body
    unsigned char postString[250];
    sprintf((char*) postString,POST_DATA_AIR,SENSOR_MAC,24.5,2,1,3);
    UART_PRINT("POST DATA: %s\n\r",postString); //- debugging
    // UART_PRINT("\n\r");

    sprintf((char *)tmpBuf, "%d", (strlen((char*) postString)));

    /* Here we are setting lastFlag = 1 as it is last header field.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
    */
    lastFlag = 1;
    lRetVal = HTTPCli_sendField(httpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (const char *)tmpBuf, lastFlag);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP POST request header.\n\r");
        return lRetVal;
    }


    /* Send POST data/body */
    lRetVal = HTTPCli_sendRequestBody(httpClient, (char*) postString, (strlen((char*) postString))); // changed! // maybe change char* to const char*
    // UART_PRINT("sendRequestBody Return Value: %d \n\r",lRetVal); - Debugging
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP POST request body.\n\r");
        return lRetVal;
    }


    lRetVal = HTTP_ReadResponse(httpClient);

    return lRetVal;
}



/*!
    \brief This function read respose from server and dump on console

    \param[in]      httpClient - HTTP Client object

    \return         0 on success else -ve

    \note

    \warning
*/
int HTTP_ReadResponse(HTTPCli_Handle httpClient)
{
    long lRetVal = 0;
    int bytesRead = 0;
    int id = 0;
    unsigned long len = 0;
    int json = 0;
    char *dataBuffer=NULL;
    bool moreFlags = 1;
    const char *ids[4] = {
                            HTTPCli_FIELD_NAME_CONTENT_LENGTH,
                            HTTPCli_FIELD_NAME_CONNECTION,
                            HTTPCli_FIELD_NAME_CONTENT_TYPE,
                            NULL
                         };

    /* Read HTTP POST request status code */
    lRetVal = HTTPCli_getResponseStatus(httpClient);
    UART_PRINT("Return Value: %d \n\r", (int) lRetVal); // Debugging - Return Value
    if(lRetVal > 0)
    {
        switch(lRetVal)
        {
        case 200:
        {
            UART_PRINT("HTTP Status 200\n\r");
            /*
                 Set response header fields to filter response headers. All
                  other than set by this call we be skipped by library.
             */
            HTTPCli_setResponseFields(httpClient, (const char **)ids);

            /* Read filter response header and take appropriate action. */
            /* Note:
                    1. id will be same as index of fileds in filter array setted
                    in previous HTTPCli_setResponseFields() call.

                    2. moreFlags will be set to 1 by HTTPCli_getResponseField(), if  field
                    value could not be completely read. A subsequent call to
                    HTTPCli_getResponseField() will read remaining field value and will
                    return HTTPCli_FIELD_ID_DUMMY. Please refer HTTP Client Libary API
                    documenation @ref HTTPCli_getResponseField for more information.
             */
            while((id = HTTPCli_getResponseField(httpClient, (char *)g_httpResponseBuff, sizeof(g_httpResponseBuff), &moreFlags))
                    != HTTPCli_FIELD_ID_END)
            {

                switch(id)
                {
                case 0: /* HTTPCli_FIELD_NAME_CONTENT_LENGTH */
                {
                    len = strtoul((char *)g_httpResponseBuff, NULL, 0);
                }
                break;
                case 1: /* HTTPCli_FIELD_NAME_CONNECTION */
                {
                }
                break;
                case 2: /* HTTPCli_FIELD_NAME_CONTENT_TYPE */
                {
                    if(!strncmp((const char *)g_httpResponseBuff, "application/json",
                            sizeof("application/json")))
                    {
                        json = 1;
                    }
                    else
                    {
                        /* Note:
                                Developers are advised to use appropriate
                                content handler. In this example all content
                                type other than json are treated as plain text.
                         */
                        json = 0;
                    }
                    UART_PRINT(HTTPCli_FIELD_NAME_CONTENT_TYPE);
                    UART_PRINT(" : ");
                    UART_PRINT("application/json\n\r");
                }
                break;
                default:
                {
                    UART_PRINT("Wrong filter id\n\r");
                    lRetVal = -1;
                    goto end;
                }
                }
            }
            bytesRead = 0;
            if(len > sizeof(g_httpResponseBuff))
            {
                dataBuffer = (char *) malloc(len);
                if(dataBuffer)
                {
                    UART_PRINT("Failed to allocate memory\n\r");
                    lRetVal = -1;
                    goto end;
                }
            }
            else
            {
                dataBuffer = (char *)g_httpResponseBuff;
            }

            /* Read response data/body */
            /* Note:
                    moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
                    data is available Or in other words content length > length of buffer.
                    The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
                    Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
                    for more information

             */
            bytesRead = HTTPCli_readResponseBody(httpClient, (char *)dataBuffer, len, &moreFlags);
            if(bytesRead < 0)
            {
                UART_PRINT("Failed to received response body\n\r");
                lRetVal = bytesRead;
                goto end;
            }
            else if( bytesRead < len || moreFlags)
            {
                UART_PRINT("Mismatch in content length and received data length\n\r");
                goto end;
            }
            dataBuffer[bytesRead] = '\0';

            if(json)
            {
                /* Parse JSON data */
                lRetVal = JSMN_ParseJSONData(dataBuffer);
                if(lRetVal < 0)
                {
                    goto end;
                }
            }
            else
            {
                /* treating data as a plain text */
            }

        }
        break;

        case 404:
            UART_PRINT("File not found. \r\n");
            /* Handle response body as per requirement.
                  Note:
                    Developers are advised to take appopriate action for HTTP
                    return status code else flush the response body.
                    In this example we are flushing response body in default
                    case for all other than 200 HTTP Status code.
             */
        default:
            /* Note:
              Need to flush received buffer explicitly as library will not do
              for next request.Apllication is responsible for reading all the
              data.
             */
            HTTP_FlushHTTPResponse(httpClient);
            break;
        }
    }
    else
    {
        UART_PRINT("Failed to receive data from server.\r\n");
        goto end;
    }

    lRetVal = 0;

end:
    if(len > sizeof(g_httpResponseBuff) && (dataBuffer != NULL))
    {
        free(dataBuffer);
    }
    return lRetVal;
}

//****************************************************************************
//
//! \brief Posting Data to InfluxDB database
//!
//! \param  influxDataPoint - data
//!
//! \return  None
//
//****************************************************************************
long HTTP_POSTInfluxDBDataPoint(HTTPCli_Handle httpClient, influxDBDataPoint data){

    long lRetVal = -1;

    UART_PRINT("\n\r");
    UART_PRINT("HTTP Post Begin:\n\r");

    lRetVal = HTTP_PostMethod(httpClient, data);

    if(lRetVal < 0){
        UART_PRINT("HTTP Post failed.\n\r");
    }

    UART_PRINT("HTTP Post End:\n\r");

    return lRetVal;
}

//****************************************************************************
//
//! \brief Opens connection to OTA server, downloads binary,
//!             and resets bootloader binary pointer
//!
//! \param  httpClient - the http handle
//!
//! \return  Success [0] or Failure [<0]
//
//****************************************************************************
long HTTP_ProcessOTASession(HTTPCli_Handle httpClient){
    unsigned char *fn = (unsigned char*)OTA_BIN_FS_NAME;
    HTTP_GETDownloadFile(httpClient, fn);
    return OTA_UpdateApplicationBinary();

}

//*****************************************************************************
//
//! \brief Handler for parsing JSON data
//!
//! \param[in]  ptr - Pointer to http response body data
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int JSMN_ParseJSONData(char *ptr)
{
    long lRetVal = 0;
    int noOfToken;
    jsmn_parser parser;
    jsmntok_t   *tokenList;


    /* Initialize JSON PArser */
    jsmn_init(&parser);

    /* Get number of JSON token in stream as we we dont know how many tokens need to pass */
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), NULL, 10);
    if(noOfToken <= 0)
    {
        UART_PRINT("Failed to initialize JSON parser\n\r");
        return -1;

    }

    /* Allocate memory to store token */
    tokenList = (jsmntok_t *) malloc(noOfToken*sizeof(jsmntok_t));
    if(tokenList == NULL)
    {
        UART_PRINT("Failed to allocate memory\n\r");
        return -1;
    }

    /* Initialize JSON Parser again */
    jsmn_init(&parser);
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), tokenList, noOfToken);
    if(noOfToken < 0)
    {
        UART_PRINT("Failed to parse JSON tokens\n\r");
        lRetVal = noOfToken;
    }
    else
    {
        UART_PRINT("Successfully parsed %ld JSON tokens\n\r", noOfToken);
    }

    free(tokenList);

    return lRetVal;
}


//****************************************************************************
//
//! \brief Moves bootloader pointer to newly downloaded binary
//!
//! \return  Success [0] or Failure [<0]
//
//****************************************************************************
long OTA_UpdateApplicationBinary(){

    sBootInfo_t sBootInfo;
    long lFileHandle;
    long lRetVal = -1;
    unsigned long ulToken;

    //
    // Set the factory default
    //
      sBootInfo.ucActiveImg = IMG_ACT_USER1;
      sBootInfo.ulImgStatus = IMG_STATUS_NOTEST;

      if(sBootInfo.ucActiveImg==IMG_ACT_USER1){
          UART_PRINT("sBootInfo set with User1 Image...\n\r");}
      else if (sBootInfo.ucActiveImg==IMG_ACT_USER2){
          UART_PRINT("sBootInfo set with User2 Image...\n\r");}
      else if (sBootInfo.ucActiveImg==IMG_ACT_FACTORY){
          UART_PRINT("sBootInfo set with Factory Image...\n\r");}
      else{
          UART_PRINT("No clue what you just loaded as image...\n\r");}

      //
      // Save the new configuration
      //
      UART_PRINT("Saving configuration...\n\r");
      if( 0 == sl_FsOpen((unsigned char *)IMG_BOOT_INFO, FS_MODE_OPEN_WRITE,
                         &ulToken, &lFileHandle) )
      {
          UART_PRINT("File successfully opened...\n\r");
          sl_FsWrite(lFileHandle, 0, (unsigned char *)&sBootInfo,
                     sizeof(sBootInfo_t));
          UART_PRINT("File succesfully written to...\n\r");
          lRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
          if(lRetVal == 0){
              UART_PRINT("File succesfully closed...\n\r");
          }
      }
      else
          UART_PRINT("File couldn't be opened...\n\r");
      return lRetVal;
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
long GetSNTPTime()
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

    memset(cDataBuf, 0, sizeof(cDataBuf));
    cDataBuf[0] = '\x1b';

    sAddr.sa_family = AF_INET;

    // the source port
    sAddr.sa_data[0] = 0x00;
    sAddr.sa_data[1] = 0x7B;    // UDP port number for NTP is 123
    sAddr.sa_data[2] = (char)((g_sSNTPData.ulDestinationIP>>24)&0xff);
    sAddr.sa_data[3] = (char)((g_sSNTPData.ulDestinationIP>>16)&0xff);
    sAddr.sa_data[4] = (char)((g_sSNTPData.ulDestinationIP>>8)&0xff);
    sAddr.sa_data[5] = (char)(g_sSNTPData.ulDestinationIP&0xff);

    lRetVal = sl_SendTo(g_sSNTPData.iSockID,
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
    if(g_sSNTPData.ulElapsedSec == 0)
    {
        lRetVal = sl_Bind(g_sSNTPData.iSockID,
                (SlSockAddr_t *)&sLocalAddr,
                sizeof(SlSockAddrIn_t));
    }

    iAddrSize = sizeof(SlSockAddrIn_t);

    lRetVal = sl_RecvFrom(g_sSNTPData.iSockID,
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
        g_sSNTPData.ulElapsedSec = cDataBuf[40];
        g_sSNTPData.ulElapsedSec <<= 8;
        g_sSNTPData.ulElapsedSec += cDataBuf[41];
        g_sSNTPData.ulElapsedSec <<= 8;
        g_sSNTPData.ulElapsedSec += cDataBuf[42];
        g_sSNTPData.ulElapsedSec <<= 8;
        g_sSNTPData.ulElapsedSec += cDataBuf[43];

        //
        // seconds are relative to 0h on 1 January 1900
        //
        g_sSNTPData.ulElapsedSec -= TIME2017;

        //
        // in order to correct the timezone
        //
        g_sSNTPData.ulElapsedSec -= (GMT_DIFF_TIME_HRS * SEC_IN_HOUR);
        g_sSNTPData.ulElapsedSec += (GMT_DIFF_TIME_MINS * SEC_IN_MIN);

        g_sSNTPData.pcCCPtr = &g_sSNTPData.acTimeStore[0];

        //
        // day, number of days since beginning of 2017
        //
        g_sSNTPData.uisDay = g_sSNTPData.isGeneralVar%7;
        g_sSNTPData.isGeneralVar = g_sSNTPData.ulElapsedSec/SEC_IN_DAY;
        memcpy(g_sSNTPData.pcCCPtr,
               g_acDaysOfWeek2017[g_sSNTPData.isGeneralVar%7], 3);
        g_sSNTPData.pcCCPtr += 3;
        *g_sSNTPData.pcCCPtr++ = '\x20';

        //
        // month
        //
        g_sSNTPData.isGeneralVar %= 365;
        for (iIndex = 0; iIndex < 12; iIndex++)
        {
            g_sSNTPData.isGeneralVar -= g_acNumOfDaysPerMonth[iIndex];
            if (g_sSNTPData.isGeneralVar < 0)
                    break;
        }
        if(iIndex == 12)
        {
            iIndex = 0;
        }
        g_sSNTPData.uisMonth = iIndex;
        memcpy(g_sSNTPData.pcCCPtr, g_acMonthOfYear[iIndex], 3);
        g_sSNTPData.pcCCPtr += 3;
        *g_sSNTPData.pcCCPtr++ = '\x20';

        //
        // date
        // restore the day in current month
        //
        g_sSNTPData.isGeneralVar += g_acNumOfDaysPerMonth[iIndex];
        g_sSNTPData.uisDay = g_sSNTPData.isGeneralVar + 1;
        g_sSNTPData.uisCCLen = itoa(g_sSNTPData.isGeneralVar + 1,
                                   g_sSNTPData.pcCCPtr);
        g_sSNTPData.pcCCPtr += g_sSNTPData.uisCCLen;
        *g_sSNTPData.pcCCPtr++ = '\x20';

        //
        // time
        //
        g_sSNTPData.ulGeneralVar = g_sSNTPData.ulElapsedSec%SEC_IN_DAY;

        // number of seconds per hour
        g_sSNTPData.ulGeneralVar1 = g_sSNTPData.ulGeneralVar%SEC_IN_HOUR;

        // number of hours
        g_sSNTPData.ulGeneralVar /= SEC_IN_HOUR;
        g_sSNTPData.uisHour = g_sSNTPData.ulGeneralVar;
        g_sSNTPData.uisCCLen = itoa(g_sSNTPData.ulGeneralVar,
                                   g_sSNTPData.pcCCPtr);
        g_sSNTPData.pcCCPtr += g_sSNTPData.uisCCLen;
        *g_sSNTPData.pcCCPtr++ = ':';

        // number of minutes per hour
        g_sSNTPData.ulGeneralVar = g_sSNTPData.ulGeneralVar1/SEC_IN_MIN;

        // number of seconds per minute
        g_sSNTPData.ulGeneralVar1 %= SEC_IN_MIN;
        g_sSNTPData.uisMinute = g_sSNTPData.ulGeneralVar;
        g_sSNTPData.uisCCLen = itoa(g_sSNTPData.ulGeneralVar,
                                   g_sSNTPData.pcCCPtr);
        g_sSNTPData.pcCCPtr += g_sSNTPData.uisCCLen;
        *g_sSNTPData.pcCCPtr++ = ':';
        g_sSNTPData.uisSecond = g_sSNTPData.ulGeneralVar1;
        g_sSNTPData.uisCCLen = itoa(g_sSNTPData.ulGeneralVar1,
                                   g_sSNTPData.pcCCPtr);
        g_sSNTPData.pcCCPtr += g_sSNTPData.uisCCLen;
        *g_sSNTPData.pcCCPtr++ = '\x20';

        //
        // year
        // number of days since beginning of 2017
        //
        g_sSNTPData.ulGeneralVar = g_sSNTPData.ulElapsedSec/SEC_IN_DAY;
        g_sSNTPData.ulGeneralVar /= 365;
        g_sSNTPData.uisYear = YEAR2017 + g_sSNTPData.ulGeneralVar;
        g_sSNTPData.uisCCLen = itoa(YEAR2017 + g_sSNTPData.ulGeneralVar,
                                   g_sSNTPData.pcCCPtr);
        g_sSNTPData.pcCCPtr += g_sSNTPData.uisCCLen;

        *g_sSNTPData.pcCCPtr++ = '\0';

       // UART_PRINT("response from server: ");
       // UART_PRINT((char *)g_acSNTPserver);
       // UART_PRINT("\n\r");
       // UART_PRINT("\n\r");
       // UART_PRINT(g_sSNTPData.acTimeStore);
//        char cTimestamp[20];
//        //char* tsptr = &cTimestamp[0];
//        NTP_GetDateTime(&cTimestamp[0]);
//        UART_PRINT("%s\n\r",cTimestamp);
//        UART_PRINT("\n\n\r| %i/%i/%i | %i:%i:%i |\n\r",\
//                   g_sSNTPData.uisMonth,g_sSNTPData.uisDay,g_sSNTPData.uisYear,\
//                   g_sSNTPData.uisHour,g_sSNTPData.uisMinute,g_sSNTPData.uisSecond);

    }
    return SUCCESS;
}

long NTP_GetServerTime(){
    long lRetVal = -1;
    int iSocketDesc = SKT_OpenUDPSocket();
    lRetVal = GetSNTPTime();
    SKT_CloseSocket(iSocketDesc);
    return lRetVal;
}

// Sets the value of the OTA Update Flag
void OTA_setOTAUpdateFlag(unsigned char updateValue){
    g_ucUsrUpdateFWRequest = updateValue;
}

// Returns the status of the OTA Update flag (triggered when user clicks "Update" button
unsigned char OTA_getOTAUpdateFlag(){
    return g_ucUsrUpdateFWRequest;
}

// return the timestamp
//  YEAR_MONTH_DAY_HOUR_MINUTE_SECOND
void NTP_GetDateTime(char *uc_timestamp){
    unsigned short us_CCLen;

    // Year
    us_CCLen = itoa(g_sSNTPData.uisYear,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Month
    us_CCLen = itoa(g_sSNTPData.uisMonth,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Day
    us_CCLen = itoa(g_sSNTPData.uisDay,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Hour
    us_CCLen = itoa(g_sSNTPData.uisHour,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Minute
    us_CCLen = itoa(g_sSNTPData.uisMinute,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Second
    us_CCLen = itoa(g_sSNTPData.uisSecond,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '\0';

}

// return the timestamp
//  YEAR_MONTH_DAY_HOUR_MINUTE_SECOND
void NTP_GetTime(char *uc_timestamp){
    unsigned short us_CCLen;
    // Hour
    us_CCLen = itoa(g_sSNTPData.uisHour,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Minute
    us_CCLen = itoa(g_sSNTPData.uisMinute,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Second
    us_CCLen = itoa(g_sSNTPData.uisSecond,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '\0';

}

// return the timestamp
//  YEAR_MONTH_DAY_HOUR_MINUTE_SECOND
void NTP_GetDate(char *uc_timestamp){
    unsigned short us_CCLen;
    // Year
    us_CCLen = itoa(g_sSNTPData.uisYear,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Month
    us_CCLen = itoa(g_sSNTPData.uisMonth,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '_';
    // Day
    us_CCLen = itoa(g_sSNTPData.uisDay,uc_timestamp);
    uc_timestamp += us_CCLen;
    *uc_timestamp++ = '\0';

}


