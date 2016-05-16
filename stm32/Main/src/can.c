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
 * 文件名  ：can.c
 * 描述    ：将printf函数重定向到USART1。这样就可以用printf函数将单片机的数据
 *           打印到PC上的超级终端或串口调试助手。         
 * 硬件连接：------------------------
 *          |     PB12-CAN-RX      |
 *          |     PB13-CAN-TX      |
 *           ------------------------
**********************************************************************************/
#include <string.h>
#include <stdio.h>
#include "can.h" 

CanTxMsg TxMessage;           //发送缓冲区
CanRxMsg RxMessage;         //接收缓冲区

__IO uint8_t RxMsgReceived = 0;
  

/*
 * 函数名：CAN_NVIC_Config
 * 描述  ：CAN的NVIC 配置,第1优先级组，0，0优先级
 * 输入  ：无
 * 输出  : 无
 * 调用  ：内部调用
 */
static void CAN_NVIC_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  /*中断设置*/

  NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn;     //CAN2 RX0中断
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;       //抢占优先级0
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;         //子优先级为0
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

/*
 * 函数名：CAN_Mode_Config
 * 描述  ：CAN的模式 配置
 * 输入  ：无
 * 输出  : 无
 * 调用  ：内部调用
 */
static void CAN_Mode_Config(void)
{
  CAN_InitTypeDef        CAN_InitStructure;
  /************************CAN通信参数设置**********************************/
  /*CAN寄存器初始化*/
  //CAN_DeInit(CAN2);
  CAN_StructInit(&CAN_InitStructure);
  /*CAN单元初始化*/
  CAN_InitStructure.CAN_TTCM=DISABLE;         //MCR-TTCM  关闭时间触发通信模式使能
  CAN_InitStructure.CAN_ABOM=DISABLE;         //MCR-ABOM  自动离线管理 
  CAN_InitStructure.CAN_AWUM=DISABLE;         //MCR-AWUM  使用自动唤醒模式
  CAN_InitStructure.CAN_NART=ENABLE;         //MCR-NART  禁止报文自动重传    DISABLE-自动重传
  CAN_InitStructure.CAN_RFLM=DISABLE;         //MCR-RFLM  接收FIFO 锁定模式  DISABLE-溢出时新报文会覆盖原有报文  
  CAN_InitStructure.CAN_TXFP=ENABLE;         //MCR-TXFP  发送FIFO优先级 DISABLE-优先级取决于报文标示符 
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;  //正常工作模式
  CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;       //BTR-SJW 重新同步跳跃宽度 2个时间单元
  CAN_InitStructure.CAN_BS1=CAN_BS1_4tq;       //BTR-TS1 时间段1 占用了6个时间单元
  CAN_InitStructure.CAN_BS2=CAN_BS2_3tq;       //BTR-TS1 时间段2 占用了3个时间单元
  CAN_InitStructure.CAN_Prescaler =9;       ////波特率分频器 9为500Kbps, 18为250Kbps
  CAN_Init(CAN2, &CAN_InitStructure);
}

/*
 * 函数名：CAN_Filter_Config
 * 描述  ：CAN的过滤器 配置
 * 输入  ：无
 * 输出  : 无
 * 调用  ：内部调用
 */
static void CAN_Filter_Config(void)
{
  CAN_FilterInitTypeDef  CAN_FilterInitStructure;

  /*CAN过滤器初始化*/
  CAN_FilterInitStructure.CAN_FilterNumber=14;            //过滤器组0
  CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;  //工作在标识符屏蔽位模式
  CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;  //过滤器位宽为单个32位。
  /* 使能报文标示符过滤器按照标示符的内容进行比对过滤，扩展ID不是如下的就抛弃掉，是的话，会存入FIFO0。 */

  CAN_FilterInitStructure.CAN_FilterIdHigh= 0x0000;        //要过滤的ID高位 
  CAN_FilterInitStructure.CAN_FilterIdLow= 0x0000; //要过滤的ID低位 
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh= 0x0000;      
  CAN_FilterInitStructure.CAN_FilterMaskIdLow= 0x0000;      
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0 ;        //过滤器被关联到FIFO0
  CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;      //使能过滤器
  CAN_FilterInit(&CAN_FilterInitStructure);
  /*CAN通信中断使能*/
  CAN_ITConfig(CAN2, CAN_IT_FMP0, ENABLE);
}


/*
 * 函数名：CAN_Config
 * 描述  ：完整配置CAN的功能
 * 输入  ：无
 * 输出  : 无
 * 调用  ：外部调用
 */
void CAN_Config(void)
{
  CAN_NVIC_Config();
  CAN_Mode_Config();
  CAN_Filter_Config();   
}

/*
 * 函数名：CAN_SetMsg
 * 描述  ：CAN通信报文内容设置
 * 输入  ：无
 * 输出  : 无
 * 调用  ：外部调用
 */   
void CAN_SetMsg(uint8_t Cmd)
{   
  memset(&TxMessage,0x00, sizeof(CanTxMsg));
  TxMessage.StdId=CAN_CHK_STDID;             
  //TxMessage.ExtId=0x1314;           //ID
  TxMessage.IDE=CAN_ID_STD;           //ID模式
  TxMessage.RTR=CAN_RTR_DATA;         //发送的是数据
  TxMessage.DLC=8;               //数据长度字节

  TxMessage.Data[0] = CAN_CHK_CMD_LEN;
  TxMessage.Data[1] = CAN_CHK_CMD_H;
  TxMessage.Data[2] = Cmd;

  CAN_Transmit(CAN2, &TxMessage) ;
  // 诊断仪发送和ECU回复时，第一个发送的数据是后面的有效数据长度，然后才是查询指令或恢复指令
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
printf("\r\n 标准ID号StdId：0x%x",RxMessage.StdId);	
 printf("\r\n 数据段的内容:Data[0]= 0x%x ，Data[1]=0x%x ，Data[2]=0x%x，Data[3]=0x%x，Data[4]=0x%x，Data[5]=0x%x，Data[6]=0x%x，Data[7]=0x%x\r\n",RxMessage.Data[0],RxMessage.Data[1],RxMessage.Data[2],RxMessage.Data[3],RxMessage.Data[4],RxMessage.Data[5],RxMessage.Data[6],RxMessage.Data[7]);

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


