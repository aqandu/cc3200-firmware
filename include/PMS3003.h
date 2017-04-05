/*
 * pms3003.h
 *
 *  Created on: Jan 31, 2017
 *      Author: Tom
 */

#ifndef PMS3003_H_
#define PMS3003_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define UART1_BAUD_RATE 9600
#define SYSCLK          80000000
#define PMS             UARTA1_BASE
#define PMS_PERIPH      PRCM_UARTA1
#define PMS_BUFFER_LENGTH 24

extern void InitPMS(void);
extern void GetPMS3003Result(unsigned char*);
extern  int CheckSum(unsigned char*);
int GetPM01(unsigned char*);
int GetPM2_5(unsigned char*);
int GetPM10(unsigned char*);

#ifdef __cplusplus
}
#endif

#endif /* PMS3003_H_ */
