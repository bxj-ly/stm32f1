#ifndef __DEBUG_H__
#define	__DEBUG_H__
#include "stm32f10x.h"
#include <stdio.h>

#define __DEBUG__ 
  
#ifdef __DEBUG__  
#define INFO(format,...) printf(format, ##__VA_ARGS__)  
#define DEBUG(format,...) printf(format, ##__VA_ARGS__)  
#define ERROR(format,...) printf("%s#%s()#%u : " format,             \
            __FILE__, __FUNCTION__, __LINE__, ## __VA_ARGS__) 
#else  
#define DEBUG(format,...) 
#define DEBUG(format,...)
#define ERROR(format,...)
#endif 

#define DBG_LED1_ON()     GPIOA->BRR  = 0x00000004
#define DBG_LED1_OFF()    GPIOA->BSRR = 0x00000004

#define DBG_LED2_ON()     GPIOA->BRR  = 0x00000008
#define DBG_LED2_OFF()    GPIOA->BSRR = 0x00000008

#define SYS_MODE_ECHO 			((uint32_t)0x00000001 << 0) 
#define SYS_MODE_MON_PRT		((uint32_t)0x00000001 << 1) 
#define SYS_MODE_1TH4  			((uint32_t)0x00000001 << 2) 
#define SYS_MODE_MON4  			((uint32_t)0x00000001 << 3) 
#define SYS_MODE_AUTO_CONN      ((uint32_t)0x00000001 << 4)

uint32_t DEBUG_GetUSART1IntCnt(void);
void DEBUG_SetUSART1IntCnt(void);
uint32_t DEBUG_GetUART4IntCnt(void);
void DEBUG_SetUART4IntCnt(void);
uint32_t DEBUG_GetDMA1Channel4IntCnt(void);
void DEBUG_SetDMA1Channel4IntCnt(void);
uint32_t DEBUG_GetDMA1Channel5IntCnt(void);
void DEBUG_SetDMA1Channel5IntCnt(void);
uint32_t DEBUG_GetDMA2Channel3IntCnt(void);
void DEBUG_SetDMA2Channel3IntCnt(void);
uint32_t DEBUG_GetDMA2Channel5IntCnt(void);
void DEBUG_SetDMA2Channel5IntCnt(void);


uint32_t DEBUG_GetPlayMode(void);
void  DEBUG_SetPlayMode(uint32_t mode);
void DEBUG_AddPlayMode(uint32_t mode);
void DEBUG_RemovePlayMode(uint32_t mode);


uint32_t  DEBUG_GetPlayStage(void);
void  DEBUG_SetPlayStage(uint32_t stage);
void DEBUG_MonitorState(void);

#endif
