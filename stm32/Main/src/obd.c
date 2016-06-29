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
#include <stdio.h>
#include "debug.h"
#include "systick.h"
#include "obd.h"
#include "formula.h"

/************************************************************************
  * @
  * @描述: 底层全局变量
  * @
  **********************************************************************/
__IO char         DTCRAM[200];

uint8_t can_id_type;

void CAN_NVIC_Config(void);
void CAN_GPIOConfig(void);
void CAN_Config32BitFilter(uint32_t id1, uint32_t id2, uint32_t mid1, uint32_t mid2);
void CAN_Config16BitFilter(uint16_t id1, uint16_t id2, uint16_t mid1, uint16_t mid2);
void SaveData(CanRxMsg* RxMessage);


/************************************************************************
  * @描述:  CAN速率配置
  * @参数:  velocity 速率分频值
  * @返回值: None
  **********************************************************************/
void CAN_Config(uint8_t velocity, uint8_t can_id_type)
{
    CAN_InitTypeDef  CAN_InitStructure;

    can_id_type = can_id_type;

    CAN_GPIOConfig();
    CAN_DeInit(CAN1);
    CAN_DeInit(CAN2);

    CAN_StructInit(&CAN_InitStructure);

    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = DISABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
    CAN_InitStructure.CAN_BS1 = CAN_BS1_11tq;
    CAN_InitStructure.CAN_BS2 = CAN_BS2_6tq;
    CAN_InitStructure.CAN_Prescaler = velocity;
    CAN_Init(CANx, &CAN_InitStructure);

    if (can_id_type == CAN_ID_STD)
    {
        CAN_Config16BitFilter(0x7e8,0x7e8,0x7e0,0x7e0);
    }
    else
    {
        CAN_Config32BitFilter(0x18DAF110,0x18DAF110,0x1FFFF100,0x1FFFF100);
    }  
}


/************************************************************************
  * @描述:  ISO15765 读取当前故障码
  * @参数:  ErrorStatus* err
  * @返回值: char* 故障码号地址
  **********************************************************************/
char* CAN_ReadDTC(CanTxMsg* DTCCmd,ErrorStatus* err)
{
  uint8_t *ram,i;
  uint16_t dtc;
  ClearRAM((uint8_t*)DTCRAM,200);
  ram = Send_CANFrame(DTCCmd,err);
  if (*err == SUCCESS)
  {
    for(i = 0;i < ram[2];i++)
	{
	  dtc = (uint16_t)(*(ram+3+2*i)<<8 | *(ram+4+2*i));
	  if (dtc != 0)
	  {
		strcpy((char*)(DTCRAM+strlen((char*)DTCRAM)),PCBU(dtc));
        if( i != ram[2] - 1) strcpy((char*)(DTCRAM+strlen((char*)DTCRAM)),",");
	  }
	}
  }
  return (char*)DTCRAM;
}


void CAN_NVIC_Config(void)
{
    NVIC_InitTypeDef  NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);

    NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn; 
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


/************************************************************************
  * @描述:   CAN所用IO引脚配置
  * @参数:   None
  * @返回值: None
  **********************************************************************/
void CAN_GPIOConfig(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APBxPeriph_CAN_IO | RCC_APB2Periph_AFIO,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2,ENABLE);
    GPIO_InitStructure.GPIO_Pin = CAN_RXD;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(CAN_IO, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = CAN_TXD;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(CAN_IO, &GPIO_InitStructure);
    if (CAN_PinRemap == ENABLE)
    { 
        GPIO_PinRemapConfig(GPIO_Remap1_CAN1,ENABLE);
    }
}
/************************************************************************
  * @描述:  CAN扩展帧滤波器设置
  * @参数:  id1,id2 效验码   mid1,mid2 屏蔽码
  * @返回值: None
  **********************************************************************/
void CAN_Config32BitFilter(uint32_t id1, uint32_t id2, uint32_t mid1, uint32_t mid2)
{
  CAN_FilterInitTypeDef  CAN_FilterInitStructure;
  CAN_FilterInitStructure.CAN_FilterNumber = 14;
  CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;  
  CAN_FilterInitStructure.CAN_FilterIdHigh = id1>>13;
  CAN_FilterInitStructure.CAN_FilterIdLow = (id1<<3)|4;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh = mid1>>13;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = (mid1<<3)|4;
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
  CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);
  CAN_FilterInitStructure.CAN_FilterNumber = 15;
  CAN_FilterInitStructure.CAN_FilterIdHigh = id2>>13;
  CAN_FilterInitStructure.CAN_FilterIdLow = (id2<<3)|4;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh = mid2>>13;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = (mid2<<3)|4;
  CAN_FilterInit(&CAN_FilterInitStructure);
  CAN_NVIC_Config();
  CAN_ITConfig(CANx, CAN_IT_FMP0, ENABLE);
}
/************************************************************************
  * @描述:  CAN标准帧滤波器设置
  * @参数:  id1,id2 效验码   mid1,mid2 屏蔽码
  * @返回值: None
  **********************************************************************/
void CAN_Config16BitFilter(uint16_t id1, uint16_t id2, uint16_t mid1, uint16_t mid2)                                                                             
{                                                                                                                         
    CAN_FilterInitTypeDef  CAN_FilterInitStructure;                                                                       
                                                                                                                          
    CAN_FilterInitStructure.CAN_FilterNumber=15;                                                                           
    CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;                                                         
    CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_16bit;                                                        
    CAN_FilterInitStructure.CAN_FilterIdHigh=id1<<5;                                                                      
    CAN_FilterInitStructure.CAN_FilterIdLow=id2<<5;                                                                       
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh=mid1<<5;                                                                  
    CAN_FilterInitStructure.CAN_FilterMaskIdLow=mid2<<5;                                                                   
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_FIFO0;                                                           
    CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;                                                                                                                                                                                       
    CAN_FilterInit(&CAN_FilterInitStructure);
	CAN_NVIC_Config();
	CAN_ITConfig(CANx, CAN_IT_FMP0, ENABLE);                                                                             
}

/************************************************************************
  * @描述:  发送一帧CAN数据(基于汽车诊断)
  * @参数:  
  * @返回值: None
  **********************************************************************/
__IO ErrorStatus RxFlay;
__IO uint8_t RxRAM[100];
uint8_t* Send_CANFrame(CanTxMsg* TxMessage,ErrorStatus* err)
{
 // uint8_t TransmitMailbox;
  uint32_t i;
  RxFlay = ERROR;
  TxMessage->IDE = can_id_type;
  CAN_Transmit(CANx, TxMessage);
 // while(CAN_TransmitStatus(CANx,TransmitMailbox) != CANTXOK);
  for (i = 0;i < 10000000/3;i++)	//大概等待1.5秒
  {
    if (RxFlay == SUCCESS) break;
  }
  *err = RxFlay;
  return (uint8_t*)&RxRAM[0];
}

/************************************************************************
  * @描述:  PCBU码的ASCII转换
  * @参数:   dtc u16的数据
  * @返回值: char*  PCBU的ASCII码
  **********************************************************************/
char PCBUCode[6];
char* PCBU(uint16_t dtc)
{
  if ( dtc > 0 && dtc < 0x4000)
  {
    strcpy((char*)PCBUCode,"P");
	sprintf((char*)(PCBUCode + strlen((char*)PCBUCode)),"%04x",dtc);
  }
  else if ( dtc > 0x4000 && dtc < 0x8000)
  {
    strcpy((char*)PCBUCode,"C");							 
	sprintf((char*)(PCBUCode + strlen((char*)PCBUCode)),"%04x",dtc - 0x4000);
  }
  else if (dtc > 0x8000 && dtc < 0xc000)
  {
    strcpy((char*)PCBUCode,"B");
	sprintf((char*)(PCBUCode + strlen((char*)PCBUCode)),"%04x",dtc - 0x8000);
  }
  else if (dtc > 0xc000 && dtc < 0xffff)
  {
    strcpy((char*)PCBUCode,"U");
	sprintf((char*)(PCBUCode + strlen((char*)PCBUCode)),"%04x",dtc - 0xc000);
  }
  return &PCBUCode[0];
}
/************************************************************************
  * @描述:  清空内存
  * @参数:   uint8_t* ram 需要清空的内存指针 uint32_t n 清空内存的大小
  * @返回值: NONE
  **********************************************************************/
void ClearRAM(uint8_t* ram,uint32_t n)
{
  uint32_t i;
  for (i = 0;i < n;i++)
  {
    ram[i] = 0x00;
  }
}

void CAN_IRQHandler(void)
{
  CanRxMsg RxMessage;

  CAN_Receive(CANx, CAN_FIFO0, &RxMessage); 
  SaveData(&RxMessage);
}

__IO uint8_t FI = 0;
__IO uint8_t FLCAN = 0;
void SaveData(CanRxMsg* RxMessage)
{
  uint8_t i,j,TransmitMailbox;
  CanTxMsg Cmd30H = {0x7DF,0x18DB33F1,CAN_ID_STD,CAN_RTR_DATA,8,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  if (RxMessage->Data[0] == 0x10)
  {
    Cmd30H.IDE = RxMessage->IDE;
    TransmitMailbox = CAN_Transmit(CANx, &Cmd30H);
    while(CAN_TransmitStatus(CANx,TransmitMailbox) != CANTXOK);
    if ((RxMessage->Data[1]+1)%7 > 0)
    {
      FLCAN = (RxMessage->Data[1]+1)/7 + 1;
    }
    else
    {
      FLCAN = (RxMessage->Data[1]+1)/7;
    }
    FI = 0;
  }
  for(i = 0; i < 8; i++)
  {
    RxRAM[i+FI*8] = RxMessage->Data[i];
  }
  if (FLCAN != 0)
  {
    FI++;
    if (FLCAN  <= FI)
    {
      FI = 0;
      for (j = 0; j < FLCAN; j++)
      {
        for (i = 0; i < 7; i++)
        {
          RxRAM[i+7*j] = RxRAM[i+1+7*j+j];
        }
      }
      RxFlay = SUCCESS;
      FLCAN = 0;
      FI = 0;
    }
  }
  else
  {
    RxFlay = SUCCESS;
    FLCAN = 0;
    FI = 0;
  }
}


