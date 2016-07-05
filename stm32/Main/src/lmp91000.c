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
#include "stm32f10x_dma.h"
#include "lmp91000.h"
#include "debug.h"

#define ADC_O2_SAMPLE_NUM 512
#define ADC1_DR_Address    ((u32)0x40012400+0x4c)
#define  REG_ADDRESS 0x00
u8 I2c_Buf_Write[256];
u8 I2c_Buf_Read[256];
u8 I2c_Buf_Write1[256];
u8 I2c_Buf_Read1[256];
u8 LMP91000S;

__IO uint16_t ADC_ConvertedValue[ADC_O2_SAMPLE_NUM][2];
uint32_t ADC_SAMPLE_AVE;
uint32_t STM32_TEMP_AVE;
//__IO u16 ADC_ConvertedValueLocal;

void LMP91000_STATUS (void);

/**
 *   ADC1_GPIO_Config
 *   Initialize GPIO for ADC1
 */
static void ADC1_GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable DMA clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  /* Enable ADC1 and GPIOC clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOB, ENABLE);


  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;	       // no need rate for input
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}


/**  
 *  ADC1_Mode_Config
 *  Set up ADC1 and DMA mode
 * 
 */
static void ADC1_Mode_Config(void)
{
  DMA_InitTypeDef DMA_InitStructure;
  ADC_InitTypeDef ADC_InitStructure;

  /* DMA channel1 configuration */
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;	 
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)ADC_ConvertedValue;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = ADC_O2_SAMPLE_NUM;
  DMA_InitStructure.DMA_PeripheralInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;  
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;		
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);

  /* Enable DMA channel1 */
  DMA_Cmd(DMA1_Channel1, ENABLE);

  /* ADC1 configuration */

  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	
  ADC_InitStructure.ADC_ScanConvMode = DISABLE ; 	
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;	
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; 	
  ADC_InitStructure.ADC_NbrOfChannel = 2;	 	
  ADC_Init(ADC1, &ADC_InitStructure);

  /*Set up clk PCLK2/8 9Hz*/
  RCC_ADCCLKConfig(RCC_PCLK2_Div8); 
  /*Smaple cycle and priority setting */ 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 1, ADC_SampleTime_55Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 2, ADC_SampleTime_55Cycles5);

  /* Enable ADC1 DMA */
  ADC_DMACmd(ADC1, ENABLE);

  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  /*Reset Calibration */   
  ADC_ResetCalibration(ADC1);
  while(ADC_GetResetCalibrationStatus(ADC1));

  /* Start ADC Calibration  */
  ADC_StartCalibration(ADC1);
  while(ADC_GetCalibrationStatus(ADC1));

  /* soft start adc sampling */ 
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

static void ADC1_NVIC_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure; 
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);  
  /*Enable DMA2 Channel3 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

//  DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE); 
}
void ADC1_Filter(void)
{
  uint32_t  sum = 0;
  uint32_t i;

  for ( i=0;i<ADC_O2_SAMPLE_NUM;i++)
  {
     sum += ADC_ConvertedValue[i][0];
  }
  ADC_SAMPLE_AVE = sum/ADC_O2_SAMPLE_NUM;  

  sum = 0;
  for ( i=0;i<ADC_O2_SAMPLE_NUM;i++)
  {
     sum += ADC_ConvertedValue[i][1];
  } 

  STM32_TEMP_AVE = sum/ADC_O2_SAMPLE_NUM; 
}

void DMA1_Channel1_IRQHandler(void)
{
  if(DMA_GetITStatus(DMA1_IT_TC1) == SET)
  {
//    ADC1_Filter();
    DMA_ClearFlag(DMA1_IT_TC1); 
  }
}


//#define I2C_Speed              400000
#define I2C_Speed              100000
#define I2C1_OWN_ADDRESS7    0x0A
#define I2C_PageSize           8			/* AT24C02 Page 8 bytes width*/

uint16_t EEPROM_ADDRESS;

/**
 *  I2C_GPIO_Config
 *  I2C GPIO config
 */
static void I2C_GPIO_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure; 


  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2,ENABLE);  
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_ADC1, ENABLE);
    
  /* PB10-I2C1_SCL¡¢PB11-I2C1_SDA*/
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;	      
  GPIO_Init(GPIOB, &GPIO_InitStructure);
	
}

/**
 * I2C_Configuration
 * 
 */
static void I2C_Mode_Configu(void)
{
  I2C_InitTypeDef  I2C_InitStructure; 

  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 =I2C1_OWN_ADDRESS7; 
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable ;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_Speed;
  
  I2C_Cmd(I2C2, ENABLE);

  I2C_Init(I2C2, &I2C_InitStructure);  
}

void ADC1_Init(void)
{
	ADC1_GPIO_Config();
	ADC1_Mode_Config();
}

void I2C_LMP91000_CONFIG(void)
{
  int temp = 0x00;
  I2c_Buf_Write1[0x1] = 0x00;   //lock register
  I2c_Buf_Write1[0x10] = 0x12;  //TINCA register
  I2c_Buf_Write1[0x11] = 0x40;  //REFCN register
  I2c_Buf_Write1[0x12] = 0x07;  //MODECN register	

  while (LMP91000S!= 0x01) LMP91000_STATUS () ;
  LMP91000S = 0x00;

  while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY)) temp++;    
  /* Send START condition */
  I2C_GenerateSTART(I2C2, ENABLE);  
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) temp++;   
  /* Send  address for write */
  I2C_Send7bitAddress(I2C2, LMP91000_ADDRESS, I2C_Direction_Transmitter);  
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) temp++; 
  /* Send internal address to write to */    
  I2C_SendData(I2C2, (REG_ADDRESS+0x01));  
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) temp++; 
  /* Send the current byte */
  I2C_SendData(I2C2, I2c_Buf_Write1[0x1]);   
  /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) temp++; 
  /* Send STOP condition */
  I2C_GenerateSTOP(I2C2, ENABLE);
	
  while (LMP91000S!= 0x01) LMP91000_STATUS () ;
  LMP91000S = 0x00;
	
  while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY)) temp++;    
  /* Send START condition */
  I2C_GenerateSTART(I2C2, ENABLE);  
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) temp++;   
  /* Send  address for write */
  I2C_Send7bitAddress(I2C2, LMP91000_ADDRESS, I2C_Direction_Transmitter);  
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) temp++; 
  /* Send internal address to write to */    
  I2C_SendData(I2C2, (REG_ADDRESS+0x10));  
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) temp++; 
  /* Send the current byte */
  I2C_SendData(I2C2, I2c_Buf_Write1[0x10]);   
  /* Test on EV8 and clear it */
  while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) temp++; 
  /* Send STOP condition */
  I2C_GenerateSTOP(I2C2, ENABLE);	

  while (LMP91000S!= 0x01) LMP91000_STATUS () ;
  LMP91000S = 0x00;

  while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY)) temp++;    
  /* Send START condition */
  I2C_GenerateSTART(I2C2, ENABLE);  
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) temp++;   
  /* Send  address for write */
  I2C_Send7bitAddress(I2C2, LMP91000_ADDRESS, I2C_Direction_Transmitter);  
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) temp++; 
  /* Send internal address to write to */    
  I2C_SendData(I2C2, (REG_ADDRESS+0x11));  
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) temp++; 
  /* Send the current byte */
  I2C_SendData(I2C2, I2c_Buf_Write1[0x11]);   
  /* Test on EV8 and clear it */
  while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) temp++; 
  /* Send STOP condition */
  I2C_GenerateSTOP(I2C2, ENABLE);	

  while (LMP91000S!= 0x01) LMP91000_STATUS () ;
  LMP91000S = 0x00;

  while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY)) temp++;    
  /* Send START condition */
  I2C_GenerateSTART(I2C2, ENABLE);  
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) temp++;   
  /* Send  address for write */
  I2C_Send7bitAddress(I2C2, LMP91000_ADDRESS, I2C_Direction_Transmitter);  
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) temp++; 
  /* Send internal address to write to */    
  I2C_SendData(I2C2, (REG_ADDRESS+0x12));  
  /* Test on EV8 and clear it */
  while(! I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) temp++; 
  /* Send the current byte */
  I2C_SendData(I2C2, I2c_Buf_Write1[0x12]);   
  /* Test on EV8 and clear it */
  while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) temp++; 
  /* Send STOP condition */
  I2C_GenerateSTOP(I2C2, ENABLE);	

	
}

void LMP91000_STATUS (void)
{ 
  int temp1 = 0x00;
  while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY)) temp1++; 
  /* Send START condition */
  I2C_GenerateSTART(I2C2, ENABLE);	
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) temp1++;
  /* Send LMP91000 address for write */
  I2C_Send7bitAddress(I2C2, LMP91000_ADDRESS, I2C_Direction_Transmitter);
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) temp1++;	
  /* Clear EV6 by setting again the PE bit */
  I2C_Cmd(I2C2, ENABLE);
  /* Send the LMP91000's internal address to write to */
  I2C_SendData(I2C2, REG_ADDRESS);  
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) temp1++;
  /* Send STRAT condition a second time */  
  I2C_GenerateSTART(I2C2, ENABLE);	
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) temp1++;	
  /* Send LMP91000 address for read */
  I2C_Send7bitAddress(I2C2, LMP91000_ADDRESS, I2C_Direction_Receiver);	
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) temp1++;
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)) temp1++;
  I2c_Buf_Read1[0] = I2C_ReceiveData(I2C2);
  /* Disable Acknowledgement */
  I2C_AcknowledgeConfig(I2C2, DISABLE);
  /* Send STOP Condition */
  I2C_GenerateSTOP(I2C2, ENABLE);
  if ((I2c_Buf_Read1[0]&0x01)==0x01) LMP91000S=0x01;
  else LMP91000S = 0x00;	
}	

/* LMP91000 initialization */
void I2C_LMP91000_Init(void)
{

  I2C_GPIO_Config(); 

  I2C_Mode_Configu();

  ADC1_GPIO_Config();
  ADC1_Mode_Config();
  ADC1_NVIC_Config();
  LMP91000S = 0x00  ;

  I2C_LMP91000_CONFIG();
}


