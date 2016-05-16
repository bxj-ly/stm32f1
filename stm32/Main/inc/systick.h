#ifndef __SYSTICK_H
#define __SYSTICK_H

#include "stm32f10x.h"

void SysTick_Delay_ms(__IO uint32_t delay_ms);
void SysTick_Delay_us(__IO uint32_t delay_us);

#endif /* __SYSTICK_H */
