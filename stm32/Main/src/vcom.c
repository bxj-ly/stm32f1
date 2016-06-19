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
#include "systick.h"
#include "debug.h"
#include "vcom.h"

#define _300BuadRate	3150
#define _600BuadRate	1700
#define _1200BuadRate	800
#define _10400BuadRate  96

#define COM_TX_PORT	GPIOA
#define COM_TX_PIN	GPIO_Pin_12
#define COM_DATA_HIGH()	GPIO_SetBits(COM_TX_PORT, COM_TX_PIN)
#define COM_DATA_LOW()	GPIO_ResetBits(COM_TX_PORT, COM_TX_PIN)

static void VirtualCOM_Config(uint16_t baudRate);
static void VirtualCOM_TX_GPIOConfig(void);
static void VirtualCOM_RX_GPIOConfig(void);
static void TIM3_Configuration(uint16_t period);
static void Delay(uint32_t t);


static uint32_t delayTime; 

void VCOM_Config(void)
{
    VirtualCOM_Config(_10400BuadRate);
}

void VCOM_SendByte(uint8_t val)
{
	uint8_t i = 0;
	COM_DATA_LOW();
	SysTick_Delay_us(delayTime); 
	for(i = 0; i < 8; i++)
	{
		if(val & 0x01)
			COM_DATA_HIGH();
		else
			COM_DATA_LOW();
		SysTick_Delay_us(delayTime);
		val >>= 1;
	}
	COM_DATA_HIGH();
	SysTick_Delay_us(delayTime); 
}

void VCOM_SendString(uint8_t *str) 
{
	while(*str != 0)
	{
		VCOM_SendByte(*str);
		str++;
	}
}


void VirtualCOM_Config(uint16_t baudRate)
{
	uint32_t period;
	VirtualCOM_TX_GPIOConfig();
	VirtualCOM_RX_GPIOConfig();
	if(baudRate == _300BuadRate)
		period = _300BuadRate + 250;
	else if (baudRate == _600BuadRate)
		period =  _600BuadRate + 50;
	else if (baudRate == _1200BuadRate)
		period =  _1200BuadRate + 50;
	else if (baudRate == _10400BuadRate)
		period =  _10400BuadRate + 0;
	TIM3_Configuration(period);		
	delayTime = baudRate;
}


static void Delay(uint32_t t)
{
	while(t--);
}

static void VirtualCOM_TX_GPIOConfig(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = COM_TX_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(COM_TX_PORT, &GPIO_InitStructure);
	GPIO_SetBits(COM_TX_PORT, COM_TX_PIN);
}


#define COM_RX_PORT	GPIOA
#define COM_RX_PIN	GPIO_Pin_11
static void VirtualCOM_RX_GPIOConfig(void) 
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStruct;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin = COM_RX_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(COM_RX_PORT, &GPIO_InitStructure);
	
	EXTI_InitStruct.EXTI_Line=EXTI_Line11;					
	EXTI_InitStruct.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger=EXTI_Trigger_Falling;
	EXTI_InitStruct.EXTI_LineCmd=ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	NVIC_InitStructure.NVIC_IRQChannel=EXTI15_10_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

static void TIM3_Configuration(uint16_t period)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_DeInit(TIM3);	
	TIM_InternalClockConfig(TIM3);

	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = period - 1;  
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);	

	TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM3,DISABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;	
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);	
}

enum{
	COM_START_BIT,		
	COM_D0_BIT,			 //bit0
	COM_D1_BIT,			 //bit1
	COM_D2_BIT,			 //bit2
	COM_D3_BIT,			 //bit3
	COM_D4_BIT,			 //bit4
	COM_D5_BIT,			 //bit5
	COM_D6_BIT,			 //bit6
	COM_D7_BIT,			 //bit7
	COM_STOP_BIT,
};

uint8_t recvStat = COM_STOP_BIT;   

#define COM_RX_STAT	GPIO_ReadInputDataBit(COM_RX_PORT, COM_RX_PIN)
void EXTI15_10_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line11)!=RESET)
	{
		if(!COM_RX_STAT)   
		{
			if(recvStat == COM_STOP_BIT)
			{
				recvStat = COM_START_BIT;
				Delay(100);       
				TIM_Cmd(TIM3, ENABLE);
			}
		}
		EXTI_ClearITPendingBit(EXTI_Line11);
	}
}

static uint8_t recvData;

void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM3 , TIM_FLAG_Update);
		recvStat++;				  
		if(recvStat == COM_STOP_BIT) 
		{		  
			TIM_Cmd(TIM3, DISABLE);
			return; 
		}
		if(COM_RX_STAT)
		{
			recvData |= (1 << (recvStat - 1));
		}
		else				          //'0'
		{
			recvData &= ~(1 <<(recvStat - 1));
		}
	}
}


#if defined(VIRTUALCOM_AS_DEBUG_COM)
int fputc(int ch, FILE *f)
{
    VCOM_SendByte(ch);
    return (ch);
}
#endif

