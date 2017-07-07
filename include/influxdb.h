/*
 * influxdb.h
 *
 *  Created on: Jun 23, 2017
 *      Author: Tom
 */

#ifndef INCLUDE_INFLUXDB_H_
#define INCLUDE_INFLUXDB_H_

#define INFLUXDB_PORT       8086
#define PROXY_IP            <proxy_ip>
#define PROXY_PORT          <proxy_port>
#define POST_REQUEST_URI    "/write?db=airU&u=airuSensor&p=YBK2bCkCK5qWwDts"
#define POST_DATA_AIR   \
"airQuality\,ID\=%s\,SensorModel\=H%s+S%s\ Altitude\=%.2f\,Latitude\=%.3f\,Longitude\=%.3f\,PM1\=%i\,PM2.5\=%i\,PM10\=%i\,Temperature\=%.2f\,Humidity\=%.2f"
/* "airQuality\,ID\=%s\ PM1\=%i\,PM2.5\=%i\,PM10\=%i\,Temp\=%.2f\,Humidity\=%.2f\,Lat\=%f\,Lon\=%f\,Alt\=%.2f"*/

//#define POST_DATA_TEST       "airQuality\,ID\=%s\,SensorSource\=test\ Temp\=%.2f\,PM1\=%i\,PM2.5\=%i\,pm10\=%i"

#define INFLUXDB_DNS_NAME   "air.eng.utah.edu"
#define SENSOR_MAC          "TOM0123"

typedef struct influxDBDataPoint {

    long timeStamp;
    float latitude,longitude, altitude;
    float ambientLight;
    float temperature;
    float humidity;
    int pm1,pm2_5, pm10;
    float concCO, concNO2;

} influxDBDataPoint;

#endif /* INCLUDE_INFLUXDB_H_ */
