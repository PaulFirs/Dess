#ifndef __DWT_DELAY_H
#define __DWT_DELAY_H

#ifdef __cplusplus
 extern "C" {
#endif



#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"


void DWT_DelayUpdate(void);
void DWT_Init(void);
void DWT_Delay_sec(uint32_t sec);
void DWT_Delay_ms(uint32_t ms);
void DWT_Delay_us(uint32_t us);

extern uint32_t SystemCoreClock;
uint32_t Delay_us, Delay_ms;





#ifdef __cplusplus
}
#endif

#endif /*__STM32F10x_TIM_H */
