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
#include "debug.h"

static uint32_t debug_usart1_int_cnt = 0;
static uint32_t debug_uart4_int_cnt = 0;
static uint32_t debug_dma1_channel4_int_cnt = 0;
static uint32_t debug_dma1_channel5_int_cnt = 0;
static uint32_t debug_dma2_channel3_int_cnt = 0;
static uint32_t debug_dma2_channel5_int_cnt = 0;

static uint32_t play_stage = 0;
static uint32_t play_mode =  SYS_MODE_ECHO |SYS_MODE_1TH4|SYS_MODE_AUTO_CONN ;//| SYS_MODE_MON_PRT;//| SYS_MODE_MON_PRT ; //  SYS_MODE_ECHO 

uint32_t DEBUG_GetUSART1IntCnt(void)
{
  return debug_usart1_int_cnt;
}

void DEBUG_SetUSART1IntCnt(void)
{
  debug_usart1_int_cnt++;
}

uint32_t DEBUG_GetUART4IntCnt(void)
{
  return debug_uart4_int_cnt;
}

void DEBUG_SetUART4IntCnt(void)
{
  debug_uart4_int_cnt++;
}

uint32_t DEBUG_GetDMA1Channel4IntCnt(void)
{
  return debug_dma1_channel4_int_cnt;
}

void DEBUG_SetDMA1Channel4IntCnt(void)
{
  debug_dma1_channel4_int_cnt++;
}

uint32_t DEBUG_GetDMA1Channel5IntCnt(void)
{
  return debug_dma1_channel5_int_cnt;
}

void DEBUG_SetDMA1Channel5IntCnt(void)
{
  debug_dma1_channel5_int_cnt++;
}

uint32_t DEBUG_GetDMA2Channel3IntCnt(void)
{
  return debug_dma2_channel3_int_cnt;
}

void DEBUG_SetDMA2Channel3IntCnt(void)
{
  debug_dma2_channel3_int_cnt++;
}

uint32_t DEBUG_GetDMA2Channel5IntCnt(void)
{
  return debug_dma2_channel5_int_cnt;
}

void DEBUG_SetDMA2Channel5IntCnt(void)
{
  debug_dma2_channel5_int_cnt++;
}

uint32_t  DEBUG_GetPlayMode(void)
{
  return play_mode;
}
void  DEBUG_SetPlayMode(uint32_t mode)
{
  play_mode = mode;
}
void DEBUG_AddPlayMode(uint32_t mode){
    play_mode |= mode;
}
void DEBUG_RemovePlayMode(uint32_t mode){
    play_mode &= ~mode;
}

uint32_t  DEBUG_GetPlayStage(void)
{
  return play_stage;
}
void  DEBUG_SetPlayStage(uint32_t stage)
{
  play_stage = stage;
}

void DEBUG_MonitorState(void)
{
  if(play_mode & SYS_MODE_MON_PRT)
  {
  
    printf("play_mode=0x%x, play_stage=%d\r\n", play_mode, play_stage);
    printf("debug_usart1_int_cnt=%d , debug_uart4_int_cnt=%d \r\n",debug_usart1_int_cnt, debug_uart4_int_cnt);
    printf("cnt_dma14=%d , cnt_dma15=%d , cnt_dma23=%d , cnt_dma25=%d \r\n", debug_dma1_channel4_int_cnt , debug_dma1_channel5_int_cnt, debug_dma2_channel3_int_cnt, debug_dma2_channel5_int_cnt);
//    printf("uart1_dma_sendbuffer = %s, uart1_dma_receivebuffer = %s\r\n",uart1_dma_sendbuffer, uart1_dma_receivebuffer);
  }

}

