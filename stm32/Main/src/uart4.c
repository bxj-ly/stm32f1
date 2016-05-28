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
#include <string.h>
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_dma.h"
#include "uart4.h"
#include "usart1.h"
#include "debug.h"
#include "gsm.h"

uint8_t uart4_dma_receivebuffer[UART4_DMA_RCVBUFFER_SIZE];

void UART4_NVIC_Config(void){
    NVIC_InitTypeDef NVIC_InitStructure; 
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);    
    /* Enable the UART4 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);  
    
    USART_ITConfig(UART4, USART_IT_IDLE, ENABLE);
}

/* UART4 Config
- BaudRate = 115200 baud
- Word Length = 8 Bits
- One Stop Bit 
- No parity 
- Hardware flow control disabled (RTS and CTS signals)  
- Receive and transmit enabled 
*/

void UART4_Config(void)
{
    USART_InitTypeDef USART_InitStructure;

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(UART4, &USART_InitStructure); 

    USART_Cmd(UART4, ENABLE);

    UART4_NVIC_Config();
}


void UART4_SendByte(uint8_t data)
{
   /* wart for transmit data register empty */
   while (!(UART4->SR & USART_FLAG_TXE));
   /* send data */
   UART4->DR = (data & 0x00FF);
   /* wait for transmission compelete */
   while (!(UART4->SR & USART_FLAG_TC));   
}

void UART4_TX_DMA_Send(void * src, size_t len)
{
    #define MAX_FAIL_TIME 100000
    int failcnt = 0;
    // wait for last dma tx finished
    while(DMA2_Channel5->CCR & DMA_CCR1_EN && failcnt++<MAX_FAIL_TIME);
    
    DMA2_Channel5->CMAR = (uint32_t)src;
    DMA_SetCurrDataCounter(DMA2_Channel5, len);
    DMA_Cmd (DMA2_Channel5,ENABLE);
}

void UART4_RX_MSG_Proc(void){
    uint16_t res_len = 0;
    res_len = DMA_GetCurrDataCounter(DMA2_Channel3);

    if(UART4_DMA_RCVBUFFER_SIZE - res_len) {
        GSM_RetrieveData(uart4_dma_receivebuffer, (size_t)(UART4_DMA_RCVBUFFER_SIZE - res_len));
        if(DEBUG_GetPlayMode() & SYS_MODE_1TH4){
            USART1_TX_DMA_Send(uart4_dma_receivebuffer, (size_t)(UART4_DMA_RCVBUFFER_SIZE - res_len));
        }    
        DMA_Cmd(DMA2_Channel3, DISABLE); 
        DMA_SetCurrDataCounter(DMA2_Channel3, UART4_DMA_RCVBUFFER_SIZE);
        DMA_Cmd(DMA2_Channel3, ENABLE);       
    }
}

void UART4_IRQHandler(void)
{
    uint8_t c;
    DEBUG_SetUART4IntCnt();

    if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET) {
        if(DEBUG_GetPlayMode() & SYS_MODE_MON4) {
            c=UART4->DR;
            INFO("%c",c);    
        }
    } 
    if(USART_GetITStatus(UART4, USART_IT_IDLE) != RESET) {
        c=UART4->SR;
        c=UART4->DR;
        //UART4_RX_MSG_Proc();
    }
}

void DMA2_Channel5_IRQHandler(void)
{
    DEBUG_SetDMA2Channel5IntCnt();
    if(DMA_GetFlagStatus(DMA2_FLAG_TC5)==SET) {
        DMA_ClearFlag(DMA2_FLAG_TC5);
        DMA_Cmd (DMA2_Channel5,DISABLE);
    }
}

void DMA2_Channel3_IRQHandler(void)
{
    DEBUG_SetDMA2Channel3IntCnt();
    if(DMA_GetFlagStatus(DMA2_FLAG_TC3)==SET) {
        DMA_ClearFlag(DMA2_FLAG_TC3); 
    }
    if(DMA_GetFlagStatus(DMA2_FLAG_HT3)==SET) {  
        DMA_ClearFlag(DMA2_FLAG_HT3); 
    }
    if(DMA_GetFlagStatus(DMA2_FLAG_TE3)==SET) {  
        DMA_ClearFlag(DMA2_FLAG_TE3); 
    }
}



void UART4_TX_DMA_NVIC_Config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure; 
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);    

    /*Enable DMA2 Channel5 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Channel5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DMA_ITConfig(DMA2_Channel5,DMA_IT_TC,ENABLE); 
}
static void UART4_TX_DMA_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    DMA_DeInit(DMA2_Channel5); 
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART4->DR;	   
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)0;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = 0;   
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode =  DMA_Mode_Normal ;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;  
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;   
    DMA_Init(DMA2_Channel5, &DMA_InitStructure);
    DMA_Cmd (DMA2_Channel5,DISABLE);

    USART_DMACmd(UART4, USART_DMAReq_Tx, ENABLE);

    UART4_TX_DMA_NVIC_Config();
}
void UART4_RX_DMA_NVIC_Config(void){
    
    NVIC_InitTypeDef NVIC_InitStructure; 
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
    /*Enable DMA2 Channel3 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Channel3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    DMA_ITConfig(DMA2_Channel3, DMA_IT_TC | DMA_IT_HT , ENABLE); 	
}

static void UART4_RX_DMA_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    DMA_DeInit(DMA2_Channel3); 
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART4->DR; 
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)uart4_dma_receivebuffer;  
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; 
    DMA_InitStructure.DMA_BufferSize = UART4_DMA_RCVBUFFER_SIZE; 
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte; 
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; 
    DMA_InitStructure.DMA_Priority = DMA_Priority_High; 
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA2_Channel3, &DMA_InitStructure);   
    DMA_Cmd(DMA2_Channel3, ENABLE); 	
    USART_DMACmd(UART4, USART_DMAReq_Rx, ENABLE);

    UART4_RX_DMA_NVIC_Config();
}

void UART4_DMA_Config(void)
{
    UART4_TX_DMA_Init();
    UART4_RX_DMA_Init();
}

