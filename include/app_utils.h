/*
 * app_utils.h
 *
 *  Created on: Jun 22, 2017
 *      Author: Tom
 */

#ifndef INCLUDE_APP_UTILS_H_
#define INCLUDE_APP_UTILS_H_

// error codes go here
// error code is 32 bits
typedef enum
{
    ERROR_PMS_BAD_CS = 0x01,
    ERROR_PMS_BAD_HEADER = ERROR_PMS_BAD_CS * 2,
    ERROR_TEMP_BAD = ERROR_PMS_BAD_HEADER * 2,
    ERROR_HUM_BAD = ERROR_TEMP_BAD * 2,
    ERROR_NTP_SERVER = ERROR_HUM_BAD * 2,
    ERROR_INTERNET = ERROR_HUM_BAD * 2,
    ERROR_FILE_WRITE = ERROR_INTERNET * 2,
    ERROR_DATA_UPLOAD = ERROR_FILE_WRITE * 2
}ErrorCodeMasks;

unsigned short itoa(short cNum, char *cString);
void stripChar(char *cStringDst, char *cStringSrc, char c);

#endif /* INCLUDE_APP_UTILS_H_ */
