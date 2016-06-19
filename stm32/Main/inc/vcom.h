#ifndef __VCOM_H
#define __VCOM_H
#include "stm32f10x.h"
#include <stdio.h>

void VCOM_Config(void);
void VCOM_SendByte(uint8_t val);
void VCOM_SendString(uint8_t *str) ;

#endif  /*__VCOM_H*/
