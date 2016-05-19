#ifndef TIME_2_H
#define TIME_2_H

#include "stm32f10x.h"
 
void TIM2_Init(void);

uint32_t TIM2_Ms_Cycle(uint32_t interval);
uint8_t TIM2_Ms_Half(uint32_t interval);
#endif	/* TIME_2_H*/
