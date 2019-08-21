
#include "dwt_delay.h"

void DWT_DelayUpdate(void)
{
    Delay_ms = SystemCoreClock / 1000;    //
    Delay_us = SystemCoreClock / 1000000; //
}

void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT_DelayUpdate();
}

uint32_t DWT_GetTick(void)
{
   return DWT->CYCCNT;
}

void DWT_Delay_sec(uint32_t sec) //
{
    uint32_t Count = DWT->CYCCNT;
    sec = sec * SystemCoreClock;
    while((DWT->CYCCNT - Count) < sec);
}

void DWT_Delay_ms(uint32_t ms) //
{
    uint32_t Count = DWT->CYCCNT;
    ms = ms * Delay_ms;
    while((DWT->CYCCNT - Count) < ms);
}

void DWT_Delay_us(uint32_t us) //
{
    uint32_t Count = DWT->CYCCNT;
    us = us * Delay_us;
    while((DWT->CYCCNT - Count) < us);
}

//
//
uint8_t DWT_Test_sec(uint32_t start, uint32_t time)
{
    return (((DWT->CYCCNT - start) >= (time*SystemCoreClock)) ? 1 : 0);
}

uint8_t DWT_Test_ms(uint32_t start, uint32_t time)
{
    return (((DWT->CYCCNT - start) >= (time*Delay_ms)) ? 1 : 0);
}

uint8_t DWT_Test_us(uint32_t start, uint32_t time)
{
    return (((DWT->CYCCNT - start) >= (time*Delay_us)) ? 1 : 0);
}


//
//
uint32_t DWT_Time_sec(uint32_t start, uint32_t current)
{
    return (current - start) / SystemCoreClock;
}

uint32_t DWT_Time_ms(uint32_t start, uint32_t current)
{
    return (current - start) / Delay_ms;
}

uint32_t DWT_Time_us(uint32_t start, uint32_t current)
{
    return (current - start) / Delay_us;
}

//------------------------------------------------------------------------------------------------------------------------
