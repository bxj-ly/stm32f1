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
#include "kline.h"
#include "systick.h"
#include "debug.h"
#include "obd.h"
#include "formula.h"
#include "ISO15765_4.h"


#define _10400BuadRate 96
#define _50000BuadRate 20  //for K-Line design

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

//**************************************************************************************************
//--------------------------------------------K Line COM Definition----------------------------------------
//**************************************************************************************************

u32 KdelayTime; 
u32 SampleTime;

#define ISOK_COM_DATA_HIGH()	GPIO_ResetBits(GPIOB, GPIO_Pin_14)    //反逻辑，总线上为高电平
#define ISOL_COM_DATA_HIGH()	GPIO_ResetBits(GPIOB, GPIO_Pin_15)    //反逻辑，总线上为高电平
#define ISOK_COM_DATA_LOW()	GPIO_SetBits(GPIOB, GPIO_Pin_14)  //反逻辑，总线上为低电平
#define ISOL_COM_DATA_LOW()	GPIO_SetBits(GPIOB, GPIO_Pin_15)  //反逻辑，总线上为低电平
#define ISOKL_COM_DATA_HIGH()	GPIO_ResetBits(GPIOB, GPIO_Pin_14 |  GPIO_Pin_15)    //反逻辑，总线上为高电平
#define ISOKL_COM_DATA_LOW()	GPIO_SetBits(GPIOB, GPIO_Pin_14 |  GPIO_Pin_15)  //反逻辑，总线上为低电平

void KL_LINE_VirtualCOM_TX_GPIOConfig(void)    //配置发送管脚
{
	GPIO_InitTypeDef GPIO_InitStructure;
	/* 模拟输出管脚TX */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	ISOKL_COM_DATA_HIGH();

}

void K_LINE_ByteSend(u8 val)  //发送单个字符
{
	u8 i = 0;
	ISOK_COM_DATA_LOW();			   //起始位	
	SysTick_Delay_us(KdelayTime); 
	for(i = 0; i < 8; i++)	           //8位数据位
	{
		if(val & 0x01)
			ISOK_COM_DATA_HIGH();            
		else			
		    ISOK_COM_DATA_LOW();
       
		SysTick_Delay_us(KdelayTime);
		val >>= 1;
	}
	ISOK_COM_DATA_HIGH();		   //停止位
	     
	SysTick_Delay_us(KdelayTime); 
}

void L_LINE_ByteSend(u8 val)  //发送单个字符
{
	u8 i = 0;
	ISOL_COM_DATA_LOW();			   //起始位
	SysTick_Delay_us(KdelayTime); 
	for(i = 0; i < 8; i++)	           //8位数据位
	{
		if(val & 0x01)
			ISOL_COM_DATA_HIGH();            
		else			
		  ISOL_COM_DATA_LOW();               
		SysTick_Delay_us(KdelayTime);
		val >>= 1;
	}
	ISOL_COM_DATA_HIGH();		   //停止位
	SysTick_Delay_us(KdelayTime); 
}


void KL_LINE_ByteSend(u8 val)  //发送单个字符
{
	u8 i = 0;
	ISOKL_COM_DATA_LOW();			   //起始位
	SysTick_Delay_us(KdelayTime); 
	for(i = 0; i < 8; i++)	           //8位数据位
	{
		if(val & 0x01)
			ISOKL_COM_DATA_HIGH();                
		else			
		  ISOKL_COM_DATA_LOW();              
		SysTick_Delay_us(KdelayTime);
		val >>= 1;
	}
	ISOKL_COM_DATA_HIGH();		   //停止位
	SysTick_Delay_us(KdelayTime); 
}

//------------------------------------------------------------------------------------------------------

#define K_LINE_COM_RX_PORT	GPIOC
#define K_LINE_COM_RX_PIN	GPIO_Pin_6
void K_LINE_VirtualCOM_RX_GPIOConfig(void)     //配置接收
{
	GPIO_InitTypeDef GPIO_InitStructure;
//	EXTI_InitTypeDef EXTI_InitStruct;
//	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	/* 模拟输入管脚RX */
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode =  GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

}

void TIM4_Configuration(u16 period)     //配置定时器
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);//使能TIM3的时钟
	TIM_DeInit(TIM4);	                        //复位TIM3时钟
	TIM_InternalClockConfig(TIM4);			//采用内部时钟给TIM3提供时钟源

	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;   //预分频系数为72，这样计数器时钟为72MHz/72 = 1MHz
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;    //设置时钟分频
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//设置计数器模式为向上计数
	TIM_TimeBaseStructure.TIM_Period = period - 1;  //设置计数溢出大小，每计period个数就产生一个更新事件
	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseStructure);	//将配置应用到TIM3中

	TIM_ClearFlag(TIM4, TIM_FLAG_Update);		//清除溢出中断标志
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);        //开启TIM3的中断
	TIM_Cmd(TIM4,DISABLE);			        //关闭定时器TIM3

//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);   // 抢占式优先级别
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;	  //通讯设置为TIM3中断
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;//响应式中断优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;	  //打开中断
    NVIC_Init(&NVIC_InitStructure);	
}


u8 KrecvStat = COM_STOP_BIT;   //初始状态机

#define KCOM_RX_STAT	GPIO_ReadInputDataBit(K_LINE_COM_RX_PORT, K_LINE_COM_RX_PIN)

u8 KrecvData;
u8 KLine_RecvData_Coming;
u8 kline_input_data;  //Indicate 8-bit data effective
u8 kline_fetch_data;

void TIM4_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)  
	{
		TIM_ClearITPendingBit(TIM4 , TIM_FLAG_Update);
        if ((GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_6)==1)&&(KLine_RecvData_Coming == 1)) 
        {
            return;
        }
        else if ((GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_6)!=1)&&(KLine_RecvData_Coming == 1)) 
        {
            KLine_RecvData_Coming = 0;  //DATA COMIING
            KrecvStat = COM_START_BIT ;
            TIM_Cmd(TIM4, DISABLE);
            TIM4_Configuration(_10400BuadRate);
            TIM_Cmd(TIM4, ENABLE);
            return;
        }
        else
        {
            KrecvStat++;		

            if(KrecvStat == COM_STOP_BIT) 		  
            {		  
                kline_input_data = KrecvData;
                KLine_RecvData_Coming = 1;
                kline_fetch_data++;
                TIM_Cmd(TIM4, DISABLE);
                TIM4_Configuration(_50000BuadRate);
                TIM_Cmd(TIM4, ENABLE);
                return;                          
            }

            if(GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_6))                          
            {
                KrecvData |= (1 << (KrecvStat - 1));
            }
            else				        
            {
                KrecvData &= ~(1 <<(KrecvStat - 1));
            }
        }    
	}
}


void KVirtualCOM_Config(u16 baudRate)
{

	KL_LINE_VirtualCOM_TX_GPIOConfig();
	K_LINE_VirtualCOM_RX_GPIOConfig();
	TIM4_Configuration(baudRate);		//设置对应波特率的定时器的定时时间
	KdelayTime = _10400BuadRate;			//设置串口发送的速率
	SampleTime = baudRate;   
}

//-------------------------------------------------------------------------------------------------

//说明:
//K线激活后，5S内没有操作激活失效
//在K线激活后，向K线发送数据流/故障码指令时，字节之间有5～20ms时序间隔要求，采用10ms为宜
//TIM_Cmd(TIM4, ENABLE)和TIM_Cmd(TIM4, DISABLE)使能和不使能K线数据接收，接收到的有效字节放于kline_input_data中
//本程序未做buffer，kline_input_data中不及时取走，后到字节覆盖前到字节
//--------
u8 ISO14230_4ADDR_start(u8 KSW[2])   //ISO14230 5 BAUD Activation
{
    u8 start_flag;
    u32 count;
    start_flag = 0;
    KSW[0]=0;
    KSW[1]=0;


	KrecvData = 0x00;
	kline_input_data =0;

	ISOKL_COM_DATA_HIGH();
	SysTick_Delay_ms(2000); //delay at lease 300ms according to spec
	
	/*begin to send 5-baud address 0x33 (1)00110011(0)*/
	ISOKL_COM_DATA_LOW();
	SysTick_Delay_ms(200);
	ISOKL_COM_DATA_HIGH();
	SysTick_Delay_ms(400);
	ISOKL_COM_DATA_LOW();
	SysTick_Delay_ms(400);
	ISOKL_COM_DATA_HIGH();
	SysTick_Delay_ms(400);
	ISOKL_COM_DATA_LOW();
	SysTick_Delay_ms(400);
	ISOKL_COM_DATA_HIGH();
	SysTick_Delay_ms(200);
	
	TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving	
	
	for(count=0;count<72000000/2;count++) 
	{  
		if ( kline_input_data == 0x55) 	
        {
            start_flag = 1; 
            kline_input_data=0; 
            break;
        }
	
	}
	
	if (start_flag!=0)
	{
        for(count=0;count<72000000/2;count++) 
        {
            if((kline_input_data!=0)&&(start_flag==1)) 
            {
                KSW[0] = kline_input_data; 
                start_flag=2; 
                kline_input_data=0;
            }
            if((kline_input_data!=0)&&(start_flag==2)) 
            {
                KSW[1] = kline_input_data; 
                start_flag=3; 
                kline_input_data=0;break;
            }             
        }
    }
    else
    {
        return 0;
    }

    TIM_Cmd(TIM4, DISABLE);	//Close K-Line Receiving
    if (start_flag==3)
    {
        SysTick_Delay_ms(30);  //timing delay
        KL_LINE_ByteSend(~KSW[1]) ;
        TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving	
        SysTick_Delay_ms(55);  //timing delay
        if (kline_input_data==0xCC)
        {                                //initiating success
            TIM_Cmd(TIM4, DISABLE);	//Close K-Line Receiving
            kline_input_data=0;
            start_flag=0;
            SysTick_Delay_ms(100);	 //time gap for receiving next effective K-Line instruction
            return 1;					
        }
        else
        {                                    //initiating failure
            TIM_Cmd(TIM4, DISABLE);	//Close K-Line Receiving
            kline_input_data=0;
            start_flag=0;
            return 0;
        }	
    }
    else
    {                                        //initiating failure
        kline_input_data=0;
        start_flag=0;
        return 0;
    }
}

u8 ISO14230_4HL_start(u8 KSW[7])    //ISO14230 5 FAST Activation
{
    u32 count;

    u8 receive_flag;
    count = 0;
    KSW[6]=0;	KSW[5]=0;	KSW[4]=0;	KSW[3]=0;	KSW[2]=0x00;	KSW[1]=0;	KSW[0]=0x00;
    receive_flag = 0;

    ISOKL_COM_DATA_HIGH();
    SysTick_Delay_ms(2000); 

    KrecvData = 0x00;
    kline_input_data =0;
    //send {0xc1,0x33,0xf1,0x81,0x66}
    ISOKL_COM_DATA_LOW();
    SysTick_Delay_ms(25); 
    ISOKL_COM_DATA_HIGH();
    SysTick_Delay_ms(25); 
    KL_LINE_ByteSend(0xC1) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x33) ;
    SysTick_Delay_ms(10); 	
    KL_LINE_ByteSend(0xf1) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x81) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x66) ;

    TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving
    for (count=0;count<100;count++)
    { 
        SysTick_Delay_ms(1);
        if (kline_fetch_data!=0)
        {
            if (receive_flag==0) {KSW[0]=kline_input_data; kline_input_data=0; kline_fetch_data=0; receive_flag=1;continue;}
            if (receive_flag==1) {KSW[1]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=2;continue;}
            if (receive_flag==2) {KSW[2]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=3;continue;}
            if (receive_flag==3) {KSW[3]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=4;continue;}
            if (receive_flag==4) {KSW[4]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=5;continue;}
            if (receive_flag==5) {KSW[5]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=6;continue;}
            if (receive_flag==6) {KSW[6]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=7;break;}		
            //Get 7 bytes, use them when necessary.
        }
        else
            kline_input_data=0;
    }
	TIM_Cmd(TIM4, DISABLE);		//Close K-Line Receiving
	if (receive_flag!=0)         //success
	{
    	KrecvData = 0x00;
    	kline_input_data =0;
    	SysTick_Delay_ms(100);  //time gap for receiving next effective K-Line instruction
    	return 1;
	}
	else
	{	                          //failure
    	KrecvData = 0x00;
    	kline_input_data =0;
    	return 0;			
	}
	
	
}

u8 ISO9141_2ADDR_start(u8 KSW[2])   //ISO14230-2 5 BAUD Activation
{
    u8 start_flag;
    u32 count;
	start_flag = 0;
	KSW[0]=0;
	KSW[1]=0;


	KrecvData = 0x00;
	kline_input_data =0;

	ISOKL_COM_DATA_HIGH();
	SysTick_Delay_ms(2000); //delay at lease 300ms according to spec
	
	/*begin to send 5-baud address 0x33 (1)00110011(0)*/
	ISOKL_COM_DATA_LOW();
	SysTick_Delay_ms(200);
	ISOKL_COM_DATA_HIGH();
	SysTick_Delay_ms(400);
	ISOKL_COM_DATA_LOW();
	SysTick_Delay_ms(400);
	ISOKL_COM_DATA_HIGH();
	SysTick_Delay_ms(400);
	ISOKL_COM_DATA_LOW();
	SysTick_Delay_ms(400);
	ISOKL_COM_DATA_HIGH();
	SysTick_Delay_ms(200);
	
	TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving	
	
	for(count=0;count<72000000/2;count++) 
	{  
		if ( kline_input_data == 0x55) 	{start_flag = 1; kline_input_data=0; break;}
	
	}
	
	if (start_flag!=0)
	{
    for(count=0;count<72000000/2;count++) 
    {
        if((kline_input_data!=0)&&(start_flag==1)) {KSW[0] = kline_input_data; start_flag=2; kline_input_data=0;}
        if((kline_input_data!=0)&&(start_flag==2)) {KSW[1] = kline_input_data; start_flag=3; kline_input_data=0;break;}			 
    }
    }
    else
    {
        return 0;
    }
 
	TIM_Cmd(TIM4, DISABLE);	//Close K-Line Receiving
 
    if (start_flag==3)
    {
        SysTick_Delay_ms(30);  //timing delay
        KL_LINE_ByteSend(~KSW[1]) ;
        TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving	
        SysTick_Delay_ms(55);  //timing delay
        if (kline_input_data==0xCC)
        {                                //success
            TIM_Cmd(TIM4, DISABLE);	//Close K-Line Receiving
            kline_input_data=0;
            start_flag=0;
            SysTick_Delay_ms(100);	 //time gap for receiving next effective K-Line instruction
            return 1;					
        }
        else
        {                                    //failure
            TIM_Cmd(TIM4, DISABLE);	//Close K-Line Receiving
            kline_input_data=0;
            start_flag=0;
            return 0;
        }	
    }
    else
    {                                        //failure
        kline_input_data=0;
        start_flag=0;
        return 0;
    }

}



u8 ISO_14230_LINK_KEEP(u8 KSW[7])   //ISO 14230 Link Keeping
{
	
    u32 count;	
    u8 receive_flag;
    count = 0;
    KSW[6]=0;	KSW[5]=0;	KSW[4]=0;	KSW[3]=0;	KSW[2]=0x00;	KSW[1]=0;	KSW[0]=0x00;
    receive_flag = 0;
    KrecvData = 0x00;
    kline_input_data =0;

    ISOKL_COM_DATA_HIGH();
    SysTick_Delay_ms(60);
    //begin to send ISO_14230 link keeping instruction {0xC2,0x33,0xF1,0x01,0x00,0xe7}
    KL_LINE_ByteSend(0xc2) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x33) ;
    SysTick_Delay_ms(10); 	
    KL_LINE_ByteSend(0xf1) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x01) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x00) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0xe7) ;
    SysTick_Delay_ms(10);
	
    TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving
    for (count=0;count<100;count++)
    { 
        SysTick_Delay_ms(1);
        if (kline_fetch_data!=0)
        {
            if (receive_flag==0) {KSW[0]=kline_input_data; kline_input_data=0; kline_fetch_data=0; receive_flag=1;continue;}
            if (receive_flag==1) {KSW[1]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=2;continue;}
            if (receive_flag==2) {KSW[2]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=3;continue;}
            if (receive_flag==3) {KSW[3]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=4;continue;}
            if (receive_flag==4) {KSW[4]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=5;continue;}
            if (receive_flag==5) {KSW[5]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=6;continue;}
            if (receive_flag==6) {KSW[6]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=7;break;}		
            //Get 7 BYTES, use them when necessary.
        }
        else
            kline_input_data=0;
    }
	TIM_Cmd(TIM4, DISABLE);		//Close K-Line Receiving
	if (receive_flag!=0)         //success
	{
    	KrecvData = 0x00;
    	kline_input_data =0;
    	SysTick_Delay_ms(100);  //time gap for receiving next effective K-Line instruction
    	return 1;
	}
	else
	{	                          //failure
    	KrecvData = 0x00;
    	kline_input_data =0;
    	return 0;			
	}
	

}


u8 ISO_14230_DATA_READ(u8 enqcode, u8 RD[255])  //ISO 14230 Read Real-time Data
{
    u32 count;
    u8 endbyte;
    u8 rnumber;
    rnumber =0; //received bytes number


    for (count=0;count<255;count++)
    {
        RD[count]=0;
    }

    TIM_Cmd(TIM4, DISABLE);
    KrecvData = 0x00;
    kline_input_data =0;
    endbyte = 0xC2+0x33+0xF1+0x01+enqcode;

    ISOK_COM_DATA_HIGH();
    SysTick_Delay_ms(60);
    K_LINE_ByteSend(0xC2) ;
    SysTick_Delay_ms(10); 	
    K_LINE_ByteSend(0x33) ;
    SysTick_Delay_ms(10);
    K_LINE_ByteSend(0xF1) ;
    SysTick_Delay_ms(10);  	
    K_LINE_ByteSend(0x01) ;
    SysTick_Delay_ms(10);
    K_LINE_ByteSend(enqcode) ;
    SysTick_Delay_ms(10);
    K_LINE_ByteSend(endbyte) ;
    SysTick_Delay_ms(10);

	
	TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving
    for (count=0;count<300;count++)
    { 
        SysTick_Delay_ms(1);
        if (kline_fetch_data!=0)
        {
            RD[rnumber++]=kline_input_data;
            kline_input_data=0;
            kline_fetch_data=0;
            //Get bytes, use them when necessary.
        }
        else
            kline_input_data=0;
    }
	TIM_Cmd(TIM4, DISABLE);		//Close K-Line Receiving
	if (rnumber!=0)         //success
	{
    	KrecvData = 0x00;
    	kline_input_data =0;
    	SysTick_Delay_ms(100);  //time gap for receiving next effective K-Line instruction
    	return 1;
	}
	else
	{	                          //failure
    	KrecvData = 0x00;
    	kline_input_data =0;
    	return 0;	
	}
	
}


u8 ISO_9141_2_LINK_KEEP(u8 KSW[7])   //ISO 9141 Link Keeping
{
	
    u32 count;	
    u8 receive_flag;
    count = 0;
    KSW[6]=0;	KSW[5]=0;	KSW[4]=0;	KSW[3]=0;	KSW[2]=0x00;	KSW[1]=0;	KSW[0]=0x00;
    receive_flag = 0;
    KrecvData = 0x00;
    kline_input_data =0;

    ISOKL_COM_DATA_HIGH();
    SysTick_Delay_ms(60);
    //begin to send ISO_9141_2 link keeping instruction {0x68,0x6a,0xF1,0x01,0x00,0xc4}
    KL_LINE_ByteSend(0x68) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x6a) ;
    SysTick_Delay_ms(10); 	
    KL_LINE_ByteSend(0xf1) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x01) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x00) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0xc4) ;
    SysTick_Delay_ms(10);
	
    TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving
    for (count=0;count<100;count++)
    { 
        SysTick_Delay_ms(1);
        if (kline_fetch_data!=0)
        {
            if (receive_flag==0) {KSW[0]=kline_input_data; kline_input_data=0; kline_fetch_data=0; receive_flag=1;continue;}
            if (receive_flag==1) {KSW[1]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=2;continue;}
            if (receive_flag==2) {KSW[2]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=3;continue;}
            if (receive_flag==3) {KSW[3]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=4;continue;}
            if (receive_flag==4) {KSW[4]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=5;continue;}
            if (receive_flag==5) {KSW[5]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=6;continue;}
            if (receive_flag==6) {KSW[6]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=7;break;}		
            //Get 7 bytes, use them when necessary.
        }
        else
            kline_input_data=0;
    }
	TIM_Cmd(TIM4, DISABLE);		//Close K-Line Receiving
	if (receive_flag!=0)         //success
	{
    	KrecvData = 0x00;
    	kline_input_data =0;
    	SysTick_Delay_ms(100);  //time gap for receiving next effective K-Line instruction
    	return 1;
	}
	else
	{	                          //failure
    	KrecvData = 0x00;
    	kline_input_data =0;
    	return 0;			
	}
	

}

u8 ISO_9141_2_DATA_READ(u8 enqcode,u8 RD[255])  // ISO 9141 Read Real-time Data
{
    u32 count;
    u8 endbyte;
    u8 rnumber; //received bytes number
    rnumber =0; 

    for (count=0;count<255;count++)
    {
    RD[count]=0;
    }
    TIM_Cmd(TIM4, DISABLE);
    KrecvData = 0x00;
    kline_input_data =0;
    endbyte = 0x68+0x6a+0xF1+0x01+enqcode;

    ISOK_COM_DATA_HIGH();
    SysTick_Delay_ms(60);
    K_LINE_ByteSend(0x68) ;
    SysTick_Delay_ms(10); 	
    K_LINE_ByteSend(0x6a) ;
    SysTick_Delay_ms(10);
    K_LINE_ByteSend(0xF1) ;
    SysTick_Delay_ms(10);  	
    K_LINE_ByteSend(0x01) ;
    SysTick_Delay_ms(10);
    K_LINE_ByteSend(enqcode) ;
    SysTick_Delay_ms(10);
    K_LINE_ByteSend(endbyte) ;
    SysTick_Delay_ms(10);

	
	TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving
    for (count=0;count<300;count++)
    { 
        SysTick_Delay_ms(1);
        if (kline_fetch_data!=0)
        {
            RD[rnumber++]=kline_input_data;
            kline_input_data=0;
            kline_fetch_data=0;
            //Get 7 bytes, use them when necessary.
        }
        else
            kline_input_data=0;

    }
	TIM_Cmd(TIM4, DISABLE);		//Close K-Line Receiving
	if (rnumber!=0)         //success
	{
    	KrecvData = 0x00;
    	kline_input_data =0;
    	SysTick_Delay_ms(100);  //time gap for receiving next effective K-Line instruction
    	return 1;
	}
	else
	{	                          //failure
    	KrecvData = 0x00;
    	kline_input_data =0;
    	return 0;	
	}
	
}

u8 ISO_14230_DTC_READ(u8 RD[255])  //ISO 14230 Read DTC
{
    u32 count;
    u8 enqcode;
    u8 endbyte;
    u8 rnumber;
    rnumber =0; //received bytes number


    for (count=0;count<255;count++)
    {
        RD[count]=0;
    }

    TIM_Cmd(TIM4, DISABLE);
    KrecvData = 0x00;
    kline_input_data =0;
    enqcode = 03;
    endbyte = 0xC1+0x33+0xF1+enqcode;

    ISOK_COM_DATA_HIGH();
    SysTick_Delay_ms(60);
    K_LINE_ByteSend(0xC1) ;
    SysTick_Delay_ms(10); 	
    K_LINE_ByteSend(0x33) ;
    SysTick_Delay_ms(10);
    K_LINE_ByteSend(0xF1) ;
    SysTick_Delay_ms(10);  	
    K_LINE_ByteSend(enqcode) ;
    SysTick_Delay_ms(10);
    K_LINE_ByteSend(endbyte) ;
    SysTick_Delay_ms(10);

	
	TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving
    for (count=0;count<300;count++)
    { 
        SysTick_Delay_ms(1);
        if (kline_fetch_data!=0)
        {
            RD[rnumber++]=kline_input_data;
            kline_input_data=0;
            kline_fetch_data=0;
            //Get bytes, use them when necessary.
        }
        else
            kline_input_data=0;

    }
	TIM_Cmd(TIM4, DISABLE);		//Close K-Line Receiving
	if (rnumber!=0)         //success
	{
    	KrecvData = 0x00;
    	kline_input_data =0;
    	SysTick_Delay_ms(100);  //time gap for receiving next effective K-Line instruction
    	return 1;
	}
	else
	{	                          //failure
    	KrecvData = 0x00;
    	kline_input_data =0;
    	return 0;	
	}
	
}


u8 ISO_9141_2_DTC_READ(u8 RD[255])  // ISO 9141 Read DTC
{
    u32 count;
    u8 enqcode;
    u8 endbyte;
    u8 rnumber; //received bytes number
    rnumber =0; 

    for (count=0;count<255;count++)
    {
        RD[count]=0;
    }
    TIM_Cmd(TIM4, DISABLE);
    KrecvData = 0x00;
    kline_input_data =0;
    enqcode = 0x03;
    endbyte = 0x68+0x6a+0xF1+enqcode;

    ISOK_COM_DATA_HIGH();
    SysTick_Delay_ms(60);
    K_LINE_ByteSend(0x68) ;
    SysTick_Delay_ms(10); 	
    K_LINE_ByteSend(0x6a) ;
    SysTick_Delay_ms(10);
    K_LINE_ByteSend(0xF1) ;
    SysTick_Delay_ms(10);  	
    K_LINE_ByteSend(enqcode) ;
    SysTick_Delay_ms(10);
    K_LINE_ByteSend(endbyte) ;
    SysTick_Delay_ms(10);

	
	TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving
    for (count=0;count<300;count++)
    { 
    SysTick_Delay_ms(1);
    if (kline_fetch_data!=0)
    {
        RD[rnumber++]=kline_input_data;
        kline_input_data=0;
        kline_fetch_data=0;
        //Get 7 bytes, use them when necessary.
    }
    else
        kline_input_data=0;
    }
	TIM_Cmd(TIM4, DISABLE);		//Close K-Line Receiving
	if (rnumber!=0)         //success
	{
    	KrecvData = 0x00;
    	kline_input_data =0;
    	SysTick_Delay_ms(100);  //time gap for receiving next effective K-Line instruction
    	return 1;
	}
	else
	{	                          //failure
    	KrecvData = 0x00;
    	kline_input_data =0;
    	return 0;	
	}
	
}


u8 ISO_14230_DTC_CLEAR(u8 KSW[7])   //ISO 14230 DTC CLEAR
{
	
    u32 count;	
    u8 receive_flag;
    count = 0;
    KSW[6]=0;	KSW[5]=0;	KSW[4]=0;	KSW[3]=0;	KSW[2]=0x00;	KSW[1]=0;	KSW[0]=0x00;
    receive_flag = 0;
    KrecvData = 0x00;
    kline_input_data =0;

    ISOKL_COM_DATA_HIGH();
    SysTick_Delay_ms(60);
    //begin to send ISO_14230 DTC clear instruction {0xC1,0x33,0xF1,0x04,0xe9}
    KL_LINE_ByteSend(0xc1) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x33) ;
    SysTick_Delay_ms(10); 	
    KL_LINE_ByteSend(0xf1) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x04) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0xe9) ;
    SysTick_Delay_ms(10);
	
    TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving
    for (count=0;count<100;count++)
    { 
        SysTick_Delay_ms(1);
        if (kline_fetch_data!=0)
        {
            if (receive_flag==0) {KSW[0]=kline_input_data; kline_input_data=0; kline_fetch_data=0; receive_flag=1;continue;}
            if (receive_flag==1) {KSW[1]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=2;continue;}
            if (receive_flag==2) {KSW[2]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=3;continue;}
            if (receive_flag==3) {KSW[3]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=4;continue;}
            if (receive_flag==4) {KSW[4]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=5;continue;}
            if (receive_flag==5) {KSW[5]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=6;continue;}
            if (receive_flag==6) {KSW[6]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=7;break;}		
            //Get 7 BYTES, use them when necessary.
        }
        else
            kline_input_data=0;

    }
	TIM_Cmd(TIM4, DISABLE);		//Close K-Line Receiving
	if (receive_flag!=0)         //success
	{
    	KrecvData = 0x00;
    	kline_input_data =0;
    	SysTick_Delay_ms(100);  //time gap for receiving next effective K-Line instruction
    	return 1;
	}
	else
	{	                          //failure
    	KrecvData = 0x00;
    	kline_input_data =0;
    	return 0;			
	}
	

}

u8 ISO_9141_2_DTC_CLEAR(u8 KSW[7])   //ISO 9141_2 DTC CLEAR
{
	
    u32 count;	
    u8 receive_flag;
    count = 0;
    KSW[6]=0;	KSW[5]=0;	KSW[4]=0;	KSW[3]=0;	KSW[2]=0x00;	KSW[1]=0;	KSW[0]=0x00;
    receive_flag = 0;
    KrecvData = 0x00;
    kline_input_data =0;

    ISOKL_COM_DATA_HIGH();
    SysTick_Delay_ms(60);
    //begin to send ISO_9141_2 DTC clear instruction {0x68,0x6a,0xF1,0x04,0xc7}
    KL_LINE_ByteSend(0x68) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x6a) ;
    SysTick_Delay_ms(10); 	
    KL_LINE_ByteSend(0xf1) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0x04) ;
    SysTick_Delay_ms(10); 
    KL_LINE_ByteSend(0xc7) ;
    SysTick_Delay_ms(10);
	
    TIM_Cmd(TIM4, ENABLE);	//Open K-Line Receiving
    for (count=0;count<100;count++)
    { 
        SysTick_Delay_ms(1);
        if (kline_fetch_data!=0)
        {
            if (receive_flag==0) {KSW[0]=kline_input_data; kline_input_data=0; kline_fetch_data=0; receive_flag=1;continue;}
            if (receive_flag==1) {KSW[1]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=2;continue;}
            if (receive_flag==2) {KSW[2]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=3;continue;}
            if (receive_flag==3) {KSW[3]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=4;continue;}
            if (receive_flag==4) {KSW[4]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=5;continue;}
            if (receive_flag==5) {KSW[5]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=6;continue;}
            if (receive_flag==6) {KSW[6]=kline_input_data; kline_input_data=0; kline_fetch_data=0;receive_flag=7;break;}		
            //Get 7 bytes, use them when necessary.
        }
        else
            kline_input_data=0;

    }
	TIM_Cmd(TIM4, DISABLE);		//Close K-Line Receiving
	if (receive_flag!=0)         //success
	{
    	KrecvData = 0x00;
    	kline_input_data =0;
    	SysTick_Delay_ms(100);  //time gap for receiving next effective K-Line instruction
    	return 1;
	}
	else
	{	                          //failure
    	KrecvData = 0x00;
    	kline_input_data =0;
    	return 0;			
	}
	

}


 void KLINE_GPIO_Config(void)
{
     GPIO_InitTypeDef GPIO_InitStructure;
     
     RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC,ENABLE);
     RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
     
     GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;           
     GPIO_Init(GPIOC, &GPIO_InitStructure);
     
     /* Open 12V circuit */
     GPIO_SetBits(GPIOC, GPIO_Pin_0);
     
     GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;           
     GPIO_Init(GPIOC, &GPIO_InitStructure);
     
     /* Close J1850bus circuit */
     GPIO_ResetBits(GPIOC, GPIO_Pin_4);
     
     GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_5;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
     GPIO_Init(GPIOC, &GPIO_InitStructure);
     
     GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;           
     GPIO_Init(GPIOA, &GPIO_InitStructure);  
     
     /* Close J1850bus circuit */
     GPIO_SetBits(GPIOC, GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_5);
     GPIO_SetBits(GPIOA, GPIO_Pin_8);
     
     /* ISO K Out, ISO L Out */
     GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_14 | GPIO_Pin_15 ;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;           
     GPIO_Init(GPIOB, &GPIO_InitStructure);
     
     /* due to inversion circuit the bus is "1" when sending "0" */
     GPIO_ResetBits(GPIOB, GPIO_Pin_14 | GPIO_Pin_15 );  
     
     /* ISO K In */
     GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;          
     GPIO_Init(GPIOC, &GPIO_InitStructure);
	
}

/************************************************************************
  * @描述:  ISO15765 读实时数据流
  * @参数:  CanTxMsg* DSCmd:数据流命令 ErrorStatus* err 
  * @返回值: char* 数据流结果地址
  **********************************************************************/
extern __IO char         DSRAM[100];
char* ISO14230_4ADDR_ReadDS(OBD_DS_E cmd, ErrorStatus* err)
{
    float result;
    char * string;
    uint8_t ram[255];
 
    DSRAM[0]=0x00;
    if (ISO_14230_DATA_READ(DSControl[cmd].PIDByte,ram)==1)
    {
        if (DSControl[cmd].Type == Numeric)
        {
            result = DSControl[cmd].Equation0(ram + DSControl[cmd].FineByte + 2);
            sprintf((char*)DSRAM,(char*)DSControl[cmd].Format, result);
        }
        else
        {
            string = DSControl[cmd].Equation1(ram + DSControl[cmd].FineByte + 2);
            strcpy((char*)DSRAM, string);
        }
        *err = SUCCESS;
    }
    else
    {
        *err = ERROR;
    }

    INFO("HAHAHA %s\r\n",DSRAM);
    return (char*)DSRAM;
}

char* ISO9141_2ADDR_ReadDS(OBD_DS_E cmd, ErrorStatus* err)
{
    float result;
    char * string;
    uint8_t ram[255];
 
    DSRAM[0]=0x00;
    if (ISO_9141_2_DATA_READ(DSControl[cmd].PIDByte,ram)==1)
    {
        if (DSControl[cmd].Type == Numeric)
        {
            result = DSControl[cmd].Equation0(ram + DSControl[cmd].FineByte + 2);
            sprintf((char*)DSRAM,(char*)DSControl[cmd].Format, result);
        }
        else
        {
            string = DSControl[cmd].Equation1(ram + DSControl[cmd].FineByte + 2);
            strcpy((char*)DSRAM, string);
        }
        *err = SUCCESS;
    }
    else
    {
        *err = ERROR;
    }

    INFO("HAHAHA %s\r\n",DSRAM);
    return (char*)DSRAM;
}


OBD_PROTOCOL_E KLINE_ProtocolDetect(ErrorStatus *err)
{
    u8 kdata[7];
    u8 kkw[2];
    int kcount;

    KLINE_GPIO_Config();
    KVirtualCOM_Config(_50000BuadRate);            //配置K Line

    KrecvStat = COM_STOP_BIT ;
    kline_input_data = 0x00;
    KLine_RecvData_Coming = 1;	
    kline_fetch_data = 0;
    for (kcount=0;kcount<3;kcount++)
    {
        if (ISO9141_2ADDR_start(kkw)==1)
            break;// {INFO("ISO9141_2ADDR_start successes \r\n"); break;}
        //else INFO("ISO9141_2ADDR_start fails \r\n"); 
    }

    for (kcount=0;kcount<3;kcount++)
    {
        if (ISO_9141_2_LINK_KEEP(kdata)==1)
        {
            //INFO("ISO9141_2ADDR_start Link keepint test successes \r\n");  
            //for(i=0;i<7;i++) INFO("0x%x ", kdata[i]);
            //INFO("\r\n");

            if((kdata[1] == 0x48) && (kdata[2] == 0x6b))
            {
                *err = SUCCESS;
                return OBD_ISO9141_2ADDR;
            }
        } 
    }
    
    for (kcount=0;kcount<3;kcount++)
    {
        if (ISO14230_4ADDR_start(kkw)==1) 
            break;
        //{INFO("ISO14230_4ADDR_start successes \r\n"); break;}
        //else INFO("ISO14230_4ADDR_start fails \r\n"); 
    }
    
    for (kcount=0;kcount<3;kcount++)
    {
        if (ISO_14230_LINK_KEEP(kdata)==1) 
        {
            //INFO("ISO14230_4ADDR_start Link keepint test successes \r\n");  
            //for(i=0;i<7;i++) INFO("0x%x ", kdata[i]);
            //INFO("\r\n");
            if(((kdata[1]&0xC0) == 0x80) && (kdata[4] == 0x41))
            {
                *err = SUCCESS;
                return OBD_ISO14230_4ADDR;
            }
        } 
    }

    for (kcount=0;kcount<3;kcount++)
    {
        if (ISO14230_4HL_start(kdata)==1) 
            break;
        //{INFO("ISO14230_4HL_start successes \r\n"); break;}
        //else INFO("ISO14230_4HL_start fails \r\n");

    }

    for (kcount=0;kcount<3;kcount++)
    {
        if (ISO_14230_LINK_KEEP(kdata)==1) 
        {
            //INFO("ISO14230_4HL_start Link keepint test successes \r\n"); 
            //for(i=0;i<7;i++) INFO("0x%x ", kdata[i]);
            //INFO("\r\n");
            if(((kdata[0]&0xC0) == 0x80) && (kdata[3] == 0x41))
            {
                *err = SUCCESS;
                return OBD_ISO14230_4HL;
            }            
        } 
    }

    *err = ERROR;
    return OBD_PROTOCOL_UNKNOWN;

}

