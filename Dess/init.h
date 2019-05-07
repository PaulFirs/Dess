#ifndef __STM32F10x_TIM_H
#define __STM32F10x_TIM_H

#ifdef __cplusplus
 extern "C" {
#endif



#define BUF_SIZE 9
 volatile uint8_t RX_BUF[BUF_SIZE];
 volatile uint8_t TX_BUF[BUF_SIZE];








 /*
  * Comands for I2C
  */
#define GET_TIME 			0x00
#define SET_TIME 			0x01
#define GET_TEMP 			0x02
#define GET_SET_ALARM 		0x03
 /*
  * Comands for MH-Z19B
  */
#define GET_CO2				0x04


 /*
  * Comands for ERRORS
  */
#define ERROR		 		 0x7F

/*
 * Errors
 */
#define NOT_FULL_DATA 		 0x01
#define NOT_EQUAL_CRC 		 0x02
#define I2C_ERR 			 0x03


#define DS_ADDRESS            		0xD0
#define DS_ADDRESS_WRITE            (DS_ADDRESS|0)
#define DS_ADDRESS_READ             (DS_ADDRESS|1)

#define DS3231_CONTROL				0x0E	// Адрес регистра управления
#define DS3231_A1IE					0x00
#define DS3231_A2IE					0x01
#define DS3231_INTCN				0x02
#define DS3231_CONV					0x05
#define DS3231_BBSQW				0x06

#define DS3231_STATUS				0x0F	// Адрес регистра состояния
#define DS3231_A1F					0x00
#define DS3231_A2F					0x01
#define DS3231_BSY					0x02

#define DS3231_T_MSB				0x11	// Адрес регистра последней измеренной температуры старшиий байт
#define DS3231_T_LSB				0x12	// Адрес регистра последней измеренной температуры младшиий байт





void SetSysClockTo72(void);
void usart1_init(void);
void usart2_init(void);
void servo_init(void);
void ports_init(void);
void timer_init(void);
void I2C1_init(void);


#ifdef __cplusplus
}
#endif

#endif /*__STM32F10x_TIM_H */
