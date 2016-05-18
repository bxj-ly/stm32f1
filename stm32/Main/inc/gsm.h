#ifndef __GSM_H
#define __GSM_H
#include "stm32f10x.h"
#include <stdio.h>

void GSM_RetrieveData(void * src, size_t len);

uint8_t GSM_PowerOn(void);
uint8_t GSM_GPRSConnect(void);
uint8_t GSM_Location(void);
uint8_t GSM_GPRSSendData(void);


#endif  /*__GSM_H*/
