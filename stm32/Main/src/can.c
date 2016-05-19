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
#include "can.h" 

#define CAN_CHK_STDID        0x07df
#define CAN_RESP_STDID       0x07E8

#define CAN_CHK_CMD_LEN      2
#define CAN_CHK_CMD_H        0x01
#define CAN_CHK_RESP_H       0x04

/* engine Revolutions Per Minute */
static uint16_t can_car_RPM;
/* Vehicle Speed Sensor */
static uint16_t can_car_VSS;
/* Engine Coolant Temperature */
static uint16_t can_car_ECT;
/* Mass Air Flow */
static uint16_t can_car_MAF;
/* Manifold Absolute Pressure */
static uint16_t can_car_MAP;
/* Throttle Positioner */
static uint16_t can_car_TP;
/* O2 sensor B1S1 */
static uint16_t can_car_O2B1S1;  
/* Load Percent */
static uint16_t can_car_load_PCT;

static CanTxMsg can_tx_msg;
static CanRxMsg can_rx_msg;

static __IO uint32_t can_rx_int_cnt = 0;
  
static void CAN_GPIO_Config(void);
static void CAN_NVIC_Config(void);
static void CAN_Mode_Config(void);
static void CAN_Filter_Config(void);

void CAN_Config(void)
{
  CAN_NVIC_Config();
  CAN_Mode_Config();
  CAN_Filter_Config();   
}
 
void CAN_CheckStatus(E_CAN_CMD Cmd)
{   
  memset(&can_tx_msg,0x00, sizeof(CanTxMsg));
  can_tx_msg.StdId=CAN_CHK_STDID;             
  //can_tx_msg.ExtId=0x1314;
  can_tx_msg.IDE=CAN_ID_STD;
  can_tx_msg.RTR=CAN_RTR_DATA;
  can_tx_msg.DLC=8;

  can_tx_msg.Data[0] = CAN_CHK_CMD_LEN;
  can_tx_msg.Data[1] = CAN_CHK_CMD_H;
  can_tx_msg.Data[2] = Cmd;

  CAN_Transmit(CAN2, &can_tx_msg) ;
}

uint16_t CAN_GetStatus(E_CAN_CMD Cmd)
{
  switch(can_rx_msg.Data[3])
  {
  case CAN_RPM:
    return can_car_RPM;
  case CAN_VSS:  
    return can_car_VSS;
  case CAN_ECT: 
    return can_car_ECT;
  case CAN_MAF:  
    return can_car_MAF;
  case CAN_MAP:   
    return can_car_MAP;
  case CAN_TP:  
    return can_car_TP;
  case CAN_O2B1S1:  
    return can_car_O2B1S1; 
  case CAN_LOAD_PCT: 
    return can_car_load_PCT;
  default: 
    break;
  } 

  return 0xFFFF;
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/
void CAN2_RX0_IRQHandler(void)
{ 
    can_rx_int_cnt++; 
    CAN_Receive(CAN2, CAN_FIFO0, &can_rx_msg);

    if(CAN_RESP_STDID == can_rx_msg.StdId)
    {
      DEBUG("\r\n 标准ID号StdId：0x%x",can_rx_msg.StdId);	
      DEBUG("\r\n 数据段的内容:Data[0]= 0x%x ，Data[1]=0x%x ，Data[2]=0x%x，Data[3]=0x%x，Data[4]=0x%x，Data[5]=0x%x，Data[6]=0x%x，Data[7]=0x%x\r\n",
        can_rx_msg.Data[0],
        can_rx_msg.Data[1],
        can_rx_msg.Data[2],
        can_rx_msg.Data[3],
        can_rx_msg.Data[4],
        can_rx_msg.Data[5],
        can_rx_msg.Data[6],
        can_rx_msg.Data[7]);

      switch(can_rx_msg.Data[3])
      {
      case CAN_RPM:
        can_car_RPM = can_rx_msg.Data[4];
        //printf("\r\n RPM(/min):%u",can_rx_msg.Data[4]); break;   
        break;
      case CAN_VSS:  
        can_car_VSS = can_rx_msg.Data[4] << 8 | can_rx_msg.Data[5];
        //printf("\r\n VVS(Km/h):%d",can_rx_msg.Data[4]); 
        break;   
      case CAN_ECT: 
        can_car_ECT = can_rx_msg.Data[4];
        //printf("\r\n ECT('C):%d",can_rx_msg.Data[4]); 
        break;      
      case CAN_MAF:  
        can_car_MAF = can_rx_msg.Data[4];
        //printf("\r\n MAF(g/s):%d",can_rx_msg.Data[4]); 
        break;     
      case CAN_MAP:   
        can_car_MAP = can_rx_msg.Data[4];
        //printf("\r\n MAP(kPa):%d",can_rx_msg.Data[4]); 
        break; 
      case CAN_TP:  
        can_car_TP = can_rx_msg.Data[4];
        //printf("\r\n TP(percent):%d",can_rx_msg.Data[4]); 
        break; 
      case CAN_O2B1S1:  
        can_car_O2B1S1 = can_rx_msg.Data[4];
        //printf("\r\n O2B1S1(V):%d",can_rx_msg.Data[4]); 
        break; 
      case CAN_LOAD_PCT: 
        can_car_load_PCT = can_rx_msg.Data[4];
        //printf("\r\n LOAD_PCT(percent):%d",can_rx_msg.Data[4]); 
        break; 
      default: 
        break;
      }	    
    }
}


static void CAN_NVIC_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn; 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

static void CAN_Mode_Config(void)
{
  CAN_InitTypeDef        CAN_InitStructure;

  //CAN_DeInit(CAN2);
  CAN_StructInit(&CAN_InitStructure);

  CAN_InitStructure.CAN_TTCM=DISABLE;
  CAN_InitStructure.CAN_ABOM=DISABLE;
  CAN_InitStructure.CAN_AWUM=DISABLE;
  CAN_InitStructure.CAN_NART=ENABLE;
  CAN_InitStructure.CAN_RFLM=DISABLE;
  CAN_InitStructure.CAN_TXFP=ENABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
  CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;
  CAN_InitStructure.CAN_BS1=CAN_BS1_4tq;
  CAN_InitStructure.CAN_BS2=CAN_BS2_3tq;
  CAN_InitStructure.CAN_Prescaler =9;
  CAN_Init(CAN2, &CAN_InitStructure);
}

static void CAN_Filter_Config(void)
{
  CAN_FilterInitTypeDef  CAN_FilterInitStructure;

  CAN_FilterInitStructure.CAN_FilterNumber=14;
  CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; 

  CAN_FilterInitStructure.CAN_FilterIdHigh= 0x0000;
  CAN_FilterInitStructure.CAN_FilterIdLow= 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh= 0x0000;      
  CAN_FilterInitStructure.CAN_FilterMaskIdLow= 0x0000;      
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0 ;
  CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);
  CAN_ITConfig(CAN2, CAN_IT_FMP0, ENABLE);
}


