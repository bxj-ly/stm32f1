#ifndef TIME_2_H
#define TIME_2_H

#include "stm32f10x.h"
 
void TIM2_Init(void);
uint8_t TIM2_Is_10ms(void);
uint8_t TIM2_Is_100ms(void);
uint8_t TIM2_Is_1s(void);

#endif	/* TIME_2_H*/
