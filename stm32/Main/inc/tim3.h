#ifndef __TIMER_H
#define __TIMER_H

#define LED_ON()     GPIOB->BRR  = 0x00002000
#define LED_OFF()    GPIOB->BSRR = 0x00002000


void DelayTimerInit(void);
void delay(int cnt);
void delay_ms(u16 cnt);

void delay_100ms(u16 cnt);


void delayslow_10ms(u16 cnt);
void delayslow_1s(u16 cnt);


#endif
