#ifndef __BEEPER_H
#define __BEEPER_H
#include "stm32f10x.h"
#include <stdio.h>

uint8_t BEEPER_GetStatus(void);
void BEEPER_ON(void);
void BEEPER_OFF(void);
void BEEPER_GPIOConfiguration(void);

#endif  /*__BEEPER_H*/
