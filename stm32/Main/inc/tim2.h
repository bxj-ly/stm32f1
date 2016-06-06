#ifndef TIME_2_H
#define TIME_2_H

#include "stm32f10x.h"
 
void TIM2_Init(void);

uint8_t TIM2_Ms_Cycle(uint32_t interval);
uint8_t TIM2_Ms_Half(uint32_t interval);
uint32_t TIM2_GetTick(void);

#endif	/* TIME_2_H*/
