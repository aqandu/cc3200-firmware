/*
 * pms3003.h
 *
 *  Created on: Jan 31, 2017
 *      Author: Tom Becnel <thomas.becnel@utah.edu>
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

extern void InitPMS(void);
extern long PMSSampleGet(unsigned short, unsigned short, unsigned short);
extern unsigned char PMSSampleCheckSum(unsigned char*);
extern unsigned short PMSGetPM01(unsigned char*);
extern unsigned short PMSGetPM2_5(unsigned char*);
extern unsigned short PMSGetPM10(unsigned char*);

#ifdef __cplusplus
}
#endif

#endif /* PMS3003_H_ */
