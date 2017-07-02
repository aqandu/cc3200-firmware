//*****************************************************************************
// internet_if.h
//
//*****************************************************************************

#ifndef __INTERNET_IF__H__
#define __INTERNET_IF__H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************

#ifdef __cplusplus
extern "C"
{
#endif

#include "influxdb.h"
#include "http/client/httpcli.h"

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent);
extern void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent);
extern void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent);
extern void SimpleLinkSockEventHandler(SlSockEvent_t *pSock);
extern void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent,
                                         SlHttpServerResponse_t *pSlHttpServerResponse);
extern void InitializeAppVariables();
static int Wlan_ConfigureMode(int iMode);
static void Wlan_ReadDeviceConfiguration(void);
extern int Wlan_NetworkTest(void);
extern long ConnectToNetwork(void);
extern long Wlan_Disconnect(void);
extern int Wlan_IsInternetAccess(void);
extern long Net_IpConfigGet(unsigned long *aucIP, unsigned long *aucSubnetMask,
                            unsigned long *aucDefaultGateway, unsigned long *aucDNSServer);
extern long Net_GetHostIP( char* pcHostName,unsigned long * pDestinationIP);
extern int SKT_OpenUDPSocket(void);
extern void SKT_CloseSocket(int iSocketDesc);
extern long HTTP_InitInfluxHTTPClient(HTTPCli_Handle httpClient);
extern long HTTP_InitOTAHTTPClient(HTTPCli_Handle httpClient);
extern long HTTP_GETDownloadFile(HTTPCli_Handle httpClient, unsigned char *fn);
extern int HTTP_ConnectToInfluxServer(HTTPCli_Handle httpClient);
extern int HTTP_ConnectToOTAServer(HTTPCli_Handle httpClient);
extern int HTTP_FlushHTTPResponse(HTTPCli_Handle httpClient);
extern int HTTP_PostMethod(HTTPCli_Handle httpClient, influxDBDataPoint data);
extern int HTTP_ReadResponse(HTTPCli_Handle httpClient);
extern long HTTP_POSTInfluxDBDataPoint(HTTPCli_Handle httpClient, influxDBDataPoint data);
extern long HTTP_ProcessOTASession(HTTPCli_Handle httpClient);
extern int JSMN_ParseJSONData(char *ptr);
extern long OTA_UpdateApplicationBinary(void);
extern long GetSNTPTime(void);
extern long NTP_GetServerTime(void);
extern void OTA_setOTAUpdateFlag(unsigned char updateValue);
extern unsigned char OTA_getOTAUpdateFlag(void);
extern void NTP_GetDateTime(char *uc_timestamp);
extern void NTP_GetTime(char *uc_timestamp);
extern void NTP_GetDate(char *uc_timestamp);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif
#endif


