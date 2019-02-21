#ifndef __STM32F10x_TIM_H
#define __STM32F10x_TIM_H

#ifdef __cplusplus
 extern "C" {
#endif


#define GET_TIME 			 0x00
#define SET_TIME 			 0x01
#define GET_TEMP 			 0x02





#define DS_ADDRESS            		0xD0
#define DS_ADDRESS_WRITE            (DS_ADDRESS|0)
#define DS_ADDRESS_READ             (DS_ADDRESS|1)

#define DS3231_CONTROL				0x0E	// ����� �������� ����������
#define DS3231_A1IE					0x00
#define DS3231_A2IE					0x01
#define DS3231_INTCN				0x02
#define DS3231_CONV					0x05
#define DS3231_BBSQW				0x06

#define DS3231_STATUS				0x0F	// ����� �������� ���������
#define DS3231_A1F					0x00
#define DS3231_A2F					0x01
#define DS3231_BSY					0x02

#define DS3231_T_MSB				0x11	// ����� �������� ��������� ���������� ����������� �������� ����
#define DS3231_T_LSB				0x12	// ����� �������� ��������� ���������� ����������� �������� ����



void SetSysClockTo72(void);
void usart_init(void);
void servo_init(void) ;



#ifdef __cplusplus
}
#endif

#endif /*__STM32F10x_TIM_H */