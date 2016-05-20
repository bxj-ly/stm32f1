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
#include <stdarg.h>
#include <string.h>
#include "usart1.h"
#include "uart4.h"
#include "debug.h"

uint8_t uart1_dma_receivebuffer[UART1_DMA_RCVBUFFER_SIZE];



void USART1_NVIC_Config(void){
	
    NVIC_InitTypeDef NVIC_InitStructure; 
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
}	
/* USART1 Config
- BaudRate = 115200 baud
- Word Length = 8 Bits
- One Stop Bit 
- No parity 
- Hardware flow control disabled (RTS and CTS signals)  
- Receive and transmit enabled 
*/
void USART1_Config(void)
{
    USART_InitTypeDef USART_InitStructure;


    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);  

    /* USART1 mode config */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure); 

    USART_Cmd(USART1, ENABLE);

    USART1_NVIC_Config();

}



int USART1_TX_DMA_Send(void * src, size_t len)
{
    #define MAX_FAIL_TIME 100000
    int failcnt = 0;
    // wait for last dma tx finished
    while(DMA1_Channel4->CCR & DMA_CCR1_EN && failcnt++<MAX_FAIL_TIME);
    

    DMA1_Channel4->CMAR = (uint32_t)src;
    DMA_SetCurrDataCounter(DMA1_Channel4, len);
    DMA_Cmd (DMA1_Channel4,ENABLE);	
    return failcnt;
}

int fputc(int ch, FILE *f)
{
    while (!(USART1->SR & USART_FLAG_TXE));
    USART_SendData(USART1, (unsigned char) ch);
    while (!(USART1->SR & USART_FLAG_TXE)); 
    return (ch);
}





/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/
void USART1_IRQHandler(void)
{
    DEBUG_SetUSART1IntCnt();

    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        USART1->SR;
        USART1->DR;
        //USART1_RX_MSG_Proc();

    }
    // after start dma , will not use RXNE interrupt
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {   

        //dma15_len = DMA_GetCurrDataCounter(DMA1_Channel5);
        //INFO("%c",c);
    }

}

void DMA1_Channel4_IRQHandler(void)
{	
	DEBUG_SetDMA1Channel4IntCnt();
	if(DMA_GetFlagStatus(DMA1_FLAG_TC4)==SET) 
	{  
		DMA_ClearFlag(DMA1_FLAG_TC4);
		DMA_Cmd (DMA1_Channel4,DISABLE);			
	}	
}

void DMA1_Channel5_IRQHandler(void)
{	
	DEBUG_SetDMA1Channel5IntCnt();
	if(DMA_GetFlagStatus(DMA1_FLAG_TC5)==SET) 
	{  		
		DMA_ClearFlag(DMA1_FLAG_TC5); 
	}	
	if(DMA_GetFlagStatus(DMA1_FLAG_HT5)==SET) 
	{  
		DMA_ClearFlag(DMA1_FLAG_HT5); 
	}	
	if(DMA_GetFlagStatus(DMA1_FLAG_TE5)==SET) 
	{  
		DMA_ClearFlag(DMA1_FLAG_TE5); 
	}		
}

void USART1_TX_DMA_NVIC_Config(void){
	
    NVIC_InitTypeDef NVIC_InitStructure; 
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	
    
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DMA_ITConfig(DMA1_Channel4,DMA_IT_TC,ENABLE);
}
	
static void USART1_TX_DMA_Init(void)
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_DeInit(DMA1_Channel4); 
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;	   
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
	DMA_Init(DMA1_Channel4, &DMA_InitStructure); 	
    DMA_Cmd (DMA1_Channel4,DISABLE);				
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);		
		
    USART1_TX_DMA_NVIC_Config();
}

void USART1_RX_DMA_NVIC_Config(void){
    NVIC_InitTypeDef NVIC_InitStructure; 
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	    

    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);	

    DMA_ITConfig(DMA1_Channel5, DMA_IT_TC | DMA_IT_HT , ENABLE); 	   
}

static void USART1_RX_DMA_Init(void)
{
	DMA_InitTypeDef DMA_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
    DMA_DeInit(DMA1_Channel5); 
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR; 
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)uart1_dma_receivebuffer;  
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; 
    DMA_InitStructure.DMA_BufferSize = UART1_DMA_RCVBUFFER_SIZE; 
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte; 
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; 
    DMA_InitStructure.DMA_Priority = DMA_Priority_High; 
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);   
    DMA_Cmd(DMA1_Channel5, ENABLE); 	
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

    USART1_RX_DMA_NVIC_Config();
}

void USART1_DMA_Config(void)
{
	USART1_TX_DMA_Init();
	USART1_RX_DMA_Init();
}
