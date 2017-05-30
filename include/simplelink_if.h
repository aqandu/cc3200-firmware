#ifndef SIMPLELINK_IF_H_
#define SIMPLELINK_IF_H_

#include "simplelink.h"

 //*****************************************************************************
 // Device Defines
 //*****************************************************************************
#define DEVICE_VERSION			"1.1.0"
#define DEVICE_MANUFACTURE		"UOFU"
#define DEVICE_NAME 			"dairu"
#define DEVICE_MODEL			"CC3200"
#define DEVICE_AP_DOMAIN_NAME	"myairu.local"
#define DEVICE_SSID_AP_NAME		"sairu"

 //*****************************************************************************
 // mDNS Defines
 //*****************************************************************************
#define MDNS_SERVICE  "._control._udp.local"
#define TTL             120
#define UNIQUE_SERVICE  1       /* Set 1 for unique services, 0 otherwise */

 //*****************************************************************************
 // SimpleLink/WiFi Defines
 //*****************************************************************************
#define UDPPORT         4000 /* Port number to which the connection is bound */
#define UDPPACKETSIZE   1024
#define SPI_BIT_RATE    14000000

#define TIMEOUT 5

//*****************************************************************************
// Date and Time Global
//*****************************************************************************
//extern SlDateTime_t dateTime;

 //*****************************************************************************
 // Function Declarations
 //*****************************************************************************

char * getMacAddress();
char * getDeviceName();
char * getApDomainName();
char * getSsidName();
int getDeviceTimeDate();

int setDeviceName();
int setApDomainName();
int setSsidName();
int setDeviceTimeDate();

int registerMdnsService();
int unregisterMdnsService();

#endif /* SIMPLELINK_IF_H_ */
