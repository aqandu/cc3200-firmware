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
#define PMS             UARTA0_BASE
#define PMS_PERIPH      PRCM_UARTA0
#define PMS_BUFFER_LENGTH 24

extern void InitPMS(void);
extern void FillBuffer(unsigned char*);
extern unsigned int CheckSum(unsigned char*);
unsigned int GetPM01(unsigned char*);
unsigned int GetPM2_5(unsigned char*);
unsigned int GetPM10(unsigned char*);

#ifdef __cplusplus
}
#endif

#endif /* PMS3003_H_ */
