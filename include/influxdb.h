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
#define POST_REQUEST_URI    "/write?db=mesowestTest&u=airuSensor&p=YBK2bCkCK5qWwDts"
#define POST_DATA_AIR       "airQuality\,ID\=%s\,SensorSource\=test\ Temp\=%.2f\,pm2.5\=%.2f\,pm1\=%.2f,pm10\=%.2f"
#define INFLUXDB_DNS_NAME   "air.eng.utah.edu"
#define SENSOR_MAC          "TOM0123"

typedef struct influxDBDataPoint {

    long timeStamp;
    float latitude,longitude, altitude; // gps data
    float ambientLight;
    float temperature;
    float humidity;
    float pm1,pm2_5, pm10;
    float concCO, concNO2;

} influxDBDataPoint;

#endif /* INCLUDE_INFLUXDB_H_ */
