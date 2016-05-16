#ifndef __GSM_H
#define __GSM_H
#include "stm32f10x.h"
#include <stdio.h>




void GSM_RetrieveData(void * src, size_t len);

uint8_t GSM_PowerOn(void);
uint8_t GSM_GPRSConnect(void);

uint8_t GSM_WaitString(uint8_t *p,uint8_t timeout);


#endif  /*__GSM_H*/
