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
#include "tim2.h"
#include "stm32f10x_dma.h"
#include "uart4.h"
#include "gsm.h"

#define START_TIME() {\
    tim2_ms_ticks=0;\
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , ENABLE); \
    TIM_Cmd(TIM2, ENABLE);\
}
#define STOP_TIME() {\
    TIM_Cmd(TIM2, DISABLE);\
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , DISABLE);\
}

static __IO uint32_t tim2_ms_ticks;


static __IO uint32_t tim2_1s_chcnt;
static __IO uint32_t tim2_hs_chcnt;

void TIM2_clear_cnts(void){
    tim2_1s_chcnt = 0;
    tim2_hs_chcnt = 0;
}


uint32_t TIM2_Ms_Cycle(uint32_t interval)
{
    tim2_1s_chcnt ++;
    if((tim2_ms_ticks % interval) == 0 && tim2_1s_chcnt == 1) {
        return 1;
    }
    return 0;
}


uint8_t TIM2_Ms_Half(uint32_t interval)
{
    tim2_hs_chcnt ++;
    if((tim2_ms_ticks % interval) == (interval/2) && tim2_hs_chcnt == 1) {
        return 1;
    }
    return 0;
}


void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2 , TIM_IT_Update) != RESET) 
    {	
        TIM_ClearITPendingBit(TIM2 , TIM_FLAG_Update); 
        TIM2_clear_cnts();
        tim2_ms_ticks++;
    }
}

static void TIM2_NVIC_Configuration(void)
{
      NVIC_InitTypeDef NVIC_InitStructure; 
      
      NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); 
      NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
}


/*TIM_Period--1000   TIM_Prescaler--71 --> 1ms*/
static void TIM2_Configuration(void)
{
      TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , ENABLE);
      TIM_DeInit(TIM2);
      TIM_TimeBaseStructure.TIM_Period=1000;
      TIM_TimeBaseStructure.TIM_Prescaler= (72 - 1); /* 72M/72 */
      TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;
      TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
      TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
      TIM_ClearFlag(TIM2, TIM_FLAG_Update);
      TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
      TIM_Cmd(TIM2, ENABLE);
      
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , DISABLE); 
}

void TIM2_Init(void)
{
	/* TIM2 config */
  TIM2_NVIC_Configuration();
  TIM2_Configuration();

  /* TIM2 start */
  START_TIME(); 
}
