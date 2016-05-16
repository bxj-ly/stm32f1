/*******************************************************************************
 * Copyright (C) 2016 Xuejun Bian, Yu Lu. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom 
 * the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software. 
 
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ******************************************************************************/
#include "systick.h"

static __IO uint32_t systick_time_delay;
static void TimingDelay_Decrement(void);

#if 0
void delay_nus(uint16_t time)
{    
  uint16_t i=0;  
  while(time--)
  {
    i=10;  /* user define */
    while(i--) ;    
  }
}

void delay_nms(uint16_t time)
{    
  uint16_t i=0;  
  while(time--)
  {
    i=12000;  /* user define */
    while(i--) ;    
  }
}

#endif

void SysTick_Delay_ms(__IO uint32_t delay_ms)
{ 
  while(SysTick_Config(SystemCoreClock / 1000));

  systick_time_delay = delay_ms;  

  /* Enable system tick */
  SysTick->CTRL |=  SysTick_CTRL_ENABLE_Msk;

  while(systick_time_delay != 0);
  
  /* Disable system tick */
  SysTick->CTRL &= ~ SysTick_CTRL_ENABLE_Msk;
  SysTick->VAL = 0x00;
}

void SysTick_Delay_us(__IO uint32_t delay_us)
{ 
  while(SysTick_Config(SystemCoreClock / 1000000));

  systick_time_delay = delay_us;  

  /* Enable system tick */
  SysTick->CTRL |=  SysTick_CTRL_ENABLE_Msk;

  while(systick_time_delay != 0);
  
  /* Disable system tick */
  SysTick->CTRL &= ~ SysTick_CTRL_ENABLE_Msk;
  SysTick->VAL = 0x00;
}


/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  TimingDelay_Decrement();
}

static void TimingDelay_Decrement(void)
{
  if (systick_time_delay != 0x00)
  { 
    systick_time_delay--;
  }
}


