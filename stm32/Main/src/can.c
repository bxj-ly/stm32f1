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
/*******************************************************************************
 * �ļ���  ��can.c
 * ����    ����printf�����ض���USART1�������Ϳ�����printf��������Ƭ��������
 *           ��ӡ��PC�ϵĳ����ն˻򴮿ڵ������֡�         
 * Ӳ�����ӣ�------------------------
 *          |     PB12-CAN-RX      |
 *          |     PB13-CAN-TX      |
 *           ------------------------
**********************************************************************************/
#include <string.h>
#include <stdio.h>
#include "can.h" 

CanTxMsg TxMessage;           //���ͻ�����
CanRxMsg RxMessage;         //���ջ�����

__IO uint8_t RxMsgReceived = 0;
  

/*
 * ��������CAN_NVIC_Config
 * ����  ��CAN��NVIC ����,��1���ȼ��飬0��0���ȼ�
 * ����  ����
 * ���  : ��
 * ����  ���ڲ�����
 */
static void CAN_NVIC_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  /*�ж�����*/

  NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn;     //CAN2 RX0�ж�
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;       //��ռ���ȼ�0
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;         //�����ȼ�Ϊ0
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

/*
 * ��������CAN_Mode_Config
 * ����  ��CAN��ģʽ ����
 * ����  ����
 * ���  : ��
 * ����  ���ڲ�����
 */
static void CAN_Mode_Config(void)
{
  CAN_InitTypeDef        CAN_InitStructure;
  /************************CANͨ�Ų�������**********************************/
  /*CAN�Ĵ�����ʼ��*/
  //CAN_DeInit(CAN2);
  CAN_StructInit(&CAN_InitStructure);
  /*CAN��Ԫ��ʼ��*/
  CAN_InitStructure.CAN_TTCM=DISABLE;         //MCR-TTCM  �ر�ʱ�䴥��ͨ��ģʽʹ��
  CAN_InitStructure.CAN_ABOM=DISABLE;         //MCR-ABOM  �Զ����߹��� 
  CAN_InitStructure.CAN_AWUM=DISABLE;         //MCR-AWUM  ʹ���Զ�����ģʽ
  CAN_InitStructure.CAN_NART=ENABLE;         //MCR-NART  ��ֹ�����Զ��ش�    DISABLE-�Զ��ش�
  CAN_InitStructure.CAN_RFLM=DISABLE;         //MCR-RFLM  ����FIFO ����ģʽ  DISABLE-���ʱ�±��ĻḲ��ԭ�б���  
  CAN_InitStructure.CAN_TXFP=ENABLE;         //MCR-TXFP  ����FIFO���ȼ� DISABLE-���ȼ�ȡ���ڱ��ı�ʾ�� 
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;  //��������ģʽ
  CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;       //BTR-SJW ����ͬ����Ծ��� 2��ʱ�䵥Ԫ
  CAN_InitStructure.CAN_BS1=CAN_BS1_4tq;       //BTR-TS1 ʱ���1 ռ����6��ʱ�䵥Ԫ
  CAN_InitStructure.CAN_BS2=CAN_BS2_3tq;       //BTR-TS1 ʱ���2 ռ����3��ʱ�䵥Ԫ
  CAN_InitStructure.CAN_Prescaler =9;       ////�����ʷ�Ƶ�� 9Ϊ500Kbps, 18Ϊ250Kbps
  CAN_Init(CAN2, &CAN_InitStructure);
}

/*
 * ��������CAN_Filter_Config
 * ����  ��CAN�Ĺ����� ����
 * ����  ����
 * ���  : ��
 * ����  ���ڲ�����
 */
static void CAN_Filter_Config(void)
{
  CAN_FilterInitTypeDef  CAN_FilterInitStructure;

  /*CAN��������ʼ��*/
  CAN_FilterInitStructure.CAN_FilterNumber=14;            //��������0
  CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;  //�����ڱ�ʶ������λģʽ
  CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;  //������λ��Ϊ����32λ��
  /* ʹ�ܱ��ı�ʾ�����������ձ�ʾ�������ݽ��бȶԹ��ˣ���չID�������µľ����������ǵĻ��������FIFO0�� */

  CAN_FilterInitStructure.CAN_FilterIdHigh= 0x0000;        //Ҫ���˵�ID��λ 
  CAN_FilterInitStructure.CAN_FilterIdLow= 0x0000; //Ҫ���˵�ID��λ 
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh= 0x0000;      
  CAN_FilterInitStructure.CAN_FilterMaskIdLow= 0x0000;      
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0 ;        //��������������FIFO0
  CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;      //ʹ�ܹ�����
  CAN_FilterInit(&CAN_FilterInitStructure);
  /*CANͨ���ж�ʹ��*/
  CAN_ITConfig(CAN2, CAN_IT_FMP0, ENABLE);
}


/*
 * ��������CAN_Config
 * ����  ����������CAN�Ĺ���
 * ����  ����
 * ���  : ��
 * ����  ���ⲿ����
 */
void CAN_Config(void)
{
  CAN_NVIC_Config();
  CAN_Mode_Config();
  CAN_Filter_Config();   
}

/*
 * ��������CAN_SetMsg
 * ����  ��CANͨ�ű�����������
 * ����  ����
 * ���  : ��
 * ����  ���ⲿ����
 */   
void CAN_SetMsg(uint8_t Cmd)
{   
  memset(&TxMessage,0x00, sizeof(CanTxMsg));
  TxMessage.StdId=CAN_CHK_STDID;             
  //TxMessage.ExtId=0x1314;           //ID
  TxMessage.IDE=CAN_ID_STD;           //IDģʽ
  TxMessage.RTR=CAN_RTR_DATA;         //���͵�������
  TxMessage.DLC=8;               //���ݳ����ֽ�

  TxMessage.Data[0] = CAN_CHK_CMD_LEN;
  TxMessage.Data[1] = CAN_CHK_CMD_H;
  TxMessage.Data[2] = Cmd;

  CAN_Transmit(CAN2, &TxMessage) ;
  // ����Ƿ��ͺ�ECU�ظ�ʱ����һ�����͵������Ǻ������Ч���ݳ��ȣ�Ȼ����ǲ�ѯָ���ָ�ָ��
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/
void CAN2_RX0_IRQHandler(void)
{ 
  RxMsgReceived = 1; 
      CAN_Receive(CAN2, CAN_FIFO0, &RxMessage);

      if(CAN_RESP_STDID == RxMessage.StdId)
      {
      }
printf("\r\n ��׼ID��StdId��0x%x",RxMessage.StdId);	
 printf("\r\n ���ݶε�����:Data[0]= 0x%x ��Data[1]=0x%x ��Data[2]=0x%x��Data[3]=0x%x��Data[4]=0x%x��Data[5]=0x%x��Data[6]=0x%x��Data[7]=0x%x\r\n",RxMessage.Data[0],RxMessage.Data[1],RxMessage.Data[2],RxMessage.Data[3],RxMessage.Data[4],RxMessage.Data[5],RxMessage.Data[6],RxMessage.Data[7]);

      switch(RxMessage.Data[3])
      {
      case CAN_RPM:
        printf("\r\n RPM(/min):%u",RxMessage.Data[4]); break;       
      case CAN_VVS:  
        printf("\r\n VVS(Km/h):%d",RxMessage.Data[4]); break;   
      case CAN_ECT: 
        printf("\r\n ECT('C):%d",RxMessage.Data[4]); break;      
      case CAN_MAF:  
        printf("\r\n MAF(g/s):%d",RxMessage.Data[4]); break;     
      case CAN_MAP:      
        printf("\r\n MAP(kPa):%d",RxMessage.Data[4]); break; 
      case CAN_TP:       
        printf("\r\n TP(percent):%d",RxMessage.Data[4]); break; 
      case CAN_O2B1S1:   
        printf("\r\n O2B1S1(V):%d",RxMessage.Data[4]); break; 
      case CAN_LOAD_PCT: 
        printf("\r\n LOAD_PCT(percent):%d",RxMessage.Data[4]); break; 
      default: break;
      }	
}


