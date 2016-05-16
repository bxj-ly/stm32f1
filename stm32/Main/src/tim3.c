#include "tim3.h"


void TIM3_Init(void)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

  TIM_DeInit(TIM3);

  TIM_TimeBaseStructure.TIM_Period           = 1000;                /* Count max 1s */
  TIM_TimeBaseStructure.TIM_Prescaler        = (72 - 1);            /* 72M/72 1ms*/
  TIM_TimeBaseStructure.TIM_ClockDivision    = TIM_CKD_DIV1;  
  TIM_TimeBaseStructure.TIM_CounterMode      = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_RepetitionCounter=0;
  TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);
  TIM_ClearFlag(TIM3, TIM_FLAG_Update);
  TIM_SetCounter(TIM3,0);
  TIM_Cmd(TIM3,ENABLE);
}
//----------------------------------------------------------------------------------------------------------------------------------------------
//延时函数，约1us
void delay(int cnt) 
{
  int i;
  TIM_SetCounter(TIM3,0);
  while(i<cnt)
  {
  i=TIM3->CNT;
  }    
}
//延时函数，约1ms
void delay_ms(u16 cnt) 
{
  u16 i;	
	for(i=0;i<cnt;i++)
	{          
		 delay(990);         
	}    
}
//延时函数，约100ms
void delay_100ms(u16 cnt) 
{
  u16 i;	
	for(i=0;i<cnt;i++)
	{          
		 delay_ms(99);         
	}    
}

//延时函数，约100ms
void delayslow_10ms(u16 cnt) 
{
    u16 i,j;
	
	for(i=0;i<cnt;i++)
	{          
		 for(j=0;j<24;j++);
         
	}    
}

//延时函数，约100ms
void delayslow_1s(u16 cnt) 
{
    u16 i;	
	for(i=0;i<cnt;i++)
	{    
		 delayslow_10ms(100); 
	}    
}


	



